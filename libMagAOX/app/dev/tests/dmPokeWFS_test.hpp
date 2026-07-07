
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

/// Test harness for dev::dmPokeWFS
/**
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

    /// Stub telemeter interface: dmPokeWFS::recordPokeLoop() calls
    /// derived().template telem<telem_pokeloop>(msg). No real dev::telemeter is used here.
    int m_telemCount{ 0 };

    template <class telT>
    int telem( const typename telT::messageT &msg )
    {
        static_cast<void>( msg );
        ++m_telemCount;
        return 0;
    }

    // -- runSensor()/analyzeSensor() interface required by dmPokeWFS, controllable by tests --

    bool m_useRealRunSensor{ false }; ///< if true, runSensor() calls the real basicRunSensor()
    int  m_runSensorRV{ 0 };          ///< canned return value when not using the real sensor
    bool m_lastFirstRun{ false };
    int  m_runSensorCalls{ 0 };

    int runSensor( bool firstRun )
    {
        m_lastFirstRun = firstRun;
        ++m_runSensorCalls;
        if( m_useRealRunSensor )
        {
            return basicRunSensor();
        }
        return m_runSensorRV;
    }

    int   m_analyzeSensorRV{ 0 };
    float m_testDeltaX{ 0 };
    float m_testDeltaY{ 0 };
    int   m_analyzeSensorCalls{ 0 };

    int analyzeSensor()
    {
        ++m_analyzeSensorCalls;
        if( m_analyzeSensorRV < 0 )
        {
            return m_analyzeSensorRV;
        }
        return updateMeasurement( m_testDeltaX, m_testDeltaY );
    }

    /// Telemeter interface used by real apps; not exercised directly here.
    int checkRecordTimes()
    {
        return 0;
    }

    // -- test-only setters mirroring shmimMonitor's real connection-time behavior --

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

    // -- wrappers exposing protected dmPokeWFS internals for direct testing --

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

    // -- INDI property accessors and callback wrappers --

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

    // -- static INDI callback trampolines --

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

    void requestShutdown()
    {
        m_shutdown = 1;
    }

    /// sem_init() the image/wfs semaphores without running the rest of appStartup(),
    /// so basicTimedPoke()/basicRunSensor()/processImage() can be exercised directly.
    void initSemaphoresForTest()
    {
        sem_init( &m_imageSemaphore, 0, 0 );
        sem_init( &m_wfsSemaphore, 0, 0 );
    }

    /// Populate the measurement INDI property's elements without running the rest of
    /// appStartup(), so updateMeasurement() can be tested in isolation.
    void prepMeasurementIndiForTest()
    {
        m_indiP_measurement.add( { "delta_x", 0.0 } );
        m_indiP_measurement.add( { "delta_y", 0.0 } );
        m_indiP_measurement.add( { "counter", 0 } );
    }
};

#ifdef XWCTEST_NAMESPACE
} // namespace XWCTEST_NAMESPACE
#endif

} // namespace dmPokeWFS_tests

// LCOV_EXCL_STOP
