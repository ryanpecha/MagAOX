
#include "../../MagAOXApp.hpp"
#include "../dm.hpp"
#include "../shmimMonitor.hpp"

// LCOV_EXCL_START

#ifndef XWCTEST_NAMESPACE
    #define MAPPNS MagAOX::app::dev
#else
    #define MAPPNS MagAOX::app::dev::XWCTEST_NAMESPACE

#endif

namespace dm_tests
{

#ifdef XWCTEST_NAMESPACE
namespace XWCTEST_NAMESPACE
{
#endif

/// Test harness for dev::dm
/**
 * \ingroup dm_tests
 */
struct dmTest : public MagAOX::app::MagAOXApp<false>,
                public MAPPNS::dm<dmTest, float>,
                public MAPPNS::shmimMonitor<dmTest>
{

    friend class MAPPNS::dm<dmTest, float>;

    typedef MAPPNS::dm<dmTest, float> dmT;

    dmTest( const std::string &git_sha1, const bool git_modified )
        : MagAOX::app::MagAOXApp<false>( git_sha1, git_modified )
    {
        m_configName  = "dmtest";
        m_calibDir    = "/tmp/dmtest_calibs";
        m_calibRelDir = "dmtest";
    }

    ~dmTest() noexcept
    {
    }

    // Constructs a real (but FIFO-less) indiDriver so m_indiDriver != nullptr, the same
    // pattern used in shmimMonitor_test.cpp/frameGrabber_test.cpp -- indi::updateIfChanged()
    // and friends catch their own send failures, so this doesn't need a live INDI server.
    void setConfigNameWithDriver( const std::string &cn )
    {
        m_configName = cn;
        m_indiDriver = new MagAOX::app::indiDriver<MagAOX::app::MagAOXApp<false>>( this, m_configName, "0", "0" );
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return dmT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return dmT::loadConfig( config );
    }

    int appStartup()
    {
        return dmT::appStartup();
    }

    int appLogic()
    {
        return dmT::appLogic();
    }

    int appShutdown()
    {
        return dmT::appShutdown();
    }

    int onPowerOff()
    {
        return dmT::onPowerOff();
    }

    int whilePowerOff()
    {
        return dmT::whilePowerOff();
    }

    int updateINDI()
    {
        return dmT::updateINDI();
    }

    int m_initDMRV{ 0 };
    int initDM()
    {
        return m_initDMRV;
    }

    int m_zeroDMRV{ 0 };
    int zeroDM()
    {
        return m_zeroDMRV;
    }

    int m_releaseDMRV{ 0 };
    int releaseDM()
    {
        return m_releaseDMRV;
    }

    void setSize( int w, int h, int d )
    {
        m_width    = w;
        m_height   = h;
        m_dataType = d;
    }

    /// Value written into m_instSatMap by commandDM(), settable by tests.
    uint8_t m_testSatValue{ 0 };

    /// If true, commandDM() returns -1 to exercise the processImage() error path.
    bool m_commandDMFail{ false };

    /// Required derivedT interface, called from dm<>::processImage(). Just sets the
    /// instantaneous saturation map to a test-controlled constant.
    int commandDM( void *curr_src )
    {
        static_cast<void>( curr_src );
        if( m_commandDMFail )
        {
            return -1;
        }
        if( m_instSatMap.rows() > 0 && m_instSatMap.cols() > 0 )
        {
            m_instSatMap.setConstant( m_testSatValue );
        }
        return 0;
    }

    // -- test-only accessors for otherwise-protected dm<> state --

    size_t numFlatCommands()
    {
        return m_flatCommands.size();
    }

    size_t numTestCommands()
    {
        return m_testCommands.size();
    }

    bool flatLoaded()
    {
        return m_flatLoaded;
    }

    bool testLoaded()
    {
        return m_testLoaded;
    }

    bool flatIsSet()
    {
        return m_flatSet;
    }

    bool testIsSet()
    {
        return m_testSet;
    }

    const std::string &flatCurrent()
    {
        return m_flatCurrent;
    }

    const std::string &testCurrent()
    {
        return m_testCurrent;
    }

    pcf::IndiProperty indiP_init()
    {
        return m_indiP_init;
    }

    pcf::IndiProperty indiP_zero()
    {
        return m_indiP_zero;
    }

    pcf::IndiProperty indiP_release()
    {
        return m_indiP_release;
    }

    pcf::IndiProperty indiP_zeroAll()
    {
        return m_indiP_zeroAll;
    }

    pcf::IndiProperty indiP_setFlat()
    {
        return m_indiP_setFlat;
    }

    pcf::IndiProperty indiP_setTest()
    {
        return m_indiP_setTest;
    }

    pcf::IndiProperty indiP_flats()
    {
        return m_indiP_flats;
    }

    pcf::IndiProperty indiP_tests()
    {
        return m_indiP_tests;
    }

    /// Build the request/toggle INDI properties normally created by appStartup(), without
    /// running the rest of appStartup() (thread creation, semaphores, etc). Lets tests drive
    /// the newCallBack_* functions directly.
    void prepIndiForCallbackTests()
    {
        createStandardIndiToggleSw( m_indiP_setFlat, "flat_set" );
        createStandardIndiToggleSw( m_indiP_setTest, "test_set" );
        createStandardIndiRequestSw( m_indiP_init, "initDM" );
        createStandardIndiRequestSw( m_indiP_zero, "zeroDM" );
        createStandardIndiRequestSw( m_indiP_release, "releaseDM" );
        createStandardIndiRequestSw( m_indiP_zeroAll, "zeroAll" );
    }

    void setAccumSatNonzero()
    {
        m_accumSatMap.setConstant( 3 );
        m_instSatMap.setConstant( 1 );
    }

    void requestShutdown()
    {
        m_shutdown = 1;
    }

    /// Exposes MagAOXApp<false>::setupBasicConfig()'s "power management enabled but
    /// _useINDI==false" critical-shutdown branch, which is otherwise unreachable from
    /// MagAOXApp_test.cpp (that harness only instantiates MagAOXApp<true>).
    void setPowerMgtEnabled( bool pme )
    {
        m_powerMgtEnabled = pme;
    }

    /// Exposes MagAOXApp<false>::startINDI()'s trivial-success branch when _useINDI is
    /// false, also otherwise unreachable from MagAOXApp_test.cpp.
    int startINDI()
    {
        return MagAOX::app::MagAOXApp<false>::startINDI();
    }

    /// Exposes MagAOXApp<false>::createINDIFIFOS()'s trivial-success branch, also
    /// otherwise unreachable from MagAOXApp_test.cpp.
    int callCreateINDIFIFOS()
    {
        return MagAOX::app::MagAOXApp<false>::createINDIFIFOS();
    }

    /// Exposes the MagAOXApp<false> (constexpr !m_useINDI) early-return branches of the
    /// INDI handle* callbacks, also otherwise unreachable from MagAOXApp_test.cpp (which
    /// only instantiates MagAOXApp<true>).
    void callHandleGetProperties( const pcf::IndiProperty &ipRecv )
    {
        MagAOX::app::MagAOXApp<false>::handleGetProperties( ipRecv );
    }

    void callHandleNewProperty( const pcf::IndiProperty &ipRecv )
    {
        MagAOX::app::MagAOXApp<false>::handleNewProperty( ipRecv );
    }

    void callHandleSetProperty( const pcf::IndiProperty &ipRecv )
    {
        MagAOX::app::MagAOXApp<false>::handleSetProperty( ipRecv );
    }

    /// Exposes the two registerIndiPropertyNew() overloads that build the property from
    /// type/perm/state (with and without an explicit switch rule) under MagAOXApp<false>,
    /// otherwise unreachable from MagAOXApp_test.cpp (only MagAOXApp<true> there, and
    /// neither overload is used by any dev:: mixin in this codebase).
    int callRegisterIndiPropertyNew( pcf::IndiProperty &prop, const std::string &propName )
    {
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyNew(
            prop, propName, pcf::IndiProperty::Number, pcf::IndiProperty::ReadWrite, pcf::IndiProperty::Idle, nullptr );
    }

    /// Exposes MagAOXApp<false>::sendNewStandardIndiToggle()'s constexpr !_useINDI
    /// trivial-success branch, also otherwise unreachable from MagAOXApp_test.cpp.
    int callSendNewStandardIndiToggle( const std::string &device, const std::string &property, bool onoff )
    {
        return MagAOX::app::MagAOXApp<false>::sendNewStandardIndiToggle( device, property, onoff );
    }

    int callRegisterIndiPropertyNewWithRule( pcf::IndiProperty &prop, const std::string &propName )
    {
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyNew( prop,
                                                                       propName,
                                                                       pcf::IndiProperty::Switch,
                                                                       pcf::IndiProperty::ReadWrite,
                                                                       pcf::IndiProperty::Idle,
                                                                       pcf::IndiProperty::OneOfMany,
                                                                       nullptr );
    }

    /// sem_init() the saturation semaphore without running the rest of appStartup(),
    /// so processImage()'s sem_post( &m_satSemaphore ) call is well-defined when tests
    /// drive processImage() directly.
    void initSatSemaphoreForTest()
    {
        sem_init( &m_satSemaphore, 0, 0 );
    }

    /// Direct control of the saturation thread's std::thread bookkeeping, mirroring
    /// shmimMonitor_test.cpp's setSmThread()/abandonSmThread() pattern, so appLogic()'s
    /// "saturation thread has exited" branch can be exercised without ever starting the
    /// real thread (avoiding the real-thread-teardown hazards noted in dm_test.cpp).
    void setSatThread( std::thread &&t )
    {
        m_satThread = std::move( t );
    }

    void abandonSatThread()
    {
        std::thread tmp;
        tmp.swap( m_satThread );
        new( &tmp ) std::thread();
    }
};

/// A minimal harness using an unsupported ImageStreamIO data type (int has no
/// ImageStreamTypeCode<> specialization, so m_dmDataType == 0), to exercise the
/// appStartup() unsupported-data-type error path.
/**
 * \ingroup dm_tests
 */
struct dmTestBadType : public MagAOX::app::MagAOXApp<false>,
                       public MAPPNS::dm<dmTestBadType, int>,
                       public MAPPNS::shmimMonitor<dmTestBadType>
{

    friend class MAPPNS::dm<dmTestBadType, int>;

    typedef MAPPNS::dm<dmTestBadType, int> dmT;

    dmTestBadType( const std::string &git_sha1, const bool git_modified )
        : MagAOX::app::MagAOXApp<false>( git_sha1, git_modified )
    {
        m_configName  = "dmtestbadtype";
        m_calibDir    = "/tmp/dmtestbadtype_calibs";
        m_calibRelDir = "dmtestbadtype";
    }

    ~dmTestBadType() noexcept
    {
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return dmT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return dmT::loadConfig( config );
    }

    int appStartup()
    {
        return dmT::appStartup();
    }

    int appLogic()
    {
        return 0;
    }

    int appShutdown()
    {
        return 0;
    }

    int initDM()
    {
        return 0;
    }

    int zeroDM()
    {
        return 0;
    }

    int releaseDM()
    {
        return 0;
    }

    int commandDM( void *curr_src )
    {
        static_cast<void>( curr_src );
        return 0;
    }
};

#ifdef XWCTEST_NAMESPACE
} // namespace XWCTEST_NAMESPACE
#endif

} // namespace dm_tests

// LCOV_EXCL_STOP
