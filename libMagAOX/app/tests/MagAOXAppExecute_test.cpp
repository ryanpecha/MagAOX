// #define CATCH_CONFIG_MAIN
#include "../../../tests/catch2/catch.hpp"

#include <filesystem>

#include <mx/sys/timeUtils.hpp>

#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_PID_LOCKED_ns
#define XWCTEST_MAGAOXAPP_PID_LOCKED
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_PID_LOCKED

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_PID_WRITE_FAIL_ns
#define XWCTEST_MAGAOXAPP_PID_WRITE_FAIL
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_PID_WRITE_FAIL

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_NORM_ns
#define XWCTEST_MAGAOXAPP_EXEC_NORM
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_NORM

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_WRONG_USER_ns
#define XWCTEST_MAGAOXAPP_EXEC_WRONG_USER
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_WRONG_USER

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_LOG_START_ns
#define XWCTEST_MAGAOXAPP_EXEC_LOG_START
#define XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_LOG_START
#undef XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR


#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_SIGTERM_ns
#define XWCTEST_MAGAOXAPP_SIGTERMH_ERR
#define XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_SIGTERMH_ERR
#undef XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR


#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_NORM_PID_UNLOCK_ns
#define XWCTEST_MAGAOXAPP_EXEC_NORM
#define XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_NORM
#undef XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_NORM_APPSTARTUP_ns
#define XWCTEST_MAGAOXAPP_EXEC_NORM
#define XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_NORM
#undef XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_NORM_APPLOGIC_ns
#define XWCTEST_MAGAOXAPP_EXEC_NORM
#define XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_NORM
#undef XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_NORM_APPSHUTDOWN_ns
#define XWCTEST_MAGAOXAPP_EXEC_NORM
#define XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_NORM
#undef XWCTEST_MAGAOXAPP_PID_UNLOCK_ERR

// NOTE: MagAOXApp's log manager (m_log) is a per-type *static* singleton, and once its
// background thread is started via logThreadStart() it is never joined/detached (by design --
// a real app's log thread runs for the life of the process). Since std::thread's move-assign
// operator calls std::terminate() if the target is already joinable, any XWCTEST_NAMESPACE type
// whose execute() call gets far enough to call logThreadStart() successfully may only be used
// ONCE, in ONE test, for the life of this binary. That's why so many near-identical namespaces
// exist below -- each test that reaches the log thread start needs its own dedicated type.

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_INDIFAIL_ns
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_WHILEPOWEROFF_ns
#define XWCTEST_MAGAOXAPP_EXEC_NORM
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_EXEC_NORM

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_EXEC_STALLED_ns
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE

#undef app_MagAOXApp_hpp
#undef app_tests_MagAOXApp_test_hpp
#define XWCTEST_NAMESPACE XWCTEST_MAGAOXAPP_SIGTERMH_SIGINT_ns
#define XWCTEST_MAGAOXAPP_SIGTERMH_SIGINT
#include "../MagAOXApp.hpp"
#include "MagAOXApp_test.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_MAGAOXAPP_SIGTERMH_SIGINT
// NOTE: as in MagAOXApp_test.cpp, XWCTEST_MAGAOXAPP_SIGTERMH_SIGINT redefines the SIGINT macro
// to SIGKILL inside setSigTermHandler(), and the header never restores it. That leaks for the
// rest of this translation unit, so only one of the SIGTERMH_SIGTERM/SIGQUIT/SIGINT hooks may
// be used per .cpp file. MagAOXApp_test.cpp already exercises SIGQUIT's branch this way; this
// file independently exercises SIGINT's branch since it's a separate translation unit.


namespace libXWCTest
{
namespace appTest
{
namespace MagAOXAppTest
{

/// running execute
/**
 * \ingroup MagAOXApp_unit_test
 */
TEST_CASE( "running execute", "[app::MagAOXApp]" )
{
    SECTION( "complete run-through" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_NORM_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == 0 );
    }

    SECTION( "No log directory" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        //don't create logs so the user check fails
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "wrong user" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_WRONG_USER_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "PID Lock Error" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_PID_WRITE_FAIL_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "Log fails to start" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_LOG_START_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "Setting sigterm handler fails" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_SIGTERM_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "appStartup failure" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_NORM_APPSTARTUP_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        app.appStartupFail = true;

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "appLogic failure" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_NORM_APPLOGIC_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );
        app.appLogicFail = true;
        int rv = app.execute(); // appLogic() failure just triggers shutdown; execute() still returns 0
        REQUIRE( rv == 0 );
    }

    SECTION( "appShutdown failure" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_NORM_APPSHUTDOWN_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );
        app.appShutdownFail = true;
        int rv = app.execute();//this still returns 0
        REQUIRE( rv == 0 );
    }

    SECTION( "INDI fails to start" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        // Deliberately do NOT create drivers/fifos, so mkfifo() inside createINDIFIFOS() fails
        // with ENOENT (missing parent directory), which makes startINDI() -- and therefore
        // execute() -- fail.

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        XWCTEST_MAGAOXAPP_EXEC_INDIFAIL_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];
        app.appShutdownFail = true; // also exercises the "error from appShutdown()" branch taken after INDI failure

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == -1 );
    }

    SECTION( "whilePowerOff failure in main loop" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        // EXEC_NORM forces the power state to 0 (off) at startup (skipping the real wait) and
        // limits the main loop to a couple of iterations, so whilePowerOff() runs for real
        // while powered off.
        XWCTEST_MAGAOXAPP_EXEC_WHILEPOWEROFF_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];
        app.whilePowerOffFail = true;

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute(); // whilePowerOff() failure just triggers shutdown; execute() still returns 0
        REQUIRE( rv == 0 );
    }

    SECTION( "stalled waiting for power state, then onPowerOff fails" )
    {
        std::vector<const char *> argv;
        std::vector<std::string>  argvstr( { "./execname", "-n", "testapp" } );

        argv.resize( argvstr.size() + 1, NULL );
        for( size_t index = 0; index < argvstr.size(); ++index )
        {
            argv[index] = argvstr[index].c_str();
        }

        char ppath[1024];
        snprintf( ppath, sizeof( ppath ), "%s=/tmp/MagAOXApp_test", MAGAOX_env_path );
        putenv( ppath );

        // First delete the directory and files in case this is a repeat call
        std::filesystem::remove_all( "/tmp/MagAOXApp_test" );

        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/config" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/logs" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/sys/testapp" );
        mx::ioutils::createDirectories( "/tmp/MagAOXApp_test/drivers/fifos" );

        std::ofstream fout;
        fout.open( "/tmp/MagAOXApp_test/config/magaox.conf" );
        fout.close();

        mx::app::writeConfigFile( "/tmp/MagAOXApp_test/config/testapp.conf",
                                  { "", "power", "power", "power", "power", "power" },
                                  { "loopPause", "device", "channel", "element", "targetElement", "powerOnWait" },
                                  { "2500", "pdu9", "thisch", "thisel", "thistgtel", "5" } );

        // No fault-injection toggle forces m_powerState here, so it stays at its startup
        // value of -1 (unknown) for real. execute() blocks in a real sleep(1) loop for 30
        // real seconds until it gives up ("stalled waiting for power state"), then falls
        // through to the onPowerOff() call, which we make fail too.
        XWCTEST_MAGAOXAPP_EXEC_STALLED_ns::MagAOXApp_test app( false );
        app.setPowerMgtEnabled( true );
        app.invokedName() = argv[0];
        app.onPowerOffFail = true;

        app.setup( argv.size() - 1, const_cast<char **>( argv.data() ) );

        int rv = app.execute();
        REQUIRE( rv == 0 );
    }
}

/// setSigTermHandler failing on the SIGINT sigaction call specifically
/**
 * \ingroup MagAOXApp_unit_test
 */
TEST_CASE( "Signal handler setup failure for SIGINT", "[app::MagAOXApp]" )
{
    // NOTE: see the comment near this hook's #include block at the top of this file --
    // XWCTEST_MAGAOXAPP_SIGTERMH_SIGINT's redefinition of SIGINT to SIGKILL leaks for the rest
    // of this translation unit, so it must be the only SIGTERMH_SIGTERM/SIGQUIT/SIGINT hook
    // used here.
    XWCTEST_MAGAOXAPP_SIGTERMH_SIGINT_ns::MagAOXApp_test app;

    REQUIRE( app.setSigTermHandler() == -1 );
}


} // namespace MagAOXAppTest
} // namespace appTest
} // namespace libXWCTest
