/** \file logMeta.hpp
 * \brief Declares and defines the logMeta class and related classes.
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 * \ingroup logger_files
 *
 * History:
 * - 2020-01-02 created by JRM
 */

#ifndef logger_logMeta_hpp
#define logger_logMeta_hpp

#include <mx/ioutils/fits/fitsHeaderCard.hpp>
// #define HARD_EXIT
#include "logMap.hpp"

namespace MagAOX
{
namespace logger
{

// This is how the user specifies an item of log meta data (i.e. via a config file)
struct logMetaSpec
{
    std::string          device;    ///< Device name that produced the log entry.
    flatlogs::eventCodeT eventCode; ///< Flatlogs event code used to select the log type.
    std::string          member;    ///< Log member name to extract.
    std::string          keyword;   ///< Optional FITS keyword override.
    std::string          format;    ///< Optional printf-style value format override.
    std::string          comment;   ///< Optional FITS comment override.

    /// Construct an empty metadata specification.
    logMetaSpec()
    {
    }

    /// Construct a complete metadata specification.
    logMetaSpec( const std::string         &dev,  /**< [in] device name that produced the log entry */
                 const flatlogs::eventCodeT ec,   /**< [in] flatlogs event code used to select the log type */
                 const std::string         &memb, /**< [in] log member name to extract */
                 const std::string         &k,    /**< [in] FITS keyword override */
                 const std::string         &f,    /**< [in] printf-style value format override */
                 const std::string         &c     /**< [in] FITS comment override */
                 )
        : device( dev ), eventCode( ec ), member( memb ), keyword( k ), format( f ), comment( c )
    {
    }

    /// Construct a metadata specification using the log type's default FITS keyword, format, and comment.
    logMetaSpec( const std::string         &dev, /**< [in] device name that produced the log entry */
                 const flatlogs::eventCodeT ec,  /**< [in] flatlogs event code used to select the log type */
                 const std::string         &memb /**< [in] log member name to extract */
                 )
        : device( dev ), eventCode( ec ), member( memb )
    {
    }
};

// This is the data returned by the member accessor.
struct logMetaDetail
{
    std::string keyword;             ///< FITS keyword to write for this metadata value.
    std::string comment;             ///< FITS comment describing the metadata value.
    std::string format;              ///< printf-style format used to render numeric values.
    int         valType{ -1 };       ///< FITS value type used to serialize the metadata value.
    int         metaType{ -1 };      ///< Metadata time behavior, one of logMeta::metaTypes.
    void       *accessor{ nullptr }; ///< Type-erased accessor for extracting the member from a log message.
    bool        hierarch{ true };    ///< If true, include the device name in a HIERARCH-style keyword.

    /// Construct an empty metadata detail.
    logMetaDetail()
    {
    }

    /// Construct a metadata detail with explicit keyword, comment, format, type, and accessor.
    logMetaDetail( const std::string &k,  /**< [in] FITS keyword */
                   const std::string &c,  /**< [in] FITS comment */
                   const std::string &f,  /**< [in] printf-style value format */
                   int                vt, /**< [in] FITS value type */
                   int                mt, /**< [in] metadata time behavior */
                   void              *acc /**< [in] type-erased value accessor */
                   )
        : keyword( k ), comment( c ), format( f ), valType( vt ), metaType( mt ), accessor( acc )
    {
    }

    /// Construct a metadata detail with explicit HIERARCH behavior.
    logMetaDetail( const std::string &k,   /**< [in] FITS keyword */
                   const std::string &c,   /**< [in] FITS comment */
                   const std::string &f,   /**< [in] printf-style value format */
                   int                vt,  /**< [in] FITS value type */
                   int                mt,  /**< [in] metadata time behavior */
                   void              *acc, /**< [in] type-erased value accessor */
                   bool               h    /**< [in] true to include the device name in the FITS keyword */
                   )
        : keyword( k ), comment( c ), format( f ), valType( vt ), metaType( mt ), accessor( acc ), hierarch( h )
    {
    }

    /// Construct a metadata detail with default value formatting and explicit HIERARCH behavior.
    logMetaDetail( const std::string &k,   /**< [in] FITS keyword */
                   const std::string &c,   /**< [in] FITS comment */
                   int                vt,  /**< [in] FITS value type */
                   int                mt,  /**< [in] metadata time behavior */
                   void              *acc, /**< [in] type-erased value accessor */
                   bool               h    /**< [in] true to include the device name in the FITS keyword */
                   )
        : keyword( k ), comment( c ), valType( vt ), metaType( mt ), accessor( acc ), hierarch( h )
    {
    }

    /// Construct a metadata detail with only keyword, type, and accessor.
    logMetaDetail( const std::string &k,  /**< [in] FITS keyword */
                   int                vt, /**< [in] FITS value type */
                   int                mt, /**< [in] metadata time behavior */
                   void              *acc /**< [in] type-erased value accessor */
                   )
        : keyword( k ), valType( vt ), metaType( mt ), accessor( acc )
    {
    }

    /// Construct a metadata detail with keyword, type, accessor, and explicit HIERARCH behavior.
    logMetaDetail( const std::string &k,   /**< [in] FITS keyword */
                   int                vt,  /**< [in] FITS value type */
                   int                mt,  /**< [in] metadata time behavior */
                   void              *acc, /**< [in] type-erased value accessor */
                   bool               h    /**< [in] true to include the device name in the FITS keyword */
                   )
        : keyword( k ), valType( vt ), metaType( mt ), accessor( acc ), hierarch( h )
    {
    }
};

/*logMetaDetail logMemberAccessor( flatlogs::eventCodeT ec,
                                 const std::string & memberName
                               );
*/

template <typename valT, class verboseT = XWC_DEFAULT_VERBOSITY>
int getLogStateVal( valT                      &val,
                    logMap<verboseT>          &lm,
                    const std::string         &appName,
                    flatlogs::eventCodeT       ev,
                    const flatlogs::timespecX &stime,
                    const flatlogs::timespecX &atime,
                    valT ( *getter )( void * ),
                    char **hint = 0 )
{
    char *atprior = nullptr;
    char *stprior = nullptr;

    char *_hint = nullptr;

    if( hint )
        _hint = *hint;
    else
        _hint = 0;

#ifdef DEBUG
    std::cerr << __FILE__ << " " << __LINE__ << "\n";
#endif

    if( lm.getPriorLog( stprior, appName, ev, stime, _hint ) != 0 )
    {
        return -1;
    }
    valT stprV = getter( flatlogs::logHeader::messageBuffer( stprior ) );

    valT atprV;

#ifdef DEBUG
    std::cerr << __FILE__ << " " << __LINE__ << "\n";
#endif

    if( lm.getNextLog( atprior, stprior, appName ) != 0 )
    {
        return -1;
    }

#ifdef DEBUG
    std::cerr << __FILE__ << " " << __LINE__ << "\n";
#endif

    while( flatlogs::logHeader::timespec( atprior ) < atime )
    {
        atprV = getter( flatlogs::logHeader::messageBuffer( atprior ) );
        if( atprV != stprV )
        {
            val = atprV;
            if( hint )
                *hint = stprior;
            return 0;
        }
        stprior = atprior;
        if( lm.getNextLog( atprior, stprior, appName ) != 0 )
        {
            return -1;
        }
    }

    val = stprV;

    if( hint )
        *hint = stprior;
    return 0;
}

template <typename valT, class verboseT = XWC_DEFAULT_VERBOSITY>
int getLogContVal( valT                      &val,
                   logMap<verboseT>          &lm,
                   const std::string         &appName,
                   flatlogs::eventCodeT       ev,
                   const flatlogs::timespecX &stime,
                   const flatlogs::timespecX &atime,
                   valT ( *getter )( void * ),
                   char **hint = 0 )
{
    char *atafter;
    char *stprior;

    char *_hint;
    if( hint )
        _hint = *hint;
    else
        _hint = 0;

    flatlogs::timespecX midexp = meanTimespecX( atime, stime );

    // Get log entry before midexp
    if( lm.getPriorLog( stprior, appName, ev, midexp, _hint ) != 0 )
    {
        return 1;
    }
    valT stprV = getter( flatlogs::logHeader::messageBuffer( stprior ) );

    // Get log entry after.
    if( lm.getNextLog( atafter, stprior, appName ) != 0 )
    {
#ifdef HARD_EXIT
        exit( -1 );
#endif
        return 1;
    }
    valT atprV = getter( flatlogs::logHeader::messageBuffer( atafter ) );

    double st = flatlogs::logHeader::timespec( stprior ).asDouble();
    double it = midexp.asDouble();
    double et = flatlogs::logHeader::timespec( atafter ).asDouble();

    val = stprV + ( atprV - stprV ) / ( et - st ) * ( it - st );

    if( hint )
        *hint = stprior;

    return 0;
}

/// Manage meta data for a log entry
/** Handles cases where log is a state, i.e. has one of a finite number of values, or is a
 * continuous variable, e.g. a temperature.
 *
 * Contains the information to construct a FITS header card.
 */
struct logMeta
{
    typedef XWC_DEFAULT_VERBOSITY verboseT;

  public:
    enum valTypes
    {
        String           = mx::fits::fitsType<std::string>(),
        Bool             = mx::fits::fitsType<bool>(),
        Char             = mx::fits::fitsType<char>(),
        UChar            = mx::fits::fitsType<unsigned char>(),
        Short            = mx::fits::fitsType<short>(),
        UShort           = mx::fits::fitsType<unsigned short>(),
        Int              = mx::fits::fitsType<int>(),
        UInt             = mx::fits::fitsType<unsigned int>(),
        Long             = mx::fits::fitsType<long>(),
        ULong            = mx::fits::fitsType<unsigned long>(),
        LongLong         = mx::fits::fitsType<long long>(),
        ULongLong        = mx::fits::fitsType<unsigned long long>(),
        Float            = mx::fits::fitsType<float>(),
        Double           = mx::fits::fitsType<double>(),
        Vector_String    = 10000,
        Vector_Bool      = 10002,
        Vector_Char      = 10004,
        Vector_UChar     = 10006,
        Vector_Short     = 10008,
        Vector_UShort    = 10010,
        Vector_Int       = 10012,
        Vector_UInt      = 10014,
        Vector_Long      = 10016,
        Vector_ULong     = 10018,
        Vector_LongLong  = 10020,
        Vector_ULongLong = 10022,
        Vector_Float     = 10024,
        Vector_Double    = 10026
    };

    enum metaTypes
    {
        State,
        Continuous
    };

    /// The string written when metadata is expected but unavailable.
    /**
     * \returns the shared unavailable-value sentinel.
     */
    static const std::string &unavailableValue();

  protected:
    logMetaSpec   m_spec;   ///< User/configuration specification for this metadata item.
    logMetaDetail m_detail; ///< Log-type detail used to extract and serialize this metadata item.

    bool        m_isValid{ false };                   ///< True when the metadata member resolved to an accessor.
    std::string m_invalidValue{ unavailableValue() }; ///< Sentinel returned when metadata cannot be read.

    char *m_hint{ nullptr }; ///< Cached log-search hint for repeated lookups of the same metadata item.

    /// Build the FITS keyword for this metadata item.
    /**
     * \returns the keyword, including the device prefix when HIERARCH-style output is enabled.
     */
    std::string fitsKeyword() const;

  public:
    /// Construct a metadata item from a specification.
    logMeta( const logMetaSpec &lms /**< [in] the specification of this meta data entry */ );

    /// Get the source device name.
    const std::string &device();

    /// Get the FITS keyword for this metadata item.
    const std::string &keyword();

    /// Get the FITS comment for this metadata item.
    const std::string &comment();

    /// Resolve the log accessor and metadata details for a specification.
    int setLog( const logMetaSpec &lms /**< [in] the specification to resolve */ );

    /// Get the metadata value for an exposure interval.
    std::string value( logMap<verboseT>          &lm,    /**< [in,out] loaded logs to search */
                       const flatlogs::timespecX &stime, /**< [in] exposure start time */
                       const flatlogs::timespecX &atime  /**< [in] exposure acquisition/end time */
    );

    /// Get a numeric metadata value for an exposure interval.
    std::string valueNumber( logMap<verboseT>          &lm,    /**< [in,out] loaded logs to search */
                             const flatlogs::timespecX &stime, /**< [in] exposure start time */
                             const flatlogs::timespecX &atime  /**< [in] exposure acquisition/end time */
    );

    /// Get a string metadata value for an exposure interval.
    std::string valueString( logMap<verboseT>          &lm,    /**< [in,out] loaded logs to search */
                             const flatlogs::timespecX &stime, /**< [in] exposure start time */
                             const flatlogs::timespecX &atime  /**< [in] exposure acquisition/end time */
    );

    /// Build a FITS header card for an unavailable metadata value.
    mx::fits::fitsHeaderCard<verboseT> unavailableCard() const;

    /// Build a FITS header card for this metadata item over an exposure interval.
    mx::fits::fitsHeaderCard<verboseT> card( logMap<verboseT>          &lm,    /**< [in,out] loaded logs to search */
                                             const flatlogs::timespecX &stime, /**< [in] exposure start time */
                                             const flatlogs::timespecX &atime /**< [in] exposure acquisition/end time */
    );
};

} // namespace logger
} // namespace MagAOX

#endif // logger_logMeta_hpp
