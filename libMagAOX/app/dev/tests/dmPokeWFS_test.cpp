/** \file dmPokeWFS_test.cpp
  * \brief Catch2 tests for the dev::dmPokeWFS CRTP mixin (libMagAOX/app/dev/dmPokeWFS.hpp).
  *
  * Drives the real poke sensor state machine under the harness in dmPokeWFS_test.hpp:
  * real shmim streams, real semaphores, and a real WFS-frame-producing thread stand in
  * for the camera, so the measurement loop (basicTimedPoke/basicRunSensor), its INDI
  * callbacks, and its error paths all execute for real.
  */
#include "../../../../tests/catch2/catch.hpp"

#include <mx/sys/timeUtils.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include <csignal>
#include <cstring>

#include "dmPokeWFS_test.hpp"

/** \defgroup dmPokeWFS_tests libXWC::app::dev::dmPokeWFS Unit Tests
 * \ingroup app_dev_unit_tests
 */

/// Test dmPokeWFS configuration, including the invalid-poke-spec error path.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS Configuration", "[dev::dmPokeWFS]" )
{
    SECTION( "a full config file loads all values" )
    {
        std::vector<std::string> s, k, v;

        s.push_back( "wfscam" );
        k.push_back( "shmimName" );
        v.push_back( "pwfscam" );

        s.push_back( "wfsdark" );
        k.push_back( "shmimName" );
        v.push_back( "pwfsdark" );

        s.push_back( "pokecen" );
        k.push_back( "dmChannel" );
        v.push_back( "pwfsdm" );

        s.push_back( "pokecen" );
        k.push_back( "pokeX" );
        v.push_back( "1,2,3" );

        s.push_back( "pokecen" );
        k.push_back( "pokeY" );
        v.push_back( "1,2,3" );

        s.push_back( "pokecen" );
        k.push_back( "pokeAmp" );
        v.push_back( "0.5" );

        s.push_back( "pokecen" );
        k.push_back( "dmSleep" );
        v.push_back( "1000" );

        s.push_back( "pokecen" );
        k.push_back( "nPokeImages" );
        v.push_back( "3" );

        s.push_back( "pokecen" );
        k.push_back( "nPokeAverage" );
        v.push_back( "2" );

        mx::app::writeConfigFile( "/tmp/dmPokeWFS_test.conf", s, k, v );

        mx::app::appConfigurator config;

        dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );

        int rv = pdt.setupConfig( config );
        REQUIRE( rv == 0 );

        config.readConfig( "/tmp/dmPokeWFS_test.conf" );

        rv = pdt.loadConfig( config );
        REQUIRE( rv == 0 );

        REQUIRE( pdt.shmimMonitor().shmimName() == "pwfscam" );
        REQUIRE( pdt.darkShmimMonitor().shmimName() == "pwfsdark" );
    }

    SECTION( "missing pokeX/pokeY is an error" )
    {
        std::vector<std::string> s, k, v;
        s.push_back( "wfscam" );
        k.push_back( "shmimName" );
        v.push_back( "pwfscam2" );
        s.push_back( "wfsdark" );
        k.push_back( "shmimName" );
        v.push_back( "pwfsdark2" );
        s.push_back( "pokecen" );
        k.push_back( "dmChannel" );
        v.push_back( "pwfsdm2" );

        mx::app::writeConfigFile( "/tmp/dmPokeWFS_test_nopoke.conf", s, k, v );

        mx::app::appConfigurator config;
        dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );

        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dmPokeWFS_test_nopoke.conf" );
        REQUIRE( pdt.loadConfig( config ) == -1 );
    }

    SECTION( "mismatched pokeX/pokeY sizes is an error" )
    {
        std::vector<std::string> s, k, v;
        s.push_back( "wfscam" );
        k.push_back( "shmimName" );
        v.push_back( "pwfscam3" );
        s.push_back( "wfsdark" );
        k.push_back( "shmimName" );
        v.push_back( "pwfsdark3" );
        s.push_back( "pokecen" );
        k.push_back( "dmChannel" );
        v.push_back( "pwfsdm3" );
        s.push_back( "pokecen" );
        k.push_back( "pokeX" );
        v.push_back( "1,2,3" );
        s.push_back( "pokecen" );
        k.push_back( "pokeY" );
        v.push_back( "1,2" );

        mx::app::writeConfigFile( "/tmp/dmPokeWFS_test_mismatch.conf", s, k, v );

        mx::app::appConfigurator config;
        dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );

        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dmPokeWFS_test_mismatch.conf" );
        REQUIRE( pdt.loadConfig( config ) == -1 );
    }
}

/// Helper to load a minimal valid config into a dmPokeWFSTest for the given shmim/dm names.
static void loadBasicConfig( dmPokeWFS_tests::dmPokeWFSTest &pdt,
                              const std::string &wfsName,
                              const std::string &darkName,
                              const std::string &dmName,
                              const std::string &confPath )
{
    std::vector<std::string> s, k, v;
    s.push_back( "wfscam" );
    k.push_back( "shmimName" );
    v.push_back( wfsName );
    s.push_back( "wfsdark" );
    k.push_back( "shmimName" );
    v.push_back( darkName );
    s.push_back( "pokecen" );
    k.push_back( "dmChannel" );
    v.push_back( dmName );
    s.push_back( "pokecen" );
    k.push_back( "pokeX" );
    v.push_back( "0,1" );
    s.push_back( "pokecen" );
    k.push_back( "pokeY" );
    v.push_back( "0,1" );
    s.push_back( "pokecen" );
    k.push_back( "pokeAmp" );
    v.push_back( "1.0" );
    s.push_back( "pokecen" );
    k.push_back( "dmSleep" );
    v.push_back( "200" );
    s.push_back( "pokecen" );
    k.push_back( "nPokeImages" );
    v.push_back( "1" );
    s.push_back( "pokecen" );
    k.push_back( "nPokeAverage" );
    v.push_back( "1" );

    mx::app::writeConfigFile( confPath, s, k, v );

    mx::app::appConfigurator config;
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( confPath );
    REQUIRE( pdt.loadConfig( config ) == 0 );
}

/// Test allocate() and processImage() for both the WFS camera and dark shmims,
/// including the dark-valid/invalid size-matching logic and the DM-channel-missing
/// error branch.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS allocate and processImage", "[dev::dmPokeWFS]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    // remove any stray shmim left behind by a previous run of this test
    std::remove( "/tmp/dmtest/shm/pwfsdmA.im.shm" );

    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    loadBasicConfig( pdt, "pwfscamA", "pwfsdarkA", "pwfsdmA", "/tmp/dmPokeWFS_test_alloc.conf" );
    // processImage() posts m_imageSemaphore; initialize it since we aren't calling
    // appStartup() (which would normally do this) in this test.
    pdt.initSemaphoresForTest();

    // dm channel doesn't exist yet -> allocate(wfsShmimT) fails
    pdt.setWfsSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == -1 );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmA", 4, 4 );

    // dark not yet sized -> mismatched -> not valid
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.darkValidFlag() == false );

    // now size the dark to match -> allocate(darkShmimT) sees matching sizes -> valid
    pdt.setDarkSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );
    REQUIRE( pdt.darkValidFlag() == true );

    // processImage(darkShmimT): copies raw data into m_darkImage
    std::vector<float> darkFrame( 16, 2.0f );
    REQUIRE( pdt.processImage( darkFrame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );
    REQUIRE( pdt.darkImage().sum() == Approx( 16 * 2.0f ) );

    // processImage(wfsShmimT): with dark valid, output = raw - dark
    std::vector<float> wfsFrame( 16, 10.0f );
    REQUIRE( pdt.processImage( wfsFrame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.rawImage()().sum() == Approx( 16 * ( 10.0f - 2.0f ) ) );

    // now make dark invalid (mismatched sizes) and verify processImage(wfsShmimT) skips subtraction
    pdt.setDarkSize( 5, 5, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );
    REQUIRE( pdt.darkValidFlag() == false );

    REQUIRE( pdt.processImage( wfsFrame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.rawImage()().sum() == Approx( 16 * 10.0f ) );
}

/// Test basicTimedPoke() and basicRunSensor(), including the "poke image not
/// allocated" error branch.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS basicTimedPoke and basicRunSensor", "[dev::dmPokeWFS]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    loadBasicConfig( pdt, "pwfscamB", "pwfsdarkB", "pwfsdmB", "/tmp/dmPokeWFS_test_run.conf" );
    // basicTimedPoke()/processImage() use m_imageSemaphore; initialize it since we
    // aren't calling appStartup() (which would normally do this) in this test.
    pdt.initSemaphoresForTest();

    // basicRunSensor() before allocate(): m_pokeImage is not valid -> error
    REQUIRE( pdt.callBasicRunSensor() == -1 );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmB", 2, 2 );

    pdt.setWfsSize( 2, 2, IMAGESTRUCT_FLOAT );
    // Leave the dark uninitialized/unallocated here (darkValid() stays false) so
    // processImage(wfsShmimT) is a plain deterministic copy with no subtraction --
    // dark subtraction itself is already covered by the allocate/processImage test.
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );

    // A background "camera" thread that keeps posting frames via processImage(), which
    // internally posts the image semaphore that basicTimedPoke()/basicRunSensor() wait on.
    // Use CHECK (not REQUIRE) for assertions taken while this thread is running, and
    // always stop/join it before returning, so a failed assertion can't leave a
    // dangling reference to `pdt` running in the background.
    std::atomic<bool> keepPosting{ true };
    std::vector<float> frame( 4, 7.0f );
    std::thread        poster(
        [&]()
        {
            while( keepPosting.load() )
            {
                pdt.processImage( frame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() );
                mx::sys::milliSleep( 2 );
            }
        } );

    pdt.setPokeLocalZero();
    int rv = pdt.callBasicTimedPoke( 1.0 );
    CHECK( rv == 0 );
    // one image of the constant frame was accumulated with sign +1
    CHECK( pdt.pokeLocal().sum() == Approx( 4 * 7.0f ) );

    rv = pdt.callBasicRunSensor();

    keepPosting = false;
    poster.join();

    CHECK( rv == 0 );
    // symmetric +1/-1 poke of a constant "sensor" frame nets to zero
    CHECK( pdt.pokeImage()().sum() == Approx( 0 ).margin( 1e-4 ) );

    // -- an already-requested stop causes an immediate shutdown-style return --
    // With m_stopMeasurement already true, both of basicTimedPoke()'s internal wait
    // loops are skipped entirely and it returns 1 (zeroing the DM command).
    pdt.setStopMeasurement( true );
    REQUIRE( pdt.callBasicTimedPoke( 1.0 ) == 1 );

    // basicRunSensor()'s positive-poke call sees the same rv==1 and propagates it.
    REQUIRE( pdt.callBasicRunSensor() == 1 );

    pdt.setStopMeasurement( false );
}

/// Test updateMeasurement().
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS updateMeasurement", "[dev::dmPokeWFS]" )
{
    mx::app::writeConfigFile( "/tmp/dmPokeWFS_test_um.conf", { "none" }, { "nada" }, { "0" } );
    mx::app::appConfigurator config;
    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dmPokeWFS_test_um.conf" );
    // no pokeX/pokeY configured: loadConfig will fail, but updateMeasurement() itself
    // does not depend on loadConfig having succeeded.
    pdt.loadConfig( config );
    pdt.prepMeasurementIndiForTest();

    REQUIRE( pdt.callUpdateMeasurement( 1.5, -2.5 ) == 0 );
    REQUIRE( pdt.testDeltaX() == Approx( 1.5 ) );
    REQUIRE( pdt.testDeltaY() == Approx( -2.5 ) );
}

/// Test recordTelem()/recordPokeLoop(), including the forced and change-detection paths.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS recordTelem and recordPokeLoop", "[dev::dmPokeWFS]" )
{
    mx::app::writeConfigFile( "/tmp/dmPokeWFS_test_telem.conf", { "none" }, { "nada" }, { "0" } );
    mx::app::appConfigurator config;
    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dmPokeWFS_test_telem.conf" );
    pdt.loadConfig( config );

    // recordTelem() forces a telem() call
    REQUIRE( pdt.callRecordTelem() == 0 );
    REQUIRE( pdt.m_telemCount >= 1 );

    int before = pdt.m_telemCount;

    // recordPokeLoop(false) with unchanged state (all defaults) does not telem() again
    REQUIRE( pdt.callRecordPokeLoop( false ) == 0 );
    REQUIRE( pdt.m_telemCount == before );

    // force always calls telem()
    REQUIRE( pdt.callRecordPokeLoop( true ) == 0 );
    REQUIRE( pdt.m_telemCount == before + 1 );
}

/// Test appLogic()'s three independent "a monitored thread has exited" propagation
/// branches (the wfs shmimMonitor, the dark shmimMonitor, and dmPokeWFS's own wfs
/// measurement thread), using direct control of each std::thread's bookkeeping --
/// mirroring shmimMonitor_test.cpp's setSmThread()/abandonSmThread() pattern -- so
/// each branch can be exercised in isolation without a full, real appStartup().
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS appLogic propagates thread-exited errors independently", "[dev::dmPokeWFS]" )
{
    SECTION( "the wfs shmimMonitor thread has already exited" )
    {
        dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );

        std::atomic<bool> darkAlive{ true }, wfsMeasAlive{ true };
        pdt.setDarkMonitorThread( std::thread(
            [&darkAlive]()
            {
                while( darkAlive )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            } ) );
        pdt.setWfsMeasurementThread( std::thread(
            [&wfsMeasAlive]()
            {
                while( wfsMeasAlive )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            } ) );

        pdt.setWfsMonitorThread( std::thread( [](){} ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        REQUIRE( pdt.appLogic() == -1 );

        pdt.abandonWfsMonitorThread();
        darkAlive    = false;
        wfsMeasAlive = false;
        pdt.joinDarkMonitorThread();
        pdt.joinWfsMeasurementThread();
    }

    SECTION( "the dark shmimMonitor thread has already exited while the wfs monitor thread is alive" )
    {
        dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );

        std::atomic<bool> wfsAlive{ true }, wfsMeasAlive{ true };
        pdt.setWfsMonitorThread( std::thread(
            [&wfsAlive]()
            {
                while( wfsAlive )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            } ) );
        pdt.setWfsMeasurementThread( std::thread(
            [&wfsMeasAlive]()
            {
                while( wfsMeasAlive )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            } ) );

        pdt.setDarkMonitorThread( std::thread( [](){} ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        REQUIRE( pdt.appLogic() == -1 );

        pdt.abandonDarkMonitorThread();
        wfsAlive     = false;
        wfsMeasAlive = false;
        pdt.joinWfsMonitorThread();
        pdt.joinWfsMeasurementThread();
    }

    SECTION( "dmPokeWFS's own wfs measurement thread has already exited while both shmimMonitor threads are alive" )
    {
        dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );

        std::atomic<bool> wfsAlive{ true }, darkAlive{ true };
        pdt.setWfsMonitorThread( std::thread(
            [&wfsAlive]()
            {
                while( wfsAlive )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            } ) );
        pdt.setDarkMonitorThread( std::thread(
            [&darkAlive]()
            {
                while( darkAlive )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            } ) );

        pdt.setWfsMeasurementThread( std::thread( [](){} ) );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        REQUIRE( pdt.appLogic() == -1 );

        pdt.abandonWfsMeasurementThread();
        wfsAlive  = false;
        darkAlive = false;
        pdt.joinWfsMonitorThread();
        pdt.joinDarkMonitorThread();
    }
}

/// Test the full appStartup/appLogic/appShutdown lifecycle, including the WFS
/// thread's single-measurement and continuous+stop state machines, and the INDI
/// callbacks (including their wrong-key error branches).
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS full lifecycle", "[dev::dmPokeWFS]" )
{
    // dmPokeWFS<>::appShutdown() sends SIGUSR1 to the wfs thread. shmimMonitor's own
    // appStartup() installs the same no-op handler for its own threads; install it here
    // too so it is guaranteed to be in place before anything relies on it.
    struct sigaction act;
    memset( &act, 0, sizeof( act ) );
    act.sa_sigaction = &MagAOX::app::sigUsr1Handler;
    act.sa_flags     = SA_SIGINFO;
    sigemptyset( &act.sa_mask );
    REQUIRE( sigaction( SIGUSR1, &act, 0 ) == 0 );

    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    loadBasicConfig( pdt, "pwfscamC", "pwfsdarkC", "pwfsdmC", "/tmp/dmPokeWFS_test_lc.conf" );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmC", 2, 2 );

    pdt.setWfsSize( 2, 2, IMAGESTRUCT_FLOAT );
    pdt.setDarkSize( 2, 2, IMAGESTRUCT_FLOAT );

    // allocate() *before* appStartup() so m_pokeImage is already valid before the wfs
    // thread could ever act on a triggered measurement (avoids an unbounded spin wait
    // inside wfsThreadExec() for m_pokeImage.valid()).
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

    // Use the fast, deterministic runSensor()/analyzeSensor() stubs (not the real
    // basicRunSensor(), which requires a live image-posting thread).
    pdt.m_useRealRunSensor = false;
    pdt.m_runSensorRV      = 0;
    pdt.m_testDeltaX       = 3;
    pdt.m_testDeltaY       = 4;

    REQUIRE( pdt.appStartup() == 0 );
    mx::sys::milliSleep( 200 );

    REQUIRE( pdt.appLogic() == 0 );

    // -- INDI callback wrong-key branches --
    pcf::IndiProperty ipWrong( pcf::IndiProperty::Number );
    ipWrong.setDevice( "somethingelse" );
    ipWrong.setName( "notarealprop" );
    REQUIRE( pdt.callNewCallBack_pokeAmp( ipWrong ) == -1 );
    REQUIRE( pdt.callNewCallBack_nPokeImages( ipWrong ) == -1 );
    REQUIRE( pdt.callNewCallBack_nPokeAverage( ipWrong ) == -1 );

    pcf::IndiProperty ipWrongSw( pcf::IndiProperty::Switch );
    ipWrongSw.setDevice( "somethingelse" );
    ipWrongSw.setName( "notarealsw" );
    REQUIRE( pdt.callNewCallBack_single( ipWrongSw ) == -1 );
    REQUIRE( pdt.callNewCallBack_continuous( ipWrongSw ) == -1 );
    REQUIRE( pdt.callNewCallBack_stop( ipWrongSw ) == -1 );

    // static callback trampolines (wrong-key branch, harmless to call again)
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_single( &pdt, ipWrongSw ) == -1 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_continuous( &pdt, ipWrongSw ) == -1 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_stop( &pdt, ipWrongSw ) == -1 );

    // -- numeric target-update callbacks --
    pcf::IndiProperty ipPokeAmp = pdt.indiP_pokeAmp();
    ipPokeAmp["target"].setValue( 0.75 );
    REQUIRE( pdt.callNewCallBack_pokeAmp( ipPokeAmp ) == 0 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_pokeAmp( &pdt, ipPokeAmp ) == 0 );

    pcf::IndiProperty ipNImg = pdt.indiP_nPokeImages();
    ipNImg["target"].setValue( 3 );
    REQUIRE( pdt.callNewCallBack_nPokeImages( ipNImg ) == 0 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_nPokeImages( &pdt, ipNImg ) == 0 );

    pcf::IndiProperty ipNAvg = pdt.indiP_nPokeAverage();
    ipNAvg["target"].setValue( 4 );
    REQUIRE( pdt.callNewCallBack_nPokeAverage( ipNAvg ) == 0 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_nPokeAverage( &pdt, ipNAvg ) == 0 );

    // -- wfsFps set-property callback --
    pcf::IndiProperty ipFps = pdt.indiP_wfsFps();
    ipFps.add( pcf::IndiElement( "current" ) );
    ipFps["current"] = 100.0;
    REQUIRE( pdt.callSetCallBack_wfsFps( ipFps ) == 0 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStSetCallBack_wfsFps( &pdt, ipFps ) == 0 );

    // -- single measurement round trip --
    // widen the measuring==1 window so appLogic() can reliably be called while it is
    // active, to exercise the single-"On"/continuous-"Off" switch-reporting branch.
    pdt.m_runSensorSleepMs = 150;

    pcf::IndiProperty ipSingle = pdt.indiP_single();
    ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

    // the wfs thread's own startup handshake (threadStart()'s internal sleep(1) poll
    // waiting for m_wfsThreadInit to clear) can take up to ~1 real second even after
    // appStartup() has returned, so this window must be generous.
    bool caughtSingleMeasuring = false;
    for( int i = 0; i < 150 && !caughtSingleMeasuring; ++i )
    {
        mx::sys::milliSleep( 20 );
        if( pdt.measuringState() == 1 )
        {
            REQUIRE( pdt.appLogic() == 0 );
            caughtSingleMeasuring = true;
        }
    }
    REQUIRE( caughtSingleMeasuring == true );

    pdt.m_runSensorSleepMs = 0;

    uint64_t counterBefore = pdt.testCounter();
    bool     completed     = false;
    for( int i = 0; i < 50; ++i )
    {
        mx::sys::milliSleep( 50 );
        if( pdt.testCounter() > counterBefore )
        {
            completed = true;
            break;
        }
    }
    REQUIRE( completed == true );
    REQUIRE( pdt.testDeltaX() == Approx( 3 ) );
    REQUIRE( pdt.testDeltaY() == Approx( 4 ) );

    // give the thread a moment to settle back to idle after the single measurement
    for( int i = 0; i < 50 && pdt.measuringState() != 0; ++i )
    {
        mx::sys::milliSleep( 20 );
    }
    REQUIRE( pdt.measuringState() == 0 );

    REQUIRE( pdt.appLogic() == 0 );

    // -- continuous measurement + stop --
    pcf::IndiProperty ipContinuous = pdt.indiP_continuous();
    ipContinuous["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_continuous( ipContinuous ) == 0 );

    // let it run for a few measurement cycles
    uint64_t counterAtStart = pdt.testCounter();
    bool     advanced       = false;
    for( int i = 0; i < 50; ++i )
    {
        mx::sys::milliSleep( 50 );
        if( pdt.testCounter() > counterAtStart + 1 )
        {
            advanced = true;
            break;
        }
    }
    REQUIRE( advanced == true );
    REQUIRE( pdt.appLogic() == 0 );

    // Widen the window between runSensor() and analyzeSensor() so both the continuous
    // callback's own Off/m_measuring!=0 branch (m_stopMeasurement=true) and the "stop"
    // property callback's identical m_measuring!=0 branch (checked after runSensor()
    // returns, before analyzeSensor()) can be reliably exercised together, mid-cycle,
    // before the measurement loop notices either one and exits on its own.
    pdt.m_runSensorSleepMs = 150;
    bool caughtContinuousMeasuring = false;
    for( int i = 0; i < 150 && !caughtContinuousMeasuring; ++i )
    {
        mx::sys::milliSleep( 20 );
        if( pdt.measuringState() == 2 )
        {
            caughtContinuousMeasuring = true;
        }
    }
    REQUIRE( caughtContinuousMeasuring == true );

    // toggling the continuous property itself to "Off" (as opposed to using the
    // separate "stop" property below) is the normal INDI way to request a stop, and
    // exercises the continuous callback's own Off/m_measuring!=0 branch.
    pcf::IndiProperty ipContinuousOff = pdt.indiP_continuous();
    ipContinuousOff["toggle"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.callNewCallBack_continuous( ipContinuousOff ) == 0 );

    // still mid-cycle (measuring hasn't reset yet) -- exercises the "stop" property
    // callback's own, separate m_measuring!=0 branch.
    REQUIRE( pdt.measuringState() != 0 );
    pcf::IndiProperty ipStop = pdt.indiP_stop();
    ipStop["request"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_stop( ipStop ) == 0 );

    pdt.m_runSensorSleepMs = 0;

    bool stopped = false;
    for( int i = 0; i < 50; ++i )
    {
        mx::sys::milliSleep( 50 );
        if( pdt.measuringState() == 0 )
        {
            stopped = true;
            break;
        }
    }
    REQUIRE( stopped == true );

    REQUIRE( pdt.appLogic() == 0 );

    // dmPokeWFS<>::appShutdown() only signals the wfs thread; it does not itself set
    // m_shutdown (that is normally done by the app's own signal handling/main loop
    // before appShutdown() is called). The thread's outer loop only exits once
    // derived().m_shutdown is nonzero, so set it explicitly here to avoid hanging.
    pdt.requestShutdown();
    REQUIRE( pdt.appShutdown() == 0 );
}

/// Test the INDI callbacks' "required element is missing" branches, which need a
/// property with a matching device/name (to pass the INDI_VALIDATE_CALLBACK_PROPS_DERIVED
/// key check) but without the specific element the callback looks for -- distinct from
/// the wrong-device/name branch already tested above.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS INDI callback missing-element branches", "[dev::dmPokeWFS]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    loadBasicConfig( pdt, "pwfscamE", "pwfsdarkE", "pwfsdmE", "/tmp/dmPokeWFS_test_missing.conf" );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmE", 2, 2 );

    pdt.setWfsSize( 2, 2, IMAGESTRUCT_FLOAT );
    pdt.setDarkSize( 2, 2, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

    REQUIRE( pdt.appStartup() == 0 );

    auto sameKeyNoElements = []( const pcf::IndiProperty &orig )
    {
        pcf::IndiProperty ip( orig.getType() );
        ip.setDevice( orig.getDevice() );
        ip.setName( orig.getName() );
        return ip;
    };

    REQUIRE( pdt.callNewCallBack_nPokeImages( sameKeyNoElements( pdt.indiP_nPokeImages() ) ) == -1 );
    REQUIRE( pdt.callNewCallBack_nPokeAverage( sameKeyNoElements( pdt.indiP_nPokeAverage() ) ) == -1 );
    REQUIRE( pdt.callNewCallBack_pokeAmp( sameKeyNoElements( pdt.indiP_pokeAmp() ) ) == -1 );
    REQUIRE( pdt.callSetCallBack_wfsFps( sameKeyNoElements( pdt.indiP_wfsFps() ) ) == 0 );
    REQUIRE( pdt.callNewCallBack_single( sameKeyNoElements( pdt.indiP_single() ) ) == -1 );
    REQUIRE( pdt.callNewCallBack_continuous( sameKeyNoElements( pdt.indiP_continuous() ) ) == -1 );
    REQUIRE( pdt.callNewCallBack_stop( sameKeyNoElements( pdt.indiP_stop() ) ) == -1 );

    pdt.requestShutdown();
    REQUIRE( pdt.appShutdown() == 0 );
}

/// Test wfsThreadExec()'s runSensor()/analyzeSensor() failure-logging branches, its
/// defensive neither-single-nor-continuous-set exit branch, and its wait for the poke
/// image to become valid before measuring.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS wfsThreadExec failure and edge-case paths", "[dev::dmPokeWFS]" )
{
    struct sigaction act;
    memset( &act, 0, sizeof( act ) );
    act.sa_sigaction = &MagAOX::app::sigUsr1Handler;
    act.sa_flags     = SA_SIGINFO;
    sigemptyset( &act.sa_mask );
    REQUIRE( sigaction( SIGUSR1, &act, 0 ) == 0 );

    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    loadBasicConfig( pdt, "pwfscamF", "pwfsdarkF", "pwfsdmF", "/tmp/dmPokeWFS_test_edge.conf" );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmF", 2, 2 );

    pdt.setWfsSize( 2, 2, IMAGESTRUCT_FLOAT );
    pdt.setDarkSize( 2, 2, IMAGESTRUCT_FLOAT );
    pdt.m_useRealRunSensor = false;

    SECTION( "runSensor() failure logs the error and stops the measurement" )
    {
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

        pdt.m_runSensorRV = -1;
        REQUIRE( pdt.appStartup() == 0 );
        // threadStart()'s internal sleep(1) poll (waiting for m_wfsThreadInit to
        // clear) can take up to ~1 real second even after appStartup() has returned --
        // wait comfortably past that worst case so the thread is guaranteed to already
        // be parked on its semaphore wait before we post to it.
        mx::sys::milliSleep( 1200 );

        pcf::IndiProperty ipSingle = pdt.indiP_single();
        ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
        REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

        bool stopped = false;
        for( int i = 0; i < 50; ++i )
        {
            mx::sys::milliSleep( 20 );
            if( pdt.measuringState() == 0 && pdt.m_runSensorCalls > 0 )
            {
                stopped = true;
                break;
            }
        }
        REQUIRE( stopped == true );
        REQUIRE( pdt.testCounter() == 0 );

        pdt.requestShutdown();
        REQUIRE( pdt.appShutdown() == 0 );
    }

    SECTION( "analyzeSensor() failure logs the error and stops the measurement" )
    {
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

        pdt.m_runSensorRV     = 0;
        pdt.m_analyzeSensorRV = -1;
        REQUIRE( pdt.appStartup() == 0 );
        mx::sys::milliSleep( 1200 );

        pcf::IndiProperty ipSingle = pdt.indiP_single();
        ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
        REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

        bool stopped = false;
        for( int i = 0; i < 50; ++i )
        {
            mx::sys::milliSleep( 20 );
            if( pdt.measuringState() == 0 && pdt.m_analyzeSensorCalls > 0 )
            {
                stopped = true;
                break;
            }
        }
        REQUIRE( stopped == true );
        REQUIRE( pdt.testCounter() == 0 );

        pdt.requestShutdown();
        REQUIRE( pdt.appShutdown() == 0 );
    }

    SECTION( "posting the wfs semaphore with neither single nor continuous set exits the thread" )
    {
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

        REQUIRE( pdt.appStartup() == 0 );
        mx::sys::milliSleep( 1200 );

        pdt.postWfsSemaphoreWithNoModeSet();
        mx::sys::milliSleep( 200 );

        // the wfs thread has now returned entirely (not just gone idle) -- a
        // subsequent single-measurement request is never serviced.
        pcf::IndiProperty ipSingle = pdt.indiP_single();
        ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
        REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

        bool completed = false;
        for( int i = 0; i < 20; ++i )
        {
            mx::sys::milliSleep( 20 );
            if( pdt.testCounter() > 0 )
            {
                completed = true;
                break;
            }
        }
        REQUIRE( completed == false );

        pdt.requestShutdown();
        REQUIRE( pdt.appShutdown() == 0 );
    }

    SECTION( "wfsThreadExec waits for the poke image to become valid before measuring" )
    {
        // note: allocate() is deliberately *not* called before appStartup() here, so
        // m_pokeImage is not yet valid when the wfs thread wakes up for a measurement.
        REQUIRE( pdt.appStartup() == 0 );
        // wait comfortably past threadStart()'s worst-case ~1 second startup handshake
        // so the thread is guaranteed to already be parked on its semaphore wait --
        // otherwise it might not check m_pokeImage.valid() until well after the
        // allocate() calls below, defeating the race this section is exercising.
        mx::sys::milliSleep( 1200 );

        pcf::IndiProperty ipSingle = pdt.indiP_single();
        ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
        REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

        // give the thread a real chance to enter (and spin in) the "poke image not
        // yet valid" wait loop before we make it valid.
        mx::sys::milliSleep( 100 );
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
        REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

        bool completed = false;
        for( int i = 0; i < 50; ++i )
        {
            mx::sys::milliSleep( 20 );
            if( pdt.measuringState() == 0 && pdt.testCounter() > 0 )
            {
                completed = true;
                break;
            }
        }
        REQUIRE( completed == true );

        pdt.requestShutdown();
        REQUIRE( pdt.appShutdown() == 0 );
    }
}
