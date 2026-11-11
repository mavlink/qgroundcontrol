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
#endif
    QList<SharedLinkConfigurationPtr> _configsNamed(const char* name);
};
