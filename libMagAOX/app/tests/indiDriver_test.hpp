#ifndef app_tests_indiDriver_test_hpp
#define app_tests_indiDriver_test_hpp

namespace indiDriver_tests
{

/// Minimal stand-in for a MagAOXApp-like parent.
/** Satisfies the interface required by MagAOX::app::indiDriver<parentT>
 * without dragging in all of MagAOXApp -- just the FIFO name accessors,
 * configName(), the handleXXXProperty callbacks, and a static log<>()
 * template matching the one used in indiDriver.hpp.
 */
struct indiDriverTestParent
{
    std::string m_driverInName;
    std::string m_driverOutName;
    std::string m_driverCtrlName;
    std::string m_configName{ "idtest" };

    std::string driverInName() { return m_driverInName; }
    std::string driverOutName() { return m_driverOutName; }
    std::string driverCtrlName() { return m_driverCtrlName; }
    std::string configName() { return m_configName; }

    int defCount{ 0 };
    int getPropCount{ 0 };
    int newPropCount{ 0 };
    int setPropCount{ 0 };

    void handleDefProperty( const pcf::IndiProperty & ) { ++defCount; }
    void handleGetProperties( const pcf::IndiProperty & ) { ++getPropCount; }
    void handleNewProperty( const pcf::IndiProperty & ) { ++newPropCount; }
    void handleSetProperty( const pcf::IndiProperty & ) { ++setPropCount; }

    template <typename logT>
    static int log( const typename logT::messageT &msg )
    {
        static_cast<void>( msg );
        return 0;
    }
};

/// Test-exposed subclass, reaching the protected members of indiDriver so
/// tests can inspect/force its internal state (e.g. clearing m_parent, or
/// forcing the outgoing client into a "disconnected" state) without adding
/// any test-only surface to the production class itself (beyond the
/// xwcTestHooks member, which only exists when XWCTEST_INDIDRIVER_HOOKS is
/// defined).
template <class parentT>
struct indiDriverExposed : public MagAOX::app::indiDriver<parentT>
{
    indiDriverExposed( parentT *parent, const std::string &name, const std::string &ver, const std::string &proto )
        : MagAOX::app::indiDriver<parentT>( parent, name, ver, proto )
    {
    }

    void clearParent()
    {
        this->m_parent = nullptr;
    }

    void setServer( const std::string &ip, int port )
    {
        this->m_serverIPAddress = ip;
        this->m_serverPort      = port;
    }

    bool hasOutGoing()
    {
        return this->m_outGoing != nullptr;
    }

    void forceOutGoingQuit()
    {
        if( this->m_outGoing )
        {
            this->m_outGoing->quitProcess();
        }
    }
};

} // namespace indiDriver_tests

#endif // app_tests_indiDriver_test_hpp
