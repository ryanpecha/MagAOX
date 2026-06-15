/** \file logMeta.cpp
 * \brief Declares and defines the logMeta class and related classes.
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 * \ingroup logger_files
 *
 */

#include "logMeta.hpp"

// #include "generated/logTypes.hpp"

#include "generated/logMemberAccessor.hpp"

namespace MagAOX
{
namespace logger
{

/*
logMetaDetail logMemberAccessor( flatlogs::eventCodeT ec,
                          const std::string & memberName
                        )
{
   switch(ec)
   {
      case telem_stdcam::eventCode:
         return telem_stdcam::getAccessor(memberName);
      case telem_telcat::eventCode:
         return telem_telcat::getAccessor(memberName);
      case telem_teldata::eventCode:
         return telem_teldata::getAccessor(memberName);
      case telem_telpos::eventCode:
         return telem_telpos::getAccessor(memberName);
      case telem_stage::eventCode:
         return telem_stage::getAccessor(memberName);
      case telem_zaber::eventCode:
         return telem_zaber::getAccessor(memberName);
      case telem_dmspeck::eventCode:
         return telem_dmspeck::getAccessor(memberName);
      case telem_observer::eventCode:
         return telem_observer::getAccessor(memberName);
      case telem_fxngen::eventCode:
         return telem_fxngen::getAccessor(memberName);
      case telem_loopgain::eventCode:
         return telem_loopgain::getAccessor(memberName);
      default:
         std::cerr << "Missing logMemberAccessor case entry for " << ec << ": " << memberName << "\n";
         return logMetaDetail();
   }
}*/

const std::string &logMeta::unavailableValue()
{
    static const std::string value = "NOT AVAILABLE";

    return value;
}

bool verifyLogEntry( flatlogs::eventCodeT ev, char *log )
{
    if( log == nullptr )
    {
        return false;
    }

    flatlogs::bufferPtrT logBuff( log, []( char * ) {} );
    flatlogs::msgLenT    len = flatlogs::logHeader::msgLen( log );

    switch( ev )
    {
    case eventCodes::GIT_STATE:
        return git_state::verify( logBuff, len );
    case eventCodes::TEXT_LOG:
        return text_log::verify( logBuff, len );
    case eventCodes::USER_LOG:
        return user_log::verify( logBuff, len );
    case eventCodes::STATE_CHANGE:
        return state_change::verify( logBuff, len );
    case eventCodes::SOFTWARE_LOG:
        return software_log::verify( logBuff, len );
    case eventCodes::CONFIG_LOG:
        return config_log::verify( logBuff, len );
    case eventCodes::INDIDRIVER_START:
        return indidriver_start::verify( logBuff, len );
    case eventCodes::INDIDRIVER_STOP:
        return indidriver_stop::verify( logBuff, len );
    case eventCodes::LOOP_CLOSED:
        return loop_closed::verify( logBuff, len );
    case eventCodes::LOOP_PAUSED:
        return loop_paused::verify( logBuff, len );
    case eventCodes::LOOP_OPEN:
        return loop_open::verify( logBuff, len );
    case eventCodes::OBSERVER:
        return observer::verify( logBuff, len );
    case eventCodes::AO_OPERATOR:
        return ao_operator::verify( logBuff, len );
    case eventCodes::PICO_CHANNEL:
        return pico_channel::verify( logBuff, len );
    case eventCodes::OUTLET_STATE:
        return outlet_state::verify( logBuff, len );
    case eventCodes::OUTLET_CHANNEL_STATE:
        return outlet_channel_state::verify( logBuff, len );
    case eventCodes::TELEM_SAVING_STATE:
        return telem_saving_state::verify( logBuff, len );
    case eventCodes::TELEM_FXNGEN:
        return telem_fxngen::verify( logBuff, len );
    case eventCodes::TTMMOD_PARAMS:
        return ttmmod_params::verify( logBuff, len );
    case eventCodes::OCAM_TEMPS:
        return ocam_temps::verify( logBuff, len );
    case eventCodes::CRED2_TEMPS:
        return cred2_temps::verify( logBuff, len );
    case eventCodes::SAVING_START:
        return saving_start::verify( logBuff, len );
    case eventCodes::SAVING_STOP:
        return saving_stop::verify( logBuff, len );
    case eventCodes::TELEM_SAVING:
        return telem_saving::verify( logBuff, len );
    case eventCodes::TELEM_TELPOS:
        return telem_telpos::verify( logBuff, len );
    case eventCodes::TELEM_TELDATA:
        return telem_teldata::verify( logBuff, len );
    case eventCodes::TELEM_TELVANE:
        return telem_telvane::verify( logBuff, len );
    case eventCodes::TELEM_TELENV:
        return telem_telenv::verify( logBuff, len );
    case eventCodes::TELEM_TELCAT:
        return telem_telcat::verify( logBuff, len );
    case eventCodes::TELEM_TELSEE:
        return telem_telsee::verify( logBuff, len );
    case eventCodes::TELEM_TCSI_TIPTILT:
        return telem_tcsi_tiptilt::verify( logBuff, len );
    case eventCodes::TELEM_TCSI_FOCUS:
        return telem_tcsi_focus::verify( logBuff, len );
    case eventCodes::TELEM_TCSI_LABMODE:
        return telem_tcsi_labmode::verify( logBuff, len );
    case eventCodes::TELEM_STAGE:
        return telem_stage::verify( logBuff, len );
    case eventCodes::TELEM_ZABER:
        return telem_zaber::verify( logBuff, len );
    case eventCodes::TELEM_PICO:
        return telem_pico::verify( logBuff, len );
    case eventCodes::TELEM_POSITION:
        return telem_position::verify( logBuff, len );
    case eventCodes::TELEM_PSFACQ:
        return telem_psfacq::verify( logBuff, len );
    case eventCodes::TELEM_POKECENTER:
        return telem_pokecenter::verify( logBuff, len );
    case eventCodes::TELEM_POKELOOP:
        return telem_pokeloop::verify( logBuff, len );
    case eventCodes::TELEM_OBSERVER:
        return telem_observer::verify( logBuff, len );
    case eventCodes::TELEM_RHUSB:
        return telem_rhusb::verify( logBuff, len );
    case eventCodes::TELEM_TEMPS:
        return telem_temps::verify( logBuff, len );
    case eventCodes::TELEM_STDCAM:
        return telem_stdcam::verify( logBuff, len );
    case eventCodes::TELEM_CORETEMPS:
        return telem_coretemps::verify( logBuff, len );
    case eventCodes::TELEM_CORELOADS:
        return telem_coreloads::verify( logBuff, len );
    case eventCodes::TELEM_DRIVETEMPS:
        return telem_drivetemps::verify( logBuff, len );
    case eventCodes::TELEM_USAGE:
        return telem_usage::verify( logBuff, len );
    case eventCodes::TELEM_COOLER:
        return telem_cooler::verify( logBuff, len );
    case eventCodes::TELEM_CHRONY_STATUS:
        return telem_chrony_status::verify( logBuff, len );
    case eventCodes::TELEM_CHRONY_STATS:
        return telem_chrony_stats::verify( logBuff, len );
    case eventCodes::TELEM_DMSPECK:
        return telem_dmspeck::verify( logBuff, len );
    case eventCodes::TELEM_FGTIMINGS:
        return telem_fgtimings::verify( logBuff, len );
    case eventCodes::TELEM_DMMODES:
        return telem_dmmodes::verify( logBuff, len );
    case eventCodes::TELEM_LOOPGAIN:
        return telem_loopgain::verify( logBuff, len );
    case eventCodes::TELEM_MODALGAINOPT:
        return telem_modalgainopt::verify( logBuff, len );
    case eventCodes::TELEM_BLOCKGAINS:
        return telem_blockgains::verify( logBuff, len );
    case eventCodes::TELEM_OFFLOADING:
        return telem_offloading::verify( logBuff, len );
    case eventCodes::TELEM_W2TCSOFFLOADER:
        return telem_w2tcsoffloader::verify( logBuff, len );
    case eventCodes::TELEM_FLOWRPM:
        return telem_flowrpm::verify( logBuff, len );
    case eventCodes::TELEM_PI335:
        return telem_pi335::verify( logBuff, len );
    case eventCodes::TELEM_SPARKLECLOCK:
        return telem_sparkleclock::verify( logBuff, len );
    case eventCodes::TELEM_POLTRACK:
        return telem_poltrack::verify( logBuff, len );
    case eventCodes::TELEM_ADCTRACK:
        return telem_adctrack::verify( logBuff, len );
    default:
        return false;
    }
}

std::string logMeta::fitsKeyword() const
{
    std::string keyw;
    if( m_detail.hierarch )
    {
        // Add spaces to make sure hierarch is invoked
        keyw = m_spec.device + " " + m_spec.keyword;
        if( keyw.size() < 9 )
        {
            keyw += std::string( 9 - keyw.size(), ' ' );
        }
    }
    else
    {
        keyw = m_spec.keyword;
    }

    return keyw;
}

logMeta::logMeta( const logMetaSpec &lms )
{
    setLog( lms );
}

const std::string &logMeta::device()
{
    return m_spec.device;
}

const std::string &logMeta::keyword()
{
    return m_spec.keyword;
}

const std::string &logMeta::comment()
{
    return m_spec.comment;
}

int logMeta::setLog( const logMetaSpec &lms )
{
    m_spec   = lms;
    m_detail = logMemberAccessor( m_spec.eventCode, m_spec.member );

    if( m_spec.keyword == "" )
        m_spec.keyword = m_detail.keyword;
    if( m_spec.format == "" )
        m_spec.format = m_detail.format;
    if( m_spec.format == "" )
    {
        switch( m_detail.valType )
        {
        case valTypes::String:
            m_spec.format = "%s";
            break;
        case valTypes::Bool:
            m_spec.format = "%d";
            break;
        case valTypes::Char:
            m_spec.format = "%d";
            break;
        case valTypes::UChar:
            m_spec.format = "%u";
            break;
        case valTypes::Short:
            m_spec.format = "%d";
            break;
        case valTypes::UShort:
            m_spec.format = "%u";
            break;
        case valTypes::Int:
            m_spec.format = "%d";
            break;
        case valTypes::UInt:
            m_spec.format = "%u";
            break;
        case valTypes::Long:
            m_spec.format = "%ld";
            break;
        case valTypes::ULong:
            m_spec.format = "%lu";
            break;
        case valTypes::Float:
            m_spec.format = "%G";
            break;
        case valTypes::Double:
            m_spec.format = "%G";
            break;
        case valTypes::Vector_Bool:
            m_spec.format = "%d";
            break;
        case valTypes::Vector_Float:
            m_spec.format = "%G";
            break;
        default:
            std::cerr << "Unrecognised value type for " + m_spec.device + " " + m_spec.keyword +
                             ".  Using format %d/\n";
            m_spec.format = "%d";
        }
    }

    if( m_spec.comment == "" )
        m_spec.comment = m_detail.comment;

    return 0;
}

std::string logMeta::value( logMap<verboseT> &lm, const flatlogs::timespecX &stime, const flatlogs::timespecX &atime )
{
    if( m_detail.accessor == nullptr )
        return m_invalidValue;

    if( m_detail.valType == valTypes::String )
    {
        return valueString( lm, stime, atime );
    }
    else
    {
        return valueNumber( lm, stime, atime );
    }
}

std::string
logMeta::valueNumber( logMap<verboseT> &lm, const flatlogs::timespecX &stime, const flatlogs::timespecX &atime )
{
    char str[64];

    if( m_detail.metaType == metaTypes::State )
    {
        switch( m_detail.valType )
        {
        case valTypes::Bool:
        {
            bool val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<bool ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Char:
        {
            char val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<char ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
            {
                return m_invalidValue;
            }
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::UChar:
        {
            unsigned char val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<unsigned char ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Short:
        {
            short val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<short ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::UShort:
        {
            unsigned short val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<unsigned short ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Int:
        {
            int val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<int ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::UInt:
        {
            unsigned int val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<unsigned int ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Long:
        {
            long val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<long ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::ULong:
        {
            unsigned long val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<unsigned long ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::LongLong:
        {
            long long val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<long long ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::ULongLong:
        {
            unsigned long long val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<unsigned long long ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Float:
        {
            float val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<float ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Double:
        {
            double val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<double ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Vector_Bool:
        {
            std::vector<bool> val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<std::vector<bool> ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;

            if( val.size() == 0 )
                return "";

            std::string res;

            for( size_t n = 0; n < val.size() - 1; ++n )
            {
                snprintf( str, sizeof( str ), m_spec.format.c_str(), (int)val[n] );
                res += str;
                res += ',';
            }

            snprintf( str, sizeof( str ), m_spec.format.c_str(), (int)val.back() );
            res += str;

            return res;
        }
        case valTypes::Vector_Float:
        {
            std::vector<float> val;
            if( getLogStateVal( val,
                                lm,
                                m_spec.device,
                                m_spec.eventCode,
                                stime,
                                atime,
                                reinterpret_cast<std::vector<float> ( * )( void * )>( m_detail.accessor ),
                                &m_hint ) != 0 )
                return m_invalidValue;

            if( val.size() == 0 )
                return "";

            std::string res;

            for( size_t n = 0; n < val.size() - 1; ++n )
            {
                snprintf( str, sizeof( str ), m_spec.format.c_str(), val[n] );
                res += str;
                res += ',';
            }

            snprintf( str, sizeof( str ), m_spec.format.c_str(), val.back() );
            res += str;

            return res;
        }
        default:
            return m_invalidValue;
        }
    }
    else if( m_detail.metaType == metaTypes::Continuous )
    {
        switch( m_detail.valType )
        {
        case valTypes::Bool:
        {
            bool val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<bool ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Char:
        {
            char val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<char ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::UChar:
        {
            unsigned char val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<unsigned char ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Short:
        {
            short val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<short ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::UShort:
        {
            unsigned short val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<unsigned short ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Int:
        {
            int val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<int ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::UInt:
        {
            unsigned int val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<unsigned int ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Long:
        {
            long val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<long ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::ULong:
        {
            unsigned long val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<unsigned long ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::LongLong:
        {
            long long val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<long long ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::ULongLong:
        {
            unsigned long long val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<unsigned long long ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Float:
        {
            float val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<float ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        case valTypes::Double:
        {
            double val;
            if( getLogContVal( val,
                               lm,
                               m_spec.device,
                               m_spec.eventCode,
                               stime,
                               atime,
                               reinterpret_cast<double ( * )( void * )>( m_detail.accessor ),
                               &m_hint ) != 0 )
                return m_invalidValue;
            snprintf( str, sizeof( str ), m_spec.format.c_str(), val );
            return std::string( str );
        }
        default:
            return m_invalidValue;
        }
    }

    return m_invalidValue;
}

std::string
logMeta::valueString( logMap<verboseT> &lm, const flatlogs::timespecX &stime, const flatlogs::timespecX &atime )
{
    std::string val;
    if( m_detail.metaType == metaTypes::State )
    {
        if( getLogStateVal( val,
                            lm,
                            m_spec.device,
                            m_spec.eventCode,
                            stime,
                            atime,
                            reinterpret_cast<std::string ( * )( void * )>( m_detail.accessor ),
                            &m_hint ) != 0 )
        {
#ifdef HARD_EXIT
            std::cerr << __FILE__ << " " << __LINE__ << "\n";

            exit( -1 );
#endif
            val = m_invalidValue;
        }
    }
    else
    {
        std::cerr << "String type specified as something other than state\n";
    }
    return val;
}

mx::fits::fitsHeaderCard<logMeta::verboseT> logMeta::unavailableCard() const
{
    return mx::fits::fitsHeaderCard<verboseT>( fitsKeyword(), unavailableValue(), m_spec.comment );
}

mx::fits::fitsHeaderCard<logMeta::verboseT>
logMeta::card( logMap<verboseT> &lm, const flatlogs::timespecX &stime, const flatlogs::timespecX &atime )
{
#ifdef DEBUG
    std::cerr << __FILE__ << " " << __LINE__ << "\n";
#endif

    std::string vstr = value( lm, stime, atime );

#ifdef DEBUG
    std::cerr << __FILE__ << " " << __LINE__ << "\n";
#endif

    std::string keyw = fitsKeyword();

    if( vstr == m_invalidValue )
    {
        // always a string sentinel value, so return here to skip the valType conditional
        return mx::fits::fitsHeaderCard<verboseT>( keyw, vstr, m_spec.comment );
    }

    if( m_detail.valType == valTypes::String || m_detail.valType == valTypes::Vector_Bool ||
        m_detail.valType == valTypes::Vector_Float )
    {
        return mx::fits::fitsHeaderCard<verboseT>( keyw, vstr, m_spec.comment );
    }
    else
    {
        return mx::fits::fitsHeaderCard<verboseT>( keyw, vstr.c_str(), m_detail.valType, m_spec.comment );
    }
}

} // namespace logger
} // namespace MagAOX
