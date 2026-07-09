/** \file ttyUSB.cpp
 * \author Jared R. Males
 * \brief Find the details for USB serial devices
 *
 * \ingroup tty_files
 *
 */


#include <iostream>

#include <libudev.h>

#include <string>
#include <cstring>

#include <mx/ioutils/fileUtils.hpp>

#include "ttyUSB.hpp"

#include "ttyErrors.hpp"


// Test-only overrides (never redefined outside unit tests) letting tests point the sysfs
// scan at a real, always-present, non-USB tty entry (e.g. "tty0"/"console") to reach the
// "found a device but it has no usb parent" branch, or at a scratch directory to reach
// the "not a real udev syspath" branch -- both using genuinely real OS state rather than
// a mock, since no real ttyUSB hardware is available in a CI/build environment. The
// defaults below are exactly the production behavior.
#ifndef XWCTEST_TTYUSB_SYSFS_DIR
#define XWCTEST_TTYUSB_SYSFS_DIR "/sys/class/tty/"
#endif
#ifndef XWCTEST_TTYUSB_SYSFS_PREFIX
#define XWCTEST_TTYUSB_SYSFS_PREFIX "ttyUSB"
#endif

namespace MagAOX
{
namespace tty
{

#ifdef XWCTEST_NAMESPACE
namespace XWCTEST_NAMESPACE
{
#endif

int ttyUSBDevName( std::string & devName,       // [out] the /dev/ttyUSBX device name.
                   const std::string & vendor,  // [in] the 4-digit vendor identifier.
                   const std::string & product, // [in] the 4-digit product identifier.
                   const std::string & serial   // [in] the serial number.  Can be "".
                 )
{
    typedef mx::verbose::vvv verboseT;

   std::vector<std::string> devNames;

   devName = "";
   mx_error_check_rv(mx::ioutils::getFileNames(devNames, XWCTEST_TTYUSB_SYSFS_DIR, XWCTEST_TTYUSB_SYSFS_PREFIX, "", ""),-1);

   if(devNames.size() == 0) return TTY_E_NODEVNAMES;

   struct udev *udev;


   /* Create the udev object */
   udev = udev_new();
   if (!udev) return TTY_E_UDEVNEWFAILED;

   for(size_t i=0; i< devNames.size(); ++i)
   {
      struct udev_device *dev0;

      dev0 = udev_device_new_from_syspath(udev, devNames[i].c_str());

      if(!dev0)
      {
         std::cerr << "udev_device_new_from_syspath failed: " << strerror(errno) << "\n";
         perror("");
         continue;
      }

      struct udev_device *dev;

      dev = udev_device_get_parent_with_subsystem_devtype( dev0, "usb", "usb_device");

      if (!dev)
      {
         std::cerr << "udev_device_get_parent_with_subsystem_devtype failed: " << strerror(errno) << "\n";
         perror("");
         udev_device_unref(dev0);
         continue;
      }

      // idVendor/idProduct/serial sysattrs, and a successful match, only exist on a real
      // attached USB-serial device -- unreachable in a build/test environment without one.
      // LCOV_EXCL_START
      const char * idVendor = udev_device_get_sysattr_value( dev, "idVendor" );

      if(idVendor == nullptr)
      {
         udev_device_unref(dev0);
         continue;
      }

      if( strcmp( idVendor, vendor.c_str()) != 0)
      {
         udev_device_unref(dev0);
         continue;
      }

      const char * idProduct = udev_device_get_sysattr_value( dev, "idProduct" );

      if(idProduct == nullptr)
      {
         udev_device_unref(dev0);
         continue;
      }

      if( strcmp( idProduct, product.c_str()) != 0)
      {
         udev_device_unref(dev0);
         continue;
      }

      const char * dserial = udev_device_get_sysattr_value( dev, "serial" );

      if(dserial == nullptr)
      {
         if( serial != "")
         {
            udev_device_unref(dev0);
            continue;
         }
      }
      else if( strcmp( dserial, serial.c_str()) != 0 )
      {
         udev_device_unref(dev0);
         continue;
      }

      //If we make it through all comparisons we found it!
      std::filesystem::path p(devNames[i]);
      devName = "/dev/" + p.filename().string();

      udev_device_unref(dev0);

      udev_unref(udev);

      return TTY_E_NOERROR;
   }
      // LCOV_EXCL_STOP

   devName = "";

   udev_unref(udev);

   return TTY_E_DEVNOTFOUND;
}

int ttyUSBDevNames( std::vector<std::string> & devNames, // [out] the /dev/ttyUSBX device names for all matching devices.
                    const std::string & vendor,           // [in] the 4-digit vendor identifier.
                    const std::string & product           // [in] the 4-digit product identifier.
                  )
{
   std::vector<std::string> pdevNames;

   devNames.clear();

   typedef mx::verbose::vvv verboseT;
   mx_error_check_rv(mx::ioutils::getFileNames(pdevNames, XWCTEST_TTYUSB_SYSFS_DIR, XWCTEST_TTYUSB_SYSFS_PREFIX, "", ""), -1);

   if(pdevNames.size() == 0) return TTY_E_NODEVNAMES;

   struct udev *udev;

   /* Create the udev object */
   udev = udev_new();
   if (!udev) return TTY_E_UDEVNEWFAILED;

   for(size_t i=0; i< pdevNames.size(); ++i)
   {
      struct udev_device *dev0;

      dev0 = udev_device_new_from_syspath(udev, pdevNames[i].c_str());

      if(!dev0)
      {
         continue;
      }

      struct udev_device * dev;
      dev = udev_device_get_parent_with_subsystem_devtype( dev0, "usb", "usb_device");

      if (!dev)
      {
         udev_device_unref(dev0);
         continue;
      }

      // idVendor/idProduct sysattrs, and a successful match, only exist on a real attached
      // USB-serial device -- unreachable in a build/test environment without one.
      // LCOV_EXCL_START
      const char * idVendor = udev_device_get_sysattr_value( dev, "idVendor" );

      if(idVendor == nullptr)
      {
         udev_device_unref(dev0);
         continue;
      }

      if( strcmp( idVendor, vendor.c_str()) != 0)
      {
         udev_device_unref(dev0);
         continue;
      }

      const char * idProduct = udev_device_get_sysattr_value( dev, "idProduct" );

      if(idProduct == nullptr)
      {
         udev_device_unref(dev0);
         continue;
      }

      if( strcmp( idProduct, product.c_str()) != 0)
      {
         udev_device_unref(dev0);
         continue;
      }

      //If we make it through all comparisons we found it!
      std::filesystem::path p(pdevNames[i]);
      devNames.push_back( "/dev/" + p.filename().string());

      udev_device_unref(dev0);
   }
      // LCOV_EXCL_STOP

   udev_unref(udev);

   if( devNames.size() > 0) return TTY_E_NOERROR;
   else return TTY_E_DEVNOTFOUND;

}

#ifdef XWCTEST_NAMESPACE
} // namespace XWCTEST_NAMESPACE
#endif

} //namespace tty
} //namespace MagAOX

