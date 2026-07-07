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
    pcf::IndiProperty ipSingle = pdt.indiP_single();
    ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

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

    pcf::IndiProperty ipStop = pdt.indiP_stop();
    ipStop["request"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_stop( ipStop ) == 0 );

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
