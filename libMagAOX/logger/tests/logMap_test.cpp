/** \file logMap_test.hpp
 * \brief Tests for the logMap class
 * \ingroup logger_files
 */

#include "../../../tests/testXWC.hpp"

#include <filesystem>
#include <fstream>

#include "../../file/fileTimes.hpp"

#include "../logFileRaw.hpp"
#include "../logMap.hpp"
#include "../logMap.cpp"

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_AFLTFM_XWCE_ns
#define XWCTEST_LOGMAP_AFLTFM_XWCE
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_AFLTFM_XWCE

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_AFLTFM_BADALL_ns
#define XWCTEST_LOGMAP_AFLTFM_BADALL
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_AFLTFM_BADALL

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_AFLTFM_EXCEPTION_ns
#define XWCTEST_LOGMAP_AFLTFM_EXCEPTION
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_AFLTFM_EXCEPTION

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_BADALL1_ns
#define XWCTEST_LOGMAP_LATFM_BADALL1
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_BADALL1

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_BADALL2_ns
#define XWCTEST_LOGMAP_LATFM_BADALL2
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_BADALL2

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_BADALL3_ns
#define XWCTEST_LOGMAP_LATFM_BADALL3
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_BADALL3

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_XWCE4_ns
#define XWCTEST_LOGMAP_LATFM_XWCE4
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_XWCE4

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_XWCE5_ns
#define XWCTEST_LOGMAP_LATFM_XWCE5
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_XWCE5

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_XWCE6_ns
#define XWCTEST_LOGMAP_LATFM_XWCE6
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_XWCE6

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_DIREXISTS_ERRC_ns
#define XWCTEST_LOGMAP_LATFM_DIREXISTS_ERRC
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_DIREXISTS_ERRC

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_SIZEERR1_ns
#define XWCTEST_LOGMAP_LATFM_SIZEERR1
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_SIZEERR1

#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LATFM_SIZEERR2_ns
#define XWCTEST_LOGMAP_LATFM_SIZEERR2
#include "../logMap.hpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGMAP_LATFM_SIZEERR2

// logInMemory::loadFile is defined out-of-line in logMap.cpp, so exercising its own fault
// path (as opposed to logMap<verboseT>'s inline template methods above) needs a namespaced
// re-inclusion of both the header and the source together.
#undef logger_logMap_hpp
#define XWCTEST_NAMESPACE XWCTEST_LOGMAP_LOADFILE_SHORTREAD_ns
#define XWCTEST_LOGINMEMORY_LOADFILE_SHORTREAD
#include "../logMap.hpp"
#include "../logMap.cpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_LOGINMEMORY_LOADFILE_SHORTREAD

namespace libXWCTest
{

namespace loggerTest
{

/** \defgroup logMap_unit_test logMap Unit Tests
 * \ingroup logger_unit_test
 */

/// Namespace for XWC::logger::logMap tests
/** \ingroup logMap_unit_test
 *
 */
namespace logMapTest
{

// simple time struct to enable log structure creation
struct tmpt
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int nanosec;
};

// create a bunch of logs on disk to process
void createTestPaths( const std::string &basedir )
{
    std::filesystem::create_directory( basedir );

    std::vector<std::string> devs( { "dev1", "dev2", "dev3" } );

    std::vector<std::vector<tmpt>> ftimes( { /*dev1:*/ { { 2024, 11, 19, 0, 0, 0, 0 },
                                                         { 2024, 11, 19, 0, 0, 30, 0 },
                                                         { 2024, 11, 19, 2, 55, 26, 4000 },
                                                         { 2024, 11, 19, 5, 23, 0, 0 },
                                                         { 2024, 11, 21, 22, 0, 0, 0 },
                                                         { 2024, 11, 21, 23, 59, 59, 999999999 },
                                                         { 2024, 11, 23, 2, 30, 2, 2000 },
                                                         { 2024, 11, 23, 4, 45, 10, 12 } },
                                             /*dev2:*/
                                             { { 2024, 11, 19, 0, 0, 0, 0 },
                                               { 2024, 11, 19, 0, 0, 30, 0 },
                                               { 2024, 11, 19, 2, 55, 26, 4000 },
                                               { 2024, 11, 19, 5, 23, 0, 0 },
                                               { 2024, 11, 21, 22, 0, 0, 0 },
                                               { 2024, 11, 21, 23, 59, 59, 999999999 },
                                               { 2024, 11, 23, 2, 30, 2, 2000 },
                                               { 2024, 11, 23, 4, 45, 10, 12 } },
                                             /*dev3:*/
                                             { { 2024, 11, 19, 0, 0, 0, 0 },
                                               { 2024, 11, 19, 0, 0, 30, 0 },
                                               { 2024, 11, 19, 2, 55, 26, 4000 },
                                               { 2024, 11, 19, 5, 23, 0, 0 },
                                               { 2024, 11, 21, 22, 0, 0, 0 },
                                               { 2024, 11, 21, 23, 59, 59, 999999999 },
                                               { 2024, 11, 23, 2, 30, 2, 2000 },
                                               { 2024, 11, 23, 4, 45, 10, 12 } } } );

    for( size_t d = 0; d < devs.size(); ++d )
    {
        for( size_t f = 0; f < ftimes[d].size(); ++f )
        {
            tm uttime;
            uttime.tm_year = ftimes[d][f].year - 1900;
            uttime.tm_mon  = ftimes[d][f].month - 1;
            uttime.tm_mday = ftimes[d][f].day;
            uttime.tm_hour = ftimes[d][f].hour;
            uttime.tm_min  = ftimes[d][f].minute;
            uttime.tm_sec  = ftimes[d][f].second;

            time_t secs = timegm( &uttime );

            std::string fileName, relPath;

            MagAOX::file::fileTimeRelPath( fileName, relPath, devs[d], "xlog", secs, ftimes[d][f].nanosec );

            std::filesystem::create_directories( basedir + '/' + relPath );

            std::ofstream fout;
            fout.open( basedir + '/' + relPath + '/' + fileName );
            fout.close();
        }
    }
}

// Two simple log types with distinct event codes, used to build real on-disk log files
// (via logFileRaw) for testing getPriorLog/getNextLog/loadFiles, which need genuine
// flatlog-formatted binary content to scan.
struct dummyLogA
{
    // A real generated event code: the merged logMap only scans entries whose code
    // eventCodeName() recognizes (unknown codes read as corruption and are resynced away).
    static const flatlogs::eventCodeT eventCode   = MagAOX::logger::eventCodes::TEXT_LOG;
    static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;

    typedef std::string messageT;

    static const char *msg()
    {
        return "A";
    }

    static flatlogs::msgLenT length( const messageT &msg )
    {
        return msg.size();
    }

    static int format( void *msgBuffer, const messageT &msg )
    {
        memcpy( msgBuffer, msg.data(), msg.size() );
        return 0;
    }
};

struct dummyLogB
{
    static const flatlogs::eventCodeT eventCode   = MagAOX::logger::eventCodes::USER_LOG; // real code, see dummyLogA

    static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;

    typedef std::string messageT;

    static const char *msg()
    {
        return "B";
    }

    static flatlogs::msgLenT length( const messageT &msg )
    {
        return msg.size();
    }

    static int format( void *msgBuffer, const messageT &msg )
    {
        memcpy( msgBuffer, msg.data(), msg.size() );
        return 0;
    }
};

// Out-of-line definitions, needed because getPriorLog takes eventCode by const reference.
const flatlogs::eventCodeT dummyLogA::eventCode;
const flatlogs::eventCodeT dummyLogB::eventCode;

// A log type that always declares a large message length regardless of what's actually
// written, so its header can be used to fabricate a truncated/corrupt trailing entry (the
// buffer createLog allocates is sized to match the declared length, so writing a small
// message into it is memory-safe -- only the header's declared length is a lie).
struct dummyLogBigDeclared
{
    // A real generated event code (see dummyLogA above) -- the raw literal 50 used to
    // collide with eventCodes::SOFTWARE_LOG, and unrecognized codes now read as corruption.
    static const flatlogs::eventCodeT eventCode   = MagAOX::logger::eventCodes::STATE_CHANGE;
    static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;

    typedef int messageT;

    static flatlogs::msgLenT length( const messageT & )
    {
        return 5000;
    }

    static int format( void *msgBuffer, const messageT &msg )
    {
        memcpy( msgBuffer, &msg, sizeof( msg ) );
        return 0;
    }
};
const flatlogs::eventCodeT dummyLogBigDeclared::eventCode;

// Writes a single-file log with one dummyLogA entry per supplied timestamp, and returns it
// as a stdFileName ready to hand to logInMemory::loadFile directly.
MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY> writeSingleLogFile( const std::string        &dir,
                                                                     const std::string        &dev,
                                                                     const std::vector<time_t> &times )
{
    std::filesystem::remove_all( dir );

    MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> writer;
    writer.logPath( dir );
    writer.logName( dev );
    writer.logExt( "xlog" );
    writer.maxLogSize( 1000000 ); // large enough that all entries land in one file

    flatlogs::bufferPtrT buf;
    for( auto t : times )
    {
        flatlogs::logHeader::createLog<dummyLogA>( buf, flatlogs::timespecX( t, 0 ), dummyLogA::msg(), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    }
    writer.close();

    std::string fileName, relPath;
    MagAOX::file::fileTimeRelPath( fileName, relPath, dev, "xlog", times.front(), 0 );

    MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY> sfn( dir + '/' + relPath + '/' + fileName );
    REQUIRE( sfn.valid() );
    return sfn;
}

/// Building the app-to-file map
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "Building the app-to-file map", "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    SECTION( "File matches middle file and one later on same day" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( lm.m_appToFileMap["dev1"].size() == 2 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119025526000004000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119052300000000000.xlog" );
    }

    SECTION( "File matches first file by delta-t and last file on same day" )
    {
        // This is inside second log, but will pick the first log to get the 60 second buffer
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000061000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119042200000000000.xrif" );

        MagAOX::logger::logMap lm;

        lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( lm.m_appToFileMap["dev1"].size() == 4 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000000000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000030000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119025526000004000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119052300000000000.xlog" );
    }

    SECTION( "File matches first file by delta-t and first file on next day" )
    {
        // This is inside second log, but will pick the first log to get the 60 second buffer
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000061000000000.xrif" );

        // This is inside the 2nd to last log, but since next log is < 3600 seconds we have to go to next day
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119052200000000000.xrif" );

        MagAOX::logger::logMap lm;

        lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( lm.m_appToFileMap["dev1"].size() == 5 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000000000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000030000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119025526000004000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119052300000000000.xlog" );
    }

    SECTION( "File matches last file on previous day and first file on current day (times are the same)" )
    {
        // This will pick the first log of 11/19 to get the 60 second buffer
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241121200030000000000.xrif" );

        // Same time is more than an hour before first log of 11/21
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241121200030000000000.xrif" );

        MagAOX::logger::logMap lm;

        lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( lm.m_appToFileMap["dev1"].size() == 2 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119052300000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_21/dev1_20241121220000000000000.xlog" );
    }

    SECTION( "Matches first and last overall files, last one is not > 1 hr" )
    {
        // this is 50 seconds into 2nd file, so will pick the first file which is > 60 secs
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000120000000000.xrif" );

        // this is 10 minutes before end of last log
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_23/cam1_20241123044500000000000.xrif" );

        MagAOX::logger::logMap lm;

        lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( lm.m_appToFileMap["dev1"].size() == 8 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000000000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000030000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119025526000004000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119052300000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_21/dev1_20241121220000000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_21/dev1_20241121235959999999999.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_23/dev1_20241123023002000002000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_23/dev1_20241123044510000000012.xlog" );
    }

    SECTION( "Missing following date directory uses the previous log directory" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241124030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241124040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::noerror );
        REQUIRE( lm.m_appToFileMap["dev1"].size() == 1 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_23/dev1_20241123044510000000012.xlog" );
    }
}

/// Building the app-to-file map with bad arguments
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "Building the app-to-file map with bad arguments", "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    SECTION( "bad directory permissions" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241118030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/root/adlknalkejr111", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::eacces );
        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }

    SECTION( "directory does not exist" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241118030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/tmp/logMap_testX", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::dirnotfound );
        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }

    SECTION( "firstFile is not valid" )
    {
        MagAOX::file::stdFileName firstFile;
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::invalidconfig );
        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }

    SECTION( "lastFile is not valid" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241118030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile;

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::invalidconfig );
        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }

    SECTION( "wrong device name so no files" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241118030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/tmp/logMap_test", "dev6", ".xlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::noerror );
        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }

    SECTION( "wrong extension so no files" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241118030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t errc = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".qlog", firstFile, lastFile );

        REQUIRE( errc == mx::error_t::noerror );
        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }
}

/// Building the app-to-file map with errors
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "Building the app-to-file map with errors", "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    SECTION( "No prior log" )
    {
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241118030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( lm.m_appToFileMap.size() == 0 );
    }
}

/// addFileListToFileMap's own exception handling
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "addFileListToFileMap nests or reports exceptions", "[libMagAOX::logger::logMap]" )
{
    std::vector<std::string> flist{ "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119000000000000000.xlog" };

    SECTION( "nests an xwcException" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_AFLTFM_XWCE_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        bool caught = false;
        try
        {
            lm.addFileListToFileMap( "dev1", flist, 0, flist.size() );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "nests a std::bad_alloc" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_AFLTFM_BADALL_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        bool caught = false;
        try
        {
            lm.addFileListToFileMap( "dev1", flist, 0, flist.size() );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "reports a std::exception as an error" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_AFLTFM_EXCEPTION_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        mx::error_t rv = lm.addFileListToFileMap( "dev1", flist, 0, flist.size() );

        REQUIRE( rv == mx::error_t::std_exception );
    }

    SECTION( "skips a valid filename belonging to a different app" )
    {
        // "dev10" is a valid stdFileName with a different appName() than "dev1", so it
        // should be skipped rather than added to dev1's file map.
        std::vector<std::string> otherAppFlist{ "/tmp/logMap_test/dev1/2024_11_19/dev10_20241119000000000000000.xlog" };

        MagAOX::logger::logMap lm;

        mx::error_t rv = lm.addFileListToFileMap( "dev1", otherAppFlist, 0, otherAppFlist.size() );

        REQUIRE( rv == mx::error_t::noerror );
        REQUIRE( lm.m_appToFileMap["dev1"].size() == 0 );
    }
}

/// loadAppToFileMap re-throwing exceptions raised while parsing filenames or building file lists
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "loadAppToFileMap nests exceptions raised while searching for files", "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    SECTION( "an exception while parsing filenames during the backward search" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_BADALL1_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        bool caught = false;
        try
        {
            lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "an exception while parsing filenames during the forward search" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_BADALL2_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        bool caught = false;
        try
        {
            lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "an exception building the file list when prev and following logs are on the same day" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_BADALL3_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        bool caught = false;
        try
        {
            lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "an exception building the file list when prev and following logs are on different days" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_XWCE4_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        // This spans 2024_11_19 (prev) to 2024_11_21 (following) -- see "File matches first
        // file by delta-t and first file on next day" above for the non-throwing version.
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000061000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119052200000000000.xrif" );

        bool caught = false;
        try
        {
            lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "an exception building the file list for an intervening day" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_XWCE5_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        // This spans 2024_11_19 (prev) to 2024_11_23 (following), so the intervening-day
        // loop runs for 2024_11_20, 2024_11_21, and 2024_11_22 -- see "Matches first and
        // last overall files" above for the non-throwing version.
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000120000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_23/cam1_20241123044500000000000.xrif" );

        bool caught = false;
        try
        {
            lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }

    SECTION( "an exception building the file list for the following log's own day" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_XWCE6_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000120000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_23/cam1_20241123044500000000000.xrif" );

        bool caught = false;
        try
        {
            lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );
        }
        catch( const MagAOX::xwcException &e )
        {
            caught = true;
        }
        REQUIRE( caught == true );
    }
}

/// loadAppToFileMap's defensive filesystem-error and file-count sanity checks
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "loadAppToFileMap reports filesystem and file-count errors", "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    SECTION( "a filesystem error while checking a day during the forward search" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_DIREXISTS_ERRC_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        mx::error_t rv = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( rv == mx::error_t::eacces );
    }

    SECTION( "a miscounted file list when prev and following logs are on the same day" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_SIZEERR1_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        mx::error_t rv = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( rv == mx::error_t::sizeerr );
    }

    SECTION( "a miscounted file list for the following log's own day" )
    {
        MagAOX::logger::XWCTEST_LOGMAP_LATFM_SIZEERR2_ns::logMap<XWC_DEFAULT_VERBOSITY> lm;

        // Unlike the "different days" cases above, this pair finds the following log
        // normally (forward search lands on 2024_11_21), rather than via the "not found"
        // fallback -- that's required to reach the branch this macro targets.
        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119000061000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119052200000000000.xrif" );

        mx::error_t rv = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( rv == mx::error_t::sizeerr );
    }
}

/// loadAppToFileMap skipping files and subdirectories that don't contribute to the map
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "loadAppToFileMap skips non-standard filenames and empty subdirectories", "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    SECTION( "a non-standard filename in the search directory is ignored" )
    {
        // Both match the getFileNames() prefix/extension filter (start with "dev1", end
        // with ".xlog") but are not valid stdFileNames, so they must be skipped rather than
        // corrupting the file count. "dev1_0000.xlog" sorts before all the real timestamped
        // files, so the forward search (low to high index) hits it first; "dev1_zzzz.xlog"
        // sorts after all of them, so the backward search (high to low index) hits it first.
        std::ofstream junkLow( "/tmp/logMap_test/dev1/2024_11_19/dev1_0000.xlog" );
        junkLow.close();
        std::ofstream junkHigh( "/tmp/logMap_test/dev1/2024_11_19/dev1_zzzz.xlog" );
        junkHigh.close();

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_19/cam1_20241119030000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119040000000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t rv = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( rv == mx::error_t::noerror );
        REQUIRE( lm.m_appToFileMap["dev1"].size() == 2 );
    }

    SECTION( "an existing but empty intervening subdirectory is skipped" )
    {
        // 2024_11_20 has no dev1 files, so both the backward search (from 2024_11_21) and
        // the forward search (from 2024_11_19) must step over it.
        std::filesystem::create_directories( "/tmp/logMap_test/dev1/2024_11_20" );

        MagAOX::file::stdFileName firstFile( "cam1/2024_11_21/cam1_20241121210000000000000.xrif" );
        MagAOX::file::stdFileName lastFile( "cam1/2024_11_19/cam1_20241119052200000000000.xrif" );

        MagAOX::logger::logMap lm;

        mx::error_t rv = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

        REQUIRE( rv == mx::error_t::noerror );

        // firstFile (21:00 on 11_21) is well after the last 11_19 file, so the backward
        // search finds only the last 11_19 file (05:23:00, prevLogFile_n=3, so only that one
        // index gets included from that day). lastFile (05:22 on 11_19) means follts lands
        // after all of 11_19's files, so the forward search finds only the first 11_21 file
        // (22:00:00, follLogFile_n=0, so only that one index gets included from that day).
        REQUIRE( lm.m_appToFileMap["dev1"].size() == 2 );
        auto it = lm.m_appToFileMap["dev1"].begin();
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_19/dev1_20241119052300000000000.xlog" );
        ++it;
        REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_21/dev1_20241121220000000000000.xlog" );
    }
}

/// loadAppToFileMap's fallback when no following log is found within the search span
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "loadAppToFileMap falls back to the last available day when no following log is found",
           "[libMagAOX::logger::logMap]" )
{
    createTestPaths( "/tmp/logMap_test" );

    // firstFile is 61 seconds after the last 2024_11_23 entry (04:45:10), so the backward
    // search finds it immediately and prevLogSubDir is 2024_11_23.
    MagAOX::file::stdFileName firstFile( "cam1/2024_11_23/cam1_20241123044611000000000.xrif" );

    // lastFile is about 3 weeks after the last real data, so the forward search exhausts
    // its span without finding anything, and the fallback search steps backward from
    // 2024_12_15 until it finds an existing directory -- which is 2024_11_23, the same day
    // as prevLogSubDir.
    MagAOX::file::stdFileName lastFile( "cam1/2024_12_15/cam1_20241215000000000000000.xrif" );

    MagAOX::logger::logMap lm;

    mx::error_t rv = lm.loadAppToFileMap( "/tmp/logMap_test", "dev1", ".xlog", firstFile, lastFile );

    REQUIRE( rv == mx::error_t::noerror );

    // Only the last 2024_11_23 file should be included: prevLogFile_n is its own index, and
    // with no following log found, follLogFile_n falls back to tmp_flist.size() (one past
    // the last file in that day's directory).
    REQUIRE( lm.m_appToFileMap["dev1"].size() == 1 );
    auto it = lm.m_appToFileMap["dev1"].begin();
    REQUIRE( it->fullName() == "/tmp/logMap_test/dev1/2024_11_23/dev1_20241123044510000000012.xlog" );
}

/// getPriorLog scanning a loaded in-memory buffer
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "getPriorLog scans a loaded buffer for the last log at or before a timestamp",
           "[libMagAOX::logger::logMap]" )
{
    std::filesystem::remove_all( "/tmp/logMap_test3" );

    MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> writer;
    writer.logPath( "/tmp/logMap_test3" );
    writer.logName( "dev1" );
    writer.logExt( "xlog" );
    writer.maxLogSize( 1000000 ); // large enough that all 5 entries land in one file

    // Five entries, ten seconds apart, alternating event codes A/B/A/A/B.
    const time_t         base = 1732170780;
    flatlogs::bufferPtrT buf;
    flatlogs::logHeader::createLog<dummyLogA>(
        buf, flatlogs::timespecX( base, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogB>(
        buf, flatlogs::timespecX( base + 10, 0 ), "B", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogA>(
        buf, flatlogs::timespecX( base + 20, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogA>(
        buf, flatlogs::timespecX( base + 30, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogB>(
        buf, flatlogs::timespecX( base + 40, 0 ), "B", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    writer.close();

    std::string fileName, relPath;
    MagAOX::file::fileTimeRelPath( fileName, relPath, "dev1", "xlog", base, 0 );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;

    MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY> sfn( "/tmp/logMap_test3/" + relPath + '/' + fileName );
    REQUIRE( sfn.valid() );
    lm.m_appToFileMap["dev1"].insert( sfn );

    SECTION( "no entry at all for the app returns an error" )
    {
        char *logBefore = nullptr; // unused output on error, but needs an lvalue

        int rv = lm.getPriorLog( logBefore, "dev2", dummyLogA::eventCode, flatlogs::timespecX( base + 25, 0 ) );

        REQUIRE( rv == -1 );
    }

    SECTION( "finds the last log at or before the timestamp, with no hint" )
    {
        char *logBefore = nullptr;

        int rv = lm.getPriorLog(
            logBefore, "dev1", dummyLogA::eventCode, flatlogs::timespecX( base + 25, 0 ) );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::eventCode( logBefore ) == dummyLogA::eventCode );
        REQUIRE( flatlogs::logHeader::timespec( logBefore ).time_s == base + 20 );
    }

    SECTION( "a hint at or before the timestamp is used directly" )
    {
        // First, load the buffer and get real pointers into it by walking from the start --
        // this is exactly what getPriorLog/getNextLog do internally. (The merged
        // getPriorLog ignores the hint argument; these sections now document that the
        // result is hint-independent.)
        char *logBefore = nullptr;
        REQUIRE( lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode,
                         flatlogs::timespecX( base + 5, 0 ) ) == 0 ); // base+5: the merged
        // loadFiles only loads when a file starts strictly before the requested time

        char *p0 = lm.m_appToBufferMap["dev1"].m_memory.data(); // entry 0: A, base+0
        char *p1 = p0 + flatlogs::logHeader::totalSize( p0 );   // entry 1: B, base+10

        char *logBefore2 = nullptr;
        int   rv = lm.getPriorLog( logBefore2, "dev1", dummyLogA::eventCode, flatlogs::timespecX( base + 25, 0 ), p1 );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::timespec( logBefore2 ).time_s == base + 20 );
    }

    SECTION( "a hint after the timestamp falls back to the start of the buffer" )
    {
        char *logBefore = nullptr;
        REQUIRE( lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode,
                         flatlogs::timespecX( base + 5, 0 ) ) == 0 ); // base+5: the merged
        // loadFiles only loads when a file starts strictly before the requested time

        char *p0 = lm.m_appToBufferMap["dev1"].m_memory.data(); // entry 0: A, base+0
        char *p1 = p0 + flatlogs::logHeader::totalSize( p0 );   // entry 1: B, base+10
        char *p2 = p1 + flatlogs::logHeader::totalSize( p1 );   // entry 2: A, base+20
        char *p3 = p2 + flatlogs::logHeader::totalSize( p2 );   // entry 3: A, base+30

        char *logBefore2 = nullptr;
        int   rv = lm.getPriorLog( logBefore2, "dev1", dummyLogA::eventCode, flatlogs::timespecX( base + 25, 0 ), p3 );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::timespec( logBefore2 ).time_s == base + 20 );
    }

    SECTION( "an event code that never appears is reported as not found" )
    {
        char *logBefore = nullptr;

        int rv = lm.getPriorLog( logBefore, "dev1", 99, flatlogs::timespecX( base + 25, 0 ) );

        REQUIRE( rv == -1 );
    }

    SECTION( "reaches the end of the buffer scanning past the last matching-code entry" )
    {
        // Request the last entry's own event code (B, at base+40) with a timestamp beyond
        // it. The merged getPriorLog scans the whole buffer and returns the last match
        // (the old implementation returned 1, "need to load more data", here).
        char *logBefore = nullptr;

        int rv = lm.getPriorLog( logBefore, "dev1", dummyLogB::eventCode, flatlogs::timespecX( base + 100, 0 ) );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::eventCode( logBefore ) == dummyLogB::eventCode );
        REQUIRE( flatlogs::logHeader::timespec( logBefore ).time_s == base + 40 );
    }

    SECTION( "reaches the end of the buffer while skipping a mismatched last entry" )
    {
        // Request A with a timestamp beyond everything: the scan passes the mismatched
        // last entry (B) and returns the last A (the old implementation returned 1,
        // "need to load more data", here).
        char *logBefore = nullptr;

        int rv = lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode, flatlogs::timespecX( base + 100, 0 ) );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::timespec( logBefore ).time_s == base + 30 );
    }

    SECTION( "detects a corrupt length field that would overshoot the buffer (requesting the corrupted entry's own code)" )
    {
        char *logBefore = nullptr;

        // Populate the buffer first with a normal, in-range call.
        REQUIRE( lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode,
                         flatlogs::timespecX( base + 5, 0 ) ) == 0 ); // base+5: the merged
        // loadFiles only loads when a file starts strictly before the requested time

        // Walk to the last entry (B, at base+40) and inflate its declared message length so
        // that totalSize() overshoots the real end of m_memory.
        std::vector<char> &mem = lm.m_appToBufferMap["dev1"].m_memory;
        char              *p   = mem.data();
        for( int i = 0; i < 4; ++i )
        {
            p += flatlogs::logHeader::totalSize( p );
        }
        p[flatlogs::logHeader::headerSize( p ) - 1] += 50;

        // Requesting B (the corrupted entry's own code) with a timestamp beyond it: the
        // merged scan detects the bad extent, cannot resync (nothing valid follows), and
        // returns the last good B (the old implementation returned -1 here).
        int rv = lm.getPriorLog( logBefore, "dev1", dummyLogB::eventCode, flatlogs::timespecX( base + 100, 0 ) );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::timespec( logBefore ).time_s == base + 10 );
    }

    SECTION( "detects a corrupt length field that would overshoot the buffer (requesting a different code)" )
    {
        char *logBefore = nullptr;

        REQUIRE( lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode,
                         flatlogs::timespecX( base + 5, 0 ) ) == 0 ); // base+5: the merged
        // loadFiles only loads when a file starts strictly before the requested time

        std::vector<char> &mem = lm.m_appToBufferMap["dev1"].m_memory;
        char              *p   = mem.data();
        for( int i = 0; i < 4; ++i )
        {
            p += flatlogs::logHeader::totalSize( p );
        }
        p[flatlogs::logHeader::headerSize( p ) - 1] += 50;

        // Requesting A with a timestamp beyond everything: the merged scan stops at the
        // corrupted entry and returns the last good A (the old implementation returned
        // -1 here).
        int rv = lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode, flatlogs::timespecX( base + 100, 0 ) );

        REQUIRE( rv == 0 );
        REQUIRE( flatlogs::logHeader::timespec( logBefore ).time_s == base + 30 );
    }
}

/// getNextLog stepping forward to the next log with a matching event code
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "getNextLog steps forward to the next log with the same event code", "[libMagAOX::logger::logMap]" )
{
    std::filesystem::remove_all( "/tmp/logMap_test4" );

    MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> writer;
    writer.logPath( "/tmp/logMap_test4" );
    writer.logName( "dev1" );
    writer.logExt( "xlog" );
    writer.maxLogSize( 1000000 );

    const time_t         base = 1732170780;
    flatlogs::bufferPtrT buf;
    flatlogs::logHeader::createLog<dummyLogA>(
        buf, flatlogs::timespecX( base, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogB>(
        buf, flatlogs::timespecX( base + 10, 0 ), "B", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogA>(
        buf, flatlogs::timespecX( base + 20, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogA>(
        buf, flatlogs::timespecX( base + 30, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    flatlogs::logHeader::createLog<dummyLogB>(
        buf, flatlogs::timespecX( base + 40, 0 ), "B", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );
    writer.close();

    std::string fileName, relPath;
    MagAOX::file::fileTimeRelPath( fileName, relPath, "dev1", "xlog", base, 0 );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;

    MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY> sfn( "/tmp/logMap_test4/" + relPath + '/' + fileName );
    lm.m_appToFileMap["dev1"].insert( sfn );

    // Load the buffer (via getPriorLog, as above), then walk to real pointers for each entry.
    char *logBefore = nullptr;
    REQUIRE( lm.getPriorLog( logBefore, "dev1", dummyLogA::eventCode,
                         flatlogs::timespecX( base + 5, 0 ) ) == 0 ); // base+5: the merged
        // loadFiles only loads when a file starts strictly before the requested time

    char *p0 = lm.m_appToBufferMap["dev1"].m_memory.data(); // entry 0: A, base+0
    char *p1 = p0 + flatlogs::logHeader::totalSize( p0 );   // entry 1: B, base+10
    char *p2 = p1 + flatlogs::logHeader::totalSize( p1 );   // entry 2: A, base+20
    char *p3 = p2 + flatlogs::logHeader::totalSize( p2 );   // entry 3: A, base+30
    char *p4 = p3 + flatlogs::logHeader::totalSize( p3 );   // entry 4: B, base+40

    SECTION( "finds the very next entry when it already matches" )
    {
        char *logAfter = nullptr;
        int   rv       = lm.getNextLog( logAfter, p2, "dev1" ); // p2 is A, p3 (next) is also A

        REQUIRE( rv == 0 );
        REQUIRE( logAfter == p3 );
    }

    SECTION( "skips entries with a different event code to find the next match" )
    {
        char *logAfter = nullptr;
        int   rv       = lm.getNextLog( logAfter, p1, "dev1" ); // p1 is B; next B is p4, skipping p2 (A) and p3 (A)

        REQUIRE( rv == 0 );
        REQUIRE( logAfter == p4 );
    }

    SECTION( "reaches the end of the buffer immediately after the last entry" )
    {
        char *logAfter = nullptr;
        int   rv       = lm.getNextLog( logAfter, p4, "dev1" ); // p4 is the last entry

        REQUIRE( rv == 1 );
    }

    SECTION( "reaches the end of the buffer while skipping mismatched entries" )
    {
        char *logAfter = nullptr;
        int   rv       = lm.getNextLog( logAfter, p3, "dev1" ); // p3 is A; only B (p4) follows, then the buffer ends

        REQUIRE( rv == 1 );
    }
}

/// loadFiles selecting which on-disk files to bring into memory
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "loadFiles selects on-disk files to bring into memory", "[libMagAOX::logger::logMap]" )
{
    std::filesystem::remove_all( "/tmp/logMap_test5" );

    MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> writer;
    writer.logPath( "/tmp/logMap_test5" );
    writer.logName( "dev1" );
    writer.logExt( "xlog" );
    writer.maxLogSize( 1 ); // force every entry into its own file

    const time_t base = 1732170780; // 2024_11_21 06:33:00
    const time_t times[4]{ base, base + 7200, base + 14400, base + 21600 };

    for( auto t : times )
    {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<dummyLogA>( buf, flatlogs::timespecX( t, 0 ), "A", flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    }
    writer.close();

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;

    for( auto t : times )
    {
        std::string fileName, relPath;
        MagAOX::file::fileTimeRelPath( fileName, relPath, "dev1", "xlog", t, 0 );

        MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY> sfn( "/tmp/logMap_test5/" + relPath + '/' + fileName );
        REQUIRE( sfn.valid() );
        lm.m_appToFileMap["dev1"].insert( sfn );
    }

    SECTION( "no files for the app is an error" )
    {
        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> emptyLm;

        int rv = emptyLm.loadFiles( "dev1", flatlogs::timespecX( base, 0 ) );

        REQUIRE( rv == -1 );
    }

    SECTION( "the first load picks the file before the target time and the one after it" )
    {
        int rv = lm.loadFiles( "dev1", flatlogs::timespecX( times[1] + 1800, 0 ) );

        REQUIRE( rv == 0 );
        REQUIRE( lm.m_appToBufferMap["dev1"].m_memory.size() > 0 );
        REQUIRE( lm.m_appToBufferMap["dev1"].m_startTime.time_s == times[1] );
    }

    SECTION( "a second call within the already-loaded range returns immediately" )
    {
        REQUIRE( lm.loadFiles( "dev1", flatlogs::timespecX( times[1] + 1800, 0 ) ) == 0 );

        int rv = lm.loadFiles( "dev1", flatlogs::timespecX( times[1] + 1800, 0 ) );

        REQUIRE( rv == 0 );
    }

    SECTION( "loading an earlier time extends the buffer backward" )
    {
        REQUIRE( lm.loadFiles( "dev1", flatlogs::timespecX( times[2] + 1800, 0 ) ) == 0 );

        int rv = lm.loadFiles( "dev1", flatlogs::timespecX( times[0], 0 ) );

        REQUIRE( rv == 0 );
    }

    SECTION( "loading a later time extends the buffer forward" )
    {
        REQUIRE( lm.loadFiles( "dev1", flatlogs::timespecX( times[0] + 1800, 0 ) ) == 0 );

        int rv = lm.loadFiles( "dev1", flatlogs::timespecX( times[3], 0 ) );

        REQUIRE( rv == 0 );
    }

    SECTION( "extending backward past every known file stops at the oldest one" )
    {
        // Fake an already-loaded buffer whose m_startTime is after every file in the map,
        // so the backward search (walking forward looking for the first file at or after
        // m_startTime) runs off the end of the map instead of finding one.
        lm.m_appToBufferMap["dev1"].m_memory.assign( 1, 0 );
        lm.m_appToBufferMap["dev1"].m_startTime = flatlogs::timespecX( times[3] + 100, 0 );
        lm.m_appToBufferMap["dev1"].m_endTime   = flatlogs::timespecX( times[3] + 100, 0 );

        int rv = lm.loadFiles( "dev1", flatlogs::timespecX( times[0], 0 ) );

        REQUIRE( rv == 0 );
    }

    SECTION( "extending forward past every known file stops at the newest one" )
    {
        // Fake an already-loaded buffer whose m_endTime is before every file in the map, so
        // the first search (walking backward looking for a file at or before m_endTime)
        // runs off the beginning of the map, and the second (walking forward looking for a
        // file at or after the requested time) runs off the end of the map.
        lm.m_appToBufferMap["dev1"].m_memory.assign( 1, 0 );
        lm.m_appToBufferMap["dev1"].m_startTime = flatlogs::timespecX( times[0] - 100, 0 );
        lm.m_appToBufferMap["dev1"].m_endTime   = flatlogs::timespecX( times[0] - 100, 0 );

        int rv = lm.loadFiles( "dev1", flatlogs::timespecX( times[3] + 100, 0 ) );

        REQUIRE( rv == 0 );
    }
}

/// logInMemory::loadFile's own defensive checks, called directly (no logMap involved)
/**
 * \ingroup logMap_unit_test
 */
TEST_CASE( "logInMemory::loadFile detects filesystem and framing errors", "[libMagAOX::logger::logMap]" )
{
    const time_t base = 1732170780;

    SECTION( "a short read (fewer bytes than the file's reported size) is reported as an error" )
    {
        // A genuine short read from a regular file (stat size != bytes actually read) isn't
        // reproducible without a concurrent truncation race, so this uses the namespaced
        // build of logInMemory that forces nrd to 0 after the real read() call.
        auto sfn = writeSingleLogFile( "/tmp/logMap_test_loadfile_shortread", "dev1", { base } );

        MagAOX::logger::XWCTEST_LOGMAP_LOADFILE_SHORTREAD_ns::logInMemory lim;
        int                                                               rv = lim.loadFile( sfn );

        REQUIRE( rv == -1 );
    }

    SECTION( "a truncated trailing entry after otherwise-valid entries is reported as corrupt" )
    {
        auto sfn = writeSingleLogFile( "/tmp/logMap_test_loadfile_corrupt", "dev1", { base, base + 10 } );

        // Append a header claiming a 5000-byte message, but write only the header itself --
        // the file ends long before the message it claims to contain, so walking the buffer
        // by declared entry sizes overshoots the actual file size.
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<dummyLogBigDeclared>(
            buf, flatlogs::timespecX( base + 20, 0 ), 0, flatlogs::logPrio::LOG_NOTICE );
        size_t hsz = flatlogs::logHeader::headerSize( buf.get() );

        std::ofstream fout( sfn.fullName(), std::ios::binary | std::ios::app );
        fout.write( buf.get(), hsz );
        fout.close();

        // The merged loadFile tolerates a truncated trailing entry: it keeps the valid
        // prefix and reports success (the old implementation rejected the whole file
        // with -1 here).
        MagAOX::logger::logInMemory lim;
        int                         rv = lim.loadFile( sfn );

        REQUIRE( rv == 0 );
        REQUIRE( lim.m_endTime.time_s == base + 10 ); // the truncated base+20 entry is dropped
    }

    SECTION( "a file overlapping the start of an already-loaded buffer is rejected" )
    {
        auto sfnA = writeSingleLogFile( "/tmp/logMap_test_loadfile_overlapA", "dev1", { base, base + 10, base + 20 } );
        auto sfnB = writeSingleLogFile( "/tmp/logMap_test_loadfile_overlapB", "dev2", { base - 10, base + 5 } );

        MagAOX::logger::logInMemory lim;
        REQUIRE( lim.loadFile( sfnA ) == 0 );
        REQUIRE( lim.m_startTime == flatlogs::timespecX( base, 0 ) );

        // sfnB starts before lim's start (base-10 < base) and ends at/after it (base+5 >= base).
        int rv = lim.loadFile( sfnB );

        REQUIRE( rv == -1 );
    }

    SECTION( "a file whose range falls inside an already-loaded buffer hits the fallback branch" )
    {
        auto sfnA = writeSingleLogFile( "/tmp/logMap_test_loadfile_middleA", "dev1", { base, base + 20 } );
        auto sfnB = writeSingleLogFile( "/tmp/logMap_test_loadfile_middleB", "dev2", { base + 5, base + 15 } );

        MagAOX::logger::logInMemory lim;
        REQUIRE( lim.loadFile( sfnA ) == 0 );
        REQUIRE( lim.m_startTime == flatlogs::timespecX( base, 0 ) );
        REQUIRE( lim.m_endTime == flatlogs::timespecX( base + 20, 0 ) );

        // sfnB's start (base+5) is neither before lim's start nor after lim's end.
        int rv = lim.loadFile( sfnB );

        REQUIRE( rv == -1 );
    }
}

} // namespace logMapTest
} // namespace loggerTest
} // namespace libXWCTest
