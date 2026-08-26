/** \file dmPokeWFS_test.cpp
  * \brief Catch2 tests for the MagAOX::app::dev::dmPokeWFS device mixin.
  *
  * The component under test is the CRTP mixin in libMagAOX/app/dev/dmPokeWFS.hpp. It pokes
  * a deformable mirror channel and reads back wavefront sensor frames to measure the response.
  *
  * The tests drive the real poke sensor state machine through the dmPokeWFSTest harness
  * declared in dmPokeWFS_test.hpp. The harness is a MagAOXApp<false> with two real
  * shmimMonitor bases. Real shared memory image streams and real semaphores are used. A
  * background thread in the test posts WFS frames and stands in for the camera. The
  * measurement loop in basicTimedPoke() and basicRunSensor(), the INDI callbacks, and the
  * error paths all run for real.
  *
  * The tests write config files under /tmp and create image streams under /tmp/dmtest/shm
  * by setting MILK_SHM_DIR. Some tests install a SIGUSR1 handler and start the real wfs thread.
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

/// Verify that loadConfig() reads every pokecen value from a config file and rejects poke
/// lists that are missing or have mismatched sizes.
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

/// Write a minimal valid config file with the given stream and DM channel names and load it
/// into the harness. The poke list has two points and one image per poke.
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

/// Verify allocate() and processImage() for both the WFS camera stream and the dark stream.
/// Covers the dark valid flag, which depends on the two streams having matching sizes, and
/// the error branch taken when the DM channel does not exist.
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
    // processImage() posts m_imageSemaphore. This test does not call appStartup(), which
    // normally initializes the semaphore, so initialize it here.
    pdt.initSemaphoresForTest();

    // The DM channel does not exist yet, so allocate() for the WFS stream fails.
    pdt.setWfsSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == -1 );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmA", 4, 4 );

    // The dark has not been sized yet, so the sizes mismatch and the dark is not valid.
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.darkValidFlag() == false );

    // Size the dark to match. allocate() for the dark stream now sees matching sizes and
    // marks the dark valid.
    pdt.setDarkSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );
    REQUIRE( pdt.darkValidFlag() == true );

    // processImage() for the dark stream copies the raw data into m_darkImage.
    std::vector<float> darkFrame( 16, 2.0f );
    REQUIRE( pdt.processImage( darkFrame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );
    REQUIRE( pdt.darkImage().sum() == Approx( 16 * 2.0f ) );

    // processImage() for the WFS stream subtracts the dark when the dark is valid.
    std::vector<float> wfsFrame( 16, 10.0f );
    REQUIRE( pdt.processImage( wfsFrame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.rawImage()().sum() == Approx( 16 * ( 10.0f - 2.0f ) ) );

    // Make the dark invalid again with mismatched sizes. processImage() for the WFS stream
    // must then skip the subtraction.
    pdt.setDarkSize( 5, 5, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );
    REQUIRE( pdt.darkValidFlag() == false );

    REQUIRE( pdt.processImage( wfsFrame.data(), dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.rawImage()().sum() == Approx( 16 * 10.0f ) );
}

/// Verify basicTimedPoke() and basicRunSensor() with a background thread posting frames.
/// Also covers the error returned when the poke image is not allocated and the early
/// return taken when a stop has already been requested.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS basicTimedPoke and basicRunSensor", "[dev::dmPokeWFS]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    dmPokeWFS_tests::dmPokeWFSTest pdt( "xx", false );
    loadBasicConfig( pdt, "pwfscamB", "pwfsdarkB", "pwfsdmB", "/tmp/dmPokeWFS_test_run.conf" );
    // basicTimedPoke() and processImage() use m_imageSemaphore. This test does not call
    // appStartup(), which normally initializes the semaphore, so initialize it here.
    pdt.initSemaphoresForTest();

    // basicRunSensor() before allocate() finds m_pokeImage not valid and returns an error.
    REQUIRE( pdt.callBasicRunSensor() == -1 );

    mx::improc::milkImage<float> dmChan;
    dmChan.create( "pwfsdmB", 2, 2 );

    pdt.setWfsSize( 2, 2, IMAGESTRUCT_FLOAT );
    // Leave the dark unallocated so darkValid() stays false. processImage() for the WFS
    // stream is then a plain deterministic copy with no subtraction. Dark subtraction is
    // already covered by the allocate and processImage test.
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );

    // A background camera thread keeps posting frames through processImage(). That call
    // posts the image semaphore that basicTimedPoke() and basicRunSensor() wait on.
    // Assertions taken while this thread runs use CHECK rather than REQUIRE. The thread is
    // always stopped and joined before returning. A failed REQUIRE would otherwise leave
    // the thread running with a dangling reference to pdt.
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
    // One image of the constant frame was accumulated with sign +1.
    CHECK( pdt.pokeLocal().sum() == Approx( 4 * 7.0f ) );

    rv = pdt.callBasicRunSensor();

    keepPosting = false;
    poster.join();

    CHECK( rv == 0 );
    // A symmetric +1 and -1 poke of a constant sensor frame nets to zero.
    CHECK( pdt.pokeImage()().sum() == Approx( 0 ).margin( 1e-4 ) );

    // An already requested stop causes an immediate shutdown style return. With
    // m_stopMeasurement already true, both internal wait loops in basicTimedPoke() are
    // skipped and it returns 1 after zeroing the DM command.
    pdt.setStopMeasurement( true );
    REQUIRE( pdt.callBasicTimedPoke( 1.0 ) == 1 );

    // The positive poke call inside basicRunSensor() sees the same return value of 1 and
    // propagates it.
    REQUIRE( pdt.callBasicRunSensor() == 1 );

    pdt.setStopMeasurement( false );
}

/// Verify that updateMeasurement() stores the given deltas and updates the measurement
/// INDI property.
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
    // No pokeX or pokeY is configured, so loadConfig() fails. updateMeasurement() itself
    // does not depend on loadConfig() having succeeded.
    pdt.loadConfig( config );
    pdt.prepMeasurementIndiForTest();

    REQUIRE( pdt.callUpdateMeasurement( 1.5, -2.5 ) == 0 );
    REQUIRE( pdt.testDeltaX() == Approx( 1.5 ) );
    REQUIRE( pdt.testDeltaY() == Approx( -2.5 ) );
}

/// Verify recordTelem() and recordPokeLoop(), including the forced path and the change
/// detection path. The harness counts telem() calls.
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

    // recordTelem() forces a telem() call.
    REQUIRE( pdt.callRecordTelem() == 0 );
    REQUIRE( pdt.m_telemCount >= 1 );

    int before = pdt.m_telemCount;

    // recordPokeLoop(false) with unchanged default state does not call telem() again.
    REQUIRE( pdt.callRecordPokeLoop( false ) == 0 );
    REQUIRE( pdt.m_telemCount == before );

    // The force flag always calls telem().
    REQUIRE( pdt.callRecordPokeLoop( true ) == 0 );
    REQUIRE( pdt.m_telemCount == before + 1 );
}

/// Verify the three independent branches in appLogic() that report a monitored thread has
/// exited. The threads are the WFS shmimMonitor thread, the dark shmimMonitor thread, and
/// the wfs measurement thread owned by dmPokeWFS. Each section installs its own std::thread
/// objects into the harness so one branch can be exercised in isolation without a real
/// appStartup(). This mirrors the setSmThread() and abandonSmThread() pattern in
/// shmimMonitor_test.cpp.
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

/// Verify the full appStartup(), appLogic(), and appShutdown() lifecycle. Covers the single
/// measurement state machine and the continuous plus stop state machine of the wfs thread,
/// and the INDI callbacks including their wrong key error branches.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS full lifecycle", "[dev::dmPokeWFS]" )
{
    // dmPokeWFS::appShutdown() sends SIGUSR1 to the wfs thread. The appStartup() of
    // shmimMonitor installs the same no-op handler for its own threads. Install it here too
    // so it is in place before anything relies on it.
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

    // Call allocate() before appStartup() so m_pokeImage is already valid before the wfs
    // thread can act on a triggered measurement. Otherwise wfsThreadExec() could spin
    // without bound waiting for m_pokeImage.valid().
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::wfsShmimT() ) == 0 );
    REQUIRE( pdt.allocate( dmPokeWFS_tests::dmPokeWFSTest::dmPokeWFST::darkShmimT() ) == 0 );

    // Use the fast and deterministic runSensor() and analyzeSensor() stubs. The real
    // basicRunSensor() would need a live image posting thread.
    pdt.m_useRealRunSensor = false;
    pdt.m_runSensorRV      = 0;
    pdt.m_testDeltaX       = 3;
    pdt.m_testDeltaY       = 4;

    REQUIRE( pdt.appStartup() == 0 );
    mx::sys::milliSleep( 200 );

    REQUIRE( pdt.appLogic() == 0 );

    // INDI callback wrong key branches. The device and name do not match any property.
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

    // The static callback trampolines take the same wrong key branch. Calling them again
    // is harmless.
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_single( &pdt, ipWrongSw ) == -1 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_continuous( &pdt, ipWrongSw ) == -1 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStNewCallBack_stop( &pdt, ipWrongSw ) == -1 );

    // Numeric target update callbacks.
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

    // The wfsFps set property callback.
    pcf::IndiProperty ipFps = pdt.indiP_wfsFps();
    ipFps.add( pcf::IndiElement( "current" ) );
    ipFps["current"] = 100.0;
    REQUIRE( pdt.callSetCallBack_wfsFps( ipFps ) == 0 );
    REQUIRE( dmPokeWFS_tests::dmPokeWFSTest::callStSetCallBack_wfsFps( &pdt, ipFps ) == 0 );

    // Single measurement round trip. Widen the window in which measuring equals 1 so
    // appLogic() can reliably be called while it is active. This exercises the switch
    // reporting branch where single is On and continuous is Off.
    pdt.m_runSensorSleepMs = 150;

    pcf::IndiProperty ipSingle = pdt.indiP_single();
    ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

    // The startup handshake of the wfs thread can take about one real second even after
    // appStartup() has returned. threadStart() polls with sleep(1) while waiting for
    // m_wfsThreadInit to clear. This wait window must therefore be generous.
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

    // Give the thread a moment to settle back to idle after the single measurement.
    for( int i = 0; i < 50 && pdt.measuringState() != 0; ++i )
    {
        mx::sys::milliSleep( 20 );
    }
    REQUIRE( pdt.measuringState() == 0 );

    REQUIRE( pdt.appLogic() == 0 );

    // Continuous measurement followed by a stop.
    pcf::IndiProperty ipContinuous = pdt.indiP_continuous();
    ipContinuous["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.callNewCallBack_continuous( ipContinuous ) == 0 );

    // Let it run for a few measurement cycles.
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

    // Widen the window between runSensor() and analyzeSensor(). This lets the test exercise
    // two branches together in the middle of one cycle, before the measurement loop
    // notices either one and exits on its own. The first is the branch of the continuous
    // callback taken when the switch is Off and m_measuring is nonzero. It sets
    // m_stopMeasurement to true. The second is the identical m_measuring nonzero branch in
    // the stop property callback. The loop checks the flag after runSensor() returns and
    // before analyzeSensor().
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

    // Toggling the continuous property itself to Off is the normal INDI way to request a
    // stop. The separate stop property is used below. This exercises the branch of the
    // continuous callback taken when the switch is Off and m_measuring is nonzero.
    pcf::IndiProperty ipContinuousOff = pdt.indiP_continuous();
    ipContinuousOff["toggle"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.callNewCallBack_continuous( ipContinuousOff ) == 0 );

    // The loop is still in the middle of a cycle and measuring has not reset yet. This
    // exercises the separate m_measuring nonzero branch in the stop property callback.
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

    // dmPokeWFS::appShutdown() only signals the wfs thread. It does not set m_shutdown
    // itself. The signal handling and main loop of the app normally do that before
    // appShutdown() is called. The outer loop of the thread only exits once
    // derived().m_shutdown is nonzero, so set it explicitly here to avoid hanging.
    pdt.requestShutdown();
    REQUIRE( pdt.appShutdown() == 0 );
}

/// Verify the branches of the INDI callbacks taken when a required element is missing.
/// These need a property whose device and name match, so that the
/// INDI_VALIDATE_CALLBACK_PROPS_DERIVED key check passes, but which lacks the element the
/// callback looks for. This is distinct from the wrong device or name branch tested above.
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

    // Build a property with the same type, device, and name as orig but with no elements.
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

/// Verify the failure logging branches of wfsThreadExec() when runSensor() or
/// analyzeSensor() fails, its defensive exit branch when neither single nor continuous is
/// set, and its wait for the poke image to become valid before measuring.
/**
 * \ingroup dmPokeWFS_tests
 */
TEST_CASE( "Test dmPokeWFS wfsThreadExec failure and edge-case paths", "[dev::dmPokeWFS]" )
{
    // Install the no-op SIGUSR1 handler that appShutdown() relies on to wake the wfs thread.
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
        // threadStart() polls with sleep(1) while waiting for m_wfsThreadInit to clear.
        // That can take about one real second even after appStartup() has returned. Wait
        // comfortably past that worst case so the thread is already parked on its
        // semaphore wait before the test posts to it.
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

        // The wfs thread has now returned entirely rather than just gone idle. A later
        // single measurement request is never serviced.
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
        // allocate() is deliberately not called before appStartup() here. m_pokeImage is
        // therefore not yet valid when the wfs thread wakes up for a measurement.
        REQUIRE( pdt.appStartup() == 0 );
        // Wait comfortably past the worst case startup handshake of about one second in
        // threadStart() so the thread is already parked on its semaphore wait. Otherwise
        // it might not check m_pokeImage.valid() until well after the allocate() calls
        // below, which would defeat the race this section exercises.
        mx::sys::milliSleep( 1200 );

        pcf::IndiProperty ipSingle = pdt.indiP_single();
        ipSingle["toggle"].setSwitchState( pcf::IndiElement::On );
        REQUIRE( pdt.callNewCallBack_single( ipSingle ) == 0 );

        // Give the thread a real chance to enter the wait loop for the poke image before
        // making it valid.
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
