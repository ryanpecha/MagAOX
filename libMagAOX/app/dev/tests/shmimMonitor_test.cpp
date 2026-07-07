//#define CATCH_CONFIG_MAIN
#include "../../../../tests/catch2/catch.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <new>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include <mx/improc/eigenImage.hpp>
#include <mx/improc/milkImage.hpp>
#include <mx/sys/timeUtils.hpp>

#include "../../MagAOXApp.hpp"
#include "../shmimMonitor.hpp"

using namespace MagAOX::app;

#ifndef XWCTEST_NAMESPACE
#define MAPPNS MagAOX::app::dev
#else
#define MAPPNS MagAOX::app::dev::XWCTEST_NAMESPACE
#endif

/** \defgroup shmimMonitor_tests libXWC::app::dev::shmimMonitor Unit Tests
 * \ingroup app_dev_unit_tests
 */
namespace shmimMonitor_tests
{

// A single shared memory directory for the whole test binary.  ImageStreamIO caches
// the shared memory directory the first time it is queried (static local in
// ImageStreamIO_shmdirname), so MILK_SHM_DIR must be set before any ImageStreamIO
// call happens anywhere in this process.  Doing this in a namespace-scope static
// initializer guarantees it runs before any TEST_CASE body (all global constructors
// run before main()).
const std::string g_shmDir = "/tmp/shmimMonitor_test/shm";

int setupShmDir()
{
    // Remove any shmim files left behind by a prior run of this binary -- stale
    // semaphores/inodes under the same names would otherwise make the tests
    // non-idempotent across repeated invocations.
    std::filesystem::remove_all( g_shmDir );
    mx::ioutils::createDirectories( g_shmDir );
    setenv( "MILK_SHM_DIR", g_shmDir.c_str(), 1 );
    return 0;
}
static int g_shmDirSetup = setupShmDir();

// A no-op handler for SIGUSR1 so that sending it to a thread interrupts a blocking
// syscall (EINTR) instead of the default action (which would terminate the process).
void noopUsr1Handler( int )
{
}

int setupUsr1Handler()
{
    struct sigaction act;
    memset( &act, 0, sizeof( act ) );
    act.sa_handler = &noopUsr1Handler;
    sigemptyset( &act.sa_mask );
    act.sa_flags = 0;
    sigaction( SIGUSR1, &act, 0 );
    return 0;
}
static int g_usr1Setup = setupUsr1Handler();

std::string shmimPath( const std::string &name )
{
    return g_shmDir + "/" + name + ".im.shm";
}

/// Directly create an IMAGE with full control over naxis/size/nbsem, bypassing both
/// milkImage (which always uses naxis=3) and shmimMonitor::create() (which also always
/// uses naxis=3), so that the naxis==1 and naxis==2 branches in shmimMonitor can be
/// exercised.
int rawCreate( IMAGE &image,
               const std::string &name,
               long naxis,
               uint32_t sz0,
               uint32_t sz1,
               uint32_t sz2,
               int nbsem = IMAGE_NB_SEMAPHORE )
{
    uint32_t imsize[3] = { sz0, sz1, sz2 };
    errno_t rv = ImageStreamIO_createIm_gpu(
        &image, name.c_str(), naxis, imsize, IMAGESTRUCT_FLOAT, -1, 1, nbsem, 0, CIRCULAR_BUFFER | ZAXIS_TEMPORAL, 0 );
    if( rv != IMAGESTREAMIO_SUCCESS )
        return -1;

    image.md->cnt1 = 0;
    return 0;
}

/// Write a constant-valued frame into slice `sliceIndex` of a (possibly circular
/// buffer) float image, bump cnt1 to that slice, and post all semaphores -- mimics
/// what a real upstream source process does on every new frame.
void writeFrame( IMAGE &image, uint32_t width, uint32_t height, uint32_t sliceIndex, float value )
{
    float *data = (float *)image.array.raw;
    size_t frameSize = (size_t)width * height;
    for( size_t i = 0; i < frameSize; ++i )
        data[sliceIndex * frameSize + i] = value;
    image.md->cnt1 = sliceIndex;
    ImageStreamIO_sempost( &image, -1 );
}

/// Mark every semaphore slot as already owned by another (always-alive) process, so
/// that ImageStreamIO_getsemwaitindex() can't find or adopt any of them.
void exhaustSemaphores( IMAGE &image )
{
    for( int i = 0; i < IMAGE_NB_SEMAPHORE; ++i )
        image.semReadPID[i] = 1; // pid 1 (init) is always alive
}

/// Test harness for dev::shmimMonitor
/**
 * \ingroup shmimMonitor_tests
 */
struct smTest : public MagAOX::app::MagAOXApp<false>, public MAPPNS::shmimMonitor<smTest>
{
    friend class MAPPNS::shmimMonitor<smTest>;

    typedef MAPPNS::shmimMonitor<smTest> shmimMonitorT;

    // ---- allocate()/processImage() instrumentation -------------------------------
    int m_allocateCount{ 0 };
    bool m_failAllocate{ false };
    bool m_mutateOnAllocate{ false }; ///< if true, mutate size[0] right after the *first* allocate() call

    std::atomic<int> m_processImageCount{ 0 };
    bool m_failProcessImage{ false };
    int m_mutateAtProcessCount{ -1 }; ///< if m_processImageCount == this value, mutate size[0] & repost

    std::vector<char> m_lastFrame;
    std::mutex m_frameMutex;

    // ---- appStartup() failure injection --------------------------------------------
    std::string m_failRegisterName; ///< if non-empty, registerIndiPropertyNew fails for this INDI property name
    bool m_failThreadStart{ false };

    smTest() : MagAOX::app::MagAOXApp<false>( "", false )
    {
        m_configName = "shmimMonitorTest";
    }

    ~smTest() noexcept
    {
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return shmimMonitorT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return shmimMonitorT::loadConfig( config );
    }

    int appStartup()
    {
        return shmimMonitorT::appStartup();
    }

    int appLogic()
    {
        return shmimMonitorT::appLogic();
    }

    int appShutdown()
    {
        return shmimMonitorT::appShutdown();
    }

    int allocate( const MAPPNS::shmimT & )
    {
        ++m_allocateCount;

        {
            std::lock_guard<std::mutex> lk( m_frameMutex );
            m_lastFrame.assign( (size_t)m_width * m_height * m_typeSize, 0 );
        }

        if( m_mutateOnAllocate && m_allocateCount == 1 )
        {
            m_imageStream.md[0].size[0] = m_width + 1000;
        }

        if( m_failAllocate )
            return -1;

        return 0;
    }

    int processImage( void *curr_src, const MAPPNS::shmimT & )
    {
        int n = ++m_processImageCount;

        if( n == m_mutateAtProcessCount )
        {
            m_imageStream.md[0].size[0] = m_width + 1000;
            ImageStreamIO_sempost( &m_imageStream, m_semaphoreNumber );
        }

        {
            std::lock_guard<std::mutex> lk( m_frameMutex );
            if( !m_lastFrame.empty() )
                memcpy( m_lastFrame.data(), curr_src, m_lastFrame.size() );
        }

        if( m_failProcessImage )
            return -1;

        return 0;
    }

    // ---- exposing protected shmimMonitor members/methods for testing -------------

    int doCreate( uint32_t w, uint32_t h, uint32_t d, uint8_t dt, void *initData = nullptr )
    {
        return shmimMonitorT::create( w, h, d, dt, initData );
    }

    void runSmThreadExec()
    {
        shmimMonitorT::smThreadExec();
    }

    void startMonitorThread()
    {
        m_smThreadInit = false; // skip the thread-priority-setup synchronizer wait
        m_smThread = std::thread( &smTest::runSmThreadExec, this );
    }

    bool smThreadJoinable()
    {
        return m_smThread.joinable();
    }

    void joinMonitorThread()
    {
        if( m_smThread.joinable() )
            m_smThread.join();
    }

    void killMonitorThread()
    {
        if( m_smThread.joinable() )
            pthread_kill( m_smThread.native_handle(), SIGUSR1 );
    }

    /// Safely abandon m_smThread's bookkeeping after its underlying OS thread has
    /// already been reaped by a raw pthread_tryjoin_np() call (as appLogic() does
    /// when it detects the thread has exited).  std::thread has no public API for
    /// this -- join()/detach() both require the OS-level thread to still be valid,
    /// and the destructor terminates the process if joinable() is (wrongly) still
    /// true.  swap() has no such check, so we swap the stale id into a throwaway
    /// local and reinitialize *that* via placement-new instead of ever invoking its
    /// destructor while it holds the stale id.
    void abandonSmThread()
    {
        std::thread tmp;
        tmp.swap( m_smThread );
        new( &tmp ) std::thread();
    }

    void setSmThread( std::thread &&t )
    {
        m_smThread = std::move( t );
    }

    void setRestart( bool r )
    {
        m_restart = r;
    }

    bool getRestart()
    {
        return m_restart;
    }

    MAPPNS::shmimMonitorState smState()
    {
        return m_smState;
    }

    void setShutdownFlag( int v )
    {
        m_shutdown = v;
    }

    void setShmimName( const std::string &name )
    {
        m_shmimName = name;
    }

    void corruptSemCount()
    {
        m_imageStream.md[0].sem = 0;
    }

    void setGetExistingFirst( bool b )
    {
        m_getExistingFirst = b;
    }

    // ---- appStartup() failure injection hooks (name-hides the MagAOXApp base) ----

    int registerIndiPropertyNew( pcf::IndiProperty &prop, int ( *callBack )( void *, const pcf::IndiProperty & ) )
    {
        if( !m_failRegisterName.empty() && prop.getName() == m_failRegisterName )
            return -1;

        return MagAOXApp<false>::registerIndiPropertyNew( prop, callBack );
    }

    template <class thisPtr, class Function>
    int threadStart( std::thread &thrd,
                      bool &thrdInit,
                      pid_t &tpid,
                      pcf::IndiProperty &thProp,
                      int thrdPrio,
                      const std::string &cpuset,
                      const std::string &thrdName,
                      thisPtr *thrdThis,
                      Function &&thrdStart )
    {
        if( m_failThreadStart )
            return -1;

        return MagAOXApp<false>::threadStart(
            thrd, thrdInit, tpid, thProp, thrdPrio, cpuset, thrdName, thrdThis, std::forward<Function>( thrdStart ) );
    }
};

/// RAII guard to make sure a test's background monitor thread never outlives the
/// harness object, even if a REQUIRE fails partway through a test (Catch2 unwinds the
/// stack on a failed REQUIRE, so this destructor still runs).  Without this, a stray
/// joinable std::thread member at harness-destruction time would call
/// std::terminate() and abort the whole test binary (and lose all coverage data).
struct ThreadGuard
{
    smTest &m_app;

    explicit ThreadGuard( smTest &app ) : m_app( app )
    {
    }

    ~ThreadGuard()
    {
        try
        {
            if( m_app.smThreadJoinable() )
            {
                m_app.setShutdownFlag( 1 );
                m_app.killMonitorThread();
                m_app.joinMonitorThread();
            }
        }
        catch( ... )
        {
        }
    }
};

/// Poll a condition until it is true or a timeout elapses.
template <typename Pred>
bool waitFor( Pred pred, int timeoutMs = 3000, int stepMs = 10 )
{
    int waited = 0;
    while( !pred() )
    {
        if( waited >= timeoutMs )
            return false;
        std::this_thread::sleep_for( std::chrono::milliseconds( stepMs ) );
        waited += stepMs;
    }
    return true;
}

} // namespace shmimMonitor_tests

using namespace shmimMonitor_tests;

/// shmimMonitor Configuration
/**
 * \ingroup shmimMonitor_tests
 */
SCENARIO( "shmimMonitor Configuration", "[dev::shmimMonitor]" )
{
    GIVEN( "a config file with no [shmimMonitor] section, loading defaults" )
    {
        mx::app::writeConfigFile( "/tmp/shmimMonitor_test.conf", { "none" }, { "nada" }, { "0" } );

        mx::app::appConfigurator config;

        smTest pdt;

        int rv;
        rv = pdt.setupConfig( config );
        REQUIRE( rv == 0 );

        config.readConfig( "/tmp/shmimMonitor_test.conf" );

        rv = pdt.loadConfig( config );
        REQUIRE( rv == 0 );

        // setupConfig sets m_shmimName to configName() by default
        REQUIRE( pdt.shmimName() == "shmimMonitorTest" );
        REQUIRE( pdt.width() == 0 );
        REQUIRE( pdt.height() == 0 );
        REQUIRE( pdt.depth() == 0 );
        REQUIRE( pdt.dataType() == 0 );
        REQUIRE( pdt.typeSize() == 0 );
    }

    GIVEN( "a config file with a [shmimMonitor] section changing everything" )
    {
        std::vector<std::string> s, k, v;

        s.push_back( "shmimMonitor" );
        k.push_back( "threadPrio" );
        v.push_back( "23" );

        s.push_back( "shmimMonitor" );
        k.push_back( "cpuset" );
        v.push_back( "myset" );

        s.push_back( "shmimMonitor" );
        k.push_back( "shmimName" );
        v.push_back( "customShmim" );

        s.push_back( "shmimMonitor" );
        k.push_back( "getExistingFirst" );
        v.push_back( "true" );

        mx::app::writeConfigFile( "/tmp/shmimMonitor_test.conf", s, k, v );

        mx::app::appConfigurator config;

        smTest pdt;

        int rv;
        rv = pdt.setupConfig( config );
        REQUIRE( rv == 0 );

        config.readConfig( "/tmp/shmimMonitor_test.conf" );

        rv = pdt.loadConfig( config );
        REQUIRE( rv == 0 );

        REQUIRE( pdt.shmimName() == "customShmim" );
    }
}

/// shmimMonitor appStartup, appLogic, and appShutdown
/**
 * \ingroup shmimMonitor_tests
 */
SCENARIO( "shmimMonitor appStartup, appLogic, appShutdown", "[dev::shmimMonitor]" )
{
    GIVEN( "a default-configured shmimMonitor" )
    {
        WHEN( "appStartup succeeds" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smStartupOk" );

            int rv = pdt.appStartup();
            REQUIRE( rv == 0 );
            REQUIRE( pdt.smThreadJoinable() == true );

            // The real monitor thread is now blocked waiting for state()==OPERATING
            // (never set here) -- appShutdown() only signals/joins, it does not set
            // the shutdown flag itself (that's the main app loop's job), so we must
            // set it first or the join below would hang forever.
            pdt.setShutdownFlag( 1 );

            rv = pdt.appShutdown();
            REQUIRE( rv == 0 );
            REQUIRE( pdt.smThreadJoinable() == false );

            // calling appShutdown again on an already-joined thread is a no-op
            rv = pdt.appShutdown();
            REQUIRE( rv == 0 );
        }

        WHEN( "registerIndiPropertyNew fails for the shmimName property" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smStartupFailReg1" );
            pdt.m_failRegisterName = "sm_shmimName";

            int rv = pdt.appStartup();
            REQUIRE( rv == -1 );
            REQUIRE( pdt.smThreadJoinable() == false );
        }

        WHEN( "registerIndiPropertyNew fails for the frameSize property" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smStartupFailReg2" );
            pdt.m_failRegisterName = "sm_frameSize";

            int rv = pdt.appStartup();
            REQUIRE( rv == -1 );
            REQUIRE( pdt.smThreadJoinable() == false );
        }

        WHEN( "threadStart fails" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smStartupFailThread" );
            pdt.m_failThreadStart = true;

            int rv = pdt.appStartup();
            REQUIRE( rv == -1 );
            REQUIRE( pdt.smThreadJoinable() == false );
        }
    }

    GIVEN( "appLogic is called directly against a controlled m_smThread" )
    {
        WHEN( "the thread is still running" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            std::atomic<bool> stayAlive{ true };
            pdt.setSmThread( std::thread(
                [&stayAlive]()
                {
                    while( stayAlive )
                        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
                } ) );

            int rv = pdt.appLogic();
            REQUIRE( rv == 0 );

            stayAlive = false;
            pdt.joinMonitorThread();
        }

        WHEN( "the thread has already exited" )
        {
            smTest pdt;

            pdt.setSmThread( std::thread( [](){} ) );

            // give the thread time to actually finish running (not just be started)
            REQUIRE( waitFor(
                [&pdt]()
                {
                    // appLogic itself performs the (destructive) tryjoin check, so we
                    // can't poll non-destructively here -- just give it a moment.
                    return true;
                },
                100 ) );
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

            int rv = pdt.appLogic();
            REQUIRE( rv == -1 );

            // appLogic()'s pthread_tryjoin_np() call has already reaped the OS thread;
            // std::thread doesn't know that, so we must neutralize it without a real
            // join()/detach() (see abandonSmThread doc).
            pdt.abandonSmThread();
            REQUIRE( pdt.smThreadJoinable() == false );
        }
    }

    GIVEN( "appShutdown is called directly" )
    {
        WHEN( "no thread was ever started" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            int rv = pdt.appShutdown();
            REQUIRE( rv == 0 );
        }

        WHEN( "a joinable thread is blocked and gets signaled" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            // Note: the thread must actually be blocked in pause() before
            // appShutdown() sends SIGUSR1 -- if the signal arrives while the thread
            // is still starting up (before it reaches pause()), it gets handled
            // (no-op) and consumed right there, and pause() then blocks forever with
            // no second signal ever coming.  Synchronize on a flag set immediately
            // before the pause() call to make that race negligible.
            std::atomic<bool> aboutToPause{ false };
            pdt.setSmThread( std::thread(
                [&aboutToPause]()
                {
                    aboutToPause = true;
                    pause();
                } ) );
            REQUIRE( waitFor( [&aboutToPause]() { return aboutToPause.load(); }, 1000 ) );
            std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

            int rv = pdt.appShutdown();
            REQUIRE( rv == 0 );
            REQUIRE( pdt.smThreadJoinable() == false );
        }
    }
}

/// shmimMonitor::create()
/**
 * \ingroup shmimMonitor_tests
 */
SCENARIO( "shmimMonitor create()", "[dev::shmimMonitor]" )
{
    GIVEN( "a shmimMonitor configured with a shmim name" )
    {
        smTest pdt;
        pdt.setShmimName( "smCreateTest" );

        WHEN( "creating a fresh image with no initial data" )
        {
            int rv = pdt.doCreate( 12, 8, 3, IMAGESTRUCT_FLOAT );
            REQUIRE( rv == 0 );

            mx::improc::milkImage<float> chk;
            bool opened = false;
            try
            {
                chk.open( "smCreateTest" );
                opened = true;
            }
            catch( ... )
            {
            }
            REQUIRE( opened == true );
            REQUIRE( chk.rows() == 12 );
            REQUIRE( chk.cols() == 8 );
        }

        WHEN( "creating with initial data" )
        {
            std::vector<float> initData( 12 * 8, 3.5f );
            int rv = pdt.doCreate( 12, 8, 3, IMAGESTRUCT_FLOAT, initData.data() );
            REQUIRE( rv == 0 );

            mx::improc::milkImage<float> chk;
            chk.open( "smCreateTest" );
            REQUIRE( chk( 0, 0 ) == 3.5f );
        }

        WHEN( "creating again over an existing image (destroy-then-recreate path)" )
        {
            int rv = pdt.doCreate( 12, 8, 3, IMAGESTRUCT_FLOAT );
            REQUIRE( rv == 0 );

            rv = pdt.doCreate( 20, 5, 2, IMAGESTRUCT_FLOAT );
            REQUIRE( rv == 0 );

            mx::improc::milkImage<float> chk;
            chk.open( "smCreateTest" );
            REQUIRE( chk.rows() == 20 );
            REQUIRE( chk.cols() == 5 );
        }

        WHEN( "the existing file can't be opened by ImageStreamIO (corrupt)" )
        {
            // Put a too-small junk file where the shmim would be -- raw open()
            // succeeds (it's a real file) but ImageStreamIO_openIm fails because
            // the file is smaller than IMAGE_METADATA.
            std::string path = shmimPath( "smCreateCorrupt" );
            FILE *f = fopen( path.c_str(), "w" );
            REQUIRE( f != nullptr );
            fputs( "x", f );
            fclose( f );

            pdt.setShmimName( "smCreateCorrupt" );
            int rv = pdt.doCreate( 4, 4, 2, IMAGESTRUCT_FLOAT );
            REQUIRE( rv == -1 );
        }

        WHEN( "ImageStreamIO_createIm_gpu itself fails (invalid datatype code)" )
        {
            pdt.setShmimName( "smCreateBadType" );
            int rv = pdt.doCreate( 4, 4, 2, 255 ); // 255 is not a valid ImageStreamIO datatype code
            REQUIRE( rv == -1 );
        }
    }
}

/// shmimMonitor::updateINDI()
/**
 * \ingroup shmimMonitor_tests
 */
SCENARIO( "shmimMonitor updateINDI", "[dev::shmimMonitor]" )
{
    GIVEN( "a shmimMonitor with no INDI driver connected (MagAOXApp<false>)" )
    {
        smTest pdt;

        int rv = pdt.updateINDI();
        REQUIRE( rv == 0 );

        // NOTE: the branch where derived().m_indiDriver is non-null (actually
        // publishing values via indi::updateIfChanged) is not covered here: it
        // requires a live, connected indiDriver<MagAOXApp> instance (real FIFO-based
        // IPC), which is out of scope for a focused unit test of shmimMonitor and
        // risks flakiness/hangs without a real INDI server on the other end.
    }
}

/// shmimMonitor's monitoring thread (smThreadExec), driven against real
/// ImageStreamIO shared memory segments.
/**
 * \ingroup shmimMonitor_tests
 */
SCENARIO( "shmimMonitor thread lifecycle", "[dev::shmimMonitor]" )
{
    GIVEN( "a shmimMonitor waiting for a shmim that does not exist yet" )
    {
        WHEN( "the shmim appears, frames are processed, and shutdown is clean via EINTR" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeA" );
            pdt.state( stateCodes::OPERATING );
            pdt.startMonitorThread();

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::notfound; } ) );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeA", 3, 6, 4, 2 ) == 0 ); // naxis=3, 6x4, depth 2

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_allocateCount == 1; } ) );
            REQUIRE( pdt.width() == 6 );
            REQUIRE( pdt.height() == 4 );
            REQUIRE( pdt.depth() == 2 );

            writeFrame( img, 6, 4, 0, 42.0f );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() == 1; } ) );

            // Clean shutdown: SIGUSR1 interrupts the blocked sem_timedwait (EINTR path).
            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            REQUIRE( pdt.smThreadJoinable() == false );

            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "images with different axis counts" )
    {
        WHEN( "naxis==1: height and depth both collapse to 1" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeNaxis1" );
            pdt.state( stateCodes::OPERATING );
            pdt.setGetExistingFirst( true );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeNaxis1", 1, 10, 0, 0 ) == 0 );
            writeFrame( img, 10, 1, 0, 1.0f ); // pre-existing frame

            pdt.startMonitorThread();

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );
            REQUIRE( pdt.height() == 1 );
            REQUIRE( pdt.depth() == 1 );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() >= 1; } ) );

            writeFrame( img, 10, 1, 0, 2.0f ); // live frame through the main loop
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() >= 2; } ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }

        WHEN( "naxis==2: depth collapses to 1" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeNaxis2" );
            pdt.state( stateCodes::OPERATING );
            pdt.setGetExistingFirst( true );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeNaxis2", 2, 8, 5, 0 ) == 0 );
            writeFrame( img, 8, 5, 0, 1.0f );

            pdt.startMonitorThread();

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );
            REQUIRE( pdt.width() == 8 );
            REQUIRE( pdt.height() == 5 );
            REQUIRE( pdt.depth() == 1 );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() >= 1; } ) );

            writeFrame( img, 8, 5, 0, 2.0f );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() >= 2; } ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a corrupt file where the shmim should be" )
    {
        WHEN( "ImageStreamIO_openIm fails and retries until a valid image replaces it" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeCorrupt" );
            pdt.state( stateCodes::OPERATING );

            std::string path = shmimPath( "smLifeCorrupt" );
            FILE *f = fopen( path.c_str(), "w" );
            REQUIRE( f != nullptr );
            fputs( "x", f );
            fclose( f );

            pdt.startMonitorThread();

            // give it time to hit the "raw open() ok, ImageStreamIO_openIm fails" retry
            std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
            REQUIRE( pdt.smState() != MAPPNS::shmimMonitorState::connected );

            remove( path.c_str() );
            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeCorrupt", 3, 4, 4, 1 ) == 0 );

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; }, 5000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "an image created with too few semaphores" )
    {
        WHEN( "it retries until destroyed and recreated with a valid semaphore count" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeSemWait" );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeSemWait", 3, 4, 4, 1, 1 ) == 0 ); // nbsem=1 < SEMAPHORE_MAXVAL

            pdt.startMonitorThread();

            std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
            REQUIRE( pdt.smState() != MAPPNS::shmimMonitorState::connected );

            ImageStreamIO_destroyIm( &img );
            REQUIRE( rawCreate( img, "smLifeSemWait", 3, 4, 4, 1 ) == 0 ); // default nbsem == IMAGE_NB_SEMAPHORE

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; }, 5000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a shmimMonitor searching for a shmim that never appears" )
    {
        WHEN( "m_restart interrupts the search, then a state change interrupts it again" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeRestartSearch" );
            pdt.state( stateCodes::OPERATING );
            pdt.startMonitorThread();

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::notfound; } ) );

            // (1) trip "if (m_restart) continue;" while searching
            pdt.setRestart( true );
            REQUIRE( waitFor( [&pdt]() { return pdt.getRestart() == false; } ) ); // reset at top of next outer pass

            // (2) trip "if (state()!=target) continue;" while searching
            pdt.state( stateCodes::READY );
            std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeRestartSearch", 3, 4, 4, 1 ) == 0 );
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; }, 5000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }

        WHEN( "shutdown is requested while waiting for the target state" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeShutdownWait" );
            pdt.state( stateCodes::READY ); // not the target -- spins in the "wait for state" loop
            pdt.startMonitorThread();

            std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            REQUIRE( pdt.smThreadJoinable() == false );
        }

        WHEN( "shutdown is requested while searching for the shmim (never opened)" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeShutdownSearch" );
            pdt.state( stateCodes::OPERATING );
            pdt.startMonitorThread();

            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::notfound; } ) );
            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            REQUIRE( pdt.smThreadJoinable() == false );
        }
    }

    GIVEN( "a connectable shmim and a derived class that fails allocate()" )
    {
        WHEN( "allocate() failing ends the monitoring thread" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeAllocFail" );
            pdt.state( stateCodes::OPERATING );
            pdt.m_failAllocate = true;

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeAllocFail", 3, 4, 4, 1 ) == 0 );

            pdt.startMonitorThread();

            REQUIRE( waitFor( [&pdt]() { return pdt.m_allocateCount == 1; } ) );
            pdt.joinMonitorThread(); // allocate() failure breaks out and the thread ends on its own
            REQUIRE( pdt.smThreadJoinable() == false );

            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a connectable shmim and a derived class that fails processImage()" )
    {
        WHEN( "processImage() failing just logs and the loop keeps running" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeProcFail" );
            pdt.state( stateCodes::OPERATING );
            pdt.m_failProcessImage = true;

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeProcFail", 3, 4, 4, 2 ) == 0 );

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );

            writeFrame( img, 4, 4, 0, 1.0f );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() == 1; } ) );

            writeFrame( img, 4, 4, 1, 2.0f );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() == 2; } ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }

        WHEN( "processImage() failing on the getExistingFirst frame just logs" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeProcFailExisting" );
            pdt.state( stateCodes::OPERATING );
            pdt.setGetExistingFirst( true );
            pdt.m_failProcessImage = true;

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeProcFailExisting", 3, 4, 4, 2 ) == 0 );
            writeFrame( img, 4, 4, 0, 1.0f ); // pre-existing frame, delivered via getExistingFirst

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() == 1; } ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a connected shmim and a pending shutdown request" )
    {
        WHEN( "shutdown is noticed right after a successful semaphore wait, before processImage()" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifePreProcessBreak" );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifePreProcessBreak", 3, 4, 4, 2 ) == 0 );

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );

            writeFrame( img, 4, 4, 0, 1.0f );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() == 1; } ) );

            // Request shutdown, then deliver one more frame: sem_timedwait succeeds
            // and the size check passes, but the shutdown/restart/state check right
            // after it now trips, breaking out *before* processImage() is called.
            pdt.setShutdownFlag( 1 );
            writeFrame( img, 4, 4, 1, 2.0f );

            pdt.joinMonitorThread();
            REQUIRE( pdt.m_processImageCount.load() == 1 ); // the 2nd frame was never handed to processImage()

            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a connected shmim whose metadata is mutated mid-stream" )
    {
        WHEN( "a size mismatch detected in the main loop triggers a full reconnect" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeMismatch" );
            pdt.state( stateCodes::OPERATING );
            pdt.m_mutateAtProcessCount = 1; // mutate + repost right after the 1st processImage() call

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeMismatch", 3, 4, 4, 2 ) == 0 );

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );

            writeFrame( img, 4, 4, 0, 1.0f );
            REQUIRE( waitFor( [&pdt]() { return pdt.m_allocateCount == 2; }, 5000 ) ); // mismatch => reconnect

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }

        WHEN( "getExistingFirst reads a stale frame whose metadata was mutated during allocate()" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeExistingMismatch" );
            pdt.state( stateCodes::OPERATING );
            pdt.setGetExistingFirst( true );
            pdt.m_mutateOnAllocate = true;

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeExistingMismatch", 3, 4, 4, 2 ) == 0 );
            writeFrame( img, 4, 4, 0, 9.0f ); // pre-existing frame before the monitor ever starts

            pdt.startMonitorThread();

            // 1st allocate() mutates size[0] -> getExistingFirst mismatch -> continue -> reconnect -> 2nd allocate()
            REQUIRE( waitFor( [&pdt]() { return pdt.m_allocateCount >= 2; }, 5000 ) );
            // after reconnecting with consistent metadata, the still-present frame is delivered
            REQUIRE( waitFor( [&pdt]() { return pdt.m_processImageCount.load() >= 1; }, 3000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a connected shmim whose source cleans up" )
    {
        WHEN( "sem<=0 triggers a break, and reconnecting waits for a fresh, valid image" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeSemZero" );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeSemZero", 3, 4, 4, 1 ) == 0 );

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );

            pdt.corruptSemCount(); // simulates the source process having cleaned up

            // wait out the ~1s sem_timedwait timeout so the "sem<=0" break is hit
            std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );

            // the reconnect attempt now finds sem < SEMAPHORE_MAXVAL forever (that
            // field can't be fixed in place) -- destroy and recreate properly.
            ImageStreamIO_destroyIm( &img );
            REQUIRE( rawCreate( img, "smLifeSemZero", 3, 4, 4, 1 ) == 0 );

            REQUIRE( waitFor(
                [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected && pdt.m_allocateCount == 2; },
                5000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "a connected shmim that disappears or is replaced" )
    {
        WHEN( "the shmim file is deleted mid-connection" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeFileGone" );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeFileGone", 3, 4, 4, 1 ) == 0 );

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );

            remove( shmimPath( "smLifeFileGone" ).c_str() );

            // wait out the ~1s sem_timedwait timeout for restart-detection to trip
            std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );
            REQUIRE( pdt.smState() != MAPPNS::shmimMonitorState::connected );

            REQUIRE( rawCreate( img, "smLifeFileGone", 3, 4, 4, 1 ) == 0 );
            REQUIRE( waitFor(
                [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected && pdt.m_allocateCount == 2; },
                5000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }

        WHEN( "the shmim is replaced with a new inode under the same name" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeInodeChange" );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeInodeChange", 3, 4, 4, 1 ) == 0 );

            pdt.startMonitorThread();
            REQUIRE( waitFor( [&pdt]() { return pdt.smState() == MAPPNS::shmimMonitorState::connected; } ) );

            // Replace the file with a fresh one (new inode) right away, so that by
            // the time the ~1s timeout check runs, both open() and stat() succeed
            // against the new file -- isolating the inode-only restart branch from
            // the file-missing one.
            ImageStreamIO_destroyIm( &img );
            REQUIRE( rawCreate( img, "smLifeInodeChange", 3, 4, 4, 1 ) == 0 );

            REQUIRE( waitFor( [&pdt]() { return pdt.m_allocateCount == 2; }, 5000 ) );

            pdt.setShutdownFlag( 1 );
            pdt.killMonitorThread();
            pdt.joinMonitorThread();
            ImageStreamIO_closeIm( &img );
        }
    }

    GIVEN( "an image whose semaphores are all held by another process" )
    {
        WHEN( "getsemwaitindex finds none available, and the thread exits" )
        {
            smTest pdt;
            ThreadGuard guard( pdt );

            pdt.setShmimName( "smLifeNoSem" );
            pdt.state( stateCodes::OPERATING );

            IMAGE img;
            REQUIRE( rawCreate( img, "smLifeNoSem", 3, 4, 4, 1 ) == 0 );
            exhaustSemaphores( img );

            pdt.startMonitorThread();
            pdt.joinMonitorThread(); // getsemwaitindex fails -> logs critical -> returns on its own
            REQUIRE( pdt.smThreadJoinable() == false );
            REQUIRE( pdt.smState() != MAPPNS::shmimMonitorState::connected );

            ImageStreamIO_closeIm( &img );
        }
    }
}
