/** \file testHarnessCommon.hpp
 * \brief Small shared helper(s) for the dev:: mixin (and MagAOXApp) Catch2 test harnesses.
 *
 * Several independently-written test harnesses under libMagAOX/app/dev/tests/ and
 * libMagAOX/app/tests/ each hand-rolled the same throwaway indiDriver construction,
 * purely to make their m_indiDriver non-null (see makeFifolessIndiDriver() below for
 * why). This header factors that one piece out to a single documented definition
 * instead of many copy-pasted ones.
 *
 * Callers must include MagAOXApp.hpp (which pulls in indiDriver.hpp) before this
 * header -- the relative include path to MagAOXApp.hpp differs between
 * libMagAOX/app/dev/tests/ and libMagAOX/app/tests/, so it is not included here.
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

/// Construct a real, but FIFO-less, indiDriver bound to `app` and `configName`.
/** This exists purely so a test harness's `m_indiDriver` is non-null, letting
 * INDI-update code paths (onPowerOff(), whilePowerOff(), updateINDI(), and the
 * various newCallBack_* / setCallBack_* handlers) run their non-trivial bodies
 * instead of taking the early `if(!m_indiDriver) return 0;` guard that many of
 * them have. Because execute()/activate() is never called on the returned
 * driver, sendSetProperty()/sendDefProperty() and friends remain harmless
 * no-ops (INDI response mode is never enabled), so this never requires a
 * live, connected INDI server.
 *
 * The caller owns the returned pointer: assign it to m_indiDriver, and if
 * constructing more than one of these over a harness's lifetime, delete the
 * previous value first to avoid a leak.
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
