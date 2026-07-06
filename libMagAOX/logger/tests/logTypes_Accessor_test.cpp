/** \file logType_Accessor_test.hpp
 * \brief Tests for the log type accessors class
 * \ingroup logger_files
 */

#include "../../../tests/testXWC.hpp"

#include "../generated/logTypes.hpp"
#include "../generated/logVerify.hpp"
#include "../generated/logCodeValid.hpp"
#include "../generated/logStdFormat.hpp"

#include <sstream>

namespace libXWCTest
{

/** \defgroup logger_unit_test libXWC::logger Unit Tests
 * \ingroup unit_test
 */

/// Namespace for XWC::logger tests
/** \ingroup logger_unit_test
 *
 */
namespace loggerTest
{

/** \defgroup logTypes_unit_test log types Unit Tests
 * \ingroup logger_unit_test
 */

/// Namespace for XWC::logger::logType_Accessor tests
/** \ingroup logTypes_unit_test
 *
 */
namespace logTypeAccessorTest
{

/// Call to accessor with invalid member
/**
 * \ingroup logTypes_unit_test
 */
TEST_CASE( "Call to accessor with invalid member", "[libMagAOX::logger::logTypes_Accessor]" )
{
    SECTION( "ao_operator" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::ao_operator::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "config_log" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::config_log::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "git_state" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::git_state::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "indidriver_start" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::indidriver_start::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "indidriver_stop" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::indidriver_stop::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "loop_closed" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::loop_closed::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "loop_open" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::loop_open::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "loop_paused" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::loop_paused::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "observer" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::observer::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "ocam_temps" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::ocam_temps::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "outlet_channel_state" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::outlet_channel_state::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "outlet_state" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::outlet_state::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "pico_channel" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::pico_channel::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "saving_start" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::saving_start::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "saving_state_change" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::saving_state_change::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "saving_stop" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::saving_stop::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "software_log" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::software_log::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "state_change" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::state_change::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "string_log" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::string_log::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_blockgains" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_blockgains::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_chrony_stats" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_chrony_stats::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_chrony_status" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_chrony_status::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_cooler" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_cooler::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_coreloads" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_coreloads::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_coretemps" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_coretemps::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_dmmodes" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_dmmodes::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_dmspeck" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_dmspeck::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_drivetemps" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_drivetemps::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_fgtimings" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_fgtimings::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_fxngen" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_fxngen::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_loopgain" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_loopgain::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_observer" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_observer::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_pi335" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_pi335::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_pico" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_pico::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_pokecenter" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_pokecenter::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_pokeloop" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_pokeloop::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_position" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_position::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_adctrack" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_adctrack::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_rhusb" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_rhusb::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_saving_state" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_saving_state::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_saving" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_saving::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_sparkleclock" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_sparkleclock::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_stage" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_stage::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_stdcam" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_stdcam::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_telcat" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_telcat::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_teldata" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_teldata::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_telenv" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_telenv::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_telpos" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_telpos::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_telsee" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_telsee::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_telvane" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_telvane::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_temps" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_temps::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_usage" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_usage::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "telem_zaber" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_zaber::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "text_log" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::text_log::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "ttmmode_params" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::ttmmod_params::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "user_log" )
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::user_log::getAccessor( "" );

        REQUIRE( lmd.keyword == "" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }
    /*
    SECTION("")
    {
        MagAOX::logger::logMetaDetail lmd = MagAOX::logger::::getAccessor("");

        REQUIRE(lmd.keyword == "");
        REQUIRE(lmd.comment == "");
        REQUIRE(lmd.format == "");
        REQUIRE(lmd.valType == -1);
        REQUIRE(lmd.metaType == -1);
        REQUIRE(lmd.accessor == nullptr);
        REQUIRE(lmd.hierarch == true);
    }
    */
}

/// telem_pokecenter and telem_pokeloop have a hand-written "not measuring" short-circuit in
/// their messageT constructors (measuring == 0) that skips serializing the rest of the
/// fields entirely -- distinct from the general-purpose flatbuffer construction the
/// generated per-type tests exercise, so it needs its own check.
/**
 * \ingroup logger_unit_test
 */
TEST_CASE( "telem_pokecenter and telem_pokeloop skip serializing fields when not measuring", "[libMagAOX::logger::types]" )
{
    SECTION( "telem_pokecenter, multi-vector constructor" )
    {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_pokecenter>(
            buf,
            flatlogs::timespecX( 1732170780, 0 ),
            MagAOX::logger::telem_pokecenter::messageT( (uint8_t)0, 1.0f, 2.0f, std::vector<float>{ 3.0f }, std::vector<float>{ 4.0f } ),
            flatlogs::logPrio::LOG_TELEM );

        void *msg = flatlogs::logHeader::messageBuffer( buf );

        REQUIRE( MagAOX::logger::telem_pokecenter::measuring( msg ) == false );
        REQUIRE( MagAOX::logger::telem_pokecenter::msgString( msg, 0 ) == "not measuring" );
    }

    SECTION( "telem_pokecenter, combined-vector constructor" )
    {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_pokecenter>(
            buf,
            flatlogs::timespecX( 1732170780, 0 ),
            MagAOX::logger::telem_pokecenter::messageT( (uint8_t)0, 1.0f, 2.0f, std::vector<float>{ 3.0f, 4.0f } ),
            flatlogs::logPrio::LOG_TELEM );

        void *msg = flatlogs::logHeader::messageBuffer( buf );

        REQUIRE( MagAOX::logger::telem_pokecenter::measuring( msg ) == false );
        REQUIRE( MagAOX::logger::telem_pokecenter::msgString( msg, 0 ) == "not measuring" );
    }

    SECTION( "telem_pokeloop" )
    {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_pokeloop>(
            buf,
            flatlogs::timespecX( 1732170780, 0 ),
            MagAOX::logger::telem_pokeloop::messageT( (uint8_t)0, 1.0f, 2.0f, 3ULL ),
            flatlogs::logPrio::LOG_TELEM );

        void *msg = flatlogs::logHeader::messageBuffer( buf );

        REQUIRE( MagAOX::logger::telem_pokeloop::measuring( msg ) == false );
        REQUIRE( MagAOX::logger::telem_pokeloop::msgString( msg, 0 ) == "[pokeloop] not measuring" );
    }
}

/// logCodeValid, logVerify, and logStdFormat each dispatch on event code via a switch that
/// is generated with one case per real log type; each also has its own default case for an
/// event code with no matching type, which -- being a real, if rare, situation (e.g. reading
/// a log stream from a newer version that added a type this build doesn't know about) -- is
/// worth checking directly rather than only implicitly via the per-type dispatch checks.
/**
 * \ingroup logger_unit_test
 */
TEST_CASE( "logCodeValid, logVerify, and logStdFormat fall back gracefully for an unrecognized event code",
           "[libMagAOX::logger::types]" )
{
    const flatlogs::eventCodeT unknownCode = 60999;

    REQUIRE( MagAOX::logger::logCodeValid( unknownCode ) == false );

    SECTION( "logVerify" )
    {
        flatlogs::bufferPtrT buf;
        REQUIRE( MagAOX::logger::logVerify( unknownCode, buf, 0 ) == false );
    }

    SECTION( "logStdFormat" )
    {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::git_state>(
            buf, flatlogs::timespecX( 1732170780, 0 ), MagAOX::logger::git_state::messageT( "repo", "sha", false ), flatlogs::logPrio::LOG_NOTICE );
        flatlogs::logHeader::eventCode( buf, unknownCode );

        std::ostringstream oss;
        MagAOX::logger::logStdFormat( oss, buf );

        REQUIRE( oss.str().find( "Unknown log type" ) != std::string::npos );
    }
}

} // namespace logTypeAccessorTest
} // namespace loggerTest
} // namespace libXWCTest
