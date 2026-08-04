/** \file frameGrabber_test.cpp
 * \brief Catch2 tests for the MagAOX::app::dev::frameGrabber CRTP mixin.
 *
 * \ingroup app_dev_unit_tests
 */

#include "../../../../tests/catch2/catch.hpp"

#include <chrono>
#include <cstdio>
#include <deque>
#include <string>
#include <thread>
#include <unistd.h>

// Flip protected to public for this whole translation unit so the tests can poke
// directly at MagAOXApp/frameGrabber internals without a pile of accessor wrappers.
#define protected public
#include "../../MagAOXApp.hpp"
#include "../telemeter.hpp"
#include "../frameGrabber.hpp"
#undef protected

#include "testHarnessCommon.hpp"

using namespace MagAOX::app;

namespace frameGrabber_tests
{

// ImageStreamIO caches the shared memory directory the first time it is queried
// (a static local in ImageStreamIO_shmdirname), so MILK_SHM_DIR must be set before
// any ImageStreamIO call happens anywhere in this process.  Doing this in a
// namespace-scope static initializer guarantees it runs before any TEST_CASE body.
const std::string g_shmDir = "/tmp/frameGrabber_test/shm";

int setupShmDir()
{
    mx::ioutils::createDirectories( g_shmDir );
    setenv( "MILK_SHM_DIR", g_shmDir.c_str(), 1 );
    return 0;
}
static int g_shmDirSetup = setupShmDir();

/// Build a unique shmim name for one temporary test stream.
std::string uniqueShmimName( const std::string &suffix )
{
    static unsigned counter = 0;
    ++counter;
    return "frameGrabber_test_" + suffix + "_" + std::to_string( ::getpid() ) + "_" + std::to_string( counter );
}

/// RAII wrapper for a temporary ImageStreamIO stream created out-of-band from the
/// frameGrabber under test (used to simulate a shmim owned by another process).
class tempStream
{
  public:
    explicit tempStream( const std::string &name,
                         long               naxis,
                         uint32_t           w,
                         uint32_t           h,
                         uint32_t           d,
                         uint8_t            dataType = _DATATYPE_UINT8,
                         int                nbsem    = IMAGE_NB_SEMAPHORE )
        : m_name( name )
    {
        uint32_t imsize[3] = { w, h, d };
        if( ImageStreamIO_createIm_gpu( &m_image,
                                        m_name.c_str(),
                                        naxis,
                                        imsize,
                                        dataType,
                                        -1,
                                        1,
                                        nbsem,
                                        0,
                                        CIRCULAR_BUFFER | ZAXIS_TEMPORAL,
                                        0 ) != IMAGESTREAMIO_SUCCESS )
        {
            throw std::runtime_error( "failed to create temporary ImageStreamIO stream" );
        }
        m_image.md[0].cnt1 = 0;
    }

    ~tempStream()
    {
        if( m_owner )
        {
            ImageStreamIO_destroyIm( &m_image );
        }
    }

    IMAGE *image()
    {
        return &m_image;
    }

    /// Release destruction ownership (e.g. after handing the stream to a frameGrabber
    /// under test that will destroy it itself).
    void dismiss()
    {
        m_owner = false;
    }

  private:
    std::string m_name;
    IMAGE       m_image{};
    bool        m_owner{ true };
};

} // namespace frameGrabber_tests

using namespace frameGrabber_tests;

namespace frameGrabber_tests
{

/// Test harness exposing `dev::frameGrabber` without an optional post-publish hook.
struct fgTest : public MagAOX::app::MagAOXApp<true>,
                public dev::frameGrabber<fgTest>,
                public dev::telemeter<fgTest>
{
    static constexpr bool c_frameGrabber_flippable = true;

    typedef dev::frameGrabber<fgTest> frameGrabberT;
    typedef dev::telemeter<fgTest>    telemeterT;

    // ---- configureAcquisition() controls -------------------------------------
    int      configureAcquisitionCalls{ 0 };
    int      configureAcquisitionFailCount{ 0 }; ///< Number of leading calls that should fail.
    uint32_t nextWidth{ 2 };
    uint32_t nextHeight{ 2 };
    uint8_t  nextDataType{ _DATATYPE_UINT8 };

    // ---- fps() controls -------------------------------------------------------
    float fpsValue{ 1000.0 };

    // ---- startAcquisition() controls ------------------------------------------
    int startAcquisitionCalls{ 0 };
    int startAcquisitionFailCount{ 0 };

    // ---- acquireAndCheckValid() controls ---------------------------------------
    std::deque<int> acquireResults; ///< Popped in call order; empty queue shuts the app down.
    int             acquireCalls{ 0 };
    bool            setReconfigOnNextAcquire{ false };
    int             bumpFpsAfterCall{ -1 }; ///< If >=0, fpsValue changes right after this call count is reached.
    float           bumpedFpsValue{ 0 };

    // ---- loadImageIntoStream() controls ----------------------------------------
    int  loadImageIntoStreamCalls{ 0 };
    bool failLoadImageIntoStream{ false };

    // ---- reconfig() controls ----------------------------------------------------
    int reconfigCalls{ 0 };

    fgTest() : MagAOX::app::MagAOXApp<true>( "", false )
    {
        m_configName = uniqueShmimName( "app" );
    }

    ~fgTest() noexcept
    {
    }

    // Constructs a real (but FIFO-less) indiDriver so m_indiDriver != nullptr -- the same
    // pattern MagAOXApp_test.hpp's setConfigName() uses. indi::updateIfChanged() catches
    // its own send failures, so this doesn't need a live, connected INDI server.
    void setConfigNameWithDriver( const std::string &cn )
    {
        m_configName = cn;
        m_indiDriver = MagAOX::app::dev::testHarness::makeFifolessIndiDriver<MagAOX::app::MagAOXApp<true>>(
            this, m_configName );
    }

    // MagAOXApp's pure virtuals -- tests call frameGrabberT::appStartup()/appLogic()/
    // appShutdown() explicitly, so these trivial overrides just satisfy instantiability
    // and disambiguate unqualified lookup between MagAOXApp and dev::frameGrabber.
    int appStartup()
    {
        return 0;
    }
    int appLogic()
    {
        return 0;
    }
    int appShutdown()
    {
        return 0;
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return frameGrabberT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return frameGrabberT::loadConfig( config );
    }

    int configureAcquisition()
    {
        ++configureAcquisitionCalls;
        if( configureAcquisitionCalls <= configureAcquisitionFailCount )
        {
            return -1;
        }
        m_width    = nextWidth;
        m_height   = nextHeight;
        m_dataType = nextDataType;
        return 0;
    }

    float fps()
    {
        return fpsValue;
    }

    int startAcquisition()
    {
        ++startAcquisitionCalls;
        if( startAcquisitionCalls <= startAcquisitionFailCount )
        {
            return -1;
        }
        return 0;
    }

    int acquireAndCheckValid()
    {
        ++acquireCalls;
        if( acquireResults.empty() )
        {
            m_shutdown = 1;
            return -1;
        }

        int r = acquireResults.front();
        acquireResults.pop_front();

        if( setReconfigOnNextAcquire )
        {
            m_reconfig               = true;
            setReconfigOnNextAcquire = false;
        }

        if( r == 0 )
        {
            clock_gettime( CLOCK_REALTIME, &m_currImageTimestamp );
        }

        if( bumpFpsAfterCall >= 0 && acquireCalls == bumpFpsAfterCall )
        {
            fpsValue = bumpedFpsValue;
        }

        return r;
    }

    int loadImageIntoStream( void *dest )
    {
        ++loadImageIntoStreamCalls;
        static_cast<void>( dest );
        if( failLoadImageIntoStream )
        {
            m_shutdown = 1;
            return -1;
        }
        return 0;
    }

    int reconfig()
    {
        ++reconfigCalls;
        m_shutdown = 1; // end the outer loop cleanly once reconfig has been exercised
        return 0;
    }
};

/// Test harness exposing `dev::frameGrabber` WITH the optional post-publish hook.
struct fgTestHook : public MagAOX::app::MagAOXApp<true>,
                    public dev::frameGrabber<fgTestHook>,
                    public dev::telemeter<fgTestHook>
{
    static constexpr bool c_frameGrabber_flippable = true;

    typedef dev::frameGrabber<fgTestHook> frameGrabberT;
    typedef dev::telemeter<fgTestHook>    telemeterT;

    uint32_t nextWidth{ 2 };
    uint32_t nextHeight{ 2 };
    uint8_t  nextDataType{ _DATATYPE_UINT8 };
    float    fpsValue{ 1000.0 };

    std::deque<int> acquireResults;
    int             postPublishCalls{ 0 };
    int             postPublishReturn{ 0 };
    int             reconfigCalls{ 0 };
    bool            setReconfigOnNextAcquire{ false };

    fgTestHook() : MagAOX::app::MagAOXApp<true>( "", false )
    {
        m_configName = uniqueShmimName( "hookapp" );
    }

    ~fgTestHook() noexcept
    {
    }

    int appStartup()
    {
        return 0;
    }
    int appLogic()
    {
        return 0;
    }
    int appShutdown()
    {
        return 0;
    }
    int setupConfig( mx::app::appConfigurator &config )
    {
        return frameGrabberT::setupConfig( config );
    }
    int loadConfig( mx::app::appConfigurator &config )
    {
        return frameGrabberT::loadConfig( config );
    }

    int configureAcquisition()
    {
        m_width    = nextWidth;
        m_height   = nextHeight;
        m_dataType = nextDataType;
        return 0;
    }

    float fps()
    {
        return fpsValue;
    }

    int startAcquisition()
    {
        return 0;
    }

    int acquireAndCheckValid()
    {
        if( acquireResults.empty() )
        {
            m_shutdown = 1;
            return -1;
        }
        int r = acquireResults.front();
        acquireResults.pop_front();

        if( setReconfigOnNextAcquire )
        {
            m_reconfig               = true;
            setReconfigOnNextAcquire = false;
        }

        if( r == 0 )
        {
            clock_gettime( CLOCK_REALTIME, &m_currImageTimestamp );
        }
        return r;
    }

    int loadImageIntoStream( void *dest )
    {
        static_cast<void>( dest );
        return 0;
    }

    int reconfig()
    {
        ++reconfigCalls;
        m_shutdown = 1;
        return 0;
    }

    int frameGrabberPostPublish( IMAGE *imageStream )
    {
        static_cast<void>( imageStream );
        ++postPublishCalls;
        if( postPublishReturn < 0 )
        {
            m_shutdown = 1;
        }
        return postPublishReturn;
    }
};

/// Minimal test harness for the non-flippable configuration option.
struct fgTestNoFlip : public MagAOX::app::MagAOXApp<true>,
                      public dev::frameGrabber<fgTestNoFlip>,
                      public dev::telemeter<fgTestNoFlip>
{
    static constexpr bool c_frameGrabber_flippable = false;

    typedef dev::frameGrabber<fgTestNoFlip> frameGrabberT;
    typedef dev::telemeter<fgTestNoFlip>    telemeterT;

    fgTestNoFlip() : MagAOX::app::MagAOXApp<true>( "", false )
    {
        m_configName = uniqueShmimName( "noflipapp" );
    }

    ~fgTestNoFlip() noexcept
    {
    }

    int appStartup()
    {
        return 0;
    }
    int appLogic()
    {
        return 0;
    }
    int appShutdown()
    {
        return 0;
    }
    int setupConfig( mx::app::appConfigurator &config )
    {
        return frameGrabberT::setupConfig( config );
    }
    int loadConfig( mx::app::appConfigurator &config )
    {
        return frameGrabberT::loadConfig( config );
    }

    int configureAcquisition()
    {
        return 0;
    }
    float fps()
    {
        return 1000.0;
    }
    int startAcquisition()
    {
        return 0;
    }
    int acquireAndCheckValid()
    {
        m_shutdown = 1;
        return -1;
    }
    int loadImageIntoStream( void *dest )
    {
        static_cast<void>( dest );
        return 0;
    }
    int reconfig()
    {
        return 0;
    }
};

/// Put a `fgTest`-like harness into the state where `fgThreadExec` will proceed
/// straight through its startup wait loops without blocking.
template <class appT> void primeForSyncExec( appT &app )
{
    app.m_fgThreadInit    = false;
    app.m_powerMgtEnabled = true;
    app.m_powerState      = 1;
    app.m_powerTargetState = 1;
    app.state( stateCodes::OPERATING );
    app.m_shutdown = 0;
}

/// Start a short-lived dummy framegrabber thread so `frameGrabber::appLogic()` has a
/// live, joinable thread to check (its first line always calls `pthread_tryjoin_np`
/// on `m_fgThread`, which is undefined behavior on a never-started `std::thread`).
template <class appT> void startDummyFgThread( appT &app, int sleepMs = 200 )
{
    app.m_fgThread = std::thread( [sleepMs]() { std::this_thread::sleep_for( std::chrono::milliseconds( sleepMs ) ); } );
}

/// RAII helper that joins a dummy framegrabber thread on scope exit.
template <class appT> struct fgThreadScope
{
    appT &m_app;
    explicit fgThreadScope( appT &app, int sleepMs = 200 ) : m_app( app )
    {
        startDummyFgThread( m_app, sleepMs );
    }
    ~fgThreadScope()
    {
        if( m_app.m_fgThread.joinable() )
        {
            m_app.m_fgThread.join();
        }
    }
};

} // namespace frameGrabber_tests

using namespace frameGrabber_tests;

/// setupConfig()/loadConfig() cover the flippable and non-flippable configuration
/// paths, along with every input-validation branch in loadConfig().
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber setupConfig and loadConfig manage every configurable option", "[frameGrabber]" )
{
    SECTION( "setupConfig adds the flip option when flippable" )
    {
        fgTest             app;
        mx::app::appConfigurator config;
        REQUIRE( app.setupConfig( config ) == 0 );
    }

    SECTION( "setupConfig omits the flip option when not flippable" )
    {
        fgTestNoFlip       app;
        mx::app::appConfigurator config;
        REQUIRE( app.setupConfig( config ) == 0 );
    }

    SECTION( "loadConfig applies defaults, corrections, and every flip option" )
    {
        struct flipCase
        {
            std::string flip;
            int         expected;
        };

        std::vector<flipCase> cases = { { "flipNone", fgTest::fgFlipNone },
                                        { "flipUD", fgTest::fgFlipUD },
                                        { "flipLR", fgTest::fgFlipLR },
                                        { "flipUDLR", fgTest::fgFlipUDLR },
                                        { "garbage", fgTest::fgFlipNone } };

        for( auto &c : cases )
        {
            fgTest app;

            mx::app::appConfigurator config;
            REQUIRE( app.setupConfig( config ) == 0 );

            std::string path = "/tmp/frameGrabber_test_flip_" + c.flip + ".conf";
            mx::app::writeConfigFile( path,
                                      { "framegrabber", "framegrabber", "framegrabber" },
                                      { "shmimName", "circBuffLength", "defaultFlip" },
                                      { "myshmim", "0", c.flip } );

            config.readConfig( path );
            REQUIRE( app.loadConfig( config ) == 0 );
            REQUIRE( app.m_shmimName == "myshmim" );
            REQUIRE( app.m_circBuffLength == 1 ); // corrected up from 0
            REQUIRE( app.m_defaultFlip == c.expected );
        }
    }

    SECTION( "loadConfig fails when shmimName is empty and configName is empty" )
    {
        fgTest app;
        app.m_configName = "";
        app.m_shmimName  = "";

        mx::app::appConfigurator config;
        REQUIRE( app.setupConfig( config ) == 0 );
        REQUIRE( app.loadConfig( config ) < 0 );
    }

    SECTION( "loadConfig clamps a negative latencyTime to 0" )
    {
        fgTest app;

        mx::app::appConfigurator config;
        REQUIRE( app.setupConfig( config ) == 0 );

        std::string path = "/tmp/frameGrabber_test_negtime.conf";
        mx::app::writeConfigFile( path, { "framegrabber" }, { "latencyTime" }, { "-5" } );
        config.readConfig( path );

        REQUIRE( app.loadConfig( config ) == 0 );
        REQUIRE( app.m_latencyCircBuffMaxTime == 0 );
    }
}

/// configCircBuffs() covers the zero-length, negative-fps, and normal clamping
/// branches.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber configCircBuffs configures or clamps the latency buffers", "[frameGrabber]" )
{
    SECTION( "zero latency length disables the circular buffers" )
    {
        fgTest app;
        app.m_latencyCircBuffMaxLength = 0;
        app.fpsValue                  = 100;
        REQUIRE( app.configCircBuffs() == 0 );
        REQUIRE( app.m_atimes.maxEntries() == 0 );
        REQUIRE( app.m_wtimes.maxEntries() == 0 );
    }

    SECTION( "a negative fps disables the buffers and reports an error" )
    {
        fgTest app;
        app.fpsValue = -1;
        REQUIRE( app.configCircBuffs() < 0 );
    }

    SECTION( "a large requested size is clamped to the configured maximum" )
    {
        fgTest app;
        app.m_latencyCircBuffMaxLength = 10;
        app.m_latencyCircBuffMaxTime   = 5;
        app.fpsValue                   = 1000; // 2*5*1000 = 10000, clamps down to 10
        REQUIRE( app.configCircBuffs() == 0 );
        REQUIRE( app.m_atimes.maxEntries() == 10 );
    }

    SECTION( "a tiny requested size is floored to 3 for a meaningful variance" )
    {
        fgTest app;
        app.m_latencyCircBuffMaxLength = 100000;
        app.m_latencyCircBuffMaxTime   = 0.0001;
        app.fpsValue                   = 1;
        REQUIRE( app.configCircBuffs() == 0 );
        REQUIRE( app.m_atimes.maxEntries() == 3 );
    }
}

/// loadImageIntoStreamCopy() covers the non-flippable shortcut and every flip case.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber loadImageIntoStreamCopy dispatches on the flip setting", "[frameGrabber]" )
{
    uint8_t src[4]  = { 1, 2, 3, 4 };
    uint8_t dest[4] = { 0, 0, 0, 0 };

    SECTION( "the non-flippable configuration always uses a plain memcpy" )
    {
        fgTestNoFlip app;
        REQUIRE( app.loadImageIntoStreamCopy( dest, src, 2, 2, 1 ) == dest );
        REQUIRE( dest[0] == 1 );
    }

    SECTION( "flippable configurations dispatch on m_defaultFlip" )
    {
        fgTest app;

        app.m_defaultFlip = fgTest::fgFlipNone;
        REQUIRE( app.loadImageIntoStreamCopy( dest, src, 2, 2, 1 ) != nullptr );

        app.m_defaultFlip = fgTest::fgFlipUD;
        REQUIRE( app.loadImageIntoStreamCopy( dest, src, 2, 2, 1 ) != nullptr );

        app.m_defaultFlip = fgTest::fgFlipLR;
        REQUIRE( app.loadImageIntoStreamCopy( dest, src, 2, 2, 1 ) != nullptr );

        app.m_defaultFlip = fgTest::fgFlipUDLR;
        REQUIRE( app.loadImageIntoStreamCopy( dest, src, 2, 2, 1 ) != nullptr );

        app.m_defaultFlip = 99; // unknown value
        REQUIRE( app.loadImageIntoStreamCopy( dest, src, 2, 2, 1 ) == nullptr );
    }
}

/// openShmim() covers a missing shmim, a not-yet-ready shmim, a corrupt shmim, and a
/// fully valid shmim with both 2D and 3D geometry.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber openShmim attaches to an external shmim or reports why it could not", "[frameGrabber]" )
{
    SECTION( "a missing shmim is retried without a pre-existing stream" )
    {
        fgTest app;
        app.m_shmimName = uniqueShmimName( "missing" );
        REQUIRE( app.openShmim() == 1 );
        REQUIRE( app.openShmim() == 1 ); // second call exercises the "already logged" branch
    }

    SECTION( "a pre-existing local stream is closed before attaching" )
    {
        fgTest app;
        app.m_shmimName = uniqueShmimName( "replace" );
        app.m_imageStream = reinterpret_cast<IMAGE *>( malloc( sizeof( IMAGE ) ) );
        *app.m_imageStream = IMAGE{};
        REQUIRE( app.openShmim() == 1 ); // still missing on disk, but the old handle was freed first
        REQUIRE( app.m_imageStream == nullptr );
    }

    SECTION( "a shmim with too few live semaphores is not yet ready" )
    {
        fgTest      app;
        std::string name = uniqueShmimName( "notready" );
        app.m_shmimName  = name;

        // Create the stream with fewer semaphores than SEMAPHORE_MAXVAL directly, since
        // `.md[0].sem` reflects the number requested at creation time.
        tempStream ext( name, 3, 4, 4, 2, _DATATYPE_UINT8, 1 );

        REQUIRE( app.openShmim() == 1 );
    }

    SECTION( "a valid 2D shmim is attached" )
    {
        fgTest      app;
        std::string name = uniqueShmimName( "valid2d" );
        app.m_shmimName  = name;

        tempStream ext( name, 2, 5, 6, 1 );

        REQUIRE( app.openShmim() == 0 );
        REQUIRE( app.m_width == 5 );
        REQUIRE( app.m_height == 6 );
        REQUIRE( app.m_circBuffLength == 1 );
        REQUIRE( app.m_dataType == _DATATYPE_UINT8 );

        if( app.m_imageStream != nullptr )
        {
            ImageStreamIO_closeIm( app.m_imageStream );
            free( app.m_imageStream );
            app.m_imageStream = nullptr;
        }
    }

    SECTION( "a valid 3D shmim reports its circular buffer depth" )
    {
        fgTest      app;
        std::string name = uniqueShmimName( "valid3d" );
        app.m_shmimName  = name;

        tempStream ext( name, 3, 5, 6, 4 );

        REQUIRE( app.openShmim() == 0 );
        REQUIRE( app.m_width == 5 );
        REQUIRE( app.m_height == 6 );
        REQUIRE( app.m_circBuffLength == 4 );

        if( app.m_imageStream != nullptr )
        {
            ImageStreamIO_closeIm( app.m_imageStream );
            free( app.m_imageStream );
            app.m_imageStream = nullptr;
        }
    }

    SECTION( "a valid 1D shmim reports a height and depth of 1" )
    {
        fgTest      app;
        std::string name = uniqueShmimName( "valid1d" );
        app.m_shmimName  = name;

        tempStream ext( name, 1, 9, 1, 1 );

        REQUIRE( app.openShmim() == 0 );
        REQUIRE( app.m_width == 9 );
        REQUIRE( app.m_height == 1 );
        REQUIRE( app.m_circBuffLength == 1 );

        if( app.m_imageStream != nullptr )
        {
            ImageStreamIO_closeIm( app.m_imageStream );
            free( app.m_imageStream );
            app.m_imageStream = nullptr;
        }
    }

    SECTION( "a corrupt shmim file fails to open and is reported as not-yet-ready" )
    {
        fgTest      app;
        std::string name = uniqueShmimName( "corrupt" );
        app.m_shmimName  = name;

        char path[1024];
        ImageStreamIO_filename( path, sizeof( path ), name.c_str() );
        FILE *f = fopen( path, "w" );
        REQUIRE( f != nullptr );
        const char junk[16] = { 0 };
        fwrite( junk, 1, sizeof( junk ), f );
        fclose( f );

        REQUIRE( app.openShmim() == 1 );

        unlink( path );
    }
}

/// updateINDI() is a no-op without a live INDI driver.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber updateINDI is a no-op without an active INDI driver", "[frameGrabber]" )
{
    fgTest app;
    REQUIRE( app.m_indiDriver == nullptr );
    REQUIRE( app.updateINDI() == 0 );
}

/// updateINDI() with a real (FIFO-less) indiDriver connected exercises its
/// indi::updateIfChanged() publishing calls (already covered directly in
/// indiUtils_test.cpp for the send-failure paths those calls take internally).
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber updateINDI publishes properties with an active INDI driver", "[frameGrabber]" )
{
    fgTest app;
    app.setConfigNameWithDriver( uniqueShmimName( "updateindi" ) );

    REQUIRE( app.frameGrabberT::appStartup() == 0 );

    // Non-zero latency stats (as appLogic() would compute from real frames) so
    // updateINDI() also exercises its fps-from-mean-latency divisions.
    app.m_mna = 0.01;
    app.m_mnw = 0.02;

    REQUIRE( app.updateINDI() == 0 );

    app.m_shutdown = 1;
    REQUIRE( app.frameGrabberT::appShutdown() == 0 );
}

/// appStartup() registers the shmimName, frameSize, and timing properties and starts
/// the framegrabber thread; each property registration failure is reported.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber appStartup registers INDI properties and starts its thread", "[frameGrabber]" )
{
    SECTION( "a duplicate fg_shmimName property blocks startup" )
    {
        fgTest            app;
        pcf::IndiProperty dup( pcf::IndiProperty::Text );
        dup.setDevice( app.configName() );
        dup.setName( "fg_shmimName" );
        REQUIRE( app.registerIndiPropertyNew( dup, nullptr ) == 0 );

        REQUIRE( app.frameGrabberT::appStartup() < 0 );
    }

    SECTION( "a duplicate fg_frameSize property blocks startup" )
    {
        fgTest            app;
        pcf::IndiProperty dup( pcf::IndiProperty::Number );
        dup.setDevice( app.configName() );
        dup.setName( "fg_frameSize" );
        REQUIRE( app.registerIndiPropertyNew( dup, nullptr ) == 0 );

        REQUIRE( app.frameGrabberT::appStartup() < 0 );
    }

    SECTION( "a duplicate fg_timing property blocks startup" )
    {
        fgTest            app;
        pcf::IndiProperty dup;
        REQUIRE( app.createROIndiNumber( dup, "fg_timing" ) == 0 );
        REQUIRE( app.registerIndiPropertyReadOnly( dup ) == 0 );

        REQUIRE( app.frameGrabberT::appStartup() < 0 );
    }

    SECTION( "a successful startup registers properties and joins cleanly on shutdown" )
    {
        fgTest app;

        REQUIRE( app.frameGrabberT::appStartup() == 0 );
        REQUIRE( app.m_fgThread.joinable() );

        // The thread is parked waiting for a READY/OPERATING state; release it.
        app.m_shutdown = 1;
        REQUIRE( app.frameGrabberT::appShutdown() == 0 );
        REQUIRE( !app.m_fgThread.joinable() );

        // A second call is a no-op (nothing left to join).
        REQUIRE( app.frameGrabberT::appShutdown() == 0 );
    }

    SECTION( "an unwritable cpuset assignment fails startup after the thread is running" )
    {
        fgTest app;
        app.m_fgCpuset = "definitely_not_a_real_cpuset_xyz";

        REQUIRE( app.frameGrabberT::appStartup() < 0 );

        // The thread itself did start (threadStart() only fails afterward, while trying
        // to move it into the cpuset), so it still needs to be released and joined.
        app.m_shutdown = 1;
        if( app.m_fgThread.joinable() )
        {
            app.m_fgThread.join();
        }
    }
}

/// appLogic() covers the thread-exited error path, the idle reset path, the
/// insufficient-history reset path, the normal statistics path, and the
/// non-monotonic-write-time error path.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber appLogic computes latency statistics or reports why it could not", "[frameGrabber]" )
{
    SECTION( "appLogic resets statistics when not operating" )
    {
        fgTest        app;
        fgThreadScope<fgTest> thr( app );
        app.state( stateCodes::READY );
        REQUIRE( app.frameGrabberT::appLogic() == 0 );
        REQUIRE( app.m_mna == 0 );
    }

    SECTION( "appLogic resets statistics when there is not enough history" )
    {
        fgTest        app;
        fgThreadScope<fgTest> thr( app );
        app.state( stateCodes::OPERATING );
        app.m_powerMgtEnabled = false; // powerState() always returns 1
        app.fpsValue          = 100;

        timespec ts{ 1, 0 };
        app.m_atimes.maxEntries( 5 );
        app.m_wtimes.maxEntries( 5 );
        app.m_atimes.nextEntry( ts );
        app.m_wtimes.nextEntry( ts );
        ts.tv_sec = 2;
        app.m_atimes.nextEntry( ts );
        app.m_wtimes.nextEntry( ts );

        // A vanishingly small max time forces latTime (and thus usedEntries) below 2.
        app.m_latencyCircBuffMaxTime = 0.0000001;
        app.m_cbFPS                  = 100;

        REQUIRE( app.frameGrabberT::appLogic() == 0 );
        REQUIRE( app.m_mna == 0 );
    }

    SECTION( "appLogic computes statistics once there is enough well-formed history" )
    {
        fgTest        app;
        fgThreadScope<fgTest> thr( app );
        app.state( stateCodes::OPERATING );
        app.m_powerMgtEnabled = false;
        app.fpsValue          = 100;
        app.m_cbFPS           = 100;
        app.m_latencyCircBuffMaxTime = 5;

        // The refEntry/usedEntries math in frameGrabber::appLogic() assumes the
        // buffers are filled to capacity (as they would be in steady-state operation),
        // so maxEntries here matches the number of entries pushed below exactly. Two
        // extra entries beyond capacity also wrap the circular index (m_latest ends up
        // less than usedEntries), exercising the `refEntry = maxEntries() + ...` branch.
        app.m_atimes.maxEntries( 6 );
        app.m_wtimes.maxEntries( 6 );

        for( int i = 0; i < 8; ++i )
        {
            timespec a{ 100 + i, 0 };
            timespec w{ 100 + i, 500000000 };
            app.m_atimes.nextEntry( a );
            app.m_wtimes.nextEntry( w );
        }

        REQUIRE( app.frameGrabberT::appLogic() == 0 );
        REQUIRE( app.m_mna > 0 );
        REQUIRE( app.m_mnw > 0 );
        REQUIRE( app.m_maxa >= app.m_mina );

        // Also exercise recordFGTimings() and updateINDI() with real statistics.
        REQUIRE( app.recordFGTimings() == 0 );
        REQUIRE( app.recordFGTimings( true ) == 0 ); // force a repeat record
        REQUIRE( app.updateINDI() == 0 );
    }

    SECTION( "appLogic reports a non-monotonic write time" )
    {
        fgTest        app;
        fgThreadScope<fgTest> thr( app );
        app.state( stateCodes::OPERATING );
        app.m_powerMgtEnabled = false;
        app.fpsValue          = 100;
        app.m_cbFPS           = 100;
        app.m_latencyCircBuffMaxTime = 5;

        app.m_atimes.maxEntries( 6 );
        app.m_wtimes.maxEntries( 6 );

        for( int i = 0; i < 6; ++i )
        {
            timespec a{ 100 + i, 0 };
            // Write times decrease on every other sample, forcing a negative delta.
            timespec w{ 100 + ( i % 2 == 0 ? i : i - 2 ), 0 };
            app.m_atimes.nextEntry( a );
            app.m_wtimes.nextEntry( w );
        }

        REQUIRE( app.frameGrabberT::appLogic() == 0 );
    }

}

/// onPowerOff() resets frame-size and latency-statistics state and requests a reconfig.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber onPowerOff resets state and requests a reconfiguration", "[frameGrabber]" )
{
    fgTest app;
    app.m_width          = 100;
    app.m_height         = 100;
    app.m_circBuffLength = 5;
    app.m_mna            = 5;

    REQUIRE( app.frameGrabberT::onPowerOff() == 0 );
    REQUIRE( app.m_width == 0 );
    REQUIRE( app.m_height == 0 );
    REQUIRE( app.m_circBuffLength == 1 );
    REQUIRE( app.m_mna == 0 );
    REQUIRE( app.m_reconfig == true );
}

/// The framegrabber thread's main acquisition loop, driven synchronously (not on a
/// background thread) so every branch can be reached deterministically.
/**
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber fgThreadExec drives the full acquisition loop", "[frameGrabber]" )
{
    SECTION( "the thread waits for a READY/OPERATING, powered state and exits cleanly on shutdown" )
    {
        fgTest app;
        app.m_fgThreadInit    = false;
        app.m_powerMgtEnabled = true;
        app.m_powerState      = 0; // not powered, so the state-wait loop must sleep and retry
        app.m_shutdown        = 0;
        app.m_shmimName       = uniqueShmimName( "waitstate" );

        std::thread runner( [&app]() { app.fgThreadExec(); } );

        // Give the state-wait loop a chance to log/sleep at least once before asking it
        // to shut down without ever becoming ready.
        std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
        app.m_shutdown = 1;

        runner.join();

        REQUIRE( app.configureAcquisitionCalls == 0 ); // never got past the state wait
    }

    SECTION( "a single frame is created, published, and cleaned up (owned shmim, no circ. buff.)" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName      = uniqueShmimName( "single" );
        app.m_circBuffLength = 1;
        app.nextWidth        = 4;
        app.nextHeight       = 4;
        app.acquireResults   = { 0 };

        app.fgThreadExec();

        REQUIRE( app.configureAcquisitionCalls == 1 );
        REQUIRE( app.startAcquisitionCalls == 1 );
        REQUIRE( app.loadImageIntoStreamCalls == 1 );
        REQUIRE( app.m_imageStream == nullptr ); // destroyed on shutdown, since it was owned
    }

    SECTION( "multiple frames wrap the circular buffer index" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName      = uniqueShmimName( "circ" );
        app.m_circBuffLength = 3;
        app.nextWidth        = 4;
        app.nextHeight       = 4;
        app.acquireResults   = { 0, 0, 0, 0 };

        app.fgThreadExec();

        REQUIRE( app.loadImageIntoStreamCalls == 4 );
    }

    SECTION( "acquireAndCheckValid returning >0 is retried without publishing" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName      = uniqueShmimName( "retry" );
        app.m_circBuffLength = 1;
        app.acquireResults   = { 1, 1, 0 };

        app.fgThreadExec();

        REQUIRE( app.acquireCalls == 4 ); // 1, 1, 0, then the empty-queue shutdown call
        REQUIRE( app.loadImageIntoStreamCalls == 1 );
    }

    SECTION( "a loadImageIntoStream failure breaks the acquisition loop" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName            = uniqueShmimName( "loadfail" );
        app.m_circBuffLength       = 1;
        app.acquireResults         = { 0 };
        app.failLoadImageIntoStream = true;

        app.fgThreadExec();

        REQUIRE( app.loadImageIntoStreamCalls == 1 );
    }

    SECTION( "a startAcquisition failure is retried from the top of the outer loop" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName               = uniqueShmimName( "startfail" );
        app.m_circBuffLength          = 1;
        app.startAcquisitionFailCount = 1;
        app.acquireResults            = { 0 };

        app.fgThreadExec();

        REQUIRE( app.startAcquisitionCalls == 2 );
        REQUIRE( app.configureAcquisitionCalls == 2 );
    }

    SECTION( "a configureAcquisition failure sleeps briefly and is retried" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName                    = uniqueShmimName( "cfgfail" );
        app.m_circBuffLength               = 1;
        app.configureAcquisitionFailCount  = 1;
        app.acquireResults                 = { 0 };

        app.fgThreadExec();

        REQUIRE( app.configureAcquisitionCalls == 2 );
    }

    SECTION( "reconfig is invoked once m_reconfig is set" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName              = uniqueShmimName( "reconfig" );
        app.m_circBuffLength         = 1;
        app.acquireResults           = { 0 };
        // m_reconfig is set as part of publishing the first frame; the inner loop's
        // condition check then sees it and exits so `reconfig()` gets called (which
        // itself sets m_shutdown=1 to end the outer loop cleanly).
        app.setReconfigOnNextAcquire = true;

        app.fgThreadExec();

        REQUIRE( app.reconfigCalls == 1 );
    }

    SECTION( "an optional post-publish hook can abort the acquisition loop" )
    {
        fgTestHook app;
        primeForSyncExec( app );
        app.m_shmimName      = uniqueShmimName( "hook" );
        app.m_circBuffLength = 1;
        app.acquireResults   = { 0, 0 };
        app.postPublishReturn = -1;

        app.fgThreadExec();

        REQUIRE( app.postPublishCalls == 1 );
    }

    SECTION( "a present post-publish hook that succeeds lets multiple circular-buffer frames publish" )
    {
        fgTestHook app;
        primeForSyncExec( app );
        app.m_shmimName      = uniqueShmimName( "hookok" );
        app.m_circBuffLength = 3;
        app.acquireResults   = { 1, 0, 0, 0, 0 }; // includes one "no data yet" retry

        app.fgThreadExec();

        REQUIRE( app.postPublishCalls == 4 );
    }

    SECTION( "a present post-publish hook still allows reconfig to be triggered" )
    {
        fgTestHook app;
        primeForSyncExec( app );
        app.m_shmimName              = uniqueShmimName( "hookreconfig" );
        app.m_circBuffLength         = 1;
        app.acquireResults           = { 0 };
        app.setReconfigOnNextAcquire = true;

        app.fgThreadExec();

        REQUIRE( app.reconfigCalls == 1 );
    }

    SECTION( "a negative fps from the derived class disables the latency circ. buffs. and is logged" )
    {
        fgTestHook app;
        primeForSyncExec( app );
        app.m_shmimName      = uniqueShmimName( "hookbadfps" );
        app.m_circBuffLength = 1;
        app.fpsValue         = -1;
        app.acquireResults   = { 0 };

        app.fgThreadExec();

        REQUIRE( app.postPublishCalls == 1 );
    }

    SECTION( "an already-connected stream with matching geometry is reused with a hook present" )
    {
        fgTestHook  app;
        primeForSyncExec( app );
        std::string name = uniqueShmimName( "hookreuse" );
        app.m_shmimName  = name;

        tempStream ext( name, 3, 4, 4, 2, _DATATYPE_UINT8 );

        app.m_imageStream = reinterpret_cast<IMAGE *>( malloc( sizeof( IMAGE ) ) );
        REQUIRE( ImageStreamIO_openIm( app.m_imageStream, name.c_str() ) == 0 );
        app.m_ownShmim       = false;
        app.nextWidth        = 4;
        app.nextHeight       = 4;
        app.m_circBuffLength = 2;
        app.acquireResults   = { 0 };

        app.fgThreadExec();

        REQUIRE( app.postPublishCalls == 1 );
        REQUIRE( app.m_imageStream == nullptr );
    }

    SECTION( "an already-connected 2D stream with matching geometry is reused" )
    {
        fgTest      app;
        primeForSyncExec( app );
        std::string name = uniqueShmimName( "reuse2d" );
        app.m_shmimName  = name;

        tempStream ext( name, 2, 4, 4, _DATATYPE_UINT8 );

        // Attach an independent handle to the externally-owned shmim; frameGrabber's
        // cleanup will `free()` this handle struct itself once it closes it (as it does
        // for every m_imageStream, owned or not), so it must be its own malloc'd IMAGE,
        // separate from `ext`'s own handle which `ext`'s destructor will clean up.
        app.m_imageStream = reinterpret_cast<IMAGE *>( malloc( sizeof( IMAGE ) ) );
        REQUIRE( ImageStreamIO_openIm( app.m_imageStream, name.c_str() ) == 0 );
        app.m_ownShmim       = false;
        app.nextWidth        = 4;
        app.nextHeight       = 4;
        app.m_circBuffLength = 1;
        app.acquireResults   = { 0 };

        app.fgThreadExec();

        REQUIRE( app.loadImageIntoStreamCalls == 1 );
        REQUIRE( app.m_imageStream == nullptr ); // closed, not destroyed, since it was not owned
    }

    SECTION( "an already-connected 3D stream with matching geometry is reused" )
    {
        fgTest      app;
        primeForSyncExec( app );
        std::string name = uniqueShmimName( "reuse3d" );
        app.m_shmimName  = name;

        tempStream ext( name, 3, 4, 4, 2, _DATATYPE_UINT8 );

        app.m_imageStream = reinterpret_cast<IMAGE *>( malloc( sizeof( IMAGE ) ) );
        REQUIRE( ImageStreamIO_openIm( app.m_imageStream, name.c_str() ) == 0 );
        app.m_ownShmim       = false;
        app.nextWidth        = 4;
        app.nextHeight       = 4;
        app.m_circBuffLength = 2;
        app.acquireResults   = { 0 };

        app.fgThreadExec();

        REQUIRE( app.loadImageIntoStreamCalls == 1 );
    }

    SECTION( "an unowned stream with the wrong size is retried until the size matches" )
    {
        fgTest      app;
        primeForSyncExec( app );
        std::string name = uniqueShmimName( "wrongsize" );
        app.m_shmimName  = name;

        tempStream ext( name, 2, 4, 4, _DATATYPE_UINT8 );

        // On the first configureAcquisition() call, report a mismatched size so the
        // "wrong size" branch (and its one-time log) is exercised; flipping the
        // geometry mid-flight lets the retry converge and publish one frame.
        app.nextWidth  = 8;
        app.nextHeight = 8;
        app.m_imageStream = reinterpret_cast<IMAGE *>( malloc( sizeof( IMAGE ) ) );
        REQUIRE( ImageStreamIO_openIm( app.m_imageStream, name.c_str() ) == 0 );
        app.m_ownShmim    = false;
        app.m_circBuffLength = 1;
        app.acquireResults = { 0 };

        // Use a background thread here since the mismatch-retry path sleeps for a
        // full second and we want to flip the geometry to match mid-flight.
        std::thread runner( [&app]() { app.fgThreadExec(); } );

        // Give the mismatch branch a chance to log and sleep at least once.
        std::this_thread::sleep_for( std::chrono::milliseconds( 1300 ) );
        app.nextWidth  = 4;
        app.nextHeight = 4;

        runner.join();

        REQUIRE( app.loadImageIntoStreamCalls == 1 );
        REQUIRE( app.m_imageStream == nullptr );
    }

    SECTION( "the fps changing mid-acquisition reconfigures the latency circ. buffs." )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName              = uniqueShmimName( "fpschange" );
        app.m_circBuffLength         = 1;
        app.fpsValue                 = 100;
        app.m_latencyCircBuffMaxTime = 5;
        app.acquireResults           = { 0, 0 };

        // fps() is sampled into m_cbFPS once per outer-loop pass (in configCircBuffs()).
        // Changing it right after the first frame's acquireAndCheckValid() call makes
        // the end-of-iteration `m_cbFPS != fps()` check fire on that same pass.
        app.bumpFpsAfterCall = 1;
        app.bumpedFpsValue   = 50;

        app.fgThreadExec();

        REQUIRE( app.loadImageIntoStreamCalls == 2 );
        REQUIRE( app.m_cbFPS == 50 );
    }

    SECTION( "the fps changing to a negative value mid-acquisition reports the "
             "configCircBuffs() failure" )
    {
        fgTest app;
        primeForSyncExec( app );
        app.m_shmimName              = uniqueShmimName( "fpsnegative" );
        app.m_circBuffLength         = 1;
        app.fpsValue                 = 100;
        app.m_latencyCircBuffMaxTime = 5;
        app.acquireResults           = { 0, 0 };

        // Same mid-iteration fps-change mechanism as above, but to a negative value:
        // configCircBuffs() itself then returns -1 (only reachable via a real, negative
        // fps() reading, not a hand-enumerated error code).
        app.bumpFpsAfterCall = 1;
        app.bumpedFpsValue   = -1;

        app.fgThreadExec();

        REQUIRE( app.loadImageIntoStreamCalls == 2 );
        REQUIRE( app.m_cbFPS == -1 );
    }
}

/// appLogic() reports an error once the framegrabber thread has already exited.
/** This must be the last TEST_CASE in this file: `pthread_tryjoin_np` reaps the
 * native thread without clearing the owning `std::thread`, so the instance it is
 * exercised on is intentionally leaked rather than destructed (which would call
 * std::terminate on a still-"joinable" thread whose native handle is no longer
 * valid to join or detach). Since `MagAOXApp` is a process-wide singleton, no other
 * `MagAOXApp`-derived object may be constructed anywhere in this binary afterward.
 * This mirrors the same pattern used by
 * apps/ocam2KCtrl/tests/ocam2KCtrl_lifecycle_test.cpp.
 * \ingroup app_dev_unit_tests
 */
TEST_CASE( "frameGrabber appLogic reports an error once the framegrabber thread has exited", "[frameGrabber]" )
{
    auto *app = new fgTest;
    startDummyFgThread( *app, 1 );
    std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );

    REQUIRE( app->frameGrabberT::appLogic() == -1 );
}
