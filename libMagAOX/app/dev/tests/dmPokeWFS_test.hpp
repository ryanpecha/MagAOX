/** \file dmPokeWFS_test.hpp
  * \brief Test harness for the MagAOX::app::dev::dmPokeWFS device mixin.
  *
  * Declares dmPokeWFSTest, a MagAOXApp<false> that mixes in dmPokeWFS and two real
  * shmimMonitor bases, one for the WFS camera stream and one for the dark stream.
  * The harness stubs out the telemeter interface with a call counter and replaces
  * runSensor() and analyzeSensor() with controllable stubs that can return canned
  * values, sleep, or call the real basicRunSensor(). It also exposes protected
  * dmPokeWFS members, INDI callbacks, and thread bookkeeping so tests can drive
  * each branch directly without a full appStartup().
  *
  * \ingroup dmPokeWFS_tests
  */

#include "../../MagAOXApp.hpp"
#include "../dmPokeWFS.hpp"
#include "../shmimMonitor.hpp"

// LCOV_EXCL_START

#ifndef XWCTEST_NAMESPACE
    #define MAPPNS MagAOX::app::dev
#else
    #define MAPPNS MagAOX::app::dev::XWCTEST_NAMESPACE
#endif

namespace dmPokeWFS_tests
{

#ifdef XWCTEST_NAMESPACE
namespace XWCTEST_NAMESPACE
{
#endif

/// Test harness for dev::dmPokeWFS.
/** A MagAOXApp<false> that mixes in dmPokeWFS and two shmimMonitor bases. It stubs
 * the telemeter interface and the runSensor() and analyzeSensor() hooks, and exposes
 * protected internals so tests can call them directly.
 *
 * \ingroup dmPokeWFS_tests
 */
struct dmPokeWFSTest : public MagAOX::app::MagAOXApp<false>,
                       public MAPPNS::dmPokeWFS<dmPokeWFSTest>,
                       public MAPPNS::shmimMonitor<dmPokeWFSTest, MAPPNS::dmPokeWFS<dmPokeWFSTest>::wfsShmimT>,
                       public MAPPNS::shmimMonitor<dmPokeWFSTest, MAPPNS::dmPokeWFS<dmPokeWFSTest>::darkShmimT>
{

    friend class MAPPNS::dmPokeWFS<dmPokeWFSTest>;
    typedef MAPPNS::dmPokeWFS<dmPokeWFSTest> dmPokeWFST;

    friend class MAPPNS::shmimMonitor<dmPokeWFSTest, dmPokeWFST::wfsShmimT>;
    typedef MAPPNS::shmimMonitor<dmPokeWFSTest, dmPokeWFST::wfsShmimT> shmimMonitorT;

    friend class MAPPNS::shmimMonitor<dmPokeWFSTest, dmPokeWFST::darkShmimT>;
    typedef MAPPNS::shmimMonitor<dmPokeWFSTest, dmPokeWFST::darkShmimT> darkShmimMonitorT;

    dmPokeWFSTest( const std::string &git_sha1, const bool git_modified )
        : MagAOX::app::MagAOXApp<false>( git_sha1, git_modified )
    {
        m_configName = "dmPokeWFStest";
    }

    ~dmPokeWFSTest() noexcept
    {
    }

    shmimMonitorT &shmimMonitor()
    {
        return *static_cast<shmimMonitorT *>( this );
    }

    darkShmimMonitorT &darkShmimMonitor()
    {
        return *static_cast<darkShmimMonitorT *>( this );
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return dmPokeWFST::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return dmPokeWFST::loadConfig( config );
    }

    int appStartup()
    {
        return dmPokeWFST::appStartup();
    }

    int appLogic()
    {
        return dmPokeWFST::appLogic();
    }

    int appShutdown()
    {
        return dmPokeWFST::appShutdown();
    }

    /// Stub of the telemeter interface. dmPokeWFS::recordPokeLoop() calls
    /// derived().template telem<telem_pokeloop>(msg). No real dev::telemeter is used here.
    /// The stub only counts calls.
    int m_telemCount{ 0 };

    template <class telT>
    int telem( const typename telT::messageT &msg )
    {
        static_cast<void>( msg );
        ++m_telemCount;
        return 0;
    }

    // The runSensor() and analyzeSensor() interface that dmPokeWFS requires. Tests control
    // the behavior through the members below.

    bool m_useRealRunSensor{ false }; ///< If true, runSensor() calls the real basicRunSensor().
    int  m_runSensorRV{ 0 };          ///< Canned return value when not using the real sensor.
    bool m_lastFirstRun{ false };     ///< The firstRun argument of the last runSensor() call.
    int  m_runSensorCalls{ 0 };       ///< Number of runSensor() calls so far.
    int  m_runSensorSleepMs{ 0 }; ///< If greater than zero, runSensor() sleeps this long so a test can observe the measuring state.

    /// Stub runSensor(). Records the call, optionally sleeps, then returns the canned value
    /// or the result of the real basicRunSensor().
    int runSensor( bool firstRun )
    {
        m_lastFirstRun = firstRun;
        ++m_runSensorCalls;
        if( m_runSensorSleepMs > 0 )
        {
            mx::sys::milliSleep( m_runSensorSleepMs );
        }
        if( m_useRealRunSensor )
        {
            return basicRunSensor();
        }
        return m_runSensorRV;
    }

    int   m_analyzeSensorRV{ 0 };    ///< If negative, analyzeSensor() returns this without updating.
    float m_testDeltaX{ 0 };         ///< Delta x that analyzeSensor() reports.
    float m_testDeltaY{ 0 };         ///< Delta y that analyzeSensor() reports.
    int   m_analyzeSensorCalls{ 0 }; ///< Number of analyzeSensor() calls so far.

    /// Stub analyzeSensor(). Either fails with the canned value or reports the test deltas.
    int analyzeSensor()
    {
        ++m_analyzeSensorCalls;
        if( m_analyzeSensorRV < 0 )
        {
            return m_analyzeSensorRV;
        }
        return updateMeasurement( m_testDeltaX, m_testDeltaY );
    }

    /// Telemeter interface used by real apps. It is not exercised directly here.
    int checkRecordTimes()
    {
        return 0;
    }

    // Test-only setters that mirror what shmimMonitor does when it connects to a real stream.

    void setWfsSize( uint32_t w, uint32_t h, uint8_t dtype )
    {
        shmimMonitorT::m_width    = w;
        shmimMonitorT::m_height   = h;
        shmimMonitorT::m_dataType = dtype;
    }

    void setDarkSize( uint32_t w, uint32_t h, uint8_t dtype )
    {
        darkShmimMonitorT::m_width    = w;
        darkShmimMonitorT::m_height   = h;
        darkShmimMonitorT::m_dataType = dtype;
    }

    // Wrappers that expose protected dmPokeWFS internals for direct testing.

    int callBasicRunSensor()
    {
        return basicRunSensor();
    }

    int callBasicTimedPoke( float pokeSign )
    {
        return basicTimedPoke( pokeSign );
    }

    int callRecordTelem()
    {
        return recordTelem( static_cast<const MagAOX::logger::telem_pokeloop *>( nullptr ) );
    }

    int callRecordPokeLoop( bool force = false )
    {
        return recordPokeLoop( force );
    }

    int callUpdateMeasurement( float deltaX, float deltaY )
    {
        return updateMeasurement( deltaX, deltaY );
    }

    mx::improc::eigenImage<float> &pokeLocal()
    {
        return m_pokeLocal;
    }

    mx::improc::milkImage<float> &pokeImage()
    {
        return m_pokeImage;
    }

    mx::improc::milkImage<float> &rawImage()
    {
        return m_rawImage;
    }

    mx::improc::eigenImage<float> &darkImage()
    {
        return m_darkImage;
    }

    bool darkValidFlag()
    {
        return m_darkValid;
    }

    float testDeltaX()
    {
        return m_deltaX;
    }

    float testDeltaY()
    {
        return m_deltaY;
    }

    uint64_t testCounter()
    {
        return m_counter;
    }

    int measuringState()
    {
        return m_measuring;
    }

    bool singleFlag()
    {
        return m_single;
    }

    bool continuousFlag()
    {
        return m_continuous;
    }

    bool stopMeasurementFlag()
    {
        return m_stopMeasurement;
    }

    void setStopMeasurement( bool b )
    {
        m_stopMeasurement = b;
    }

    void setPokeLocalZero()
    {
        m_pokeLocal.setZero();
    }

    // INDI property accessors and callback wrappers.

    pcf::IndiProperty indiP_pokeAmp()
    {
        return m_indiP_poke_amp;
    }

    pcf::IndiProperty indiP_nPokeImages()
    {
        return m_indiP_nPokeImages;
    }

    pcf::IndiProperty indiP_nPokeAverage()
    {
        return m_indiP_nPokeAverage;
    }

    pcf::IndiProperty indiP_wfsFps()
    {
        return m_indiP_wfsFps;
    }

    pcf::IndiProperty indiP_single()
    {
        return m_indiP_single;
    }

    pcf::IndiProperty indiP_continuous()
    {
        return m_indiP_continuous;
    }

    pcf::IndiProperty indiP_stop()
    {
        return m_indiP_stop;
    }

    pcf::IndiProperty indiP_measurement()
    {
        return m_indiP_measurement;
    }

    int callNewCallBack_pokeAmp( const pcf::IndiProperty &ip )
    {
        return newCallBack_m_indiP_poke_amp( ip );
    }

    int callNewCallBack_nPokeImages( const pcf::IndiProperty &ip )
    {
        return newCallBack_m_indiP_nPokeImages( ip );
    }

    int callNewCallBack_nPokeAverage( const pcf::IndiProperty &ip )
    {
        return newCallBack_m_indiP_nPokeAverage( ip );
    }

    int callSetCallBack_wfsFps( const pcf::IndiProperty &ip )
    {
        return setCallBack_m_indiP_wfsFps( ip );
    }

    int callNewCallBack_single( const pcf::IndiProperty &ip )
    {
        return newCallBack_m_indiP_single( ip );
    }

    int callNewCallBack_continuous( const pcf::IndiProperty &ip )
    {
        return newCallBack_m_indiP_continuous( ip );
    }

    int callNewCallBack_stop( const pcf::IndiProperty &ip )
    {
        return newCallBack_m_indiP_stop( ip );
    }

    // Wrappers for the static INDI callback trampolines.

    static int callStNewCallBack_pokeAmp( void *app, const pcf::IndiProperty &ip )
    {
        return st_newCallBack_m_indiP_poke_amp( app, ip );
    }

    static int callStNewCallBack_nPokeImages( void *app, const pcf::IndiProperty &ip )
    {
        return st_newCallBack_m_indiP_nPokeImages( app, ip );
    }

    static int callStNewCallBack_nPokeAverage( void *app, const pcf::IndiProperty &ip )
    {
        return st_newCallBack_m_indiP_nPokeAverage( app, ip );
    }

    static int callStSetCallBack_wfsFps( void *app, const pcf::IndiProperty &ip )
    {
        return st_setCallBack_m_indiP_wfsFps( app, ip );
    }

    static int callStNewCallBack_single( void *app, const pcf::IndiProperty &ip )
    {
        return st_newCallBack_m_indiP_single( app, ip );
    }

    static int callStNewCallBack_continuous( void *app, const pcf::IndiProperty &ip )
    {
        return st_newCallBack_m_indiP_continuous( app, ip );
    }

    static int callStNewCallBack_stop( void *app, const pcf::IndiProperty &ip )
    {
        return st_newCallBack_m_indiP_stop( app, ip );
    }

    /// Set the app shutdown flag so the wfs thread's outer loop can exit.
    void requestShutdown()
    {
        m_shutdown = 1;
    }

    /// Initialize the image and wfs semaphores without running the rest of appStartup().
    /// This lets basicTimedPoke(), basicRunSensor(), and processImage() be exercised directly.
    void initSemaphoresForTest()
    {
        sem_init( &m_imageSemaphore, 0, 0 );
        sem_init( &m_wfsSemaphore, 0, 0 );
    }

    /// Populate the elements of the measurement INDI property without running the rest of
    /// appStartup(). This lets updateMeasurement() be tested in isolation.
    void prepMeasurementIndiForTest()
    {
        m_indiP_measurement.add( { "delta_x", 0.0 } );
        m_indiP_measurement.add( { "delta_y", 0.0 } );
        m_indiP_measurement.add( { "counter", 0 } );
    }

    /// Post the real wfs semaphore without setting m_single or m_continuous first.
    /// Production code never does this. Both callbacks always set one of the flags
    /// immediately before posting. This exercises the defensive branch in wfsThreadExec()
    /// taken when neither flag is set, which makes the thread return outright.
    void postWfsSemaphoreWithNoModeSet()
    {
        m_single     = 0;
        m_continuous = 0;
        sem_post( &m_wfsSemaphore );
    }

    // Direct control of the std::thread bookkeeping in dmPokeWFS and shmimMonitor. This
    // mirrors the setSmThread() and abandonSmThread() pattern in shmimMonitor_test.cpp.
    // It lets each of the three separate "thread has exited" propagation branches in
    // appLogic() be exercised on its own without a full, real appStartup().

    /// Install a test thread as the dmPokeWFS wfs measurement thread.
    void setWfsMeasurementThread( std::thread &&t )
    {
        m_wfsThread = std::move( t );
    }

    /// Drop the wfs measurement thread object without joining it.
    /// The thread has already finished. Swapping it into a local and then placement
    /// constructing a fresh std::thread over that local leaves the local not joinable, so
    /// its destructor does not call std::terminate(). The member is left default constructed.
    void abandonWfsMeasurementThread()
    {
        std::thread tmp;
        tmp.swap( m_wfsThread );
        new( &tmp ) std::thread();
    }

    /// Join the wfs measurement thread if it is joinable.
    void joinWfsMeasurementThread()
    {
        if( m_wfsThread.joinable() )
            m_wfsThread.join();
    }

    /// Install a test thread as the WFS shmimMonitor thread.
    void setWfsMonitorThread( std::thread &&t )
    {
        shmimMonitorT::m_smThread = std::move( t );
    }

    /// Same as abandonWfsMeasurementThread() for the WFS shmimMonitor thread.
    void abandonWfsMonitorThread()
    {
        std::thread tmp;
        tmp.swap( shmimMonitorT::m_smThread );
        new( &tmp ) std::thread();
    }

    /// Join the WFS shmimMonitor thread if it is joinable.
    void joinWfsMonitorThread()
    {
        if( shmimMonitorT::m_smThread.joinable() )
            shmimMonitorT::m_smThread.join();
    }

    /// Install a test thread as the dark shmimMonitor thread.
    void setDarkMonitorThread( std::thread &&t )
    {
        darkShmimMonitorT::m_smThread = std::move( t );
    }

    /// Join the dark shmimMonitor thread if it is joinable.
    void joinDarkMonitorThread()
    {
        if( darkShmimMonitorT::m_smThread.joinable() )
            darkShmimMonitorT::m_smThread.join();
    }

    /// Same as abandonWfsMeasurementThread() for the dark shmimMonitor thread.
    void abandonDarkMonitorThread()
    {
        std::thread tmp;
        tmp.swap( darkShmimMonitorT::m_smThread );
        new( &tmp ) std::thread();
    }
};

#ifdef XWCTEST_NAMESPACE
} // namespace XWCTEST_NAMESPACE
#endif

} // namespace dmPokeWFS_tests

// LCOV_EXCL_STOP
