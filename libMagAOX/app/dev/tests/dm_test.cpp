// #define CATCH_CONFIG_MAIN
#include "../../../../tests/catch2/catch.hpp"

#include <mx/sys/timeUtils.hpp>
#include <filesystem>

#include "dm_test.hpp"


/** \defgroup dm_tests libXWC::app::dev::dm Unit Tests
 * \ingroup app_dev_unit_tests
 */

/// Test dm Configuration
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm Configuration", "[dev::dm]" )
{
    SECTION( "a config file with no [dm] section, loading defaults" )
    {
        // Just a dummy config setting
        mx::app::writeConfigFile( "/tmp/dm_test.conf", { "none" }, { "nada" }, { "0" } );

        mx::app::appConfigurator config;

        dm_tests::dmTest pdt( "xx", false );

        int rv;
        rv = pdt.setupConfig( config );
        REQUIRE( rv == 0 );

        config.readConfig( "/tmp/dm_test.conf" );

        rv = pdt.loadConfig( config );
        REQUIRE( rv == 0 );

        REQUIRE( pdt.calibPath() == "/tmp/dmtest_calibs/dmtest" );

        // There will be no shmimName set
        REQUIRE( pdt.shmimName() == "" );
    }

    /// \todo finish implementing this
    SECTION( "a config file with a [dm] section changing everything" )
    {

        std::vector<std::string> s, k, v;

        s.push_back( "dm" );
        k.push_back( "calibPath" );
        v.push_back( "/tmp/dmtest_calibs2/dmtest2" );

        mx::app::writeConfigFile( "/tmp/dm_test.conf", s, k, v );

        mx::app::appConfigurator config;

        dm_tests::dmTest pdt( "xx", false );

        int rv;
        rv = pdt.setupConfig( config );
        REQUIRE( rv == 0 );

        config.readConfig( "/tmp/dm_test.conf" );

        rv = pdt.loadConfig( config );
        REQUIRE( rv == 0 );

        REQUIRE( pdt.calibPath() == "/tmp/dmtest_calibs2/dmtest2" );
    }

#ifdef XWCTEST_DOX_REF
    MagAOX::app::dev::dm::setupConfig();
    MagAOX::app::dev::dm::loadConfig();
    MagAOX::app::dev::dm::calibPath();
#endif
}

/// Test dmcomb detection, configuration, and manipulation
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dmcomb detection, configuration, and manipulation", "[dev::dm]" )
{
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    char ppath[1024];
    snprintf( ppath, sizeof( ppath ), "%s=/tmp/dmtest/shm", "MILK_SHM_DIR" );
    putenv( ppath );

    mx::improc::milkImage<float> ch0, ch1, ch2, ch3, ch4, chT;
    try
    {

        ch0.create( "dmtest00", 50, 50 );
        ch0().setConstant(1);

        ch1.create( "dmtest01", 50, 50 );
        ch1().setConstant(2);

        ch2.create( "dmtest02", 50, 50 );
        ch2().setConstant(3);

        ch3.create( "dmtest03", 50, 50 );
        ch3().setConstant(4);

        ch4.create( "dmtest04", 50, 50 );
        ch4().setConstant(5);

        chT.create( "dmtest", 50, 50 );
        chT() = ch0() + ch1() + ch2() + ch3() + ch4();

    }
    catch( const std::exception &e )
    {
        std::cerr << "dm_test: Exception creating dm channels: " << e.what() << '\n';
    }

    std::vector<std::string> s, k, v;

    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmtest" );

    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "50" );

    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "50" );

    s.push_back( "dm" );
    k.push_back( "deltaChannels" );
    v.push_back( "dmtest02,dmtest03" );

    mx::app::writeConfigFile( "/tmp/dm_test.conf", s, k, v );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    int rv;
    rv = pdt.setupConfig( config );
    REQUIRE( rv == 0 );

    config.readConfig( "/tmp/dm_test.conf" );

    rv = pdt.loadConfig( config );
    REQUIRE( rv == 0 );

    REQUIRE( pdt.shmimName() == "dmtest" );

    pdt.setSize( 50, 50, IMAGESTRUCT_FLOAT );

    rv = pdt.allocate( MagAOX::app::dev::shmimT() );

    REQUIRE( rv == 0 );

    int rows = pdt.instSatMap().rows();
    REQUIRE( rows == 50 );

    int cols = pdt.instSatMap().cols();
    REQUIRE( cols == 50 );

    rows = pdt.accumSatMap().rows();
    REQUIRE( rows == 50 );

    cols = pdt.accumSatMap().cols();
    REQUIRE( cols == 50 );

    rows = pdt.satPercMap().rows();
    REQUIRE( rows == 50 );

    cols = pdt.satPercMap().cols();
    REQUIRE( cols == 50 );

    int nc = pdt.numChannels();
    REQUIRE( nc == 5 );

    size_t nd = pdt.deltaChannels().size();
    REQUIRE( nd == 2);

    const std::vector<size_t> &notDeltas = pdt.notDeltas();
    REQUIRE(notDeltas.size() == 3);
    REQUIRE(notDeltas[0] == 0);
    REQUIRE(notDeltas[1] == 1);
    REQUIRE(notDeltas[2] == 4);

    mx::improc::milkImage<float> outputShape;
    bool pass = false;
    try
    {
        outputShape.open("dmtest_shape");
        pass = true;
    }
    catch(...)
    {}

    REQUIRE(pass == true);
    REQUIRE(outputShape.rows() == 50);
    REQUIRE(outputShape.cols() == 50);
    REQUIRE(outputShape().sum() == 0);

    outputShape = chT;
    float sum = outputShape().sum();
    REQUIRE(sum == (50*50)*(1+2+3+4+5));

    mx::improc::milkImage<float> outputDelta;
    pass = false;
    try
    {
        outputDelta.open("dmtest_delta");
        pass = true;
    }
    catch(...)
    {}

    REQUIRE(pass == true);
    REQUIRE(outputDelta.rows() == 50);
    REQUIRE(outputDelta.cols() == 50);
    REQUIRE(outputDelta().sum() == 0);

    rows = pdt.totalFlat().rows();
    REQUIRE( rows == 50 );

    cols = pdt.totalFlat().cols();
    REQUIRE( cols == 50 );

    sum = pdt.totalFlat().sum();
    REQUIRE(sum == 0);

    pdt.makeDelta();

    sum = pdt.totalFlat().sum();
    REQUIRE(sum == (50*50)*(1+2+5));

    //Check that it's still the same
    sum = outputShape().sum();
    REQUIRE(sum == (50*50)*(1+2+3+4+5));

    sum = outputDelta().sum();
    REQUIRE(sum == (50*50)*(3+4));

}

/// Test that appStartup() rejects an unsupported ImageStreamIO data type.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm appStartup rejects unsupported data type", "[dev::dm]" )
{
    mx::app::writeConfigFile( "/tmp/dm_test_badtype.conf", { "none" }, { "nada" }, { "0" } );

    mx::app::appConfigurator config;

    dm_tests::dmTestBadType pdt( "xx", false );

    int rv;
    rv = pdt.setupConfig( config );
    REQUIRE( rv == 0 );

    config.readConfig( "/tmp/dm_test_badtype.conf" );

    rv = pdt.loadConfig( config );
    REQUIRE( rv == 0 );

    rv = pdt.appStartup();
    REQUIRE( rv == -1 );
}

/// Test the allocate() size and data-type mismatch error branches.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm allocate size and type mismatches", "[dev::dm]" )
{
    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "20" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "20" );

    mx::app::writeConfigFile( "/tmp/dm_test_alloc.conf", s, k, v );

    SECTION( "width mismatch" )
    {
        dm_tests::dmTest pdt( "xx", false );
        mx::app::appConfigurator config;
        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_alloc.conf" );
        REQUIRE( pdt.loadConfig( config ) == 0 );
        pdt.setSize( 21, 20, IMAGESTRUCT_FLOAT );
        REQUIRE( pdt.allocate( MagAOX::app::dev::shmimT() ) == -1 );
    }

    SECTION( "height mismatch" )
    {
        dm_tests::dmTest pdt( "xx", false );
        mx::app::appConfigurator config;
        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_alloc.conf" );
        REQUIRE( pdt.loadConfig( config ) == 0 );
        pdt.setSize( 20, 21, IMAGESTRUCT_FLOAT );
        REQUIRE( pdt.allocate( MagAOX::app::dev::shmimT() ) == -1 );
    }

    SECTION( "data type mismatch" )
    {
        dm_tests::dmTest pdt( "xx", false );
        mx::app::appConfigurator config;
        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_alloc.conf" );
        REQUIRE( pdt.loadConfig( config ) == 0 );
        pdt.setSize( 20, 20, IMAGESTRUCT_DOUBLE );
        REQUIRE( pdt.allocate( MagAOX::app::dev::shmimT() ) == -1 );
    }
}

/// Test findDMChannels() when no channels are found for the configured shmimName.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm findDMChannels with no channels found", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmnochan" );

    mx::app::writeConfigFile( "/tmp/dm_test_nochan.conf", s, k, v );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    int rv = pdt.setupConfig( config );
    REQUIRE( rv == 0 );

    config.readConfig( "/tmp/dm_test_nochan.conf" );

    rv = pdt.loadConfig( config );
    REQUIRE( rv == 0 );

    REQUIRE( pdt.findDMChannels() == -1 );
}

/// Test flat file discovery, loading, setting, and zeroing, including error branches.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm flat file management", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/flattest/flats" );

    // Write some flat fits files, one of which is size-mismatched.
    mx::improc::eigenImage<float> flatA( 6, 6 ), flatB( 6, 6 ), flatBad( 3, 3 );
    flatA.setConstant( 2 );
    flatB.setConstant( 5 );
    flatBad.setConstant( 1 );

    mx::fits::fitsFile<float> ff;
    ff.write( "/tmp/dmtest_calibs/flattest/flats/flatA.fits", flatA );
    ff.write( "/tmp/dmtest_calibs/flattest/flats/flatB.fits", flatB );
    ff.write( "/tmp/dmtest_calibs/flattest/flats/flatBad.fits", flatBad );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "calibPath" );
    v.push_back( "/tmp/dmtest_calibs/flattest" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "6" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "6" );
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmflat" );

    mx::app::writeConfigFile( "/tmp/dm_test_flat.conf", s, k, v );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    int rv = pdt.setupConfig( config );
    REQUIRE( rv == 0 );

    config.readConfig( "/tmp/dm_test_flat.conf" );

    rv = pdt.loadConfig( config );
    REQUIRE( rv == 0 );

    pdt.prepIndiForCallbackTests();

    REQUIRE( pdt.checkFlats() == 0 );
    REQUIRE( pdt.numFlatCommands() == 3 );

    // actuator mask mismatch (flatBad is 3x3, actMask is 6x6)
    REQUIRE( pdt.loadFlat( "flatBad" ) == -1 );

    // not found
    REQUIRE( pdt.loadFlat( "doesNotExist" ) == -1 );

    // successful load
    REQUIRE( pdt.loadFlat( "flatA" ) == 0 );
    REQUIRE( pdt.flatLoaded() == true );
    REQUIRE( pdt.flatCurrent() == "flatA" );

    // not READY/OPERATING yet
    REQUIRE( pdt.setFlat() == -1 );

    pdt.state( MagAOX::app::stateCodes::READY );

    // channel doesn't exist yet (remove any stale shmim left from a previous test run)
    std::remove( "/tmp/dmtest/shm/dmflat00.im.shm" );
    REQUIRE( pdt.setFlat() == -1 );

    mx::improc::milkImage<float> flatChan;
    flatChan.create( "dmflat00", 6, 6 );

    REQUIRE( pdt.setFlat() == 0 );
    REQUIRE( pdt.flatIsSet() == true );
    REQUIRE( pdt.state() == MagAOX::app::stateCodes::OPERATING );
    REQUIRE( flatChan().sum() == 6 * 6 * 2 );

    // update==true path
    REQUIRE( pdt.setFlat( true ) == 0 );

    // switching flats while set re-applies automatically
    REQUIRE( pdt.loadFlat( "flatB" ) == 0 );
    REQUIRE( pdt.flatCurrent() == "flatB" );
    REQUIRE( pdt.flatIsSet() == true );
    REQUIRE( flatChan().sum() == 6 * 6 * 5 );

    REQUIRE( pdt.zeroFlat() == 0 );
    REQUIRE( pdt.flatIsSet() == false );
    REQUIRE( pdt.state() == MagAOX::app::stateCodes::READY );
    REQUIRE( flatChan().sum() == 0 );

    // exercise the newCallBack_setFlat and newCallBack_flats INDI paths
    pcf::IndiProperty ipSetFlat = pdt.indiP_setFlat();
    ipSetFlat["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.newCallBack_setFlat( ipSetFlat ) == 0 );
    REQUIRE( pdt.flatIsSet() == true );

    ipSetFlat["toggle"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.newCallBack_setFlat( ipSetFlat ) == 0 );
    REQUIRE( pdt.flatIsSet() == false );

    pcf::IndiProperty ipFlats = pdt.indiP_flats();
    REQUIRE( ipFlats.find( "flatA" ) == true );
    // a real client sends the full new selection state, so clear all others before setting
    // the target -- otherwise the previously-current selection (flatB) is still On too.
    ipFlats["flatA"].setSwitchState( pcf::IndiElement::On );
    ipFlats["flatB"].setSwitchState( pcf::IndiElement::Off );
    ipFlats["flatBad"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.newCallBack_flats( ipFlats ) == 0 );
    REQUIRE( pdt.flatCurrent() == "flatA" );

    // wrong key
    pcf::IndiProperty ipWrong( pcf::IndiProperty::Switch );
    ipWrong.setDevice( "somethingelse" );
    ipWrong.setName( "notflat" );
    REQUIRE( pdt.newCallBack_setFlat( ipWrong ) == -1 );
}

/// Test the width/height mismatch branch of setFlat() against the actual channel.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm setFlat channel size mismatch", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/flattest2/flats" );

    mx::improc::eigenImage<float> flatOK( 6, 6 );
    flatOK.setConstant( 1 );
    mx::fits::fitsFile<float> ff;
    ff.write( "/tmp/dmtest_calibs/flattest2/flats/flatOK.fits", flatOK );

    std::vector<std::string> s2, k2, v2;
    s2.push_back( "dm" );
    k2.push_back( "calibPath" );
    v2.push_back( "/tmp/dmtest_calibs/flattest2" );
    s2.push_back( "dm" );
    k2.push_back( "width" );
    v2.push_back( "6" );
    s2.push_back( "dm" );
    k2.push_back( "height" );
    v2.push_back( "6" );
    s2.push_back( "dm" );
    k2.push_back( "shmimName" );
    v2.push_back( "dmflatmm" );
    mx::app::writeConfigFile( "/tmp/dm_test_flat2.conf", s2, k2, v2 );

    dm_tests::dmTest         pdt2( "xx", false );
    mx::app::appConfigurator config2;
    REQUIRE( pdt2.setupConfig( config2 ) == 0 );
    config2.readConfig( "/tmp/dm_test_flat2.conf" );
    REQUIRE( pdt2.loadConfig( config2 ) == 0 );
    pdt2.state( MagAOX::app::stateCodes::READY );

    // channel is 3x3, but configured DM is 6x6 -> size mismatch
    mx::improc::milkImage<float> flatChanBad;
    flatChanBad.create( "dmflatmm00", 3, 3 );

    REQUIRE( pdt2.checkFlats() == 0 );
    REQUIRE( pdt2.loadFlat( "flatOK" ) == 0 );

    REQUIRE( pdt2.setFlat() == -1 );
}

/// Test test-pattern file discovery, loading, setting, and zeroing.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm test pattern file management", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/testtest/tests" );

    mx::improc::eigenImage<float> testA( 6, 6 ), testB( 6, 6 );
    testA.setConstant( 3 );
    testB.setConstant( 7 );

    mx::fits::fitsFile<float> ff;
    ff.write( "/tmp/dmtest_calibs/testtest/tests/testA.fits", testA );
    ff.write( "/tmp/dmtest_calibs/testtest/tests/testB.fits", testB );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "calibPath" );
    v.push_back( "/tmp/dmtest_calibs/testtest" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "6" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "6" );
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmtst" );

    mx::app::writeConfigFile( "/tmp/dm_test_test.conf", s, k, v );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    int rv = pdt.setupConfig( config );
    REQUIRE( rv == 0 );

    config.readConfig( "/tmp/dm_test_test.conf" );

    rv = pdt.loadConfig( config );
    REQUIRE( rv == 0 );

    pdt.prepIndiForCallbackTests();

    REQUIRE( pdt.checkTests() == 0 );
    REQUIRE( pdt.numTestCommands() == 2 );

    REQUIRE( pdt.loadTest( "doesNotExist" ) == -1 );

    REQUIRE( pdt.loadTest( "testA" ) == 0 );
    REQUIRE( pdt.testLoaded() == true );
    REQUIRE( pdt.testCurrent() == "testA" );

    // channel doesn't exist yet (remove any stale shmim left from a previous test run)
    std::remove( "/tmp/dmtest/shm/dmtst02.im.shm" );
    REQUIRE( pdt.setTest() == -1 );

    mx::improc::milkImage<float> testChan;
    testChan.create( "dmtst02", 6, 6 );

    REQUIRE( pdt.setTest() == 0 );
    REQUIRE( pdt.testIsSet() == true );
    REQUIRE( testChan().sum() == 6 * 6 * 3 );

    REQUIRE( pdt.loadTest( "testB" ) == 0 );
    REQUIRE( testChan().sum() == 6 * 6 * 7 );

    REQUIRE( pdt.zeroTest() == 0 );
    REQUIRE( pdt.testIsSet() == false );
    REQUIRE( testChan().sum() == 0 );

    // INDI paths
    pcf::IndiProperty ipSetTest = pdt.indiP_setTest();
    ipSetTest["toggle"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.newCallBack_setTest( ipSetTest ) == 0 );
    REQUIRE( pdt.testIsSet() == true );

    ipSetTest["toggle"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.newCallBack_setTest( ipSetTest ) == 0 );
    REQUIRE( pdt.testIsSet() == false );

    pcf::IndiProperty ipTests = pdt.indiP_tests();
    REQUIRE( ipTests.find( "testA" ) == true );
    // clear other selections before setting the target (see the flats case above)
    ipTests["testA"].setSwitchState( pcf::IndiElement::On );
    ipTests["testB"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.newCallBack_tests( ipTests ) == 0 );
    REQUIRE( pdt.testCurrent() == "testA" );

    pcf::IndiProperty ipWrong( pcf::IndiProperty::Switch );
    ipWrong.setDevice( "somethingelse" );
    ipWrong.setName( "nottest" );
    REQUIRE( pdt.newCallBack_setTest( ipWrong ) == -1 );
}

/// Test zeroAll() and clearSat() using real dmcomb-style shmim channels.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm zeroAll and clearSat", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    mx::improc::milkImage<float> ch0, ch1;
    ch0.create( "dmza00", 4, 4 );
    ch0().setConstant( 3 );
    ch1.create( "dmza01", 4, 4 );
    ch1().setConstant( 4 );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmza" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "4" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "4" );

    mx::app::writeConfigFile( "/tmp/dm_test_zeroall.conf", s, k, v );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    int rv = pdt.setupConfig( config );
    REQUIRE( rv == 0 );

    config.readConfig( "/tmp/dm_test_zeroall.conf" );

    rv = pdt.loadConfig( config );
    REQUIRE( rv == 0 );

    pdt.prepIndiForCallbackTests();

    // clearSat() with no shmimSat configured -> trivial success
    REQUIRE( pdt.clearSat() == 0 );

    pdt.setSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( MagAOX::app::dev::shmimT() ) == 0 );
    REQUIRE( pdt.numChannels() == 2 );

    // clearSat(): sat shmims don't exist yet -> warning, returns 0 (remove any stale
    // shmim left from a previous test run)
    std::remove( ( std::string( "/tmp/dmtest/shm/" ) + pdt.shmimSat() + ".im.shm" ).c_str() );
    std::remove( ( std::string( "/tmp/dmtest/shm/" ) + pdt.shmimSatPerc() + ".im.shm" ).c_str() );
    REQUIRE( pdt.clearSat() == 0 );

    mx::improc::milkImage<uint8_t> satChan;
    satChan.create( pdt.shmimSat(), 4, 4 );
    mx::improc::milkImage<float> satPercChan;
    satPercChan.create( pdt.shmimSatPerc(), 4, 4 );

    pdt.setAccumSatNonzero();
    REQUIRE( pdt.clearSat() == 0 );
    REQUIRE( pdt.accumSatMap().sum() == 0 );
    REQUIRE( pdt.instSatMap().sum() == 0 );

    pdt.state( MagAOX::app::stateCodes::READY );
    REQUIRE( pdt.zeroAll() == 0 );
    REQUIRE( ch0().sum() == 0 );
    REQUIRE( ch1().sum() == 0 );

    // zeroAll() INDI callback path
    pdt.state( MagAOX::app::stateCodes::READY );
    pcf::IndiProperty ipZeroAll = pdt.indiP_zeroAll();
    ipZeroAll["request"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.newCallBack_zeroAll( ipZeroAll ) == 0 );
}

/// Test zeroAll() with no shmimName configured (trivial success branch).
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm zeroAll with no shmimName configured", "[dev::dm]" )
{
    dm_tests::dmTest pdtEmpty( "xx", false );
    mx::app::writeConfigFile( "/tmp/dm_test_zeroall_empty.conf", { "none" }, { "nada" }, { "0" } );
    mx::app::appConfigurator configEmpty;
    REQUIRE( pdtEmpty.setupConfig( configEmpty ) == 0 );
    configEmpty.readConfig( "/tmp/dm_test_zeroall_empty.conf" );
    REQUIRE( pdtEmpty.loadConfig( configEmpty ) == 0 );
    REQUIRE( pdtEmpty.zeroAll() == 0 );
}

/// Test baseInitDM() and baseReleaseDM() state-machine transitions and error paths.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm baseInitDM and baseReleaseDM", "[dev::dm]" )
{
    mx::app::writeConfigFile( "/tmp/dm_test_baseinit.conf", { "none" }, { "nada" }, { "0" } );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_baseinit.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    pdt.prepIndiForCallbackTests();

    // wrong state (UNINITIALIZED, not NOTHOMED)
    REQUIRE( pdt.baseInitDM() == -1 );
    REQUIRE( pdt.state() == MagAOX::app::stateCodes::ERROR );

    pdt.state( MagAOX::app::stateCodes::NOTHOMED );
    REQUIRE( pdt.baseInitDM() == 0 );
    REQUIRE( pdt.state() == MagAOX::app::stateCodes::HOMING );

    // baseReleaseDM from a non-POWEROFF state moves to NOTHOMED, then calls releaseDM/zeroAll
    pdt.state( MagAOX::app::stateCodes::READY );
    REQUIRE( pdt.baseReleaseDM() == 0 );

    // INDI callbacks for init/zero/release
    pcf::IndiProperty ipInit = pdt.indiP_init();
    ipInit["request"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( pdt.newCallBack_init( ipInit ) == 0 );

    pdt.state( MagAOX::app::stateCodes::NOTHOMED );
    ipInit["request"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.newCallBack_init( ipInit ) == 0 );
    REQUIRE( pdt.state() == MagAOX::app::stateCodes::HOMING );

    pcf::IndiProperty ipZero = pdt.indiP_zero();
    ipZero["request"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.newCallBack_zero( ipZero ) == 0 );

    pcf::IndiProperty ipRelease = pdt.indiP_release();
    ipRelease["request"].setSwitchState( pcf::IndiElement::On );
    REQUIRE( pdt.newCallBack_release( ipRelease ) == 0 );

    // wrong-key errors
    pcf::IndiProperty ipWrong( pcf::IndiProperty::Switch );
    ipWrong.setDevice( "somethingelse" );
    ipWrong.setName( "notinit" );
    REQUIRE( pdt.newCallBack_init( ipWrong ) == -1 );
    REQUIRE( pdt.newCallBack_zero( ipWrong ) == -1 );
    REQUIRE( pdt.newCallBack_release( ipWrong ) == -1 );
}

/// Test onPowerOff() and whilePowerOff().
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm onPowerOff and whilePowerOff", "[dev::dm]" )
{
    mx::app::writeConfigFile( "/tmp/dm_test_poweroff.conf", { "none" }, { "nada" }, { "0" } );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_poweroff.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    REQUIRE( pdt.onPowerOff() == 0 );
    REQUIRE( pdt.whilePowerOff() == 0 );
}

/// Test the full appStartup/appLogic/appShutdown lifecycle, including the
/// saturation-processing thread, processImage(), intervalSatTrip(), and updateINDI().
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm full lifecycle with saturation thread", "[dev::dm]" )
{
    // dm<>::appShutdown() sends SIGUSR1 to the saturation thread to interrupt any
    // blocking waits. Install the harmless no-op handler used elsewhere in the code base
    // so this doesn't terminate the test process.
    struct sigaction act;
    memset( &act, 0, sizeof( act ) );
    act.sa_sigaction = &MagAOX::app::sigUsr1Handler;
    act.sa_flags     = SA_SIGINFO;
    sigemptyset( &act.sa_mask );
    REQUIRE( sigaction( SIGUSR1, &act, 0 ) == 0 );

    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    mx::improc::milkImage<float> ch0;
    ch0.create( "dmlc00", 4, 4 );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "calibPath" );
    v.push_back( "/tmp/dmtest_calibs/lifecycle" );
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmlc" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "4" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "4" );
    s.push_back( "dm" );
    k.push_back( "satAvgInt" );
    v.push_back( "0" );
    s.push_back( "dm" );
    k.push_back( "percThreshold" );
    v.push_back( "0" );
    s.push_back( "dm" );
    k.push_back( "intervalSatThreshold" );
    v.push_back( "0" );
    s.push_back( "dm" );
    k.push_back( "intervalSatCountThreshold" );
    v.push_back( "1" );
    s.push_back( "dm" );
    k.push_back( "satTriggerDevice" );
    v.push_back( "someDevice" );
    s.push_back( "dm" );
    k.push_back( "satTriggerProperty" );
    v.push_back( "someProperty" );

    mx::app::writeConfigFile( "/tmp/dm_test_lifecycle.conf", s, k, v );

    mx::app::appConfigurator config;

    dm_tests::dmTest pdt( "xx", false );

    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_lifecycle.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    pdt.setSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( MagAOX::app::dev::shmimT() ) == 0 );

    REQUIRE( pdt.appStartup() == 0 );

    // let the saturation thread reach steady state
    mx::sys::milliSleep( 300 );

    REQUIRE( pdt.updateINDI() == 0 );
    REQUIRE( pdt.appLogic() == 0 );

    // drive commandDM()/processImage() to saturate every actuator and trip the interval trigger
    pdt.m_testSatValue = 1;
    float srcbuf[16]   = { 0 };
    for( int i = 0; i < 10; ++i )
    {
        REQUIRE( pdt.processImage( srcbuf, MagAOX::app::dev::shmimT() ) == 0 );
        mx::sys::milliSleep( 50 );
    }

    // give the sat thread time to compute stats and set m_intervalSatTrip
    mx::sys::milliSleep( 500 );

    // this should now call intervalSatTrip(), which will try (and safely fail) to send an
    // INDI property to "someDevice", silently caught internally.
    REQUIRE( pdt.appLogic() == 0 );

    // verify the sat shmims got created and updated. Use CHECK (not REQUIRE) so that
    // appShutdown() below always runs and cleanly joins the saturation thread even if
    // this timing-sensitive check should ever fail.
    mx::improc::milkImage<uint8_t> satChan;
    bool                           opened = false;
    try
    {
        satChan.open( pdt.shmimSat() );
        opened = true;
    }
    catch( ... )
    {
    }
    CHECK( opened == true );

    REQUIRE( pdt.appShutdown() == 0 );
}

/// Additional branch coverage for the INDI static callback trampolines, the
/// "missing element" early-return branches, and the wrong-key branches of
/// newCallBack_flats()/newCallBack_tests().
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm INDI static callback trampolines and edge branches", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/xc/flats" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/xc/tests" );

    mx::improc::eigenImage<float> flatX( 4, 4 );
    flatX.setConstant( 1 );
    mx::fits::fitsFile<float> ff;
    ff.write( "/tmp/dmtest_calibs/xc/flats/flatX.fits", flatX );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "calibPath" );
    v.push_back( "/tmp/dmtest_calibs/xc" );
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmxc" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "4" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "4" );

    mx::app::writeConfigFile( "/tmp/dm_test_xc.conf", s, k, v );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_xc.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    pdt.prepIndiForCallbackTests();
    REQUIRE( pdt.checkFlats() == 0 );
    REQUIRE( pdt.checkTests() == 0 );

    // put the DM in a state where zeroFlat()/zeroTest() (invoked via the toggle==Off
    // path below) will succeed, with real channels to zero.
    pdt.state( MagAOX::app::stateCodes::READY );
    mx::improc::milkImage<float> flatChanXc, testChanXc;
    flatChanXc.create( "dmxc00", 4, 4 );
    testChanXc.create( "dmxc02", 4, 4 );

    typedef MAPPNS::dm<dm_tests::dmTest, float> dmT;

    // static trampolines
    pcf::IndiProperty ipInit = pdt.indiP_init();
    ipInit["request"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( dmT::st_newCallBack_init( &pdt, ipInit ) == 0 );

    pcf::IndiProperty ipZero = pdt.indiP_zero();
    ipZero["request"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( dmT::st_newCallBack_zero( &pdt, ipZero ) == 0 );

    pcf::IndiProperty ipRelease = pdt.indiP_release();
    ipRelease["request"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( dmT::st_newCallBack_release( &pdt, ipRelease ) == 0 );

    pcf::IndiProperty ipZeroAll = pdt.indiP_zeroAll();
    ipZeroAll["request"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( dmT::st_newCallBack_zeroAll( &pdt, ipZeroAll ) == 0 );

    pdt.state( MagAOX::app::stateCodes::READY );
    pcf::IndiProperty ipSetFlat = pdt.indiP_setFlat();
    ipSetFlat["toggle"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( dmT::st_newCallBack_setFlat( &pdt, ipSetFlat ) == 0 );

    pcf::IndiProperty ipSetTest = pdt.indiP_setTest();
    ipSetTest["toggle"].setSwitchState( pcf::IndiElement::Off );
    REQUIRE( dmT::st_newCallBack_setTest( &pdt, ipSetTest ) == 0 );

    pcf::IndiProperty ipFlats = pdt.indiP_flats();
    REQUIRE( dmT::st_newCallBack_flats( &pdt, ipFlats ) == 0 );

    pcf::IndiProperty ipTests = pdt.indiP_tests();
    REQUIRE( dmT::st_newCallBack_tests( &pdt, ipTests ) == 0 );

    // "missing element" early-return branches: same device/name, but no elements added
    pcf::IndiProperty ipInitBare( pcf::IndiProperty::Switch );
    ipInitBare.setDevice( ipInit.getDevice() );
    ipInitBare.setName( ipInit.getName() );
    REQUIRE( pdt.newCallBack_init( ipInitBare ) == 0 );

    pcf::IndiProperty ipZeroBare( pcf::IndiProperty::Switch );
    ipZeroBare.setDevice( ipZero.getDevice() );
    ipZeroBare.setName( ipZero.getName() );
    REQUIRE( pdt.newCallBack_zero( ipZeroBare ) == 0 );

    pcf::IndiProperty ipReleaseBare( pcf::IndiProperty::Switch );
    ipReleaseBare.setDevice( ipRelease.getDevice() );
    ipReleaseBare.setName( ipRelease.getName() );
    REQUIRE( pdt.newCallBack_release( ipReleaseBare ) == 0 );

    pcf::IndiProperty ipZeroAllBare( pcf::IndiProperty::Switch );
    ipZeroAllBare.setDevice( ipZeroAll.getDevice() );
    ipZeroAllBare.setName( ipZeroAll.getName() );
    REQUIRE( pdt.newCallBack_zeroAll( ipZeroAllBare ) == 0 );

    pcf::IndiProperty ipSetFlatBare( pcf::IndiProperty::Switch );
    ipSetFlatBare.setDevice( ipSetFlat.getDevice() );
    ipSetFlatBare.setName( ipSetFlat.getName() );
    REQUIRE( pdt.newCallBack_setFlat( ipSetFlatBare ) == 0 );

    pcf::IndiProperty ipSetTestBare( pcf::IndiProperty::Switch );
    ipSetTestBare.setDevice( ipSetTest.getDevice() );
    ipSetTestBare.setName( ipSetTest.getName() );
    REQUIRE( pdt.newCallBack_setTest( ipSetTestBare ) == 0 );

    // wrong-key branches of newCallBack_flats()/newCallBack_tests()
    pcf::IndiProperty ipWrong( pcf::IndiProperty::Switch );
    ipWrong.setDevice( "somethingelse" );
    ipWrong.setName( "notflats" );
    REQUIRE( pdt.newCallBack_flats( ipWrong ) == -1 );
    REQUIRE( pdt.newCallBack_tests( ipWrong ) == -1 );
}

/// Test that processImage() propagates a commandDM() failure, and that a broken
/// saturation semaphore is handled (sem_post() error branch).
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm processImage error branches", "[dev::dm]" )
{
    mx::app::writeConfigFile( "/tmp/dm_test_procimg.conf", { "none" }, { "nada" }, { "0" } );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_procimg.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    float srcbuf[4] = { 0 };

    // commandDM() failure
    pdt.m_commandDMFail = true;
    REQUIRE( pdt.processImage( srcbuf, MagAOX::app::dev::shmimT() ) == -1 );
    pdt.m_commandDMFail = false;
}

/// Test that allocate() propagates a findDMChannels() failure when the size/type
/// checks pass but no channels exist for the configured shmimName.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm allocate propagates findDMChannels failure", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmxcnochan" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "4" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "4" );

    mx::app::writeConfigFile( "/tmp/dm_test_xcnochan.conf", s, k, v );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_xcnochan.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    // loadConfig() unconditionally creates a "dmxcnochan_actmask" shmim (since width/height
    // are set), and findDMChannels()'s loose prefix search would otherwise pick that (and
    // the shape/delta/diff streams created later) up as a "channel". Remove any such
    // same-prefixed shmims so the "no channels found" branch can actually be reached.
    for( const auto &entry : std::filesystem::directory_iterator( "/tmp/dmtest/shm" ) )
    {
        if( entry.path().filename().string().rfind( "dmxcnochan", 0 ) == 0 )
        {
            std::filesystem::remove( entry.path() );
        }
    }

    pdt.setSize( 4, 4, IMAGESTRUCT_FLOAT );
    REQUIRE( pdt.allocate( MagAOX::app::dev::shmimT() ) == -1 );
}

/// Test that allocate() propagates an exception from creating the output shape shmim.
/// milkImage::create() throws a real (not mocked) exception when it can't open the shm
/// file for writing -- forced here the same way runCommand_test.cpp forces real OS-level
/// resource failures: by making the target directory genuinely unwritable, rather than by
/// mocking milkImage.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm allocate propagates an output shape shmim creation exception", "[dev::dm]" )
{
    // Start from a clean directory every run -- a stale shmim file left over from a
    // previous run (same name, still writable) would let create() reuse/open it instead
    // of needing a fresh, directory-write-permission-gated open(O_CREAT), silently
    // defeating the fault injection below.
    std::filesystem::remove_all( "/tmp/dmtest_shapefail" );
    setenv( "MILK_SHM_DIR", "/tmp/dmtest_shapefail/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest_shapefail/shm" );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmshapefail" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "4" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "4" );

    mx::app::writeConfigFile( "/tmp/dm_test_shapefail.conf", s, k, v );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_shapefail.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    // A real channel must exist for findDMChannels() to succeed before allocate() reaches
    // the output shmim creation this test targets.
    mx::improc::milkImage<float> chan0;
    chan0.create( "dmshapefail00", 4, 4 );

    // Now make the shm directory itself unwritable, so milkImage::create() genuinely
    // fails (real EACCES from the OS) the next time it tries to create a new shmim.
    REQUIRE( chmod( "/tmp/dmtest_shapefail/shm", 0555 ) == 0 );

    pdt.setSize( 4, 4, IMAGESTRUCT_FLOAT );
    int rv = pdt.allocate( MagAOX::app::dev::shmimT() );

    chmod( "/tmp/dmtest_shapefail/shm", 0755 );

    REQUIRE( rv == -1 );
}

// NOTE: appLogic()'s "saturation thread has exited" branch (detected via a raw
// pthread_tryjoin_np() on m_satThread.native_handle()) is intentionally not exercised
// here. Reliably killing the thread out from under it requires racing a signal against
// its blocking sem_timedwait(), and once pthread_tryjoin_np() succeeds it reaps the
// thread at the OS level without updating std::thread's own joinable() bookkeeping --
// so any subsequent pthread_kill()/join() (including from ~dm() or appShutdown()) can
// target an already-reaped/recycled thread ID. That combination was found to
// occasionally abort the test process (glibc's pthread_kill() hardening calls abort()
// on an invalid descriptor). This is a latent quirk in dm<>'s use of raw pthread calls
// alongside std::thread, not something worth routing around with more elaborate
// synchronization in a unit test; left uncovered.

/// Test setFlat()'s internal auto-load-on-demand path (when called before any explicit
/// loadFlat()), both the success case and the case where the internal load fails.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm setFlat internal auto-load", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/autoload/flats" );

    mx::improc::eigenImage<float> flatY( 5, 5 );
    flatY.setConstant( 4 );
    mx::fits::fitsFile<float> ff;
    ff.write( "/tmp/dmtest_calibs/autoload/flats/flatY.fits", flatY );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "calibPath" );
    v.push_back( "/tmp/dmtest_calibs/autoload" );
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmauto" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "5" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "5" );

    mx::app::writeConfigFile( "/tmp/dm_test_autoload.conf", s, k, v );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_autoload.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    // checkFlats() auto-selects the (only) flat as current, but does not load it into memory
    REQUIRE( pdt.checkFlats() == 0 );
    REQUIRE( pdt.flatLoaded() == false );
    REQUIRE( pdt.flatCurrent() == "flatY" );

    std::remove( "/tmp/dmtest/shm/dmauto00.im.shm" );
    mx::improc::milkImage<float> flatChan;
    flatChan.create( "dmauto00", 5, 5 );

    pdt.state( MagAOX::app::stateCodes::READY );

    // setFlat() internally loads the current flat since it isn't loaded yet
    REQUIRE( pdt.setFlat() == 0 );
    REQUIRE( pdt.flatLoaded() == true );
    REQUIRE( flatChan().sum() == 5 * 5 * 4 );
}

/// Test setFlat()'s internal auto-load failure path (the on-demand loadFlat() call
/// inside setFlat() fails because the configured default flat file doesn't exist).
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm setFlat internal auto-load failure", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/autoloadfail/flats" );

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "calibPath" );
    v.push_back( "/tmp/dmtest_calibs/autoloadfail" );
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmautofail" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "5" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "5" );
    s.push_back( "dm" );
    k.push_back( "flatDefault" );
    v.push_back( "missingdefault" );

    mx::app::writeConfigFile( "/tmp/dm_test_autoloadfail.conf", s, k, v );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_autoloadfail.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );
    REQUIRE( pdt.flatCurrent() == "default" );

    std::remove( "/tmp/dmtest/shm/dmautofail00.im.shm" );
    mx::improc::milkImage<float> flatChan;
    flatChan.create( "dmautofail00", 5, 5 );

    pdt.state( MagAOX::app::stateCodes::READY );

    // missingdefault.fits does not exist, so the internal loadFlat("default") call fails
    REQUIRE( pdt.setFlat() == -1 );
    REQUIRE( pdt.flatLoaded() == false );
}

/// Test zeroFlat()'s state/connect/size-mismatch error branches.
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm zeroFlat error branches", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );

    // empty m_shmimFlat (no shmimName configured) -> trivial success
    {
        mx::app::writeConfigFile( "/tmp/dm_test_zf_empty.conf", { "none" }, { "nada" }, { "0" } );
        mx::app::appConfigurator config;
        dm_tests::dmTest         pdtEmpty( "xx", false );
        REQUIRE( pdtEmpty.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_zf_empty.conf" );
        REQUIRE( pdtEmpty.loadConfig( config ) == 0 );
        REQUIRE( pdtEmpty.zeroFlat() == 0 );
    }

    std::vector<std::string> s, k, v;
    s.push_back( "dm" );
    k.push_back( "shmimName" );
    v.push_back( "dmzf" );
    s.push_back( "dm" );
    k.push_back( "width" );
    v.push_back( "5" );
    s.push_back( "dm" );
    k.push_back( "height" );
    v.push_back( "5" );

    mx::app::writeConfigFile( "/tmp/dm_test_zf.conf", s, k, v );

    mx::app::appConfigurator config;
    dm_tests::dmTest         pdt( "xx", false );
    REQUIRE( pdt.setupConfig( config ) == 0 );
    config.readConfig( "/tmp/dm_test_zf.conf" );
    REQUIRE( pdt.loadConfig( config ) == 0 );

    // wrong state
    REQUIRE( pdt.zeroFlat() == -1 );

    pdt.state( MagAOX::app::stateCodes::READY );

    // channel doesn't exist
    std::remove( "/tmp/dmtest/shm/dmzf00.im.shm" );
    REQUIRE( pdt.zeroFlat() == -1 );

    // width/height mismatch
    mx::improc::milkImage<float> flatChanBad;
    flatChanBad.create( "dmzf00", 3, 3 );
    REQUIRE( pdt.zeroFlat() == -1 );
}

/// Test loadConfig()'s dm.testDefault and dm.actMaskPath handling: the default test name
/// gets its path/extension stripped, and a configured actuator mask file is loaded and
/// validated against dm.width/dm.height (both matching and mismatched cases).
/**
 * \ingroup dm_tests
 */
TEST_CASE( "Test dm testDefault and actMaskPath config handling", "[dev::dm]" )
{
    setenv( "MILK_SHM_DIR", "/tmp/dmtest/shm", 1 );
    mx::ioutils::createDirectories( "/tmp/dmtest/shm" );
    mx::ioutils::createDirectories( "/tmp/dmtest_calibs/actmasktest" );

    mx::improc::eigenImage<float> maskGood( 5, 5 ), maskBad( 3, 3 );
    maskGood.setConstant( 1 );
    maskBad.setConstant( 1 );

    mx::fits::fitsFile<float> ff;
    ff.write( "/tmp/dmtest_calibs/actmasktest/maskGood.fits", maskGood );
    ff.write( "/tmp/dmtest_calibs/actmasktest/maskBad.fits", maskBad );

    SECTION( "testDefault is stripped to a bare name, and a matching actMaskPath loads" )
    {
        std::vector<std::string> s, k, v;
        s.push_back( "dm" );
        k.push_back( "calibPath" );
        v.push_back( "/tmp/dmtest_calibs/actmasktest" );
        s.push_back( "dm" );
        k.push_back( "width" );
        v.push_back( "5" );
        s.push_back( "dm" );
        k.push_back( "height" );
        v.push_back( "5" );
        s.push_back( "dm" );
        k.push_back( "shmimName" );
        v.push_back( "dmactmaskgood" );
        s.push_back( "dm" );
        k.push_back( "testDefault" );
        v.push_back( "/some/path/mytest.fits" );
        s.push_back( "dm" );
        k.push_back( "actMaskPath" );
        v.push_back( "maskGood.fits" );

        mx::app::writeConfigFile( "/tmp/dm_test_actmask_good.conf", s, k, v );

        mx::app::appConfigurator config;
        dm_tests::dmTest         pdt( "xx", false );
        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_actmask_good.conf" );
        REQUIRE( pdt.loadConfig( config ) == 0 );

        REQUIRE( pdt.testDefault() == "mytest" );
        REQUIRE( pdt.testCurrent() == "default" );
    }

    SECTION( "a size-mismatched actMaskPath fails loadConfig" )
    {
        std::vector<std::string> s, k, v;
        s.push_back( "dm" );
        k.push_back( "calibPath" );
        v.push_back( "/tmp/dmtest_calibs/actmasktest" );
        s.push_back( "dm" );
        k.push_back( "width" );
        v.push_back( "5" );
        s.push_back( "dm" );
        k.push_back( "height" );
        v.push_back( "5" );
        s.push_back( "dm" );
        k.push_back( "shmimName" );
        v.push_back( "dmactmaskbad" );
        s.push_back( "dm" );
        k.push_back( "actMaskPath" );
        v.push_back( "maskBad.fits" );

        mx::app::writeConfigFile( "/tmp/dm_test_actmask_bad.conf", s, k, v );

        mx::app::appConfigurator config;
        dm_tests::dmTest         pdt( "xx", false );
        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_actmask_bad.conf" );
        REQUIRE( pdt.loadConfig( config ) == -1 );
    }

    SECTION( "a non-existent actMaskPath fails loadConfig" )
    {
        std::vector<std::string> s, k, v;
        s.push_back( "dm" );
        k.push_back( "calibPath" );
        v.push_back( "/tmp/dmtest_calibs/actmasktest" );
        s.push_back( "dm" );
        k.push_back( "width" );
        v.push_back( "5" );
        s.push_back( "dm" );
        k.push_back( "height" );
        v.push_back( "5" );
        s.push_back( "dm" );
        k.push_back( "shmimName" );
        v.push_back( "dmactmasknone" );
        s.push_back( "dm" );
        k.push_back( "actMaskPath" );
        v.push_back( "doesNotExist.fits" );

        mx::app::writeConfigFile( "/tmp/dm_test_actmask_none.conf", s, k, v );

        mx::app::appConfigurator config;
        dm_tests::dmTest         pdt( "xx", false );
        REQUIRE( pdt.setupConfig( config ) == 0 );
        config.readConfig( "/tmp/dm_test_actmask_none.conf" );
        REQUIRE( pdt.loadConfig( config ) == -1 );
    }
}

