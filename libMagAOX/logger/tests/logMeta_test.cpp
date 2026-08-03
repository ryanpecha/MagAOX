/** \file logMeta_test.cpp
 * \brief Tests for the logMeta class and its free functions
 * \ingroup logger_files
 */

#include "../../../tests/testXWC.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>

#include "../../file/fileTimes.hpp"

#include "../logFileRaw.hpp"
#include "../logMap.hpp"
#include "../logMap.cpp"

#include "../logMeta.hpp"
#include "../generated/logTypes.hpp"
#include "../logMeta.cpp"

namespace libXWCTest
{

namespace loggerTest
{

/** \defgroup logMeta_unit_test logMeta Unit Tests
 * \ingroup logger_unit_test
 */

/// Namespace for XWC::logger::logMeta tests
/** \ingroup logMeta_unit_test
 *
 */
namespace logMetaTest
{

// An int-valued "state variable" log used to exercise getLogStateVal. The merged
// logMeta flatbuffer-verifies every scanned entry against the schema of its event code
// (verifyLogEntry), so raw-int payloads under an invented code are no longer accepted:
// this now wraps a real state_change log and carries the value in its 'to' field.
struct dummyLogInt
{
    static const flatlogs::eventCodeT eventCode;
    static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;

    typedef int messageT;

    static flatlogs::msgLenT length( const messageT &msg )
    {
        MagAOX::logger::state_change::messageT m( 0, msg );
        return MagAOX::logger::state_change::length( m );
    }

    static int format( void *msgBuffer, const messageT &msg )
    {
        MagAOX::logger::state_change::messageT m( 0, msg );
        return MagAOX::logger::state_change::format( msgBuffer, m );
    }
};
const flatlogs::eventCodeT dummyLogInt::eventCode = MagAOX::logger::eventCodes::STATE_CHANGE;

// A second int "state" log, used for the state event code that never changes value.
// Same real-state_change wrapping as dummyLogInt (tests use separate devices/dirs, so
// sharing the STATE_CHANGE event code is fine).
struct dummyLogIntConst
{
    static const flatlogs::eventCodeT eventCode;
    static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;

    typedef int messageT;

    static flatlogs::msgLenT length( const messageT &msg )
    {
        MagAOX::logger::state_change::messageT m( 0, msg );
        return MagAOX::logger::state_change::length( m );
    }

    static int format( void *msgBuffer, const messageT &msg )
    {
        MagAOX::logger::state_change::messageT m( 0, msg );
        return MagAOX::logger::state_change::format( msgBuffer, m );
    }
};
const flatlogs::eventCodeT dummyLogIntConst::eventCode = MagAOX::logger::eventCodes::STATE_CHANGE;

// A double-valued "continuous variable" log used to exercise getLogContVal. Wraps a
// real telem_teldata log (see dummyLogInt for why) and carries the value in 'az'.
struct dummyLogDouble
{
    static const flatlogs::eventCodeT eventCode;
    static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;

    typedef double messageT;

    static flatlogs::msgLenT length( const messageT &msg )
    {
        MagAOX::logger::telem_teldata::messageT m( 1, 1, 1, 0, 0, msg, 45.0, 5.0, 90.0, 0 );
        return MagAOX::logger::telem_teldata::length( m );
    }

    static int format( void *msgBuffer, const messageT &msg )
    {
        MagAOX::logger::telem_teldata::messageT m( 1, 1, 1, 0, 0, msg, 45.0, 5.0, 90.0, 0 );
        return MagAOX::logger::telem_teldata::format( msgBuffer, m );
    }
};
const flatlogs::eventCodeT dummyLogDouble::eventCode = MagAOX::logger::eventCodes::TELEM_TELDATA;

int getIntVal( void *buf )
{
    return MagAOX::logger::GetState_change_fb( buf )->to();
}

double getDoubleVal( void *buf )
{
    return MagAOX::logger::GetTelem_teldata_fb( buf )->az();
}

char getCharVal( void *buf )
{
    char v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

unsigned char getUCharVal( void *buf )
{
    unsigned char v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

short getShortVal( void *buf )
{
    short v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

unsigned short getUShortVal( void *buf )
{
    unsigned short v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

long getLongVal( void *buf )
{
    long v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

unsigned long getULongVal( void *buf )
{
    unsigned long v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

long long getLongLongVal( void *buf )
{
    long long v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

unsigned int getUIntVal( void *buf )
{
    unsigned int v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

unsigned long long getULongLongVal( void *buf )
{
    unsigned long long v;
    memcpy( &v, buf, sizeof( v ) );
    return v;
}

// Exposes logMeta's protected m_detail so a test can drive valueNumber()/valueString() with
// valType/metaType combinations that no real MagAO-X log type uses (verified by grepping
// every types/*.hpp getAccessor()), without needing a substitute logMemberAccessor.
struct logMetaExposed : public MagAOX::logger::logMeta
{
    logMetaExposed( const MagAOX::logger::logMetaSpec &lms ) : MagAOX::logger::logMeta( lms )
    {
    }

    void setDetail( const MagAOX::logger::logMetaDetail &d )
    {
        m_detail = d;
    }
};

// Writes a single-file log containing one entry per (offset-from-base, value) pair, in
// order. Mirrors the on-disk setup used by the logMap tests, but only ever needs one file
// since these tests don't exercise loadFiles.
template <class dummyLogT, typename valT>
void writeLogFile( const std::string                             &dir,
                   const std::string                             &dev,
                   time_t                                          base,
                   const std::vector<std::pair<time_t, valT>> &entries )
{
    std::filesystem::remove_all( dir );

    MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> writer;
    writer.logPath( dir );
    writer.logName( dev );
    writer.logExt( "xlog" );
    writer.maxLogSize( 1000000 ); // large enough that all entries land in one file

    // Lead with a text_log entry 10 s before base: it sets the on-disk file's
    // timestamp, which the merged loadFiles requires to be strictly *before* any
    // queried instant (a file starting exactly at the query time is not loaded).
    // No test in this file queries TEXT_LOG, so it never affects a result.
    flatlogs::bufferPtrT buf;
    flatlogs::logHeader::createLog<MagAOX::logger::text_log>(
        buf, flatlogs::timespecX( base - 10, 0 ), "lead-in", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( buf );

    for( auto &e : entries )
    {
        flatlogs::logHeader::createLog<dummyLogT>(
            buf, flatlogs::timespecX( base + e.first, 0 ), e.second, flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    }
    writer.close();
}

// Inserts the single file written above into lm's file map directly, bypassing directory
// scanning -- the same shortcut the logMap tests use for getPriorLog/getNextLog/loadFiles.
template <class verboseT>
void insertLogFile( MagAOX::logger::logMap<verboseT> &lm, const std::string &dir, const std::string &dev, time_t base )
{
    std::string fileName, relPath;
    // base-10: the file is named for its first entry, the lead-in text_log (see
    // writeLogFile/writeRealLogFile).
    MagAOX::file::fileTimeRelPath( fileName, relPath, dev, "xlog", base - 10, 0 );

    MagAOX::file::stdFileName<verboseT> sfn( dir + '/' + relPath + '/' + fileName );
    REQUIRE( sfn.valid() );
    lm.m_appToFileMap[dev].insert( sfn );
}

/// logMetaSpec and logMetaDetail store the fields they are constructed with
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMetaSpec and logMetaDetail store the supplied fields", "[libMagAOX::logger::logMeta]" )
{
    SECTION( "logMetaSpec default construction" )
    {
        MagAOX::logger::logMetaSpec lms;
        REQUIRE( lms.device == "" );
        REQUIRE( lms.member == "" );
    }

    SECTION( "logMetaSpec full construction" )
    {
        MagAOX::logger::logMetaSpec lms( "dev1", 7, "memb", "KEYW", "%d", "a comment" );
        REQUIRE( lms.device == "dev1" );
        REQUIRE( lms.eventCode == 7 );
        REQUIRE( lms.member == "memb" );
        REQUIRE( lms.keyword == "KEYW" );
        REQUIRE( lms.format == "%d" );
        REQUIRE( lms.comment == "a comment" );
    }

    SECTION( "logMetaSpec minimal construction leaves overrides empty" )
    {
        MagAOX::logger::logMetaSpec lms( "dev2", 8, "memb2" );
        REQUIRE( lms.device == "dev2" );
        REQUIRE( lms.eventCode == 8 );
        REQUIRE( lms.member == "memb2" );
        REQUIRE( lms.keyword == "" );
        REQUIRE( lms.format == "" );
        REQUIRE( lms.comment == "" );
    }

    SECTION( "logMetaDetail default construction" )
    {
        MagAOX::logger::logMetaDetail lmd;
        REQUIRE( lmd.valType == -1 );
        REQUIRE( lmd.metaType == -1 );
        REQUIRE( lmd.accessor == nullptr );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "logMetaDetail(k,c,f,vt,mt,acc)" )
    {
        int acc = 0;
        MagAOX::logger::logMetaDetail lmd( "KEYW", "comment", "%d", 3, 0, &acc );
        REQUIRE( lmd.keyword == "KEYW" );
        REQUIRE( lmd.comment == "comment" );
        REQUIRE( lmd.format == "%d" );
        REQUIRE( lmd.valType == 3 );
        REQUIRE( lmd.metaType == 0 );
        REQUIRE( lmd.accessor == &acc );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "logMetaDetail(k,c,f,vt,mt,acc,h)" )
    {
        int acc = 0;
        MagAOX::logger::logMetaDetail lmd( "KEYW", "comment", "%d", 3, 0, &acc, false );
        REQUIRE( lmd.hierarch == false );
        REQUIRE( lmd.accessor == &acc );
    }

    SECTION( "logMetaDetail(k,c,vt,mt,acc,h) has no format override" )
    {
        int acc = 0;
        MagAOX::logger::logMetaDetail lmd( "KEYW", "comment", 3, 0, &acc, false );
        REQUIRE( lmd.keyword == "KEYW" );
        REQUIRE( lmd.comment == "comment" );
        REQUIRE( lmd.format == "" );
        REQUIRE( lmd.valType == 3 );
        REQUIRE( lmd.metaType == 0 );
        REQUIRE( lmd.accessor == &acc );
        REQUIRE( lmd.hierarch == false );
    }

    SECTION( "logMetaDetail(k,vt,mt,acc)" )
    {
        int acc = 0;
        MagAOX::logger::logMetaDetail lmd( "KEYW", 3, 0, &acc );
        REQUIRE( lmd.keyword == "KEYW" );
        REQUIRE( lmd.comment == "" );
        REQUIRE( lmd.valType == 3 );
        REQUIRE( lmd.metaType == 0 );
        REQUIRE( lmd.accessor == &acc );
        REQUIRE( lmd.hierarch == true );
    }

    SECTION( "logMetaDetail(k,vt,mt,acc,h)" )
    {
        int acc = 0;
        MagAOX::logger::logMetaDetail lmd( "KEYW", 3, 0, &acc, false );
        REQUIRE( lmd.keyword == "KEYW" );
        REQUIRE( lmd.valType == 3 );
        REQUIRE( lmd.metaType == 0 );
        REQUIRE( lmd.accessor == &acc );
        REQUIRE( lmd.hierarch == false );
    }
}

/// getLogStateVal tracking a state variable's value over a requested interval
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "getLogStateVal tracks a state variable's value over a requested interval",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_state";
    const std::string dev  = "devS1";
    const time_t      base = 1732170780;

    // Five entries, ten seconds apart: values 1,1,2,2,3
    writeLogFile<dummyLogInt, int>(
        dir, dev, base, { { 0, 1 }, { 10, 1 }, { 20, 2 }, { 30, 2 }, { 40, 3 } } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    insertLogFile( lm, dir, dev, base );

    SECTION( "detects a state change before atime and reports the new value, updating the hint" )
    {
        char *h  = nullptr;
        int   val;
        int   rv = MagAOX::logger::getLogStateVal(
            val, lm, dev, dummyLogInt::eventCode, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 45, 0 ), getIntVal, &h );

        REQUIRE( rv == 0 );
        REQUIRE( val == 2 );
        REQUIRE( h != nullptr );
    }

    SECTION( "no state change occurs before atime, returns the anchor value, with no hint requested" )
    {
        int val;
        int rv = MagAOX::logger::getLogStateVal(
            val, lm, dev, dummyLogInt::eventCode, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 15, 0 ), getIntVal );

        REQUIRE( rv == 0 );
        REQUIRE( val == 1 );
    }

    SECTION( "no state change occurs before atime, and the hint is still updated on the natural loop exit" )
    {
        char *h  = nullptr;
        int   val;
        int   rv = MagAOX::logger::getLogStateVal(
            val, lm, dev, dummyLogInt::eventCode, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 15, 0 ), getIntVal, &h );

        REQUIRE( rv == 0 );
        REQUIRE( val == 1 );
        REQUIRE( h != nullptr );
    }

    SECTION( "an event code with no entries reports an error from the initial getPriorLog" )
    {
        int val;
        int rv = MagAOX::logger::getLogStateVal(
            val, lm, dev, 9999, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 45, 0 ), getIntVal );

        REQUIRE( rv == -1 );
    }

    SECTION( "an anchor at the very last entry causes the initial getNextLog to fail" )
    {
        // getPriorLog's forward search always lands one entry short of an exact time match
        // (it looks for the last entry strictly before ts), so the only way to make it land
        // on the literal last entry in the buffer is to point a hint directly at it -- the
        // same technique the logMap tests use to get real pointers into the loaded buffer.
        char *tmp = nullptr;
        REQUIRE( lm.getPriorLog( tmp, dev, dummyLogInt::eventCode, flatlogs::timespecX( base, 0 ) ) == 0 );

        char *p0 = lm.m_appToBufferMap[dev].m_memory.data();
        char *p1 = p0 + flatlogs::logHeader::totalSize( p0 );
        char *p2 = p1 + flatlogs::logHeader::totalSize( p1 );
        char *p3 = p2 + flatlogs::logHeader::totalSize( p2 );
        char *p4 = p3 + flatlogs::logHeader::totalSize( p3 ); // the last entry (base+40, val 3)

        char *hint = p4;
        int   val;
        int   rv = MagAOX::logger::getLogStateVal( val,
                                                    lm,
                                                    dev,
                                                    dummyLogInt::eventCode,
                                                    flatlogs::timespecX( base + 40, 0 ),
                                                    flatlogs::timespecX( base + 1000, 0 ),
                                                    getIntVal,
                                                    &hint );

        REQUIRE( rv == -1 );
    }
}

/// getLogStateVal exhausting the buffer while searching for a state change
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "getLogStateVal reports an error when it runs out of entries while searching for a state change",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_state_tail";
    const std::string dev  = "devS2";
    const time_t      base = 1732170780;

    // Five entries, ten seconds apart: values 1,1,2,2,2 -- the last three never change, so
    // continuing to search from the 3rd entry runs off the end of the buffer.
    writeLogFile<dummyLogIntConst, int>(
        dir, dev, base, { { 0, 1 }, { 10, 1 }, { 20, 2 }, { 30, 2 }, { 40, 2 } } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    insertLogFile( lm, dir, dev, base );

    int val;
    // stime is strictly between the 3rd and 4th entries, so the anchor is the 3rd (val 2).
    int rv = MagAOX::logger::getLogStateVal( val,
                                              lm,
                                              dev,
                                              dummyLogIntConst::eventCode,
                                              flatlogs::timespecX( base + 21, 0 ),
                                              flatlogs::timespecX( base + 1000, 0 ),
                                              getIntVal );

    REQUIRE( rv == -1 );
}

/// getLogContVal interpolating a continuous variable between two log entries
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "getLogContVal interpolates a continuous variable between two log entries",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_cont";
    const std::string dev  = "devC1";
    const time_t      base = 1732170780;

    // Three entries, non-uniformly spaced: t+0 -> 0.0, t+10 -> 10.0, t+40 -> 40.0
    writeLogFile<dummyLogDouble, double>( dir, dev, base, { { 0, 0.0 }, { 10, 10.0 }, { 40, 40.0 } } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    insertLogFile( lm, dir, dev, base );

    SECTION( "interpolates linearly between the surrounding entries, using and updating a hint" )
    {
        char  *h  = nullptr;
        double val;
        // midpoint of [base+5, base+25] is base+15, which anchors on the t+10 entry and
        // interpolates toward the t+40 entry.
        int rv = MagAOX::logger::getLogContVal(
            val, lm, dev, dummyLogDouble::eventCode, flatlogs::timespecX( base + 5, 0 ), flatlogs::timespecX( base + 25, 0 ), getDoubleVal, &h );

        REQUIRE( rv == 0 );
        REQUIRE( val == Approx( 15.0 ) );
        REQUIRE( h != nullptr );
    }

    SECTION( "an unknown app reports an error from the initial getPriorLog" )
    {
        double val;
        int    rv = MagAOX::logger::getLogContVal(
            val, lm, "nope", dummyLogDouble::eventCode, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 20, 0 ), getDoubleVal );

        REQUIRE( rv == 1 );
    }

    SECTION( "midpoint lands on the final entry so the following getNextLog fails" )
    {
        // As above, getPriorLog can only land on the literal last entry via a hint pointed
        // directly at it.
        char *tmp = nullptr;
        REQUIRE( lm.getPriorLog( tmp, dev, dummyLogDouble::eventCode, flatlogs::timespecX( base, 0 ) ) == 0 );

        char *p0 = lm.m_appToBufferMap[dev].m_memory.data();
        char *p1 = p0 + flatlogs::logHeader::totalSize( p0 );
        char *p2 = p1 + flatlogs::logHeader::totalSize( p1 ); // the last entry (base+40, val 40.0)

        char  *hint = p2;
        double val;
        int    rv = MagAOX::logger::getLogContVal( val,
                                                  lm,
                                                  dev,
                                                  dummyLogDouble::eventCode,
                                                  flatlogs::timespecX( base + 40, 0 ),
                                                  flatlogs::timespecX( base + 40, 0 ),
                                                  getDoubleVal,
                                                  &hint );

        REQUIRE( rv == 1 );
    }
}

// Writes a single-file log for a real (non-dummy) flatlog type. `writeEntries` is handed the
// writer and is responsible for calling logHeader::createLog<Type>(...)/writer.writeLog(...)
// for each entry it wants; `firstTime` must match the timestamp of the first entry it writes,
// since that determines the on-disk filename.
MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY>
writeRealLogFile( const std::string                                              &dir,
                  const std::string                                              &dev,
                  time_t                                                          firstTime,
                  const std::function<void( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> & )> &writeEntries )
{
    std::filesystem::remove_all( dir );

    MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> writer;
    writer.logPath( dir );
    writer.logName( dev );
    writer.logExt( "xlog" );
    writer.maxLogSize( 1000000 );

    // Same lead-in as writeLogFile: gives the file a timestamp strictly before
    // firstTime, which the merged loadFiles requires.
    flatlogs::bufferPtrT leadBuf;
    flatlogs::logHeader::createLog<MagAOX::logger::text_log>(
        leadBuf, flatlogs::timespecX( firstTime - 10, 0 ), "lead-in", flatlogs::logPrio::LOG_NOTICE );
    writer.writeLog( leadBuf );

    writeEntries( writer );

    writer.close();

    std::string fileName, relPath;
    MagAOX::file::fileTimeRelPath( fileName, relPath, dev, "xlog", firstTime - 10, 0 );

    MagAOX::file::stdFileName<XWC_DEFAULT_VERBOSITY> sfn( dir + '/' + relPath + '/' + fileName );
    REQUIRE( sfn.valid() );
    return sfn;
}

/// logMeta dispatches through the real generated log-type accessors
/**
 * These exercise logMeta.cpp's constructor, setLog(), value(), valueNumber(), valueString(),
 * and card() against genuine on-disk flatlog entries for real MagAO-X log types, since
 * logMeta::setLog() always calls the real (generated) logMemberAccessor() -- there is no way
 * to substitute a fake accessor without fabricating a real log type's binary format.
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reads String/State and Bool/Continuous members from a real git_state log",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_gitstate";
    const std::string dev  = "devGit";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::git_state>(
            buf, flatlogs::timespecX( base, 0 ), MagAOX::logger::git_state::messageT( "repoA", "sha1A", false ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::git_state>(
            buf, flatlogs::timespecX( base + 10, 0 ), MagAOX::logger::git_state::messageT( "repoB", "sha1B", true ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );

    SECTION( "String/State: repoName, with explicit keyword/format/comment overrides" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::git_state::eventCode, "repoName", "REPO", "%s", "custom comment" );
        MagAOX::logger::logMeta     lmeta( lms );

        REQUIRE( lmeta.device() == dev );
        REQUIRE( lmeta.keyword() == "REPO" );
        REQUIRE( lmeta.comment() == "custom comment" );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "repoA" );

        auto card = lmeta.card( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( card.keyword() == "REPO" ); // hierarch is false for repoName, so no device prefix/padding
        REQUIRE( card.comment() == "custom comment" );
    }

    SECTION( "Bool/Continuous: modified, with default keyword/comment from the accessor" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::git_state::eventCode, "modified" );
        MagAOX::logger::logMeta     lmeta( lms );

        REQUIRE( lmeta.keyword() == "GIT REPO MODIFIED" );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) );
        REQUIRE( val != "invalid" );
    }

    SECTION( "an unrecognized member leaves the accessor null, so value() returns empty" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::git_state::eventCode, "notAMember" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "NOT AVAILABLE" ); // the merged logMeta's unavailableValue() sentinel (was "")
    }

    SECTION( "a failed lookup (unknown device) produces the invalid-value sentinel via card()" )
    {
        MagAOX::logger::logMetaSpec lms( "no-such-device", MagAOX::logger::git_state::eventCode, "repoName" );
        MagAOX::logger::logMeta     lmeta( lms );

        auto card = lmeta.card( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( card.valueStr() == "NOT AVAILABLE" ); // merged unavailableValue() sentinel (was "invalid")
    }
}

/// logMeta reads Bool/State, Float/State, Vector_Bool/State, and Vector_Float/State from
/// telem_dmspeck, and also exercises card()'s hierarch-true keyword-padding branches.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reads Bool/Float/Vector members from a real telem_dmspeck log", "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_dmspeck";
    const std::string dev  = "devDmspeck";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_dmspeck>(
            buf,
            flatlogs::timespecX( base, 0 ),
            MagAOX::logger::telem_dmspeck::messageT(
                true, false, 12.5f, std::vector<float>{ 1.5f, 2.5f }, std::vector<float>{ 0.1f, 0.2f }, std::vector<float>{ 3.0f, 4.0f }, std::vector<bool>{ true, false } ),
            flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::telem_dmspeck>(
            buf,
            flatlogs::timespecX( base + 10, 0 ),
            MagAOX::logger::telem_dmspeck::messageT(
                true, false, 12.5f, std::vector<float>{ 1.5f, 2.5f }, std::vector<float>{ 0.1f, 0.2f }, std::vector<float>{ 3.0f, 4.0f }, std::vector<bool>{ true, false } ),
            flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );
    // logMap keys files purely by the appName string, independent of what's inside the log
    // content, so the same file can be registered under extra aliases purely to let each
    // section pick a device-name length that exercises card()'s padding logic.
    lm.m_appToFileMap["cameraOne"].insert( sfn );
    lm.m_appToFileMap["d"].insert( sfn );

    SECTION( "Bool/State: modulating, with a long device+keyword so card() skips padding" )
    {
        MagAOX::logger::logMetaSpec lms( "cameraOne", MagAOX::logger::telem_dmspeck::eventCode, "modulating" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "1" );

        auto card = lmeta.card( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( card.keyword() == "cameraOne MODULATING" ); // already >= 9 chars, no padding needed
    }

    SECTION( "Float/State: frequency, with a short device+keyword override so card() pads" )
    {
        MagAOX::logger::logMetaSpec lms( "d", MagAOX::logger::telem_dmspeck::eventCode, "frequency", "F", "", "" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val != "invalid" );

        auto card = lmeta.card( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( card.keyword().size() >= 9 ); // "d F" (3 chars) padded out to at least 9
    }

    SECTION( "Vector_Bool/State: crosses" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_dmspeck::eventCode, "crosses" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "1,0" );
    }

    SECTION( "Vector_Float/State: separations" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_dmspeck::eventCode, "separations" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val != "invalid" );
        REQUIRE( val.find( ',' ) != std::string::npos );
    }
}

/// logMeta reads Int/State and Float/Continuous from a real telem_psfacq log.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reads Int/State and Float/Continuous members from a real telem_psfacq log",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_psfacq";
    const std::string dev  = "devPsfacq";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_psfacq>(
            buf, flatlogs::timespecX( base, 0 ), MagAOX::logger::telem_psfacq::messageT( 1, 3, 10.0f, 20.0f, 100.0f, 2.5f, 0.8f ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::telem_psfacq>(
            buf, flatlogs::timespecX( base + 10, 0 ), MagAOX::logger::telem_psfacq::messageT( 1, 3, 30.0f, 20.0f, 100.0f, 2.5f, 0.8f ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );

    SECTION( "Int/State: star_no" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_psfacq::eventCode, "star_no" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "1" );
    }

    SECTION( "Float/Continuous: x_pos interpolates between the two entries" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_psfacq::eventCode, "x_pos" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) );
        REQUIRE( val != "invalid" );
        REQUIRE( std::stof( val ) > 10.0f );
        REQUIRE( std::stof( val ) < 30.0f );
    }
}

/// logMeta reads UInt/State and Double/State from a real telem_saving log.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reads UInt/State and Double/State members from a real telem_saving log",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_saving";
    const std::string dev  = "devSaving";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_saving>(
            buf, flatlogs::timespecX( base, 0 ), MagAOX::logger::telem_saving::messageT( 1000, 500, 1.5f, 0.5f, 0.2f, 0.3f ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::telem_saving>(
            buf, flatlogs::timespecX( base + 10, 0 ), MagAOX::logger::telem_saving::messageT( 1000, 500, 1.5f, 0.5f, 0.2f, 0.3f ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );

    SECTION( "UInt/State: raw_size" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_saving::eventCode, "raw_size" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "1000" );
    }

    SECTION( "Double/State: encode_sate" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_saving::eventCode, "encode_sate" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val != "invalid" );
    }
}

/// logMeta reads Double/Continuous from a real telem_teldata log.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reads a Double/Continuous member from a real telem_teldata log", "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_teldata";
    const std::string dev  = "devTeldata";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_teldata>(
            buf, flatlogs::timespecX( base, 0 ), MagAOX::logger::telem_teldata::messageT( 1, 1, 1, 0, 0, 10.0, 45.0, 5.0, 90.0, 0 ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::telem_teldata>(
            buf, flatlogs::timespecX( base + 10, 0 ), MagAOX::logger::telem_teldata::messageT( 1, 1, 1, 0, 0, 20.0, 45.0, 5.0, 90.0, 0 ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );

    MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_teldata::eventCode, "az" );
    MagAOX::logger::logMeta     lmeta( lms );

    std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) );
    REQUIRE( val != "invalid" );
    REQUIRE( std::stod( val ) > 10.0 );
    REQUIRE( std::stod( val ) < 20.0 );
}

/// logMeta reads ULongLong/State from a real telem_pokeloop log, both with the default
/// (unrecognized-type) format fallback in setLog(), and with an explicit format override so
/// valueNumber() can safely format it.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reads a ULongLong/State member from a real telem_pokeloop log", "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_pokeloop";
    const std::string dev  = "devPokeloop";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT buf;
        flatlogs::logHeader::createLog<MagAOX::logger::telem_pokeloop>(
            buf, flatlogs::timespecX( base, 0 ), MagAOX::logger::telem_pokeloop::messageT( 1, 0.5f, 0.5f, 12345ULL ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::telem_pokeloop>(
            buf, flatlogs::timespecX( base + 10, 0 ), MagAOX::logger::telem_pokeloop::messageT( 1, 0.5f, 0.5f, 12345ULL ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );

    SECTION( "no format override: setLog() falls through to its unrecognized-type default" )
    {
        // ULongLong isn't one of setLog()'s explicit format cases, so this exercises that
        // default branch. A mismatched printf format for a 64-bit value is unsafe to actually
        // format, so this only checks setLog()'s own bookkeeping, not value()/card().
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_pokeloop::eventCode, "counter" );
        MagAOX::logger::logMeta     lmeta( lms );

        REQUIRE( lmeta.keyword() == "LOOP COUNTER" );
    }

    SECTION( "with an explicit format override, valueNumber()'s ULongLong case runs safely" )
    {
        MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_pokeloop::eventCode, "counter", "", "%llu", "" );
        MagAOX::logger::logMeta     lmeta( lms );

        std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) );
        REQUIRE( val == "12345" );
    }
}

/// logMeta falls back to the invalid-value sentinel when a real accessor's valType isn't
/// handled for its metaType -- here, Vector_Float has no Continuous case in valueNumber(),
/// which also covers card()'s "got invalid value" branch.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta falls back to the invalid sentinel for a valType unhandled by its metaType",
           "[libMagAOX::logger::logMeta]" )
{
    const std::string dir  = "/tmp/logMeta_test_dmmodes";
    const std::string dev  = "devDmmodes";
    const time_t      base = 1732170780;

    auto sfn = writeRealLogFile( dir, dev, base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
        flatlogs::bufferPtrT      buf;
        std::vector<float>        amps{ 1.0f, 2.0f };
        flatlogs::logHeader::createLog<MagAOX::logger::telem_dmmodes>(
            buf, flatlogs::timespecX( base, 0 ), MagAOX::logger::telem_dmmodes::messageT( amps ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
        flatlogs::logHeader::createLog<MagAOX::logger::telem_dmmodes>(
            buf, flatlogs::timespecX( base + 10, 0 ), MagAOX::logger::telem_dmmodes::messageT( amps ), flatlogs::logPrio::LOG_NOTICE );
        writer.writeLog( buf );
    } );

    MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
    lm.m_appToFileMap[dev].insert( sfn );

    MagAOX::logger::logMetaSpec lms( dev, MagAOX::logger::telem_dmmodes::eventCode, "amps" );
    MagAOX::logger::logMeta     lmeta( lms );

    std::string val = lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) );
    REQUIRE( val == "NOT AVAILABLE" ); // merged unavailableValue() sentinel (was "invalid")

    auto card = lmeta.card( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) );
    REQUIRE( card.valueStr() == "NOT AVAILABLE" );
}

// Raw (non-flatbuffer) dummy log types, one per C++ type needed to reach a valType/metaType
// combination that no real MagAO-X log type uses (verified by grepping every types/*.hpp
// getAccessor()). Event codes are reserved (60001+), well above any real event code.
#define DEFINE_DUMMY_RAW_LOG( NAME, TYPE, EVCODE )                                                                    \
    struct NAME                                                                                                       \
    {                                                                                                                 \
        static const flatlogs::eventCodeT eventCode;                                                                  \
        static const flatlogs::logPrioT   defaultLevel = flatlogs::logPrio::LOG_NOTICE;                               \
        typedef TYPE                      messageT;                                                                   \
        static flatlogs::msgLenT          length( const messageT & )                                                  \
        {                                                                                                             \
            return sizeof( messageT );                                                                               \
        }                                                                                                             \
        static int format( void *msgBuffer, const messageT &msg )                                                     \
        {                                                                                                             \
            memcpy( msgBuffer, &msg, sizeof( msg ) );                                                                 \
            return 0;                                                                                                 \
        }                                                                                                             \
    };                                                                                                                \
    const flatlogs::eventCodeT NAME::eventCode = EVCODE;

DEFINE_DUMMY_RAW_LOG( dummyLogChar, char, 60001 )
DEFINE_DUMMY_RAW_LOG( dummyLogUChar, unsigned char, 60002 )
DEFINE_DUMMY_RAW_LOG( dummyLogShort, short, 60003 )
DEFINE_DUMMY_RAW_LOG( dummyLogUShort, unsigned short, 60004 )
DEFINE_DUMMY_RAW_LOG( dummyLogLong, long, 60005 )
DEFINE_DUMMY_RAW_LOG( dummyLogULong, unsigned long, 60006 )
DEFINE_DUMMY_RAW_LOG( dummyLogLongLong, long long, 60007 )
DEFINE_DUMMY_RAW_LOG( dummyLogIntCont, int, 60008 )
DEFINE_DUMMY_RAW_LOG( dummyLogUIntCont, unsigned int, 60009 )
DEFINE_DUMMY_RAW_LOG( dummyLogULongLongCont, unsigned long long, 60010 )

#undef DEFINE_DUMMY_RAW_LOG

/// logMeta's handling of valType/metaType combinations no real log type uses, driven by
/// overwriting the protected m_detail (via logMetaExposed) with a hand-built accessor.
/// The merged logMeta flatbuffer-verifies every entry it reads from a log file
/// (verifyLogEntry), and these sections' raw payloads under invented event codes cannot
/// pass that gate -- so the file-driven lookups now uniformly yield the
/// "NOT AVAILABLE" sentinel instead of reaching the per-valType read branches the old
/// implementation exposed. The sections document that gate; whether the now-unreachable
/// branches need exclusions is for the next coverage pass over the merged code.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "logMeta reaches valType/metaType branches unreachable via any real log type",
           "[libMagAOX::logger::logMeta]" )
{
    using MagAOX::logger::logMeta;
    const time_t base = 1732170780;

    SECTION( "Char/State and Char/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_char", "devChar", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogChar>( buf, flatlogs::timespecX( base, 0 ), (char)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogChar>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (char)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devChar"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devChar", dummyLogChar::eventCode, "state" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::Char, logMeta::metaTypes::State, reinterpret_cast<void *>( &getCharVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devChar", dummyLogChar::eventCode, "cont", "", "%d", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::Char, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getCharVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );

        // Char/State has its own verbose (braced) error-reporting form, unlike the other
        // scalar cases. A stime past the buffer's last entry makes the internal getPriorLog()
        // walk run off the end (as in the logMap.hpp getPriorLog tests), so getLogStateVal()
        // fails and this specific case's error branch runs.
        MagAOX::logger::logMetaSpec lmsFail( "devChar", dummyLogChar::eventCode, "state" );
        logMetaExposed               lmetaFail( lmsFail );
        lmetaFail.setDetail( { "", logMeta::valTypes::Char, logMeta::metaTypes::State, reinterpret_cast<void *>( &getCharVal ) } );
        REQUIRE( lmetaFail.value( lm, flatlogs::timespecX( base + 11, 0 ), flatlogs::timespecX( base + 1000, 0 ) ) ==
                 "NOT AVAILABLE" ); // merged unavailableValue() sentinel (was "invalid")
    }

    SECTION( "UChar/State and UChar/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_uchar", "devUChar", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogUChar>(
                    buf, flatlogs::timespecX( base, 0 ), (unsigned char)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogUChar>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (unsigned char)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devUChar"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devUChar", dummyLogUChar::eventCode, "state" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::UChar, logMeta::metaTypes::State, reinterpret_cast<void *>( &getUCharVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devUChar", dummyLogUChar::eventCode, "cont", "", "%u", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::UChar, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getUCharVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "Short/State and Short/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_short", "devShort", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogShort>(
                    buf, flatlogs::timespecX( base, 0 ), (short)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogShort>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (short)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devShort"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devShort", dummyLogShort::eventCode, "state" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::Short, logMeta::metaTypes::State, reinterpret_cast<void *>( &getShortVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devShort", dummyLogShort::eventCode, "cont", "", "%d", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::Short, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getShortVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "UShort/State and UShort/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_ushort", "devUShort", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogUShort>(
                    buf, flatlogs::timespecX( base, 0 ), (unsigned short)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogUShort>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (unsigned short)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devUShort"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devUShort", dummyLogUShort::eventCode, "state" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::UShort, logMeta::metaTypes::State, reinterpret_cast<void *>( &getUShortVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devUShort", dummyLogUShort::eventCode, "cont", "", "%u", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::UShort, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getUShortVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "Long/State and Long/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_long", "devLong", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogLong>(
                    buf, flatlogs::timespecX( base, 0 ), (long)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogLong>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (long)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devLong"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devLong", dummyLogLong::eventCode, "state" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::Long, logMeta::metaTypes::State, reinterpret_cast<void *>( &getLongVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devLong", dummyLogLong::eventCode, "cont", "", "%ld", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::Long, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getLongVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "ULong/State and ULong/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_ulong", "devULong", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogULong>(
                    buf, flatlogs::timespecX( base, 0 ), (unsigned long)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogULong>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (unsigned long)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devULong"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devULong", dummyLogULong::eventCode, "state" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::ULong, logMeta::metaTypes::State, reinterpret_cast<void *>( &getULongVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devULong", dummyLogULong::eventCode, "cont", "", "%lu", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::ULong, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getULongVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "LongLong/State and LongLong/Continuous" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_longlong", "devLongLong", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogLongLong>(
                    buf, flatlogs::timespecX( base, 0 ), (long long)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogLongLong>(
                    buf, flatlogs::timespecX( base + 10, 0 ), (long long)5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devLongLong"].insert( sfn );

        MagAOX::logger::logMetaSpec lmsState( "devLongLong", dummyLogLongLong::eventCode, "state", "", "%lld", "" );
        logMetaExposed               lmetaState( lmsState );
        lmetaState.setDetail( { "", logMeta::valTypes::LongLong, logMeta::metaTypes::State, reinterpret_cast<void *>( &getLongLongVal ) } );
        REQUIRE( lmetaState.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 5, 0 ) ) ==
                 "NOT AVAILABLE" ); // raw payload rejected by the merged verifyLogEntry gate (see the test case doc)

        MagAOX::logger::logMetaSpec lmsCont( "devLongLong", dummyLogLongLong::eventCode, "cont", "", "%lld", "" );
        logMetaExposed               lmetaCont( lmsCont );
        lmetaCont.setDetail( { "", logMeta::valTypes::LongLong, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getLongLongVal ) } );
        REQUIRE( lmetaCont.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "Int/Continuous (State is already covered by real telem_psfacq/telem_teldata)" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_intcont", "devIntCont", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT buf;
                flatlogs::logHeader::createLog<dummyLogIntCont>( buf, flatlogs::timespecX( base, 0 ), 5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogIntCont>(
                    buf, flatlogs::timespecX( base + 10, 0 ), 5, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devIntCont"].insert( sfn );

        MagAOX::logger::logMetaSpec lms( "devIntCont", dummyLogIntCont::eventCode, "cont", "", "%d", "" );
        logMetaExposed               lmeta( lms );
        lmeta.setDetail( { "", logMeta::valTypes::Int, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getIntVal ) } );
        REQUIRE( lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "UInt/Continuous (State is already covered by real telem_saving)" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_uintcont", "devUIntCont", base, [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT      buf;
                unsigned int               v = 5;
                flatlogs::logHeader::createLog<dummyLogUIntCont>( buf, flatlogs::timespecX( base, 0 ), v, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogUIntCont>(
                    buf, flatlogs::timespecX( base + 10, 0 ), v, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devUIntCont"].insert( sfn );

        MagAOX::logger::logMetaSpec lms( "devUIntCont", dummyLogUIntCont::eventCode, "cont", "", "%u", "" );
        logMetaExposed               lmeta( lms );
        lmeta.setDetail( { "", logMeta::valTypes::UInt, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getUIntVal ) } );
        REQUIRE( lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "ULongLong/Continuous (State is already covered by real telem_pokeloop)" )
    {
        auto sfn = writeRealLogFile(
            "/tmp/logMeta_test_dead_ulonglongcont",
            "devULongLongCont",
            base,
            [&]( MagAOX::logger::logFileRaw<XWC_DEFAULT_VERBOSITY> &writer ) {
                flatlogs::bufferPtrT      buf;
                unsigned long long         v = 5;
                flatlogs::logHeader::createLog<dummyLogULongLongCont>(
                    buf, flatlogs::timespecX( base, 0 ), v, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
                flatlogs::logHeader::createLog<dummyLogULongLongCont>(
                    buf, flatlogs::timespecX( base + 10, 0 ), v, flatlogs::logPrio::LOG_NOTICE );
                writer.writeLog( buf );
            } );

        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;
        lm.m_appToFileMap["devULongLongCont"].insert( sfn );

        MagAOX::logger::logMetaSpec lms( "devULongLongCont", dummyLogULongLongCont::eventCode, "cont", "", "%llu", "" );
        logMetaExposed               lmeta( lms );
        lmeta.setDetail( { "", logMeta::valTypes::ULongLong, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getULongLongVal ) } );
        REQUIRE( lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) != "invalid" );
    }

    SECTION( "String + Continuous: valueString()'s not-state fallback returns empty" )
    {
        // valueString() only touches the logMap when metaType==State, so an empty map is fine.
        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::logger::logMetaSpec lms( "devAny", 60011, "cont" );
        logMetaExposed               lmeta( lms );
        lmeta.setDetail( { "", logMeta::valTypes::String, logMeta::metaTypes::Continuous, reinterpret_cast<void *>( &getIntVal ) } );
        REQUIRE( lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) == "" );
    }

    SECTION( "an out-of-range metaType falls through valueNumber()'s final return" )
    {
        // valueNumber() only touches the logMap inside the State/Continuous branches, neither
        // of which matches metaType==99, so an empty map is fine.
        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::logger::logMetaSpec lms( "devAny", 60012, "bad" );
        logMetaExposed               lmeta( lms );
        lmeta.setDetail( { "", logMeta::valTypes::Int, 99, reinterpret_cast<void *>( &getIntVal ) } );
        REQUIRE( lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) ==
                 "NOT AVAILABLE" ); // merged unavailableValue() sentinel (was "invalid")
    }

    SECTION( "a valType unhandled by the State switch itself hits its own internal default" )
    {
        // Vector_String isn't a case in valueNumber()'s State switch (only Vector_Bool and
        // Vector_Float are), so this reaches that switch's own default rather than the
        // final fallback after the State/Continuous if-else (covered by the badmeta case).
        MagAOX::logger::logMap<XWC_DEFAULT_VERBOSITY> lm;

        MagAOX::logger::logMetaSpec lms( "devAny", 60013, "whatever" );
        logMetaExposed               lmeta( lms );
        lmeta.setDetail( { "", logMeta::valTypes::Vector_String, logMeta::metaTypes::State, reinterpret_cast<void *>( &getIntVal ) } );
        REQUIRE( lmeta.value( lm, flatlogs::timespecX( base, 0 ), flatlogs::timespecX( base + 10, 0 ) ) ==
                 "NOT AVAILABLE" ); // merged unavailableValue() sentinel (was "invalid")
    }

    SECTION( "an unrecognized event code hits the real logMemberAccessor's own default" )
    {
        MagAOX::logger::logMetaSpec lms( "devAny", 60014, "whatever" );
        MagAOX::logger::logMeta     lmeta( lms );
        REQUIRE( lmeta.keyword() == "" );
    }
}


/// Validate shortest-path angle deltas across wrap boundaries.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "Log metadata angle deltas wrap correctly", "[libMagAOX::logger::logMeta]" )
{
    using MagAOX::logger::logMetaAngleDelta;

    REQUIRE( logMetaAngleDelta( 10.0, 20.0 ) == Approx( 10.0 ) );
    REQUIRE( logMetaAngleDelta( 20.0, 10.0 ) == Approx( -10.0 ) );
    REQUIRE( logMetaAngleDelta( -179.0, 179.0 ) == Approx( -2.0 ) );
    REQUIRE( logMetaAngleDelta( 179.0, -179.0 ) == Approx( 2.0 ) );
    REQUIRE( logMetaAngleDelta( 359.0, 1.0 ) == Approx( 2.0 ) );
    REQUIRE( logMetaAngleDelta( 1.0, 359.0 ) == Approx( -2.0 ) );
}

/// Validate interpolated angle normalization across wrap boundaries.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "Log metadata angle interpolation normalizes wrapped midpoints", "[libMagAOX::logger::logMeta]" )
{
    using MagAOX::logger::logMetaAngleDelta;
    using MagAOX::logger::logMetaNormalizeInterpolatedAngle;

    auto midpoint = []( double a0, double a1 )
    {
        double interp = a0 + 0.5 * logMetaAngleDelta( a0, a1 );
        return logMetaNormalizeInterpolatedAngle( interp, a0, a1 );
    };

    REQUIRE( midpoint( -179.0, 179.0 ) == Approx( -180.0 ) );
    REQUIRE( midpoint( 179.0, -179.0 ) == Approx( -180.0 ) );
    REQUIRE( midpoint( 359.0, 1.0 ) == Approx( 0.0 ) );
    REQUIRE( midpoint( 1.0, 359.0 ) == Approx( 0.0 ) );
    REQUIRE( midpoint( 10.0, 20.0 ) == Approx( 15.0 ) );
    REQUIRE( midpoint( 20.0, 10.0 ) == Approx( 15.0 ) );
}

/// Validate angle normalization edge cases.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "Log metadata angle normalization handles exact and repeated wraps", "[libMagAOX::logger::logMeta]" )
{
    using MagAOX::logger::logMetaNormalizeAngle180;
    using MagAOX::logger::logMetaNormalizeAngle360;

    REQUIRE( logMetaNormalizeAngle360( 0.0 ) == Approx( 0.0 ) );
    REQUIRE( logMetaNormalizeAngle360( 360.0 ) == Approx( 0.0 ) );
    REQUIRE( logMetaNormalizeAngle360( 720.0 ) == Approx( 0.0 ) );
    REQUIRE( logMetaNormalizeAngle360( -1.0 ) == Approx( 359.0 ) );

    REQUIRE( logMetaNormalizeAngle180( 180.0 ) == Approx( -180.0 ) );
    REQUIRE( logMetaNormalizeAngle180( 540.0 ) == Approx( -180.0 ) );
    REQUIRE( logMetaNormalizeAngle180( -181.0 ) == Approx( 179.0 ) );
}

/// Validate that parallactic angle metadata uses wrapped interpolation.
/**
 * \ingroup logMeta_unit_test
 */
TEST_CASE( "telem_teldata parallactic angle accessor is continuous angle metadata", "[libMagAOX::logger::logMeta]" )
{
    MagAOX::logger::logMetaDetail lmd = MagAOX::logger::telem_teldata::getAccessor( "pa" );

    REQUIRE( lmd.keyword == "PARANG" );
    REQUIRE( lmd.valType == MagAOX::logger::logMeta::valTypes::Double );
    REQUIRE( lmd.metaType == MagAOX::logger::logMeta::metaTypes::Continuous_Angle );
}

} // namespace logMetaTest
} // namespace loggerTest
} // namespace libXWCTest
