// #define CATCH_CONFIG_MAIN
#include "../../../tests/catch2/catch.hpp"

#include <filesystem>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Enables the xwcTestHooks member on MagAOX::app::indiDriver (see
// indiDriver.hpp), which lets us reach a handful of branches that can't be
// triggered by any real, reproducible external condition (e.g. a FIFO write
// failing right after the FIFO opened successfully, or the outgoing INDI
// client throwing/disconnecting at one very specific instant). The hooks are
// runtime bools (default false, i.e. normal behavior), so this is a single
// ordinary compile of indiDriver.hpp/MagAOXApp.hpp -- not the
// multiply-recompiled-per-namespace XWCTEST_NAMESPACE pattern used elsewhere,
// which would otherwise multiply this file's line count in the coverage
// report once per variant.
#define XWCTEST_INDIDRIVER_HOOKS

// indiDriver.hpp and MagAOXApp.hpp include each other (indiDriver.hpp needs
// MagAOXApp's logger types, MagAOXApp needs indiDriver<MagAOXApp> as a
// member type). MagAOXApp.hpp must be included first so that by the time it
// reaches its own member declaration of indiDriver<MagAOXApp>, indiDriver.hpp
// (pulled in from within MagAOXApp.hpp) has already been fully processed.
#include "../MagAOXApp.hpp"
#include "../indiDriver.hpp"
#include "indiDriver_test.hpp"

using namespace indiDriver_tests;

namespace
{

/// Make sure a fresh FIFO exists at path, removing anything already there first.
void makeFreshFifo( const std::string &path )
{
    std::filesystem::remove( path );
    REQUIRE( ::mkfifo( path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ) == 0 );
}

/// Build a simple, well-formed Number property with one element, suitable
/// for use as a NEW property message (i.e. not of type Unknown).
pcf::IndiProperty makeNumberProperty( const std::string &device, const std::string &name )
{
    pcf::IndiProperty ip( pcf::IndiProperty::Number );
    ip.setDevice( device );
    ip.setName( name );
    ip.add( pcf::IndiElement( "value" ) );
    ip["value"].setValue( 1.0 );
    return ip;
}

/// A minimal TCP listener on 127.0.0.1, used only so that IndiClient's
/// connect() call succeeds (a real INDI server on the other end is not
/// required -- accept() is never called, the completed TCP handshake sitting
/// in the kernel's backlog is enough for connect() to return success).
struct TcpListener
{
    int fd{ -1 };
    int port{ 0 };

    TcpListener()
    {
        fd = ::socket( AF_INET, SOCK_STREAM, 0 );
        REQUIRE( fd >= 0 );

        int opt = 1;
        ::setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) );

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
        addr.sin_port        = 0; // let the OS choose a free port

        REQUIRE( ::bind( fd, reinterpret_cast<sockaddr *>( &addr ), sizeof( addr ) ) == 0 );
        REQUIRE( ::listen( fd, 8 ) == 0 );

        socklen_t len = sizeof( addr );
        REQUIRE( ::getsockname( fd, reinterpret_cast<sockaddr *>( &addr ), &len ) == 0 );
        port = ntohs( addr.sin_port );
    }

    ~TcpListener()
    {
        if( fd >= 0 )
        {
            ::close( fd );
        }
    }
};

/// RAII helper which sets a bool to true for the life of the guard, and
/// restores it to false on scope exit (even if a REQUIRE throws), so that
/// one test's fault-injection hook can never leak into another test.
struct HookGuard
{
    bool &flag;

    explicit HookGuard( bool &f ) : flag( f )
    {
        flag = true;
    }

    ~HookGuard()
    {
        flag = false;
    }
};

} // namespace

/** \defgroup indiDriver_tests libXWC::app::indiDriver Unit Tests
 * \ingroup app_unit_test
 */

/// Test construction of indiDriver, including all the FIFO-related failure modes.
/**
 * \ingroup indiDriver_tests
 */
SCENARIO( "Constructing an indiDriver", "[app::indiDriver]" )
{
    std::filesystem::create_directories( "/tmp/indiDriver_test" );

    GIVEN( "all three FIFOs can be opened and written to" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/good.in" );
        makeFreshFifo( "/tmp/indiDriver_test/good.out" );
        makeFreshFifo( "/tmp/indiDriver_test/good.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/good.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/good.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/good.ctrl";

        WHEN( "constructed" )
        {
            indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );

            THEN( "good() is true" )
            {
                REQUIRE( drv.good() == true );
            }
        }
    }

    GIVEN( "the input FIFO can not be opened" )
    {
        indiDriverTestParent parent;
        parent.m_driverInName   = "/tmp/indiDriver_test/does_not_exist/bad.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/unused.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/unused.ctrl";

        WHEN( "constructed" )
        {
            indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );

            THEN( "good() is false" )
            {
                REQUIRE( drv.good() == false );
            }
        }
    }

    GIVEN( "the input FIFO opens but the output FIFO can not be opened" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/in2.in" );
        parent.m_driverInName   = "/tmp/indiDriver_test/in2.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/does_not_exist/bad.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/unused2.ctrl";

        WHEN( "constructed" )
        {
            indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );

            THEN( "good() is false" )
            {
                REQUIRE( drv.good() == false );
            }
        }
    }

    GIVEN( "input and output FIFOs open but the ctrl FIFO can not be opened" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/in3.in" );
        makeFreshFifo( "/tmp/indiDriver_test/out3.out" );
        parent.m_driverInName   = "/tmp/indiDriver_test/in3.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/out3.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/does_not_exist/bad.ctrl";

        WHEN( "constructed" )
        {
            indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );

            THEN( "good() is false" )
            {
                REQUIRE( drv.good() == false );
            }
        }
    }

    GIVEN( "all three FIFOs open but the write to the ctrl FIFO fails" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/in4.in" );
        makeFreshFifo( "/tmp/indiDriver_test/out4.out" );
        makeFreshFifo( "/tmp/indiDriver_test/ctrl4.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/in4.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/out4.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/ctrl4.ctrl";

        WHEN( "constructed with the ctrl-write-fail hook engaged" )
        {
            // The hook is a static member (see indiDriver.hpp), so it can be
            // armed before the object (and thus its constructor) exists.
            HookGuard guard( indiDriverExposed<indiDriverTestParent>::xwcTestHooks.forceCtrlWriteFail );

            indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );

            THEN( "good() is false" )
            {
                REQUIRE( drv.good() == false );
            }
        }
    }
}

/// Test that the handleXXXProperty overrides forward to the parent, and are
/// safe no-ops when the parent has been cleared.
/**
 * \ingroup indiDriver_tests
 */
SCENARIO( "indiDriver property handlers", "[app::indiDriver]" )
{
    std::filesystem::create_directories( "/tmp/indiDriver_test" );

    indiDriverTestParent parent;
    makeFreshFifo( "/tmp/indiDriver_test/hp.in" );
    makeFreshFifo( "/tmp/indiDriver_test/hp.out" );
    makeFreshFifo( "/tmp/indiDriver_test/hp.ctrl" );
    parent.m_driverInName   = "/tmp/indiDriver_test/hp.in";
    parent.m_driverOutName  = "/tmp/indiDriver_test/hp.out";
    parent.m_driverCtrlName = "/tmp/indiDriver_test/hp.ctrl";

    indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
    REQUIRE( drv.good() == true );

    pcf::IndiProperty ip;

    GIVEN( "a parent is set" )
    {
        WHEN( "each handler is called" )
        {
            drv.handleDefProperty( ip );
            drv.handleGetProperties( ip );
            drv.handleNewProperty( ip );
            drv.handleSetProperty( ip );

            THEN( "the parent's counters are incremented" )
            {
                REQUIRE( parent.defCount == 1 );
                REQUIRE( parent.getPropCount == 1 );
                REQUIRE( parent.newPropCount == 1 );
                REQUIRE( parent.setPropCount == 1 );
            }
        }
    }

    GIVEN( "the parent has been cleared" )
    {
        drv.clearParent();

        WHEN( "each handler is called" )
        {
            drv.handleDefProperty( ip );
            drv.handleGetProperties( ip );
            drv.handleNewProperty( ip );
            drv.handleSetProperty( ip );

            THEN( "nothing happens (no crash, no counters incremented)" )
            {
                REQUIRE( parent.defCount == 0 );
                REQUIRE( parent.getPropCount == 0 );
                REQUIRE( parent.newPropCount == 0 );
                REQUIRE( parent.setPropCount == 0 );
            }
        }
    }
}

/// Test that execute()/update() run the underlying INDI processing loop
/// and can be cleanly stopped.
/**
 * \ingroup indiDriver_tests
 */
SCENARIO( "indiDriver execute and update", "[app::indiDriver]" )
{
    std::filesystem::create_directories( "/tmp/indiDriver_test" );

    GIVEN( "a valid indiDriver" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/ex.in" );
        makeFreshFifo( "/tmp/indiDriver_test/ex.out" );
        makeFreshFifo( "/tmp/indiDriver_test/ex.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/ex.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/ex.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/ex.ctrl";

        indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
        REQUIRE( drv.good() == true );

        WHEN( "execute() is run and then stopped" )
        {
            std::thread t( [&drv]() { drv.execute(); } );

            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            drv.quitProcess();

            t.join();

            THEN( "it returns cleanly" )
            {
                REQUIRE( drv.getQuitProcess() == true );
            }
        }
    }

    GIVEN( "a heap-allocated indiDriver of the exact (non-derived) type" )
    {
        // Exercises deletion via `delete ptr` on the exact
        // MagAOX::app::indiDriver<parentT> type, matching how MagAOXApp
        // itself owns and deletes its m_indiDriver (as opposed to the
        // stack-allocated/automatic destruction used by every other test in
        // this file, and unlike deleting via the derived indiDriverExposed
        // test helper, which would invoke indiDriverExposed's own generated
        // destructor entry point instead of indiDriver<parentT>'s).
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/heap.in" );
        makeFreshFifo( "/tmp/indiDriver_test/heap.out" );
        makeFreshFifo( "/tmp/indiDriver_test/heap.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/heap.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/heap.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/heap.ctrl";

        auto *drv = new MagAOX::app::indiDriver<indiDriverTestParent>( &parent, "test", "0", "0" );

        WHEN( "it is deleted" )
        {
            REQUIRE( drv->good() == true );
            delete drv;

            THEN( "no crash occurs" )
            {
                REQUIRE( true );
            }
        }
    }
}

/// Test indiClient (the small helper class used internally by indiDriver to
/// send outgoing NEW property commands) directly, to exercise its execute()
/// override.
/**
 * \ingroup indiDriver_tests
 */
SCENARIO( "indiClient execute", "[app::indiDriver]" )
{
    TcpListener listener;

    MagAOX::app::indiClient client( "indiClientTest", "127.0.0.1", listener.port );

    client.activate();
    // Thread::runLoop() sleeps for its 1000ms interval before its *first*
    // call to execute(), so we must wait longer than that here to guarantee
    // execute() actually runs at least once before we tear the thread down.
    std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
    client.quitProcess();
    client.deactivate();

    REQUIRE( client.getQuitProcess() == true );
}

/// Test the sendNewProperty connect/reconnect/reuse logic against a real
/// (but never accept()-ed) local TCP listener, so that IndiClient's connect()
/// call succeeds.
/**
 * \ingroup indiDriver_tests
 */
SCENARIO( "indiDriver sendNewProperty connects, reuses, and reconnects", "[app::indiDriver]" )
{
    std::filesystem::create_directories( "/tmp/indiDriver_test" );

    indiDriverTestParent parent;
    makeFreshFifo( "/tmp/indiDriver_test/snp.in" );
    makeFreshFifo( "/tmp/indiDriver_test/snp.out" );
    makeFreshFifo( "/tmp/indiDriver_test/snp.ctrl" );
    parent.m_driverInName   = "/tmp/indiDriver_test/snp.in";
    parent.m_driverOutName  = "/tmp/indiDriver_test/snp.out";
    parent.m_driverCtrlName = "/tmp/indiDriver_test/snp.ctrl";

    indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
    REQUIRE( drv.good() == true );

    TcpListener listener;
    drv.setServer( "127.0.0.1", listener.port );

    pcf::IndiProperty ip = makeNumberProperty( "test", "nprop" );

    GIVEN( "no existing outgoing connection" )
    {
        WHEN( "sendNewProperty is called" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "a client is created and the send succeeds" )
            {
                REQUIRE( rv == 0 );
                REQUIRE( drv.hasOutGoing() == true );
            }
        }
    }

    GIVEN( "an existing, still-good outgoing connection" )
    {
        REQUIRE( drv.sendNewProperty( ip ) == 0 );

        WHEN( "sendNewProperty is called again" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "the existing connection is reused" )
            {
                REQUIRE( rv == 0 );
            }
        }
    }

    GIVEN( "an existing outgoing connection which has quit" )
    {
        REQUIRE( drv.sendNewProperty( ip ) == 0 );
        drv.forceOutGoingQuit();

        WHEN( "sendNewProperty is called again" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "the old connection is torn down and a new one is created" )
            {
                REQUIRE( rv == 0 );
                REQUIRE( drv.hasOutGoing() == true );
            }
        }
    }
}

/// Test the exception/disconnect handling paths in sendNewProperty.
/**
 * \ingroup indiDriver_tests
 */
SCENARIO( "indiDriver sendNewProperty exception handling", "[app::indiDriver]" )
{
    std::filesystem::create_directories( "/tmp/indiDriver_test" );

    GIVEN( "no listener at all on the configured server address" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/exc0.in" );
        makeFreshFifo( "/tmp/indiDriver_test/exc0.out" );
        makeFreshFifo( "/tmp/indiDriver_test/exc0.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/exc0.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/exc0.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/exc0.ctrl";

        indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
        REQUIRE( drv.good() == true );

        // Nothing is listening on this port, so the connection attempt made
        // while creating the outgoing IndiClient fails, which is caught by
        // the generic catch(...) around client creation.
        drv.setServer( "127.0.0.1", 1 );

        pcf::IndiProperty ip = makeNumberProperty( "test", "nprop" );

        WHEN( "sendNewProperty is called" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "the exception thrown while connecting is caught and -1 is returned" )
            {
                REQUIRE( rv == -1 );
                REQUIRE( drv.hasOutGoing() == false );
            }
        }
    }

    GIVEN( "connection creation succeeds but is then forced to null" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/excnull.in" );
        makeFreshFifo( "/tmp/indiDriver_test/excnull.out" );
        makeFreshFifo( "/tmp/indiDriver_test/excnull.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/excnull.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/excnull.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/excnull.ctrl";

        indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
        REQUIRE( drv.good() == true );

        TcpListener listener;
        drv.setServer( "127.0.0.1", listener.port );
        HookGuard guard( indiDriverExposed<indiDriverTestParent>::xwcTestHooks.forceOutGoingNull );

        pcf::IndiProperty ip = makeNumberProperty( "test", "nprop" );

        WHEN( "sendNewProperty is called" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "the defensive null check fires and -1 is returned" )
            {
                REQUIRE( rv == -1 );
                REQUIRE( drv.hasOutGoing() == false );
            }
        }
    }

    GIVEN( "a property of Unknown type, which the underlying INDI library rejects" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/exc.in" );
        makeFreshFifo( "/tmp/indiDriver_test/exc.out" );
        makeFreshFifo( "/tmp/indiDriver_test/exc.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/exc.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/exc.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/exc.ctrl";

        indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
        REQUIRE( drv.good() == true );

        TcpListener listener;
        drv.setServer( "127.0.0.1", listener.port );

        pcf::IndiProperty badProp; // default-constructed: type == Unknown

        WHEN( "sendNewProperty is called" )
        {
            int rv = drv.sendNewProperty( badProp );

            THEN( "the std::exception thrown by the INDI library is caught and -1 is returned" )
            {
                REQUIRE( rv == -1 );
            }
        }
    }

    GIVEN( "a forced non-std::exception thrown from the send call" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/exc3.in" );
        makeFreshFifo( "/tmp/indiDriver_test/exc3.out" );
        makeFreshFifo( "/tmp/indiDriver_test/exc3.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/exc3.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/exc3.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/exc3.ctrl";

        indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
        REQUIRE( drv.good() == true );

        TcpListener listener;
        drv.setServer( "127.0.0.1", listener.port );
        HookGuard guard( indiDriverExposed<indiDriverTestParent>::xwcTestHooks.forceSendNonStdThrow );

        pcf::IndiProperty ip = makeNumberProperty( "test", "nprop" );

        WHEN( "sendNewProperty is called" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "the catch(...) branch is hit and -1 is returned" )
            {
                REQUIRE( rv == -1 );
            }
        }
    }

    GIVEN( "the outgoing connection quits immediately after the send call" )
    {
        indiDriverTestParent parent;
        makeFreshFifo( "/tmp/indiDriver_test/exc4.in" );
        makeFreshFifo( "/tmp/indiDriver_test/exc4.out" );
        makeFreshFifo( "/tmp/indiDriver_test/exc4.ctrl" );
        parent.m_driverInName   = "/tmp/indiDriver_test/exc4.in";
        parent.m_driverOutName  = "/tmp/indiDriver_test/exc4.out";
        parent.m_driverCtrlName = "/tmp/indiDriver_test/exc4.ctrl";

        indiDriverExposed<indiDriverTestParent> drv( &parent, "test", "0", "0" );
        REQUIRE( drv.good() == true );

        TcpListener listener;
        drv.setServer( "127.0.0.1", listener.port );
        HookGuard guard( indiDriverExposed<indiDriverTestParent>::xwcTestHooks.forceQuitAfterSend );

        pcf::IndiProperty ip = makeNumberProperty( "test", "nprop" );

        WHEN( "sendNewProperty is called" )
        {
            int rv = drv.sendNewProperty( ip );

            THEN( "the disconnect is detected and -1 is returned" )
            {
                REQUIRE( rv == -1 );
            }
        }
    }
}
