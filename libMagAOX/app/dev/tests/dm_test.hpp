
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
