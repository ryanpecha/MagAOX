/** \file logTypes_test.cpp
 * \brief Tests for individual log types' formatting and validation branches, and for the
 *        code-generated central dispatchers' unrecognized-event-code fallback.
 * \ingroup logger_files
 */

#include "../../../tests/testXWC.hpp"

#include <sstream>

#include "../generated/logTypes.hpp"
#include "../generated/logVerify.hpp"
#include "../generated/logCodeValid.hpp"
#include "../generated/logStdFormat.hpp"

namespace libXWCTest
{

namespace loggerTest
{

/** \defgroup logTypes_formatting_unit_test log type formatting Unit Tests
 * \ingroup logger_unit_test
 */

/// Namespace for XWC::logger log type formatting/validation tests
/** \ingroup logTypes_formatting_unit_test
 *
 */
namespace logTypesTest
{

/// Builds a full log entry for TYPE from msg and returns both the buffer and the message
/// length verify()/msgString() need -- the same construction the generated per-type tests
/// use, factored out here for these supplementary branch-coverage checks.
template <class TYPE>
std::pair<flatlogs::bufferPtrT, flatlogs::msgLenT> makeLog( const typename TYPE::messageT &msg )
{
    flatlogs::msgLenT    len = TYPE::length( msg );
    flatlogs::bufferPtrT buf;
    flatlogs::logHeader::createLog<TYPE>(
        buf, flatlogs::timespecX( 1732170780, 0 ), msg, flatlogs::logPrio::LOG_TELEM );
    return { buf, len };
}

/// logCodeValid, logVerify, logStdFormat, eventCode, and eventCodeName each dispatch on
/// event code (or name) via a switch/if-chain with one case per real log type (exercised
/// per-type by the generated tests); each also has its own fallback for an event code or
/// name with no matching type, which is worth checking directly here since no per-type
/// test can reach it.
/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "logCodeValid, logVerify, logStdFormat, eventCode, and eventCodeName fall back "
           "gracefully for an unrecognized event code",
           "[libMagAOX::logger::logCodes]" )
{
    const flatlogs::eventCodeT unknownCode = 60999;

    SECTION( "logCodeValid reports the code as not valid" )
    {
        REQUIRE( MagAOX::logger::logCodeValid( unknownCode ) == false );
    }

    SECTION( "logVerify reports failure without touching the buffer" )
    {
        flatlogs::bufferPtrT buf;
        REQUIRE( MagAOX::logger::logVerify( unknownCode, buf, 0 ) == false );
    }

    SECTION( "logStdFormat writes an unknown-log-type message instead of formatting anything" )
    {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::git_state>( buf,
                                                                    flatlogs::timespecX( 1732170780, 0 ),
                                                                    MagAOX::logger::git_state::messageT( "repo", "sha", false ),
                                                                    flatlogs::logPrio::LOG_NOTICE );
        flatlogs::logHeader::eventCode( buf, unknownCode );

        std::ostringstream oss;
        MagAOX::logger::logStdFormat( oss, buf );

        REQUIRE( oss.str().find( "Unknown log type" ) != std::string::npos );
    }

    SECTION( "eventCode maps an unrecognized name to UNKNOWN" )
    {
        REQUIRE( MagAOX::logger::eventCode( "not_a_real_log_type" ) == MagAOX::logger::eventCodes::UNKNOWN );
    }

    SECTION( "eventCodeName maps an unrecognized code to a placeholder string" )
    {
        REQUIRE( MagAOX::logger::eventCodeName( unknownCode ) == "unknown event code" );
    }
}

/// The per-type generated tests always construct bool-gated fields as true (the
/// generator's dummy value for every bool field), so the "false" side of each type's own
/// msgString() branching is never reached there. These are genuine, type-specific
/// formatting branches (not generated dispatch code), so they're covered here directly
/// rather than by complicating the generator's single bool dummy value for every type.
/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "telem_dmspeck, telem_sparkleclock, telem_fxngen, and telem_poltrack format "
           "their false/off/not-tracking states",
           "[libMagAOX::logger::telem_dmspeck]" )
{
    SECTION( "a telem_dmspeck message that is not modulating" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_dmspeck>(
            MagAOX::logger::telem_dmspeck::messageT( false, false, 1.0f, { 1.0f }, { 2.0f }, { 3.0f }, { true } ) );
        REQUIRE( MagAOX::logger::telem_dmspeck::msgString( flatlogs::logHeader::messageBuffer( buf ), len ) ==
                 "[speckles] not modulating" );
    }

    SECTION( "a telem_dmspeck message that is modulating by frequency, not trigger" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_dmspeck>(
            MagAOX::logger::telem_dmspeck::messageT( true, false, 12.5f, { 1.0f }, { 2.0f }, { 3.0f }, { true, false } ) );
        std::string s = MagAOX::logger::telem_dmspeck::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "at 12.5" ) != std::string::npos );
        REQUIRE( s.find( "+-" ) != std::string::npos );
    }

    SECTION( "a telem_sparkleclock message that is not modulating" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_sparkleclock>(
            MagAOX::logger::telem_sparkleclock::messageT( false, false, 1.0f, 2.0f, { 1.0f }, 3.0f, 4.0f ) );
        REQUIRE( MagAOX::logger::telem_sparkleclock::msgString( flatlogs::logHeader::messageBuffer( buf ), len ) ==
                 "[sparkleclock] not modulating" );
    }

    SECTION( "a telem_sparkleclock message that is modulating by frequency, not trigger" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_sparkleclock>(
            MagAOX::logger::telem_sparkleclock::messageT( true, false, 12.5f, 2.0f, { 1.0f }, 3.0f, 4.0f ) );
        std::string s = MagAOX::logger::telem_sparkleclock::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "at 12.5" ) != std::string::npos );
    }

    SECTION( "a telem_fxngen message with both channels' sync off" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_fxngen>( MagAOX::logger::telem_fxngen::messageT(
            1, 1.0, 1.0, 1.0, 1.0, 0, 1, 1.0, 1.0, 1.0, 1.0, 0, (uint8_t)0, (uint8_t)0, 1.0, 1.0 ) );
        std::string s = MagAOX::logger::telem_fxngen::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "SYNC OFF" ) != std::string::npos );
    }

    SECTION( "a telem_poltrack message that is not tracking" )
    {
        auto [buf, len] =
            makeLog<MagAOX::logger::telem_poltrack>( MagAOX::logger::telem_poltrack::messageT( 1.0f, 1.0f, "home", false ) );
        std::string s = MagAOX::logger::telem_poltrack::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "NONE" ) != std::string::npos );
    }
}

/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "ttmmod_params and outlet/outlet-channel state format every named state value",
           "[libMagAOX::logger::ttmmod_params]" )
{
    SECTION( "ttmmod_params in each of its named modState values" )
    {
        struct
        {
            uint8_t     state;
            std::string expect;
        } cases[] = { { 0, "OFF" }, { 1, "REST" }, { 2, "INT" }, { 3, "SET" } };

        for( auto &c : cases )
        {
            auto [buf, len] =
                makeLog<MagAOX::logger::ttmmod_params>( MagAOX::logger::ttmmod_params::messageT( c.state, 0.0, 0.0, 0.0, 0.0 ) );
            REQUIRE( MagAOX::logger::ttmmod_params::msgString( flatlogs::logHeader::messageBuffer( buf ), len ) == c.expect );
            // modState() duplicates msgString()'s state-name mapping for the meta-data
            // accessor system, rather than delegating to it -- exercise it directly too.
            REQUIRE( MagAOX::logger::ttmmod_params::modState( flatlogs::logHeader::messageBuffer( buf ) ) == c.expect );
        }

        auto [buf, len] = makeLog<MagAOX::logger::ttmmod_params>( MagAOX::logger::ttmmod_params::messageT( 4, 10.0, 0.5, 0.0, 0.0 ) );
        std::string s = MagAOX::logger::ttmmod_params::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "MOD" ) != std::string::npos );
        REQUIRE( s.find( "Freq: 10" ) != std::string::npos );
        REQUIRE( s.find( "Rad: 0.5" ) != std::string::npos );
        REQUIRE( MagAOX::logger::ttmmod_params::modState( flatlogs::logHeader::messageBuffer( buf ) ) == "MOD" );
    }

    SECTION( "outlet_state in each of its named state values" )
    {
        struct
        {
            uint8_t     state;
            std::string expect;
        } cases[] = { { 0, "OFF" }, { 1, "INT" }, { 2, "ON" } };

        for( auto &c : cases )
        {
            auto [buf, len] = makeLog<MagAOX::logger::outlet_state>( MagAOX::logger::outlet_state::messageT( 1, c.state ) );
            std::string s = MagAOX::logger::outlet_state::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
            REQUIRE( s.find( c.expect ) != std::string::npos );
        }
    }

    SECTION( "outlet_channel_state in each of its named state values" )
    {
        struct
        {
            uint8_t     state;
            std::string expect;
        } cases[] = { { 0, "OFF" }, { 1, "INT" }, { 2, "ON" } };

        for( auto &c : cases )
        {
            auto [buf, len] =
                makeLog<MagAOX::logger::outlet_channel_state>( MagAOX::logger::outlet_channel_state::messageT( "ch1", c.state ) );
            std::string s = MagAOX::logger::outlet_channel_state::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
            REQUIRE( s.find( c.expect ) != std::string::npos );
        }
    }
}

/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "telem_blockgains reports a placeholder when a gain vector and its "
           "constant-flag vector have mismatched lengths",
           "[libMagAOX::logger::telem_blockgains]" )
{
    SECTION( "gains and gains_constant of different lengths" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_blockgains>( MagAOX::logger::telem_blockgains::messageT(
            { 1.0f, 2.0f }, { 1 }, std::vector<float>{}, std::vector<uint8_t>{}, std::vector<float>{}, std::vector<uint8_t>{} ) );
        std::string s = MagAOX::logger::telem_blockgains::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "1.0" ) != std::string::npos );
    }

    SECTION( "mcs and mcs_constant of different lengths" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_blockgains>( MagAOX::logger::telem_blockgains::messageT(
            std::vector<float>{}, std::vector<uint8_t>{}, { 1.0f, 2.0f }, { 1 }, std::vector<float>{}, std::vector<uint8_t>{} ) );
        MagAOX::logger::telem_blockgains::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
    }

    SECTION( "lims and lims_constant of different lengths" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_blockgains>( MagAOX::logger::telem_blockgains::messageT(
            std::vector<float>{}, std::vector<uint8_t>{}, std::vector<float>{}, std::vector<uint8_t>{}, { 1.0f, 2.0f }, { 1 } ) );
        MagAOX::logger::telem_blockgains::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
    }
}

/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "telem_observer's verify() catches malformed buffers and non-printable field content",
           "[libMagAOX::logger::telem_observer]" )
{
    SECTION( "a truncated buffer" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_observer>(
            MagAOX::logger::telem_observer::messageT( "a@b.c", "obs", true, "tgt", "op@b.c" ) );
        REQUIRE( MagAOX::logger::telem_observer::verify( buf, 1 ) == false );
    }

    SECTION( "a non-printable character in each string field in turn" )
    {
        std::string bad( 1, '\x01' );

        auto [buf1, len1] = makeLog<MagAOX::logger::telem_observer>(
            MagAOX::logger::telem_observer::messageT( bad, "obs", true, "tgt", "op@b.c" ) );
        REQUIRE( MagAOX::logger::telem_observer::verify( buf1, len1 ) == false );

        auto [buf2, len2] = makeLog<MagAOX::logger::telem_observer>(
            MagAOX::logger::telem_observer::messageT( "a@b.c", bad, true, "tgt", "op@b.c" ) );
        REQUIRE( MagAOX::logger::telem_observer::verify( buf2, len2 ) == false );

        auto [buf3, len3] = makeLog<MagAOX::logger::telem_observer>(
            MagAOX::logger::telem_observer::messageT( "a@b.c", "obs", true, bad, "op@b.c" ) );
        REQUIRE( MagAOX::logger::telem_observer::verify( buf3, len3 ) == false );

        auto [buf4, len4] = makeLog<MagAOX::logger::telem_observer>(
            MagAOX::logger::telem_observer::messageT( "a@b.c", "obs", true, "tgt", bad ) );
        REQUIRE( MagAOX::logger::telem_observer::verify( buf4, len4 ) == false );
    }
}

/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "telem_stdcam formats every named shutter/sync/crop/led state", "[libMagAOX::logger::telem_stdcam]" )
{
    // Common placeholder values for fields this test doesn't care about.
    auto build = []( int8_t shutterState, uint8_t synchro, float vshift, uint8_t cropMode, int8_t led )
    {
        return MagAOX::logger::telem_stdcam::messageT( "mode", 0.0f, 0.0f, 1, 1, 1, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                        (uint8_t)0, (uint8_t)0, "", "", shutterState, synchro, vshift, cropMode,
                                                        "", "", "", led );
    };

    SECTION( "shutter state unknown (-1), sync off, vshift at zero, crop off (-1), led off (-1)" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_stdcam>( build( -1, 0, 0.0f, (uint8_t)-1, -1 ) );
        void       *msgBuffer = flatlogs::logHeader::messageBuffer( buf );
        std::string s         = MagAOX::logger::telem_stdcam::msgString( msgBuffer, len );
        REQUIRE( s.find( "UNKN" ) != std::string::npos );
        REQUIRE( s.find( "Sync: OFF" ) != std::string::npos );
        REQUIRE( s.find( "vshift: ---" ) != std::string::npos );
        REQUIRE( s.find( "crop: ---" ) != std::string::npos );
        REQUIRE( s.find( "led: ---" ) != std::string::npos );
        // shutterState() duplicates msgString()'s shutter-state-name mapping for the
        // meta-data accessor system, rather than delegating to it -- exercise it directly.
        REQUIRE( MagAOX::logger::telem_stdcam::shutterState( msgBuffer ) == "UNKNOWN" );
    }

    SECTION( "shutter shut (0), crop on (1), led on (1)" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_stdcam>( build( 0, 1, 1.0f, 1, 1 ) );
        void       *msgBuffer = flatlogs::logHeader::messageBuffer( buf );
        std::string s         = MagAOX::logger::telem_stdcam::msgString( msgBuffer, len );
        REQUIRE( s.find( "SHUT" ) != std::string::npos );
        REQUIRE( s.find( "crop: ON" ) != std::string::npos );
        REQUIRE( s.find( "led: ON" ) != std::string::npos );
        REQUIRE( MagAOX::logger::telem_stdcam::shutterState( msgBuffer ) == "SHUT" );
        // cropMode() and led() likewise duplicate msgString()'s on/off mapping for the
        // meta-data accessor system as their own bool-valued accessors.
        REQUIRE( MagAOX::logger::telem_stdcam::cropMode( msgBuffer ) == true );
        REQUIRE( MagAOX::logger::telem_stdcam::led( msgBuffer ) == true );
    }

    SECTION( "shutter open (1)" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_stdcam>( build( 1, 1, 1.0f, 0, 0 ) );
        void       *msgBuffer = flatlogs::logHeader::messageBuffer( buf );
        std::string s         = MagAOX::logger::telem_stdcam::msgString( msgBuffer, len );
        REQUIRE( s.find( "OPEN" ) != std::string::npos );
        REQUIRE( MagAOX::logger::telem_stdcam::shutterState( msgBuffer ) == "OPEN" );
    }
}

/// telem_pokecenter and telem_pokeloop's "measuring" is a uint8_t (not a bool field, so it
/// isn't covered by the generator's fixed dummy value at all) that gates their own
/// constructors: measuring==0 short-circuits and skips serializing the rest of the fields.
/**
 * \ingroup logTypes_formatting_unit_test
 */
TEST_CASE( "telem_pokecenter and telem_pokeloop format every named measuring state",
           "[libMagAOX::logger::telem_pokecenter]" )
{
    SECTION( "telem_pokecenter not measuring (multi-vector constructor)" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_pokecenter>( MagAOX::logger::telem_pokecenter::messageT(
            (uint8_t)0, 1.0f, 2.0f, std::vector<float>{ 3.0f }, std::vector<float>{ 4.0f } ) );
        REQUIRE( MagAOX::logger::telem_pokecenter::measuring( flatlogs::logHeader::messageBuffer( buf ) ) == false );
        REQUIRE( MagAOX::logger::telem_pokecenter::msgString( flatlogs::logHeader::messageBuffer( buf ), len ) ==
                 "not measuring" );
    }

    SECTION( "telem_pokecenter not measuring (combined-vector constructor)" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_pokecenter>(
            MagAOX::logger::telem_pokecenter::messageT( (uint8_t)0, 1.0f, 2.0f, std::vector<float>{ 3.0f, 4.0f } ) );
        REQUIRE( MagAOX::logger::telem_pokecenter::msgString( flatlogs::logHeader::messageBuffer( buf ), len ) ==
                 "not measuring" );
    }

    SECTION( "telem_pokecenter measuring a single poke, with mismatched poke_x/poke_y lengths" )
    {
        auto [buf, len] = makeLog<MagAOX::logger::telem_pokecenter>( MagAOX::logger::telem_pokecenter::messageT(
            (uint8_t)1, 1.0f, 2.0f, std::vector<float>{ 3.0f, 4.0f }, std::vector<float>{ 5.0f } ) );
        std::string s = MagAOX::logger::telem_pokecenter::msgString( flatlogs::logHeader::messageBuffer( buf ), len );
        REQUIRE( s.find( "single" ) != std::string::npos );
        REQUIRE( s.find( "[poke-avg] ? [pokes] ?" ) != std::string::npos );
    }

    SECTION( "telem_pokeloop not measuring" )
    {
        auto [buf, len] =
            makeLog<MagAOX::logger::telem_pokeloop>( MagAOX::logger::telem_pokeloop::messageT( (uint8_t)0, 1.0f, 2.0f, 3ULL ) );
        REQUIRE( MagAOX::logger::telem_pokeloop::measuring( flatlogs::logHeader::messageBuffer( buf ) ) == false );
        REQUIRE( MagAOX::logger::telem_pokeloop::msgString( flatlogs::logHeader::messageBuffer( buf ), len ) ==
                 "[pokeloop] not measuring" );
    }
}

} // namespace logTypesTest
} // namespace loggerTest
} // namespace libXWCTest
