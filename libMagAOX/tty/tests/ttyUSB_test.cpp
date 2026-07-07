/** \file ttyUSB_test.cpp
  * \brief Catch2 tests for ttyUSBDevName/ttyUSBDevNames
  *
  * The vendor/product/serial matching logic in ttyUSB.cpp walks real udev sysfs entries
  * under /sys/class/tty/ttyUSB*, which only exist when actual USB-serial hardware (or a
  * kernel-level USB-serial gadget) is attached. Neither is available in this build/test
  * environment. Rather than mock libudev, ttyUSB.cpp exposes the (test-only, defaulted to
  * production values) XWCTEST_TTYUSB_SYSFS_DIR/XWCTEST_TTYUSB_SYSFS_PREFIX overrides, so
  * these tests can point the exact same real getFileNames()/libudev calls at genuinely
  * real OS state instead: a scratch directory (for the "not a real udev syspath" branch)
  * and always-present non-USB tty entries like /sys/class/tty/tty0 (for the "found a
  * device but it has no usb parent" branch). The vendor/product/serial match and success
  * paths still require real USB-serial hardware and remain untested here.
  */
#include "../../../tests/catch2/catch.hpp"

#include "../ttyUSB.hpp"
#include "../ttyErrors.hpp"

#include <filesystem>
#include <fstream>

namespace libXWCTest
{
namespace ttyUSBTest
{
// Set by ScratchSysfsDir before XWCTEST_TTYUSB_NOSYSPATH_ns::ttyUSBDev{Name,Names} run --
// declared here, ahead of the re-include below, so its name is visible where the macro
// substitutes it into ttyUSB.cpp's getFileNames() call.
std::string xwcTestTtyUsbScratchDir;
} // namespace ttyUSBTest
} // namespace libXWCTest

#define XWCTEST_NAMESPACE XWCTEST_TTYUSB_NOSYSPATH_ns
#define XWCTEST_TTYUSB_SYSFS_DIR libXWCTest::ttyUSBTest::xwcTestTtyUsbScratchDir
#include "../ttyUSB.cpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_TTYUSB_SYSFS_DIR
#undef XWCTEST_TTYUSB_SYSFS_PREFIX

#define XWCTEST_NAMESPACE XWCTEST_TTYUSB_NOUSBPARENT_ns
#define XWCTEST_TTYUSB_SYSFS_PREFIX "tty0"
#include "../ttyUSB.cpp"
#undef XWCTEST_NAMESPACE
#undef XWCTEST_TTYUSB_SYSFS_PREFIX

namespace libXWCTest
{
namespace ttyUSBTest
{

TEST_CASE( "ttyUSBDevName returns TTY_E_NODEVNAMES when no ttyUSB devices exist", "[libMagAOX::tty::ttyUSBDevName]" )
{
    std::string devName = "unset";
    int rv = MagAOX::tty::ttyUSBDevName( devName, "0403", "6001", "" );

    REQUIRE( rv == TTY_E_NODEVNAMES );
    REQUIRE( devName == "" );
}

TEST_CASE( "ttyUSBDevNames returns TTY_E_NODEVNAMES when no ttyUSB devices exist", "[libMagAOX::tty::ttyUSBDevNames]" )
{
    std::vector<std::string> devNames = { "stale", "entries" };
    int rv = MagAOX::tty::ttyUSBDevNames( devNames, "0403", "6001" );

    REQUIRE( rv == TTY_E_NODEVNAMES );
    REQUIRE( devNames.size() == 0 );
}

/// A real scratch directory containing one plain file named like a ttyUSB syspath entry
/// -- getFileNames() finds it via genuinely reading the directory, but since it isn't a
/// real kernel sysfs path, udev_device_new_from_syspath() genuinely fails on it too.
struct ScratchSysfsDir
{
    std::filesystem::path dir;

    ScratchSysfsDir()
    {
        std::string dirTemplate = ( std::filesystem::temp_directory_path() / "ttyUSB_test_sysfs_XXXXXX" ).string();
        REQUIRE( mkdtemp( dirTemplate.data() ) != nullptr );
        dir = dirTemplate;
        std::ofstream( dir / "ttyUSB0" ) << "not a real sysfs device\n";
    }

    ~ScratchSysfsDir()
    {
        std::filesystem::remove_all( dir );
    }
};

TEST_CASE( "ttyUSBDevName reports device-not-found when a syspath entry isn't a real udev syspath",
           "[libMagAOX::tty::ttyUSBDevName]" )
{
    ScratchSysfsDir scratch;
    xwcTestTtyUsbScratchDir = scratch.dir.string() + "/";

    std::string devName = "unset";
    int rv = MagAOX::tty::XWCTEST_TTYUSB_NOSYSPATH_ns::ttyUSBDevName( devName, "0403", "6001", "" );

    REQUIRE( rv == TTY_E_DEVNOTFOUND );
    REQUIRE( devName == "" );
}

TEST_CASE( "ttyUSBDevNames reports device-not-found when a syspath entry isn't a real udev syspath",
           "[libMagAOX::tty::ttyUSBDevNames]" )
{
    ScratchSysfsDir scratch;
    xwcTestTtyUsbScratchDir = scratch.dir.string() + "/";

    std::vector<std::string> devNames = { "stale" };
    int rv = MagAOX::tty::XWCTEST_TTYUSB_NOSYSPATH_ns::ttyUSBDevNames( devNames, "0403", "6001" );

    REQUIRE( rv == TTY_E_DEVNOTFOUND );
    REQUIRE( devNames.size() == 0 );
}

TEST_CASE( "ttyUSBDevName reports device-not-found for a real tty device with no usb parent",
           "[libMagAOX::tty::ttyUSBDevName]" )
{
    std::string devName = "unset";
    int rv = MagAOX::tty::XWCTEST_TTYUSB_NOUSBPARENT_ns::ttyUSBDevName( devName, "0403", "6001", "" );

    REQUIRE( rv == TTY_E_DEVNOTFOUND );
    REQUIRE( devName == "" );
}

TEST_CASE( "ttyUSBDevNames reports device-not-found for a real tty device with no usb parent",
           "[libMagAOX::tty::ttyUSBDevNames]" )
{
    std::vector<std::string> devNames = { "stale" };
    int rv = MagAOX::tty::XWCTEST_TTYUSB_NOUSBPARENT_ns::ttyUSBDevNames( devNames, "0403", "6001" );

    REQUIRE( rv == TTY_E_DEVNOTFOUND );
    REQUIRE( devNames.size() == 0 );
}

} // namespace ttyUSBTest
} // namespace libXWCTest
