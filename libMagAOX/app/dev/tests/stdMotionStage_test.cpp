/** \file stdMotionStage_test.cpp
 * \brief Catch2 tests for the stdMotionStage helper.
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 * \ingroup testing
 */

#include <cmath>

#include "../../../../tests/testXWC.hpp"

#include "../../MagAOXApp.hpp"
#include "../stdMotionStage.hpp"

using namespace MagAOX::app;

namespace libXWCTest
{
namespace appTest
{
namespace devTest
{

/** \defgroup stdMotionStage_tests libXWC::app::dev::stdMotionStage Unit Tests
 * \ingroup app_dev_unit_tests
 */

/// Test harness for exercising stdMotionStage without needing a real INDI/hardware stack.
/** \ingroup stdMotionStage_tests
 */
class stdMotionStageHarness : public MagAOXApp<false>, public dev::stdMotionStage<stdMotionStageHarness>
{
    friend class dev::stdMotionStage<stdMotionStageHarness>;

  protected:
    float m_lastMoveTarget{ -1.0f }; ///< Last target passed to moveTo by the helper.

    int m_moveCalls{ 0 }; ///< Number of motion requests issued by the helper.

    int m_stopCalls{ 0 }; ///< Number of times stop() was invoked.

    int m_homingCalls{ 0 }; ///< Number of times startHoming() was invoked.

    float m_presetNumberValue{ 0.0f }; ///< Value returned by presetNumber().

    std::string m_failRegisterName; ///< If non-empty, registerIndiPropertyNew fails for the property with this name.

    static std::string s_lastLogMessage; ///< Most recent text log message captured from stdMotionStage.

    static logPrioT s_lastLogLevel; ///< Most recent log priority captured from stdMotionStage.

    static int s_logCount; ///< Number of captured stdMotionStage log messages.

    static int s_telemCount; ///< Number of captured stdMotionStage telemetry records.

  public:
    /// Construct a stdMotionStage test harness with a presetName callback property.
    stdMotionStageHarness();

    /// Destroy the stdMotionStage test harness.
    ~stdMotionStageHarness() noexcept override;

    /// Reset the captured stdMotionStage logging state shared across harness instances.
    static void resetLogState();

    /// Reset the captured stdMotionStage telemetry count shared across harness instances.
    static void resetTelemCount();

    /// Configure the preset-name list and notation used by stdMotionStage.
    void
    configurePresets( const std::vector<std::string> &presetNames /**< [in] configured preset names */,
                      const std::string &presetNotation /**< [in] singular preset notation such as preset or filter */
    );

    /// Directly set the preset positions used to resolve preset-name aliases.
    void setPresetPositions( const std::vector<float> &positions /**< [in] the preset positions to use */ );

    /// Override the configured device name (used to probe INDI unique-key edge cases).
    void setConfigName( const std::string &name /**< [in] the new device/config name */ );

    /// Control the m_defaultPositions flag exercised by loadConfig().
    void setDefaultPositions( bool defaultPositions /**< [in] the new flag value */ );

    /// Control the m_fractionalPresets flag exercised by appStartup().
    void setFractionalPresets( bool fractionalPresets /**< [in] the new flag value */ );

    /// Directly set m_moving, simulating the derived class's motion-state responsibility.
    void setMoving( int8_t moving /**< [in] the new m_moving value */ );

    /// Directly set the value returned by presetNumber().
    void setPresetNumberValue( float presetNumber /**< [in] the new preset number to report */ );

    /// Force registerIndiPropertyNew to fail for the property with this name, simulating a registration failure.
    void setFailRegisterName( const std::string &name /**< [in] the property name that should fail to register */ );

    /// Construct (or tear down) a non-activated real indiDriver, to exercise the m_indiDriver-present code paths.
    void setFakeIndiDriver( bool present /**< [in] whether a fake driver should be installed */ );

    /// Apply a presetName request property to the stdMotionStage callback under test.
    int applyPresetNameRequest(
        const std::vector<std::pair<std::string, pcf::IndiElement::SwitchStateType>> &elements /**< [in] requested
                                                                                                        switch
                                                                                                        elements and
                                                                                                        states */
    );

    /// Apply a numeric preset-position request property to the stdMotionStage callback under test.
    int applyPresetRequest( float target /**< [in] the requested target position */,
                            bool includeTargetElement = true /**< [in] whether to include a target element */,
                            bool matchProperty = true /**< [in] whether the request should match the real property */
    );

    /// Apply a home request property to the stdMotionStage callback under test.
    int applyHomeRequest(
        pcf::IndiElement::SwitchStateType state /**< [in] the requested switch state */,
        bool includeRequestElement = true /**< [in] whether to include the request element */,
        bool matchProperty = true /**< [in] whether the request should match the real property */
    );

    /// Apply a stop request property to the stdMotionStage callback under test, with optional key overrides.
    int applyStopRequest(
        pcf::IndiElement::SwitchStateType state /**< [in] the requested switch state */,
        bool includeRequestElement = true /**< [in] whether to include the request element */,
        const std::string &deviceOverride = "" /**< [in] if non-empty, overrides the device name used */,
        const std::string &nameOverride = "" /**< [in] if non-empty, overrides the property name used */
    );

    /// Get the number of move requests accepted by stdMotionStage.
    int moveCalls() const;

    /// Get the last move target accepted by stdMotionStage.
    float lastMoveTarget() const;

    /// Get the number of times stop() was invoked.
    int stopCalls() const;

    /// Get the number of times startHoming() was invoked.
    int homingCalls() const;

    /// Get the currently tracked movingState value.
    int8_t movingStateValue() const;

    /// Get the currently tracked m_moving value.
    int8_t movingValue() const;

    /// Get the currently tracked preset-name alias index.
    int presetNameIndexValue() const;

    /// Get the currently tracked numerical preset target.
    float presetTargetValue() const;

    /// Get the currently tracked numerical preset current value.
    float presetCurrentValue() const;

    /// Get the configured preset names.
    const std::vector<std::string> &presetNamesValue() const;

    /// Get the configured/resolved preset positions.
    const std::vector<float> &presetPositionsValue() const;

    /// Get the configured powerOnHome flag.
    bool powerOnHomeValue() const;

    /// Get the configured homePreset value.
    int homePresetValue() const;

    /// Get the name of the built preset (position) INDI property.
    std::string presetPropertyName() const;

    /// Get the name of the built presetName INDI property.
    std::string presetNamePropertyName() const;

    /// Get the name of the built home INDI property.
    std::string homePropertyName() const;

    /// Get the name of the built stop INDI property.
    std::string stopPropertyName() const;

    /// Get the most recent stdMotionStage text log message captured by the harness.
    static const std::string &lastLogMessage();

    /// Get the most recent stdMotionStage log priority captured by the harness.
    static logPrioT lastLogLevel();

    /// Get the number of stdMotionStage log messages captured by the harness.
    static int logCount();

    /// Get the number of stdMotionStage telemetry records captured by the harness.
    static int telemCount();

    /// Capture stdMotionStage log messages instead of sending them to the normal logger.
    template <typename logT, int retval = 0>
    static int log( const typename logT::messageT &msg /**< [in] the logged message */,
                    logPrioT                       level = logPrio::LOG_DEFAULT /**< [in] the logged priority */
    );

    /// Capture stdMotionStage telemetry records instead of sending them to a real telemetry logger.
    template <typename telT, int retval = 0>
    int telem( const typename telT::messageT &msg /**< [in] the telemetry message */ );

    /// Delegate to the stdMotionStage base setupConfig, resolving the multiple-inheritance ambiguity.
    int setupConfig( mx::app::appConfigurator &config /**< [out] the configurator to use */ );

    /// Delegate to the stdMotionStage base loadConfig, resolving the multiple-inheritance ambiguity.
    int loadConfig( mx::app::appConfigurator &config /**< [in] the configurator to use */ );

    /// No-op startup implementation required by MagAOXApp for testing.
    int appStartup() override;

    /// No-op logic implementation required by MagAOXApp for testing.
    int appLogic() override;

    /// No-op shutdown implementation required by MagAOXApp for testing.
    int appShutdown() override;

    /// Simulate stop, tracking how many times it was called.
    int stop();

    /// Simulate homing, tracking how many times it was called.
    int startHoming();

    /// Return the currently configured preset number for testing paths that query the current preset.
    float presetNumber();

    /// Record a requested move target when stdMotionStage accepts a motion request.
    int moveTo( float target /**< [in] the accepted move target */ );

    /// Test-only seam allowing appStartup() registration failures to be simulated one property at a time.
    int registerIndiPropertyNew( pcf::IndiProperty &prop /**< [in,out] the property to register */,
                                 int ( *callBack )( void *, const pcf::IndiProperty & ) /**< [in] the callback */
    );

    /// Test-only wrapper exposing the protected setPresetNameTracking, to probe its defensive out-of-range guard.
    int callSetPresetNameTracking( int presetNameIndex /**< [in] the (possibly invalid) index to record */ );
};

std::string stdMotionStageHarness::s_lastLogMessage;

logPrioT stdMotionStageHarness::s_lastLogLevel = logPrio::LOG_DEFAULT;

int stdMotionStageHarness::s_logCount = 0;

int stdMotionStageHarness::s_telemCount = 0;

stdMotionStageHarness::stdMotionStageHarness() : MagAOXApp<false>( "", false )
{
    m_configName = "stest";

    m_indiP_presetName = pcf::IndiProperty( pcf::IndiProperty::Switch );
    m_indiP_presetName.setDevice( m_configName );
    m_indiP_presetName.setName( "presetName" );

    resetLogState();
}

stdMotionStageHarness::~stdMotionStageHarness() noexcept
{
    setFakeIndiDriver( false );
}

void stdMotionStageHarness::resetLogState()
{
    s_lastLogMessage.clear();
    s_lastLogLevel = logPrio::LOG_DEFAULT;
    s_logCount     = 0;
}

void stdMotionStageHarness::resetTelemCount()
{
    s_telemCount = 0;
}

void stdMotionStageHarness::configurePresets( const std::vector<std::string> &presetNames,
                                              const std::string              &presetNotation )
{
    m_presetNames    = presetNames;
    m_presetNotation = presetNotation;
}

void stdMotionStageHarness::setPresetPositions( const std::vector<float> &positions )
{
    m_presetPositions = positions;
}

void stdMotionStageHarness::setConfigName( const std::string &name )
{
    m_configName = name;
}

void stdMotionStageHarness::setDefaultPositions( bool defaultPositions )
{
    m_defaultPositions = defaultPositions;
}

void stdMotionStageHarness::setFractionalPresets( bool fractionalPresets )
{
    m_fractionalPresets = fractionalPresets;
}

void stdMotionStageHarness::setMoving( int8_t moving )
{
    m_moving = moving;
}

void stdMotionStageHarness::setPresetNumberValue( float presetNumber )
{
    m_presetNumberValue = presetNumber;
}

void stdMotionStageHarness::setFailRegisterName( const std::string &name )
{
    m_failRegisterName = name;
}

void stdMotionStageHarness::setFakeIndiDriver( bool present )
{
    if( present )
    {
        if( !m_indiDriver )
        {
            // This never activates (no execute() thread is started) so sendSetProperty/sendDefProperty
            // remain harmless no-ops (INDI response mode is never enabled), matching the pattern used by
            // libMagAOX/app/tests/MagAOXApp_test.hpp and apps/zaberLowLevel/tests/zaberLowLevel_test.cpp.
            m_indiDriver = new indiDriver<MagAOXApp<false>>( this, m_configName, "0", "0" );
        }
    }
    else if( m_indiDriver )
    {
        delete m_indiDriver;
        m_indiDriver = nullptr;
    }
}

int stdMotionStageHarness::applyPresetNameRequest(
    const std::vector<std::pair<std::string, pcf::IndiElement::SwitchStateType>> &elements )
{
    pcf::IndiProperty ip( pcf::IndiProperty::Switch );
    ip.setDevice( m_configName );
    ip.setName( "presetName" );

    for( const auto &element : elements )
    {
        ip.add( pcf::IndiElement( element.first ) );
        ip[element.first].setSwitchState( element.second );
    }

    return newCallBack_m_indiP_presetName( ip );
}

int stdMotionStageHarness::applyPresetRequest( float target, bool includeTargetElement, bool matchProperty )
{
    pcf::IndiProperty ip( pcf::IndiProperty::Number );
    ip.setDevice( matchProperty ? m_configName : m_configName + "_other" );
    ip.setName( matchProperty ? m_presetNotation : m_presetNotation + "_other" );

    if( includeTargetElement )
    {
        ip.add( pcf::IndiElement( "target" ) );
        ip["target"].set( target );
    }

    return newCallBack_m_indiP_preset( ip );
}

int stdMotionStageHarness::applyHomeRequest( pcf::IndiElement::SwitchStateType state,
                                             bool                              includeRequestElement,
                                             bool                              matchProperty )
{
    pcf::IndiProperty ip( pcf::IndiProperty::Switch );
    ip.setDevice( matchProperty ? m_configName : m_configName + "_other" );
    ip.setName( matchProperty ? "home" : "home_other" );

    if( includeRequestElement )
    {
        ip.add( pcf::IndiElement( "request" ) );
        ip["request"].setSwitchState( state );
    }

    return newCallBack_m_indiP_home( ip );
}

int stdMotionStageHarness::applyStopRequest( pcf::IndiElement::SwitchStateType state,
                                             bool                              includeRequestElement,
                                             const std::string                &deviceOverride,
                                             const std::string                &nameOverride )
{
    pcf::IndiProperty ip( pcf::IndiProperty::Switch );
    ip.setDevice( deviceOverride.empty() ? m_configName : deviceOverride );
    ip.setName( nameOverride.empty() ? "stop" : nameOverride );

    if( includeRequestElement )
    {
        ip.add( pcf::IndiElement( "request" ) );
        ip["request"].setSwitchState( state );
    }

    return newCallBack_m_indiP_stop( ip );
}

int stdMotionStageHarness::moveCalls() const
{
    return m_moveCalls;
}

float stdMotionStageHarness::lastMoveTarget() const
{
    return m_lastMoveTarget;
}

int stdMotionStageHarness::stopCalls() const
{
    return m_stopCalls;
}

int stdMotionStageHarness::homingCalls() const
{
    return m_homingCalls;
}

int8_t stdMotionStageHarness::movingStateValue() const
{
    return m_movingState;
}

int8_t stdMotionStageHarness::movingValue() const
{
    return m_moving;
}

int stdMotionStageHarness::presetNameIndexValue() const
{
    return m_presetNameIndex;
}

float stdMotionStageHarness::presetTargetValue() const
{
    return m_preset_target;
}

float stdMotionStageHarness::presetCurrentValue() const
{
    return m_preset;
}

const std::vector<std::string> &stdMotionStageHarness::presetNamesValue() const
{
    return m_presetNames;
}

const std::vector<float> &stdMotionStageHarness::presetPositionsValue() const
{
    return m_presetPositions;
}

bool stdMotionStageHarness::powerOnHomeValue() const
{
    return m_powerOnHome;
}

int stdMotionStageHarness::homePresetValue() const
{
    return m_homePreset;
}

std::string stdMotionStageHarness::presetPropertyName() const
{
    return m_indiP_preset.getName();
}

std::string stdMotionStageHarness::presetNamePropertyName() const
{
    return m_indiP_presetName.getName();
}

std::string stdMotionStageHarness::homePropertyName() const
{
    return m_indiP_home.getName();
}

std::string stdMotionStageHarness::stopPropertyName() const
{
    return m_indiP_stop.getName();
}

const std::string &stdMotionStageHarness::lastLogMessage()
{
    return s_lastLogMessage;
}

logPrioT stdMotionStageHarness::lastLogLevel()
{
    return s_lastLogLevel;
}

int stdMotionStageHarness::logCount()
{
    return s_logCount;
}

int stdMotionStageHarness::telemCount()
{
    return s_telemCount;
}

template <typename logT, int retval>
int stdMotionStageHarness::log( const typename logT::messageT &msg, logPrioT level )
{
    s_lastLogMessage =
        logT::msgString( const_cast<uint8_t *>( msg.builder.GetBufferPointer() ), msg.builder.GetSize() );
    s_lastLogLevel = level;
    ++s_logCount;

    return retval;
}

template <typename telT, int retval>
int stdMotionStageHarness::telem( const typename telT::messageT &msg )
{
    static_cast<void>( msg );
    ++s_telemCount;

    return retval;
}

int stdMotionStageHarness::setupConfig( mx::app::appConfigurator &config )
{
    return dev::stdMotionStage<stdMotionStageHarness>::setupConfig( config );
}

int stdMotionStageHarness::loadConfig( mx::app::appConfigurator &config )
{
    return dev::stdMotionStage<stdMotionStageHarness>::loadConfig( config );
}

int stdMotionStageHarness::appStartup()
{
    return 0;
}

int stdMotionStageHarness::appLogic()
{
    return 0;
}

int stdMotionStageHarness::appShutdown()
{
    return 0;
}

int stdMotionStageHarness::stop()
{
    ++m_stopCalls;
    return 0;
}

int stdMotionStageHarness::startHoming()
{
    ++m_homingCalls;
    return 0;
}

float stdMotionStageHarness::presetNumber()
{
    return m_presetNumberValue;
}

int stdMotionStageHarness::moveTo( float target )
{
    m_lastMoveTarget = target;
    ++m_moveCalls;
    m_preset = target;

    // Simulate a derived class that immediately reports its new preset number, by locating a matching
    // configured preset position (if any).
    m_presetNumberValue = -1.0f;
    for( size_t n = 0; n < m_presetPositions.size(); ++n )
    {
        if( std::fabs( m_presetPositions[n] - target ) < 1e-4f )
        {
            m_presetNumberValue = static_cast<float>( n );
            break;
        }
    }

    return 0;
}

int stdMotionStageHarness::registerIndiPropertyNew( pcf::IndiProperty &prop,
                                                    int ( *callBack )( void *, const pcf::IndiProperty & ) )
{
    if( !m_failRegisterName.empty() && prop.getName() == m_failRegisterName )
    {
        return -1;
    }

    return MagAOXApp<false>::registerIndiPropertyNew( prop, callBack );
}

int stdMotionStageHarness::callSetPresetNameTracking( int presetNameIndex )
{
    return setPresetNameTracking( presetNameIndex );
}

/// Verify stdMotionStage logs and rejects invalid preset-name selections before issuing motion requests.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage rejects invalid preset-name selections", "[dev::stdMotionStage]" )
{
    // clang-format off
    #ifdef STDMOTIONSTAGE_TEST_DOXYGEN_REF
    MagAOX::app::dev::stdMotionStage<libXWCTest::appTest::devTest::stdMotionStageHarness>::newCallBack_m_indiP_presetName( pcf::IndiProperty() );
    #endif
    // clang-format on

    SECTION( "quoted preset names are logged and rejected" )
    {
        stdMotionStageHarness app;

        app.configurePresets( { "open", "focus" }, "preset" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.applyPresetNameRequest( { { "\"focus\"", pcf::IndiElement::On } } ) == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        REQUIRE( stdMotionStageHarness::lastLogLevel() == logPrio::LOG_ERROR );
        REQUIRE( stdMotionStageHarness::lastLogMessage() == "Unknown presetName selected: \"focus\"" );
        REQUIRE( app.moveCalls() == 0 );
        REQUIRE( app.lastMoveTarget() == -1.0f );
    }

    SECTION( "invalid names are rejected even when a valid preset is also selected" )
    {
        stdMotionStageHarness app;

        app.configurePresets( { "open", "focus" }, "filter" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.applyPresetNameRequest(
                     { { "open", pcf::IndiElement::On }, { "bogus", pcf::IndiElement::On } } ) == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        REQUIRE( stdMotionStageHarness::lastLogLevel() == logPrio::LOG_ERROR );
        REQUIRE( stdMotionStageHarness::lastLogMessage() == "Unknown filterName selected: bogus" );
        REQUIRE( app.moveCalls() == 0 );
        REQUIRE( app.lastMoveTarget() == -1.0f );
    }

    SECTION( "multiple invalid names are joined in the log message" )
    {
        stdMotionStageHarness app;

        app.configurePresets( { "open", "focus" }, "preset" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.applyPresetNameRequest(
                     { { "bogus1", pcf::IndiElement::On }, { "bogus2", pcf::IndiElement::On } } ) == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        REQUIRE( stdMotionStageHarness::lastLogLevel() == logPrio::LOG_ERROR );
        REQUIRE( stdMotionStageHarness::lastLogMessage() == "Unknown presetName selected: bogus1, bogus2" );
        REQUIRE( app.moveCalls() == 0 );
        REQUIRE( app.lastMoveTarget() == -1.0f );
    }
}

/// Verify stdMotionStage's setupConfig/loadConfig handle defaults, overrides, and the defaultPositions flag.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage setupConfig and loadConfig", "[dev::stdMotionStage]" )
{
    SECTION( "setupConfig registers the expected options" )
    {
        stdMotionStageHarness app;
        mx::app::appConfigurator config;

        REQUIRE( app.setupConfig( config ) == 0 );
    }

    SECTION( "loadConfig fills default positions from preset order when positions are omitted" )
    {
        mx::app::writeConfigFile( "/tmp/stdMotionStage_test.conf",
                                  { "presets" },
                                  { "names" },
                                  { "open,focus,block" } );

        mx::app::appConfigurator config;

        stdMotionStageHarness app;
        REQUIRE( app.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/stdMotionStage_test.conf" );
        REQUIRE( app.loadConfig( config ) == 0 );

        REQUIRE( app.presetNamesValue().size() == 3 );
        REQUIRE( app.presetPositionsValue().size() == 3 );
        REQUIRE( app.presetPositionsValue()[0] == 1 );
        REQUIRE( app.presetPositionsValue()[1] == 2 );
        REQUIRE( app.presetPositionsValue()[2] == 3 );
        REQUIRE( app.powerOnHomeValue() == false );
        REQUIRE( app.homePresetValue() == -1 );
    }

    SECTION( "loadConfig repairs zero-valued positions and keeps configured non-zero positions" )
    {
        mx::app::writeConfigFile( "/tmp/stdMotionStage_test.conf",
                                  { "stage", "stage", "presets", "presets" },
                                  { "powerOnHome", "homePreset", "names", "positions" },
                                  { "true", "2", "open,focus,block", "0,5,0" } );

        mx::app::appConfigurator config;

        stdMotionStageHarness app;
        REQUIRE( app.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/stdMotionStage_test.conf" );
        REQUIRE( app.loadConfig( config ) == 0 );

        REQUIRE( app.powerOnHomeValue() == true );
        REQUIRE( app.homePresetValue() == 2 );
        REQUIRE( app.presetPositionsValue().size() == 3 );
        REQUIRE( app.presetPositionsValue()[0] == 1 );
        REQUIRE( app.presetPositionsValue()[1] == 5 );
        REQUIRE( app.presetPositionsValue()[2] == 3 );
    }

    SECTION( "loadConfig leaves positions alone when defaultPositions is disabled" )
    {
        mx::app::writeConfigFile( "/tmp/stdMotionStage_test.conf",
                                  { "presets", "presets" },
                                  { "names", "positions" },
                                  { "open,focus,block", "0,5,0" } );

        mx::app::appConfigurator config;

        stdMotionStageHarness app;
        app.setDefaultPositions( false );

        REQUIRE( app.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/stdMotionStage_test.conf" );
        REQUIRE( app.loadConfig( config ) == 0 );

        // Without the default-fill/repair logic, the raw configured values (including the zeros) pass through.
        REQUIRE( app.presetPositionsValue().size() == 3 );
        REQUIRE( app.presetPositionsValue()[0] == 0 );
        REQUIRE( app.presetPositionsValue()[1] == 5 );
        REQUIRE( app.presetPositionsValue()[2] == 0 );
    }
}

/// Verify stdMotionStage::appStartup builds and registers INDI properties, and reports failures distinctly.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage appStartup", "[dev::stdMotionStage]" )
{
    SECTION( "appStartup succeeds with fractional presets and builds property elements" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus", "block" }, "preset" );

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );
        REQUIRE( app.presetPropertyName() == "preset" );
        REQUIRE( app.presetNamePropertyName() == "presetName" );
        REQUIRE( app.homePropertyName() == "home" );
        REQUIRE( app.stopPropertyName() == "stop" );
    }

    SECTION( "appStartup succeeds with non-fractional (integer step) presets" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus", "block" }, "preset" );
        app.setFractionalPresets( false );

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );
    }

    SECTION( "appStartup fails when there are no configured preset names" )
    {
        stdMotionStageHarness app;
        app.configurePresets( {}, "preset" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
    }

    SECTION( "appStartup fails when the preset property fails to register" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        app.setFailRegisterName( "preset" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
    }

    SECTION( "appStartup fails when the presetName property fails to register" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        app.setFailRegisterName( "presetName" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
    }

    SECTION( "appStartup fails when the home property fails to register" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        app.setFailRegisterName( "home" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
    }

    SECTION( "appStartup fails when the stop property fails to register" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        app.setFailRegisterName( "stop" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
    }
}

/// Verify stdMotionStage's trivial lifecycle passthroughs.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage appLogic, onPowerOff, whilePowerOff, and appShutdown", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;

    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appLogic() == 0 );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::whilePowerOff() == 0 );

    app.setMoving( 1 );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::onPowerOff() == 0 );
    REQUIRE( app.movingValue() == -2 );

    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appShutdown() == 0 );
}

/// Verify the static st_newCallBack_stdMotionStage dispatcher routes to the correct handler by property name.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage static callback dispatcher routes by property name", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;
    app.configurePresets( { "open", "focus" }, "preset" );
    app.setPresetPositions( { 1, 2 } );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

    SECTION( "stop requests are routed to newCallBack_m_indiP_stop" )
    {
        pcf::IndiProperty ip( pcf::IndiProperty::Switch );
        ip.setDevice( "stest" );
        ip.setName( "stop" );
        ip.add( pcf::IndiElement( "request" ) );
        ip["request"].setSwitchState( pcf::IndiElement::On );

        REQUIRE( dev::stdMotionStage<stdMotionStageHarness>::st_newCallBack_stdMotionStage( &app, ip ) == 0 );
        REQUIRE( app.stopCalls() == 1 );
    }

    SECTION( "home requests are routed to newCallBack_m_indiP_home" )
    {
        pcf::IndiProperty ip( pcf::IndiProperty::Switch );
        ip.setDevice( "stest" );
        ip.setName( "home" );
        ip.add( pcf::IndiElement( "request" ) );
        ip["request"].setSwitchState( pcf::IndiElement::On );

        REQUIRE( dev::stdMotionStage<stdMotionStageHarness>::st_newCallBack_stdMotionStage( &app, ip ) == 0 );
        REQUIRE( app.homingCalls() == 1 );
    }

    SECTION( "preset requests are routed to newCallBack_m_indiP_preset" )
    {
        pcf::IndiProperty ip( pcf::IndiProperty::Number );
        ip.setDevice( "stest" );
        ip.setName( "preset" );
        ip.add( pcf::IndiElement( "target" ) );
        ip["target"].set( 2.0f );

        REQUIRE( dev::stdMotionStage<stdMotionStageHarness>::st_newCallBack_stdMotionStage( &app, ip ) == 0 );
        REQUIRE( app.moveCalls() == 1 );
    }

    SECTION( "presetName requests are routed to newCallBack_m_indiP_presetName" )
    {
        pcf::IndiProperty ip( pcf::IndiProperty::Switch );
        ip.setDevice( "stest" );
        ip.setName( "presetName" );
        ip.add( pcf::IndiElement( "open" ) );
        ip["open"].setSwitchState( pcf::IndiElement::On );

        REQUIRE( dev::stdMotionStage<stdMotionStageHarness>::st_newCallBack_stdMotionStage( &app, ip ) == 0 );
        REQUIRE( app.moveCalls() == 1 );
    }

    SECTION( "unrecognized property names are rejected" )
    {
        pcf::IndiProperty ip( pcf::IndiProperty::Switch );
        ip.setDevice( "stest" );
        ip.setName( "somethingElse" );

        REQUIRE( dev::stdMotionStage<stdMotionStageHarness>::st_newCallBack_stdMotionStage( &app, ip ) == -1 );
    }
}

/// Verify stdMotionStage's numerical preset (position) callback.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage numerical preset callback", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;
    app.configurePresets( { "open", "focus", "block" }, "preset" );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

    SECTION( "mismatched property is rejected" )
    {
        REQUIRE( app.applyPresetRequest( 2.0f, true, false ) == -1 );
        REQUIRE( app.moveCalls() == 0 );
    }

    SECTION( "missing target/current element is rejected and logged" )
    {
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.applyPresetRequest( 2.0f, false, true ) == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        REQUIRE( app.moveCalls() == 0 );
    }

    SECTION( "a valid target position is accepted" )
    {
        app.setMoving( 1 );

        REQUIRE( app.applyPresetRequest( 2.0f ) == 0 );
        REQUIRE( app.moveCalls() == 1 );
        REQUIRE( app.lastMoveTarget() == 2.0f );
        REQUIRE( app.movingStateValue() == 0 );
        REQUIRE( app.presetNameIndexValue() == -1 );
        REQUIRE( app.presetTargetValue() == 2.0f );
    }
}

/// Verify stdMotionStage's named-preset callback across acceptance, rejection, and no-op paths.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage preset-name callback", "[dev::stdMotionStage]" )
{
    SECTION( "mismatched property is rejected" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );

        pcf::IndiProperty ip( pcf::IndiProperty::Switch );
        ip.setDevice( "wrongDevice" );
        ip.setName( "presetName" );

        REQUIRE( app.newCallBack_m_indiP_presetName( ip ) == -1 );
    }

    SECTION( "no selection on is a no-op" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );

        REQUIRE( app.applyPresetNameRequest( { { "open", pcf::IndiElement::Off } } ) == 0 );
        REQUIRE( app.moveCalls() == 0 );
    }

    SECTION( "more than one selection is rejected and logged" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.applyPresetNameRequest(
                     { { "open", pcf::IndiElement::On }, { "focus", pcf::IndiElement::On } } ) == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        REQUIRE( stdMotionStageHarness::lastLogLevel() == logPrio::LOG_ERROR );
        REQUIRE( stdMotionStageHarness::lastLogMessage() == "More than one preset selected" );
        REQUIRE( app.moveCalls() == 0 );
    }

    SECTION( "a valid single selection is accepted and logged" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus", "block" }, "preset" );
        app.setPresetPositions( { 1, 2, 3 } );
        stdMotionStageHarness::resetLogState();

        REQUIRE( app.applyPresetNameRequest( { { "focus", pcf::IndiElement::On } } ) == 0 );
        REQUIRE( app.moveCalls() == 1 );
        REQUIRE( app.lastMoveTarget() == 2.0f );
        REQUIRE( app.movingStateValue() == 1 );
        REQUIRE( app.presetNameIndexValue() == 1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        REQUIRE( stdMotionStageHarness::lastLogLevel() == logPrio::LOG_NOTICE );
    }
}

/// Verify stdMotionStage's home-request callback.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage home callback", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;
    app.configurePresets( { "open", "focus" }, "preset" );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

    SECTION( "mismatched property is rejected" )
    {
        REQUIRE( app.applyHomeRequest( pcf::IndiElement::On, true, false ) == -1 );
        REQUIRE( app.homingCalls() == 0 );
    }

    SECTION( "missing request element is a no-op" )
    {
        REQUIRE( app.applyHomeRequest( pcf::IndiElement::On, false ) == 0 );
        REQUIRE( app.homingCalls() == 0 );
    }

    SECTION( "request element off is a no-op" )
    {
        REQUIRE( app.applyHomeRequest( pcf::IndiElement::Off ) == 0 );
        REQUIRE( app.homingCalls() == 0 );
    }

    SECTION( "request element on triggers startHoming" )
    {
        REQUIRE( app.applyHomeRequest( pcf::IndiElement::On ) == 0 );
        REQUIRE( app.homingCalls() == 1 );
        REQUIRE( app.movingStateValue() == 0 );
    }
}

/// Verify stdMotionStage's stop-request callback, including the unique-key ambiguity guard.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage stop callback", "[dev::stdMotionStage]" )
{
    SECTION( "mismatched property is rejected" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

        REQUIRE( app.applyStopRequest( pcf::IndiElement::On, true, "wrongDevice" ) == -1 );
        REQUIRE( app.stopCalls() == 0 );
    }

    SECTION( "missing request element is a no-op" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

        REQUIRE( app.applyStopRequest( pcf::IndiElement::On, false ) == 0 );
        REQUIRE( app.stopCalls() == 0 );
    }

    SECTION( "request element off is a no-op" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

        REQUIRE( app.applyStopRequest( pcf::IndiElement::Off ) == 0 );
        REQUIRE( app.stopCalls() == 0 );
    }

    SECTION( "request element on triggers stop" )
    {
        stdMotionStageHarness app;
        app.configurePresets( { "open", "focus" }, "preset" );
        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

        REQUIRE( app.applyStopRequest( pcf::IndiElement::On ) == 0 );
        REQUIRE( app.stopCalls() == 1 );
        REQUIRE( app.movingStateValue() == 0 );
    }

    SECTION( "colliding unique keys with a mismatched name are rejected and logged" )
    {
        // pcf::IndiProperty::createUniqueKey() concatenates device + "." + name. If the device name
        // itself contains a '.', two different (device, name) pairs can produce the same unique key.
        // stdMotionStage's stop callback explicitly guards against this ambiguity by also comparing names.
        stdMotionStageHarness app;
        app.setConfigName( "stest.st" );
        app.configurePresets( { "open", "focus" }, "preset" );
        REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

        stdMotionStageHarness::resetLogState();

        // Real key: "stest.st" + "." + "stop" == "stest.st.stop"
        // Colliding key: "stest" + "." + "st.stop" == "stest.st.stop" (same string, different name)
        REQUIRE( app.applyStopRequest( pcf::IndiElement::On, true, "stest", "st.stop" ) == -1 );
        REQUIRE( stdMotionStageHarness::logCount() == 1 );
        // stdMotionStage logs this particular mismatch with the default priority (no level is passed).
        REQUIRE( stdMotionStageHarness::lastLogLevel() == logPrio::LOG_DEFAULT );
        REQUIRE( app.stopCalls() == 0 );
    }
}

/// Verify stdMotionStage's preset-name resolution and telemetry recording logic.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage recordStage and preset-name resolution", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;
    app.configurePresets( { "open", "focus", "block" }, "preset" );
    app.setPresetPositions( { 1, 2, 3 } );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

    // Before any motion, m_preset defaults to 0, so telemetryPresetName() should resolve to "".
    int before = stdMotionStageHarness::telemCount();
    REQUIRE( app.recordStage( true ) == 0 );
    REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

    // Calling again with no state change and force=false should not record again.
    before = stdMotionStageHarness::telemCount();
    REQUIRE( app.recordStage( false ) == 0 );
    REQUIRE( stdMotionStageHarness::telemCount() == before );

    // A raw numeric move to a known preset position resolves via the fallback presetIndex branch.
    REQUIRE( app.applyPresetRequest( 3.0f ) == 0 ); // -> index 2 ("block"), presetNameIndex cleared
    REQUIRE( app.presetNameIndexValue() == -1 );

    before = stdMotionStageHarness::telemCount();
    REQUIRE( app.recordStage( false ) == 0 );
    REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

    // A named-preset move while still "moving" resolves via the primary presetNameIndex branch.
    REQUIRE( app.applyPresetNameRequest( { { "focus", pcf::IndiElement::On } } ) == 0 ); // -> index 1
    app.setMoving( 1 );
    REQUIRE( app.presetNameIndexValue() == 1 );

    before = stdMotionStageHarness::telemCount();
    REQUIRE( app.recordStage( false ) == 0 );
    REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

    // Simulate arrival (moving stops) while movingState/presetNameIndex are unchanged: resolves via the
    // "same position" secondary presetNameIndex branch.
    app.setMoving( 0 );

    before = stdMotionStageHarness::telemCount();
    REQUIRE( app.recordStage( false ) == 0 );
    REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

    // Move to a position with no matching configured preset: presetNumber() reports no match (-1),
    // exercising the final "no match" fallback in activePresetNameIndex/activePresetName.
    REQUIRE( app.applyPresetRequest( 42.0f ) == 0 );
    REQUIRE( app.presetNameIndexValue() == -1 );

    before = stdMotionStageHarness::telemCount();
    REQUIRE( app.recordStage( false ) == 0 );
    REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );
}

/// Verify stdMotionStage::updateINDI, including the no-driver early return and the full INDI-update path.
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage updateINDI", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;
    app.configurePresets( { "open", "focus", "block" }, "preset" );
    app.setPresetPositions( { 1, 2, 3 } );
    REQUIRE( app.dev::stdMotionStage<stdMotionStageHarness>::appStartup() == 0 );

    SECTION( "updateINDI is a no-op without an INDI driver" )
    {
        int before = stdMotionStageHarness::telemCount();
        REQUIRE( app.updateINDI() == 0 );
        REQUIRE( stdMotionStageHarness::telemCount() == before );
    }

    SECTION( "updateINDI drives INDI property state once a driver is present" )
    {
        app.setFakeIndiDriver( true );

        // First call: selects preset index 0 by default (no match yet), busy state (moving).
        app.setPresetNumberValue( 0 );
        app.setMoving( 1 );
        int before = stdMotionStageHarness::telemCount();
        REQUIRE( app.updateINDI() == 0 );
        REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

        // Second call: switch the active preset and stop moving, exercising the idle-state branch and
        // the presetName element toggle-off/toggle-on branches.
        REQUIRE( app.applyPresetRequest( 2.0f ) == 0 ); // -> index 1
        app.setMoving( 0 );
        before = stdMotionStageHarness::telemCount();
        REQUIRE( app.updateINDI() == 0 );
        REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

        // Third call: a named-preset move while moving, exercising the "moving && movingState<1" busy branch.
        REQUIRE( app.applyPresetRequest( 3.0f ) == 0 );
        app.setMoving( 1 );
        before = stdMotionStageHarness::telemCount();
        REQUIRE( app.updateINDI() == 0 );
        REQUIRE( stdMotionStageHarness::telemCount() == before + 1 );

        app.setFakeIndiDriver( false );
    }
}

/// Verify setPresetNameTracking's defensive out-of-range guard, which is not reachable through the class's
/// only public call site (newCallBack_m_indiP_presetName always passes an index it has already validated).
/**
 * \ingroup stdMotionStage_tests
 */
TEST_CASE( "stdMotionStage setPresetNameTracking rejects out-of-range indices", "[dev::stdMotionStage]" )
{
    stdMotionStageHarness app;
    app.configurePresets( { "open", "focus" }, "preset" );

    REQUIRE( app.callSetPresetNameTracking( -1 ) == -1 );
    REQUIRE( app.presetNameIndexValue() == -1 );

    REQUIRE( app.callSetPresetNameTracking( 5 ) == -1 );
    REQUIRE( app.presetNameIndexValue() == -1 );

    REQUIRE( app.callSetPresetNameTracking( 1 ) == 0 );
    REQUIRE( app.presetNameIndexValue() == 1 );
}

} // namespace devTest
} // namespace appTest
} // namespace libXWCTest
