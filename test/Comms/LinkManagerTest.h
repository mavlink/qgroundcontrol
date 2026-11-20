#pragma once

#include "CommsTest.h"
#include "LinkConfiguration.h"

/// Tests for LinkManager::_reconnectAutoConnectLinks() — the auto-connect link
/// re-establishment driven off the auto-connect timer (PR #14547).
class LinkManagerTest : public CommsTest
{
    Q_OBJECT

private slots:
#ifndef QGC_NO_SERIAL_LINK
    void _testOccupiedSerialAutoConnectRecovers();
    void _testReservedSerialPortNotOpened();
    void _testManualConnectToSerialPortInUseSaysWhy();
    void _testManualSerialConnectOpensPortAtOnce();
    void _testManualSerialConnectWaitsForPortAccess_data();
    void _testManualSerialConnectWaitsForPortAccess();
    void _testManualSerialConnectEndsWhenPortAccessIsDenied_data();
    void _testManualSerialConnectEndsWhenPortAccessIsDenied();
    void _testLinkWaitingForPortAccessNotConnected_data();
    void _testLinkWaitingForPortAccessNotConnected();
    void _testLinkWaitingForPortAccessReplacedByOneThatAsks_data();
    void _testLinkWaitingForPortAccessReplacedByOneThatAsks();
    void _testLinkWaitingForPortAccessTriesPortInUseAgain();
    void _testSerialPortFlowControlIsThatOfItsDriver();
    void _testSerialLinkSavedByStableName();
    void _testSerialLinkSavedByStableNameOnceConnected_data();
    void _testSerialLinkSavedByStableNameOnceConnected();
    void _testSerialLinkSavedByStableNameWhenEditorCloses_data();
    void _testSerialLinkSavedByStableNameWhenEditorCloses();
    void _testSerialReservationFollowsLink();
#endif
    void _testReconnectsDroppedAutoConnectLink();
    void _testSuppressedLinkNotReconnected();
    void _testDynamicLinkNotReconnected();
    void _testNonAutoConnectLinkNotReconnected();
    void _testNeverStartedLinkNotConnected();
    void _testLinkActiveStableAcrossReconnect();
    void _testUdpUnresolvedHostFailsConnect();
    void _testUdpUnresolvedHostAllowedStaysConnected();
    void _testUdpBindFailureDisconnects();
    void _testUdpAutoConnectBindFailureReusesConfig();
    void _testForwardingLinkUnresolvedHostRetries();
    void _testSupportForwardingFailureAllowsRetry();
    void _testDynamicUdpLinkIgnoresSameNamedUserLink();

private:
    SharedLinkConfigurationPtr _addMockConfig(const QString &name, bool dynamic, bool autoConnect);
    void _reconnect();
    void _expectUdpBindFailureLogs();
    void _verifyUdpBindFailureLogs();
#ifndef QGC_NO_SERIAL_LINK
    SharedLinkConfigurationPtr _addSerialConfig(const QString& name, const QString& port);
    /// Connects a link on the serial port holderPort. Then verifies that a link whose port is secondPort, a name
    /// of the same port, is refused with a message that names the first link.
    void _verifyOpenPortRefusesSecondLink(const QString& holderPort, const QString& secondPort);
    /// The serial port of the only link in the saved list of configurations; empty unless the list has one link.
    QString _savedSerialPort();
    /// Pseudo-terminals as the serial ports, which a test can have wait for the user's permission.
    struct PseudoTerminalPorts;
    /// What becomes of a link between the request for the access to its port and the grant.
    enum class WhileWaitingForAccess
    {
        MinutePassed,
        LinkStopped,
        ConnectionsSuspended,
        LinkConnectedAndDropped,
    };
    Q_ENUM(WhileWaitingForAccess)
#endif
    QList<SharedLinkConfigurationPtr> _configsNamed(const char* name);
};
