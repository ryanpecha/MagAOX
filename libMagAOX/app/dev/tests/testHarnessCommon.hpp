/** \file testHarnessCommon.hpp
 * \brief Shared helper for the dev:: mixin and MagAOXApp Catch2 test harnesses.
 *
 * Several test harnesses under libMagAOX/app/dev/tests/ and libMagAOX/app/tests/
 * each built the same throwaway indiDriver by hand. They did this only to make
 * their m_indiDriver pointer non-null. See makeFifolessIndiDriver() below for why.
 * This header holds that one piece in a single documented definition instead of
 * many copies.
 *
 * Callers must include MagAOXApp.hpp before this header. MagAOXApp.hpp pulls in
 * indiDriver.hpp. This header does not include MagAOXApp.hpp itself because the
 * relative include path differs between libMagAOX/app/dev/tests/ and
 * libMagAOX/app/tests/.
 *
 * \ingroup testing
 */
#ifndef app_dev_tests_testHarnessCommon_hpp
#define app_dev_tests_testHarnessCommon_hpp

namespace MagAOX
{
namespace app
{
namespace dev
{
namespace testHarness
{

/// Construct a real indiDriver with no FIFO, bound to `app` and `configName`.
/** This exists only so a test harness's `m_indiDriver` is non-null. Many INDI
 * update code paths start with an early `if(!m_indiDriver) return 0;` guard.
 * Examples are onPowerOff(), whilePowerOff(), updateINDI(), and the various
 * newCallBack_* and setCallBack_* handlers. With a non-null driver those
 * functions run their full bodies instead of returning at the guard.
 *
 * The test never calls execute() or activate() on the returned driver, so INDI
 * response mode is never enabled. As a result sendSetProperty(),
 * sendDefProperty(), and similar calls are harmless no-ops. No live INDI server
 * is needed.
 *
 * The caller owns the returned pointer and should assign it to m_indiDriver.
 * A harness that constructs more than one of these over its lifetime must
 * delete the previous value first to avoid a leak.
 *
 * \returns a newly allocated indiDriver<AppT>
 */
template <typename AppT>
MagAOX::app::indiDriver<AppT> *makeFifolessIndiDriver( AppT *app /**< [in] the app the driver is bound to */,
                                                        const std::string &configName /**< [in] the device/config name to bind the driver to */
                                                      )
{
    return new MagAOX::app::indiDriver<AppT>( app, configName, "0", "0" );
}

} // namespace testHarness
} // namespace dev
} // namespace app
} // namespace MagAOX

#endif // app_dev_tests_testHarnessCommon_hpp
