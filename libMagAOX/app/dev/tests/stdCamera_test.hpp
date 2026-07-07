/** \file stdCamera_test.hpp
 * \brief Test harnesses for MagAOX::app::dev::stdCamera.
 * \author Claude
 *
 * \ingroup testing
 */

#include "../../../../tests/catch2/catch.hpp"

#include "../../MagAOXApp.hpp"
#include "../stdCamera.hpp"
#include "../telemeter.hpp"

// LCOV_EXCL_START

namespace stdCamera_tests
{

/// Test harness with every stdCamera capability flag turned on.
/** This harness is used to drive every "enabled" code path in stdCamera: temperature control,
 * readout/vshift speed, EM gain, exposure time, FPS control, fan speed, analog gain, LED, synchro,
 * camera modes, ROI, crop mode, shutter, focus (both direct and helper-driven), and the state string.
 *
 * It also overrides registerIndiPropertyNew/ReadOnly/Set so that individual registration calls can be
 * made to fail one at a time, exercising every error-return branch in stdCamera<derivedT>::appStartup().
 *
 * \ingroup stdCamera_tests
 */
struct stdCameraFullHarness : public MagAOX::app::MagAOXApp<false>,
                              public MagAOX::app::dev::stdCamera<stdCameraFullHarness>,
                              public MagAOX::app::dev::telemeter<stdCameraFullHarness>
{
    friend class MagAOX::app::dev::stdCamera<stdCameraFullHarness>;
    friend class MagAOX::app::dev::telemeter<stdCameraFullHarness>;

    typedef MagAOX::app::dev::stdCamera<stdCameraFullHarness>   stdCameraT;
    typedef MagAOX::app::dev::telemeter<stdCameraFullHarness>   telemeterT;

    // Re-publish MagAOXApp's protected power-management state for direct test inspection/mutation.
    using MagAOX::app::MagAOXApp<false>::m_powerMgtEnabled;
    using MagAOX::app::MagAOXApp<false>::m_powerOnWait;
    using MagAOX::app::MagAOXApp<false>::m_powerOnCounter;

    // Re-publish stdCamera's protected state for direct test inspection/mutation.
    using stdCameraT::m_adcSpeed;
    using stdCameraT::m_analogGainName;
    using stdCameraT::m_analogGainNameLabels;
    using stdCameraT::m_analogGainNameSet;
    using stdCameraT::m_analogGainNames;
    using stdCameraT::m_analogGainValid;
    using stdCameraT::m_cameraModes;
    using stdCameraT::m_ccdTemp;
    using stdCameraT::m_ccdTempSetpt;
    using stdCameraT::m_cropMode;
    using stdCameraT::m_cropModeSet;
    using stdCameraT::m_currentROI;
    using stdCameraT::m_defaultFanSpeed;
    using stdCameraT::m_defaultLEDState;
    using stdCameraT::m_defaultReadoutSpeed;
    using stdCameraT::m_defaultVShiftSpeed;
    using stdCameraT::m_default_bin_x;
    using stdCameraT::m_default_bin_y;
    using stdCameraT::m_default_h;
    using stdCameraT::m_default_w;
    using stdCameraT::m_default_x;
    using stdCameraT::m_default_y;
    using stdCameraT::m_emGain;
    using stdCameraT::m_emGainSet;
    using stdCameraT::m_expTime;
    using stdCameraT::m_expTimeSet;
    using stdCameraT::m_fanSpeedControlEnabled;
    using stdCameraT::m_fanSpeedName;
    using stdCameraT::m_fanSpeedNameLabels;
    using stdCameraT::m_fanSpeedNameSet;
    using stdCameraT::m_fanSpeedNames;
    using stdCameraT::m_fanSpeedValid;
    using stdCameraT::m_focusGotoFormat;
    using stdCameraT::m_focusGotoHelperConfigured;
    using stdCameraT::m_focusGotoSourceIndices;
    using stdCameraT::m_focusGotoSourceProperties;
    using stdCameraT::m_focusGotoTargetDevice;
    using stdCameraT::m_focusGotoTargetName;
    using stdCameraT::m_focusGotoTargetProperty;
    using stdCameraT::m_focusMonitoredPropertyKeys;
    using stdCameraT::m_focusStateElement;
    using stdCameraT::m_focusStateHelperConfigured;
    using stdCameraT::m_focusStateOnMeansInFocus;
    using stdCameraT::m_focusStateSource;
    using stdCameraT::m_focusStateSourceIndex;
    using stdCameraT::m_fps;
    using stdCameraT::m_fpsSet;
    using stdCameraT::m_full_bin_x;
    using stdCameraT::m_full_bin_y;
    using stdCameraT::m_full_currbin_h;
    using stdCameraT::m_full_currbin_w;
    using stdCameraT::m_full_currbin_x;
    using stdCameraT::m_full_currbin_y;
    using stdCameraT::m_full_h;
    using stdCameraT::m_full_w;
    using stdCameraT::m_full_x;
    using stdCameraT::m_full_y;
    using stdCameraT::m_hasFocus;
    using stdCameraT::m_indiP_analogGain;
    using stdCameraT::m_indiP_cropMode;
    using stdCameraT::m_indiP_emGain;
    using stdCameraT::m_indiP_exptime;
    using stdCameraT::m_indiP_fanSpeed;
    using stdCameraT::m_indiP_focus;
    using stdCameraT::m_indiP_focusMonitoredProperties;
    using stdCameraT::m_indiP_fps;
    using stdCameraT::m_indiP_fullROI;
    using stdCameraT::m_indiP_gotoFocus;
    using stdCameraT::m_indiP_led;
    using stdCameraT::m_indiP_mode;
    using stdCameraT::m_indiP_readoutSpeed;
    using stdCameraT::m_indiP_reconfig;
    using stdCameraT::m_indiP_roi_bin_x;
    using stdCameraT::m_indiP_roi_bin_y;
    using stdCameraT::m_indiP_roi_check;
    using stdCameraT::m_indiP_roi_default;
    using stdCameraT::m_indiP_roi_full;
    using stdCameraT::m_indiP_roi_fullbin;
    using stdCameraT::m_indiP_roi_h;
    using stdCameraT::m_indiP_roi_last;
    using stdCameraT::m_indiP_roi_loadlast;
    using stdCameraT::m_indiP_roi_set;
    using stdCameraT::m_indiP_roi_w;
    using stdCameraT::m_indiP_roi_x;
    using stdCameraT::m_indiP_roi_y;
    using stdCameraT::m_indiP_shutter;
    using stdCameraT::m_indiP_shutterStatus;
    using stdCameraT::m_indiP_stateString;
    using stdCameraT::m_indiP_synchro;
    using stdCameraT::m_indiP_temp;
    using stdCameraT::m_indiP_tempcont;
    using stdCameraT::m_indiP_tempstat;
    using stdCameraT::m_indiP_vShiftSpeed;
    using stdCameraT::m_lastROI;
    using stdCameraT::m_ledState;
    using stdCameraT::m_ledStateSet;
    using stdCameraT::m_ledStateValid;
    using stdCameraT::m_maxEMGain;
    using stdCameraT::m_maxExpTime;
    using stdCameraT::m_maxFPS;
    using stdCameraT::m_maxROIBinning_x;
    using stdCameraT::m_maxROIBinning_y;
    using stdCameraT::m_maxROIHeight;
    using stdCameraT::m_maxROIWidth;
    using stdCameraT::m_maxROIx;
    using stdCameraT::m_maxROIy;
    using stdCameraT::m_maxTemp;
    using stdCameraT::m_minExpTime;
    using stdCameraT::m_minFPS;
    using stdCameraT::m_minROIBinning_x;
    using stdCameraT::m_minROIBinning_y;
    using stdCameraT::m_minROIHeight;
    using stdCameraT::m_minROIWidth;
    using stdCameraT::m_minROIx;
    using stdCameraT::m_minROIy;
    using stdCameraT::m_minTemp;
    using stdCameraT::m_modeName;
    using stdCameraT::m_nextMode;
    using stdCameraT::m_nextROI;
    using stdCameraT::m_readoutSpeedName;
    using stdCameraT::m_readoutSpeedNameLabels;
    using stdCameraT::m_readoutSpeedNameSet;
    using stdCameraT::m_readoutSpeedNames;
    using stdCameraT::m_shutterState;
    using stdCameraT::m_shutterStatus;
    using stdCameraT::m_startupMode;
    using stdCameraT::m_startupTemp;
    using stdCameraT::m_stepExpTime;
    using stdCameraT::m_stepFPS;
    using stdCameraT::m_stepROIBinning_x;
    using stdCameraT::m_stepROIBinning_y;
    using stdCameraT::m_stepROIHeight;
    using stdCameraT::m_stepROIWidth;
    using stdCameraT::m_stepROIx;
    using stdCameraT::m_stepROIy;
    using stdCameraT::m_stepTemp;
    using stdCameraT::m_synchro;
    using stdCameraT::m_synchroSet;
    using stdCameraT::m_tempControlOnTarget;
    using stdCameraT::m_tempControlStatus;
    using stdCameraT::m_tempControlStatusSet;
    using stdCameraT::m_tempControlStatusStr;
    using stdCameraT::m_vShiftSpeedName;
    using stdCameraT::m_vShiftSpeedNameLabels;
    using stdCameraT::m_vShiftSpeedNameSet;
    using stdCameraT::m_vShiftSpeedNames;
    using stdCameraT::m_vshiftSpeed;



    static constexpr bool c_stdCamera_tempControl      = true;
    static constexpr bool c_stdCamera_temp             = true;
    static constexpr bool c_stdCamera_readoutSpeed      = true;
    static constexpr bool c_stdCamera_vShiftSpeed        = true;
    static constexpr bool c_stdCamera_fanSpeed           = true;
    static constexpr bool c_stdCamera_led                = true;
    static constexpr bool c_stdCamera_analogGain         = true;
    static constexpr bool c_stdCamera_hasFocus           = true;
    static constexpr bool c_stdCamera_emGain             = true;
    static constexpr bool c_stdCamera_exptimeCtrl        = true;
    static constexpr bool c_stdCamera_fpsCtrl            = true;
    static constexpr bool c_stdCamera_fps                = true;
    static constexpr bool c_stdCamera_synchro            = true;
    static constexpr bool c_stdCamera_usesModes          = true;
    static constexpr bool c_stdCamera_usesROI            = true;
    static constexpr bool c_stdCamera_cropMode           = true;
    static constexpr bool c_stdCamera_hasShutter         = true;
    static constexpr bool c_stdCamera_usesStateString    = true;

    // -- call counters / captured values, for assertions --
    int m_powerOnDefaultsCalls{ 0 };
    int m_setTempControlCalls{ 0 };
    int m_setTempSetPtCalls{ 0 };
    int m_setReadoutSpeedCalls{ 0 };
    int m_setVShiftSpeedCalls{ 0 };
    int m_setFanSpeedCalls{ 0 };
    int m_setAnalogGainCalls{ 0 };
    int m_setLEDCalls{ 0 };
    int m_setEMGainCalls{ 0 };
    int m_setExpTimeCalls{ 0 };
    int m_setFPSCalls{ 0 };
    int m_setSynchroCalls{ 0 };
    int m_setCropModeCalls{ 0 };
    int m_checkNextROICalls{ 0 };
    int m_setNextROICalls{ 0 };
    int m_setShutterCalls{ 0 };
    int m_lastShutterSS{ -100 };
    int m_gotoFocusCalls{ 0 };

    // -- forced return values, to exercise error-propagation branches --
    int  m_setTempSetPtResult{ 0 };
    int  m_setTempControlResult{ 0 };
    int  m_setReadoutSpeedResult{ 0 };
    int  m_setVShiftSpeedResult{ 0 };
    int  m_setFanSpeedResult{ 0 };
    int  m_setAnalogGainResult{ 0 };
    int  m_setLEDResult{ 0 };
    int  m_setEMGainResult{ 0 };
    int  m_setExpTimeResult{ 0 };
    int  m_setFPSResult{ 0 };
    int  m_setSynchroResult{ 0 };
    int  m_setCropModeResult{ 0 };
    int  m_checkNextROIResult{ 0 };
    int  m_setNextROIResult{ 0 };
    int  m_setShutterResult{ 0 };
    bool m_checkFocusResult{ false };
    int  m_gotoFocusResult{ 0 };
    std::string m_stateStringResult{ "STATESTRING" };
    bool        m_stateStringValidResult{ true };

    bool m_reconfig{ false };

    // -- captures used to test the goto-focus helper without a real INDI driver --
    pcf::IndiProperty m_lastSentProperty;
    int               m_sendNewPropertyResult{ 0 };

    // -- register-call fault injection, used to hit every registration failure branch in appStartup() --
    int m_regCallCount{ 0 };
    int m_regFailAt{ -1 };

    stdCameraFullHarness() : MagAOX::app::MagAOXApp<false>( "", false )
    {
        m_configName = "stdcamtest";

        m_defaultReadoutSpeed = "slow";
        m_readoutSpeedNames       = { "slow", "fast" };
        m_readoutSpeedNameLabels  = { "Slow", "Fast" };

        m_defaultVShiftSpeed = "std";
        m_vShiftSpeedNames      = { "std", "fast" };
        m_vShiftSpeedNameLabels = { "Standard", "Fast" };

        m_fanSpeedNames      = { "low", "high" };
        m_fanSpeedNameLabels = { "Low", "High" };
        m_defaultFanSpeed    = "low"; // a valid default so tests unrelated to fan-speed validation don't need to set it

        m_analogGainNames      = { "low", "high" };
        m_analogGainNameLabels = { "Low", "High" };

        m_maxEMGain = 100;

        m_minROIx = 0;
        m_maxROIx = 1023;
        m_minROIy = 0;
        m_maxROIy = 1023;

        m_default_x = 511.5;
        m_default_y = 511.5;
        m_default_w = 1024;
        m_default_h = 1024;
        m_default_bin_x = 1;
        m_default_bin_y = 1;

        m_full_x = 511.5;
        m_full_y = 511.5;
        m_full_w = 1024;
        m_full_h = 1024;
        m_full_bin_x = 1;
        m_full_bin_y = 1;

        m_currentROI.x = m_default_x;
        m_currentROI.y = m_default_y;
        m_currentROI.w = m_default_w;
        m_currentROI.h = m_default_h;
        m_currentROI.bin_x = m_default_bin_x;
        m_currentROI.bin_y = m_default_bin_y;
        m_nextROI = m_currentROI;
    }

    ~stdCameraFullHarness() noexcept override
    {
    }

    // -- disambiguating wrappers (both MagAOXApp and stdCamera/telemeter declare these) --
    int setupConfig( mx::app::appConfigurator &config )
    {
        if( stdCameraT::setupConfig( config ) < 0 )
            return -1;

        // stdCamera::loadConfig() reads camera.full_x/y/w/h/bin_x/bin_y via config(), which only resolves
        // keys previously registered with config.add(). stdCamera itself never registers these (the docs
        // say derivedT must set them directly before appStartup()), so a real device app that wants them
        // configurable from a file must add them itself, exactly as done here for testing purposes.
        config.add( "camera.full_x", "", "camera.full_x", argType::Required, "camera", "full_x", false, "float", "" );
        config.add( "camera.full_y", "", "camera.full_y", argType::Required, "camera", "full_y", false, "float", "" );
        config.add( "camera.full_w", "", "camera.full_w", argType::Required, "camera", "full_w", false, "int", "" );
        config.add( "camera.full_h", "", "camera.full_h", argType::Required, "camera", "full_h", false, "int", "" );
        config.add(
            "camera.full_bin_x", "", "camera.full_bin_x", argType::Required, "camera", "full_bin_x", false, "int", "" );
        config.add(
            "camera.full_bin_y", "", "camera.full_bin_y", argType::Required, "camera", "full_bin_y", false, "int", "" );

        return telemeterT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        if( stdCameraT::loadConfig( config ) < 0 )
            return -1;
        return telemeterT::loadConfig( config );
    }

    // Bring the trueFalseT tag-dispatch overloads back into scope: the no-argument "derived-class
    // interface" methods with the same names declared below in this harness would otherwise hide them.
    using stdCameraT::checkFocus;
    using stdCameraT::checkNextROI;
    using stdCameraT::gotoFocus;
    using stdCameraT::setAnalogGain;
    using stdCameraT::setCropMode;
    using stdCameraT::setEMGain;
    using stdCameraT::setExpTime;
    using stdCameraT::setFPS;
    using stdCameraT::setFanSpeed;
    using stdCameraT::setLED;
    using stdCameraT::setNextROI;
    using stdCameraT::setReadoutSpeed;
    using stdCameraT::setShutter;
    using stdCameraT::setSynchro;
    using stdCameraT::setTempControl;
    using stdCameraT::setTempSetPt;
    using stdCameraT::setVShiftSpeed;
    using stdCameraT::stateString;
    using stdCameraT::stateStringValid;

    // Note: appStartup() intentionally does NOT also start the telemeter log thread (unlike a real
    // device app's appStartup()). Most tests only need stdCamera's own startup logic, and starting a
    // real background log thread in every one of the many Catch2 SECTION re-executions across this test
    // file is needlessly slow. Call startTelemetry() explicitly in the few tests that actually record
    // telemetry (see recordCamera()).
    int appStartup() override
    {
        return stdCameraT::appStartup();
    }

    /// Start the telemeter log thread. Only needed by tests that actually call recordCamera()/telem().
    int startTelemetry()
    {
        return telemeterT::appStartup();
    }

    int appLogic() override
    {
        return stdCameraT::appLogic();
    }

    int appShutdown() override
    {
        stdCameraT::appShutdown();
        return telemeterT::appShutdown();
    }

    int onPowerOff() override
    {
        return stdCameraT::onPowerOff();
    }

    int whilePowerOff() override
    {
        return stdCameraT::whilePowerOff();
    }

    int checkRecordTimes()
    {
        return telemeterT::checkRecordTimes( MagAOX::logger::telem_stdcam() );
    }

    // -- register-call fault injection --
    int registerIndiPropertyNew( pcf::IndiProperty &prop, int ( *cb )( void *, const pcf::IndiProperty & ) )
    {
        ++m_regCallCount;
        if( m_regCallCount == m_regFailAt )
        {
            return -1;
        }
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyNew( prop, cb );
    }

    int registerIndiPropertyReadOnly( pcf::IndiProperty &prop )
    {
        ++m_regCallCount;
        if( m_regCallCount == m_regFailAt )
        {
            return -1;
        }
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyReadOnly( prop );
    }

    int registerIndiPropertySet( pcf::IndiProperty         &prop,
                                 const std::string         &devName,
                                 const std::string         &propName,
                                 int ( *cb )( void *, const pcf::IndiProperty & ) )
    {
        ++m_regCallCount;
        if( m_regCallCount == m_regFailAt )
        {
            return -1;
        }
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertySet( prop, devName, propName, cb );
    }

    // -- capture outgoing goto-focus commands instead of requiring a real INDI driver --
    int sendNewProperty( const pcf::IndiProperty &ipSend )
    {
        m_lastSentProperty = ipSend;
        return m_sendNewPropertyResult;
    }

    // -- stdCamera derived-class interface --
    int powerOnDefaults()
    {
        ++m_powerOnDefaultsCalls;
        return 0;
    }

    int setTempControl()
    {
        ++m_setTempControlCalls;
        m_tempControlStatus    = m_tempControlStatusSet;
        m_tempControlOnTarget  = true;
        m_tempControlStatusStr = m_tempControlStatusSet ? "COOLING" : "OFF";
        return m_setTempControlResult;
    }

    int setTempSetPt()
    {
        ++m_setTempSetPtCalls;
        m_ccdTemp = m_ccdTempSetpt;
        return m_setTempSetPtResult;
    }

    int setReadoutSpeed()
    {
        ++m_setReadoutSpeedCalls;
        m_readoutSpeedName = m_readoutSpeedNameSet;
        return m_setReadoutSpeedResult;
    }

    int setVShiftSpeed()
    {
        ++m_setVShiftSpeedCalls;
        m_vShiftSpeedName = m_vShiftSpeedNameSet;
        return m_setVShiftSpeedResult;
    }

    int setFanSpeed()
    {
        ++m_setFanSpeedCalls;
        m_fanSpeedName  = m_fanSpeedNameSet;
        m_fanSpeedValid = true;
        return m_setFanSpeedResult;
    }

    int setAnalogGain()
    {
        ++m_setAnalogGainCalls;
        m_analogGainName  = m_analogGainNameSet;
        m_analogGainValid = true;
        return m_setAnalogGainResult;
    }

    int setLED()
    {
        ++m_setLEDCalls;
        m_ledState      = m_ledStateSet;
        m_ledStateValid = true;
        return m_setLEDResult;
    }

    int setEMGain()
    {
        ++m_setEMGainCalls;
        m_emGain = m_emGainSet;
        return m_setEMGainResult;
    }

    int setExpTime()
    {
        ++m_setExpTimeCalls;
        m_expTime = m_expTimeSet;
        return m_setExpTimeResult;
    }

    int setFPS()
    {
        ++m_setFPSCalls;
        m_fps = m_fpsSet;
        return m_setFPSResult;
    }

    int setSynchro()
    {
        ++m_setSynchroCalls;
        m_synchro = m_synchroSet;
        return m_setSynchroResult;
    }

    int checkNextROI()
    {
        ++m_checkNextROICalls;
        return m_checkNextROIResult;
    }

    int setNextROI()
    {
        ++m_setNextROICalls;
        m_currentROI = m_nextROI;
        return m_setNextROIResult;
    }

    int setCropMode()
    {
        ++m_setCropModeCalls;
        m_cropMode = m_cropModeSet;
        return m_setCropModeResult;
    }

    int setShutter( int ss )
    {
        ++m_setShutterCalls;
        m_lastShutterSS = ss;
        m_shutterState  = ( ss == 0 ) ? 0 : 1;
        return m_setShutterResult;
    }

    bool checkFocus()
    {
        // Mirror the pattern documented for checkFocusSwitchState(): when the focus-state helper is
        // configured, defer to it; otherwise fall back to the directly-injected test result.
        if( m_focusStateHelperConfigured )
        {
            return checkFocusSwitchState();
        }
        return m_checkFocusResult;
    }

    int gotoFocus()
    {
        ++m_gotoFocusCalls;
        return m_gotoFocusResult;
    }

    std::string stateString()
    {
        return m_stateStringResult;
    }

    bool stateStringValid()
    {
        return m_stateStringValidResult;
    }

    int recordTelem( const MagAOX::logger::telem_stdcam * )
    {
        return recordCamera( true );
    }

    // -- exposers for protected members, used to directly touch tag-dispatch overloads and helpers
    //    that are otherwise unreachable through the normal call paths (see report for details). --
    int exposeCreateReadoutSpeedTrue()
    {
        mx::meta::trueFalseT<true> t;
        return createReadoutSpeed( t );
    }

    int exposeCreateReadoutSpeedFalse()
    {
        mx::meta::trueFalseT<false> f;
        return createReadoutSpeed( f );
    }

    int exposeCreateVShiftSpeedTrue()
    {
        mx::meta::trueFalseT<true> t;
        return createVShiftSpeed( t );
    }

    int exposeCreateVShiftSpeedFalse()
    {
        mx::meta::trueFalseT<false> f;
        return createVShiftSpeed( f );
    }

    int exposeCreateFanSpeedTrue()
    {
        mx::meta::trueFalseT<true> t;
        return createFanSpeed( t );
    }

    int exposeCreateFanSpeedFalse()
    {
        mx::meta::trueFalseT<false> f;
        return createFanSpeed( f );
    }

    void exposeUpdateFocusStateProperty()
    {
        updateFocusStateProperty();
    }
};

/// Test harness with only "report only" temperature/FPS status (no control), and every other capability off.
/** This harness exercises the `else if (derivedT::c_stdCamera_temp)` and `else if (derivedT::c_stdCamera_fps)`
 * branches in stdCamera, as well as the "disabled" skip branches for every other feature.
 *
 * \ingroup stdCamera_tests
 */
struct stdCameraReportOnlyHarness : public MagAOX::app::MagAOXApp<false>,
                                    public MagAOX::app::dev::stdCamera<stdCameraReportOnlyHarness>
{
    friend class MagAOX::app::dev::stdCamera<stdCameraReportOnlyHarness>;

    typedef MagAOX::app::dev::stdCamera<stdCameraReportOnlyHarness> stdCameraT;

    // Re-publish stdCamera's protected state for direct test inspection/mutation.
    using stdCameraT::m_adcSpeed;
    using stdCameraT::m_analogGainName;
    using stdCameraT::m_analogGainNameLabels;
    using stdCameraT::m_analogGainNameSet;
    using stdCameraT::m_analogGainNames;
    using stdCameraT::m_analogGainValid;
    using stdCameraT::m_cameraModes;
    using stdCameraT::m_ccdTemp;
    using stdCameraT::m_ccdTempSetpt;
    using stdCameraT::m_cropMode;
    using stdCameraT::m_cropModeSet;
    using stdCameraT::m_currentROI;
    using stdCameraT::m_defaultFanSpeed;
    using stdCameraT::m_defaultLEDState;
    using stdCameraT::m_defaultReadoutSpeed;
    using stdCameraT::m_defaultVShiftSpeed;
    using stdCameraT::m_default_bin_x;
    using stdCameraT::m_default_bin_y;
    using stdCameraT::m_default_h;
    using stdCameraT::m_default_w;
    using stdCameraT::m_default_x;
    using stdCameraT::m_default_y;
    using stdCameraT::m_emGain;
    using stdCameraT::m_emGainSet;
    using stdCameraT::m_expTime;
    using stdCameraT::m_expTimeSet;
    using stdCameraT::m_fanSpeedControlEnabled;
    using stdCameraT::m_fanSpeedName;
    using stdCameraT::m_fanSpeedNameLabels;
    using stdCameraT::m_fanSpeedNameSet;
    using stdCameraT::m_fanSpeedNames;
    using stdCameraT::m_fanSpeedValid;
    using stdCameraT::m_focusGotoFormat;
    using stdCameraT::m_focusGotoHelperConfigured;
    using stdCameraT::m_focusGotoSourceIndices;
    using stdCameraT::m_focusGotoSourceProperties;
    using stdCameraT::m_focusGotoTargetDevice;
    using stdCameraT::m_focusGotoTargetName;
    using stdCameraT::m_focusGotoTargetProperty;
    using stdCameraT::m_focusMonitoredPropertyKeys;
    using stdCameraT::m_focusStateElement;
    using stdCameraT::m_focusStateHelperConfigured;
    using stdCameraT::m_focusStateOnMeansInFocus;
    using stdCameraT::m_focusStateSource;
    using stdCameraT::m_focusStateSourceIndex;
    using stdCameraT::m_fps;
    using stdCameraT::m_fpsSet;
    using stdCameraT::m_full_bin_x;
    using stdCameraT::m_full_bin_y;
    using stdCameraT::m_full_currbin_h;
    using stdCameraT::m_full_currbin_w;
    using stdCameraT::m_full_currbin_x;
    using stdCameraT::m_full_currbin_y;
    using stdCameraT::m_full_h;
    using stdCameraT::m_full_w;
    using stdCameraT::m_full_x;
    using stdCameraT::m_full_y;
    using stdCameraT::m_hasFocus;
    using stdCameraT::m_indiP_analogGain;
    using stdCameraT::m_indiP_cropMode;
    using stdCameraT::m_indiP_emGain;
    using stdCameraT::m_indiP_exptime;
    using stdCameraT::m_indiP_fanSpeed;
    using stdCameraT::m_indiP_focus;
    using stdCameraT::m_indiP_focusMonitoredProperties;
    using stdCameraT::m_indiP_fps;
    using stdCameraT::m_indiP_fullROI;
    using stdCameraT::m_indiP_gotoFocus;
    using stdCameraT::m_indiP_led;
    using stdCameraT::m_indiP_mode;
    using stdCameraT::m_indiP_readoutSpeed;
    using stdCameraT::m_indiP_reconfig;
    using stdCameraT::m_indiP_roi_bin_x;
    using stdCameraT::m_indiP_roi_bin_y;
    using stdCameraT::m_indiP_roi_check;
    using stdCameraT::m_indiP_roi_default;
    using stdCameraT::m_indiP_roi_full;
    using stdCameraT::m_indiP_roi_fullbin;
    using stdCameraT::m_indiP_roi_h;
    using stdCameraT::m_indiP_roi_last;
    using stdCameraT::m_indiP_roi_loadlast;
    using stdCameraT::m_indiP_roi_set;
    using stdCameraT::m_indiP_roi_w;
    using stdCameraT::m_indiP_roi_x;
    using stdCameraT::m_indiP_roi_y;
    using stdCameraT::m_indiP_shutter;
    using stdCameraT::m_indiP_shutterStatus;
    using stdCameraT::m_indiP_stateString;
    using stdCameraT::m_indiP_synchro;
    using stdCameraT::m_indiP_temp;
    using stdCameraT::m_indiP_tempcont;
    using stdCameraT::m_indiP_tempstat;
    using stdCameraT::m_indiP_vShiftSpeed;
    using stdCameraT::m_lastROI;
    using stdCameraT::m_ledState;
    using stdCameraT::m_ledStateSet;
    using stdCameraT::m_ledStateValid;
    using stdCameraT::m_maxEMGain;
    using stdCameraT::m_maxExpTime;
    using stdCameraT::m_maxFPS;
    using stdCameraT::m_maxROIBinning_x;
    using stdCameraT::m_maxROIBinning_y;
    using stdCameraT::m_maxROIHeight;
    using stdCameraT::m_maxROIWidth;
    using stdCameraT::m_maxROIx;
    using stdCameraT::m_maxROIy;
    using stdCameraT::m_maxTemp;
    using stdCameraT::m_minExpTime;
    using stdCameraT::m_minFPS;
    using stdCameraT::m_minROIBinning_x;
    using stdCameraT::m_minROIBinning_y;
    using stdCameraT::m_minROIHeight;
    using stdCameraT::m_minROIWidth;
    using stdCameraT::m_minROIx;
    using stdCameraT::m_minROIy;
    using stdCameraT::m_minTemp;
    using stdCameraT::m_modeName;
    using stdCameraT::m_nextMode;
    using stdCameraT::m_nextROI;
    using stdCameraT::m_readoutSpeedName;
    using stdCameraT::m_readoutSpeedNameLabels;
    using stdCameraT::m_readoutSpeedNameSet;
    using stdCameraT::m_readoutSpeedNames;
    using stdCameraT::m_shutterState;
    using stdCameraT::m_shutterStatus;
    using stdCameraT::m_startupMode;
    using stdCameraT::m_startupTemp;
    using stdCameraT::m_stepExpTime;
    using stdCameraT::m_stepFPS;
    using stdCameraT::m_stepROIBinning_x;
    using stdCameraT::m_stepROIBinning_y;
    using stdCameraT::m_stepROIHeight;
    using stdCameraT::m_stepROIWidth;
    using stdCameraT::m_stepROIx;
    using stdCameraT::m_stepROIy;
    using stdCameraT::m_stepTemp;
    using stdCameraT::m_synchro;
    using stdCameraT::m_synchroSet;
    using stdCameraT::m_tempControlOnTarget;
    using stdCameraT::m_tempControlStatus;
    using stdCameraT::m_tempControlStatusSet;
    using stdCameraT::m_tempControlStatusStr;
    using stdCameraT::m_vShiftSpeedName;
    using stdCameraT::m_vShiftSpeedNameLabels;
    using stdCameraT::m_vShiftSpeedNameSet;
    using stdCameraT::m_vShiftSpeedNames;
    using stdCameraT::m_vshiftSpeed;



    static constexpr bool c_stdCamera_tempControl      = false;
    static constexpr bool c_stdCamera_temp             = true;
    static constexpr bool c_stdCamera_readoutSpeed      = false;
    static constexpr bool c_stdCamera_vShiftSpeed        = false;
    static constexpr bool c_stdCamera_emGain             = false;
    static constexpr bool c_stdCamera_exptimeCtrl        = false;
    static constexpr bool c_stdCamera_fpsCtrl            = false;
    static constexpr bool c_stdCamera_fps                = true;
    static constexpr bool c_stdCamera_synchro            = false;
    static constexpr bool c_stdCamera_usesModes          = false;
    static constexpr bool c_stdCamera_usesROI            = false;
    static constexpr bool c_stdCamera_cropMode           = false;
    static constexpr bool c_stdCamera_hasShutter         = false;
    static constexpr bool c_stdCamera_usesStateString    = false;

    bool m_reconfig{ false };

    // -- register-call fault injection, mirroring stdCameraFullHarness (see there for rationale) --
    int m_regCallCount{ 0 };
    int m_regFailAt{ -1 };

    stdCameraReportOnlyHarness() : MagAOX::app::MagAOXApp<false>( "", false )
    {
        m_configName = "stdcamreportonlytest";
    }

    ~stdCameraReportOnlyHarness() noexcept override
    {
    }

    int setupConfig( mx::app::appConfigurator &config )
    {
        return stdCameraT::setupConfig( config );
    }

    int loadConfig( mx::app::appConfigurator &config )
    {
        return stdCameraT::loadConfig( config );
    }

    int registerIndiPropertyReadOnly( pcf::IndiProperty &prop )
    {
        ++m_regCallCount;
        if( m_regCallCount == m_regFailAt )
        {
            return -1;
        }
        return MagAOX::app::MagAOXApp<false>::registerIndiPropertyReadOnly( prop );
    }

    int appStartup() override
    {
        return stdCameraT::appStartup();
    }

    int appLogic() override
    {
        return stdCameraT::appLogic();
    }

    int appShutdown() override
    {
        return stdCameraT::appShutdown();
    }

    int onPowerOff() override
    {
        return stdCameraT::onPowerOff();
    }

    int whilePowerOff() override
    {
        return stdCameraT::whilePowerOff();
    }

    int powerOnDefaults()
    {
        return 0;
    }
};

} // namespace stdCamera_tests

// LCOV_EXCL_STOP
