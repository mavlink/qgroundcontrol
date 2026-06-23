#pragma once

#include "CommsTest.h"
#include "LinkConfiguration.h"

/// Tests for LinkManager::_reconnectAutoConnectLinks() — the auto-connect link
/// re-establishment driven off the auto-connect timer (PR #14547).
class LinkManagerTest : public CommsTest
{
    Q_OBJECT

private slots:
    void _testReconnectsDroppedAutoConnectLink();
    void _testSuppressedLinkNotReconnected();
    void _testDynamicLinkNotReconnected();
    void _testNonAutoConnectLinkNotReconnected();
    void _testNeverStartedLinkNotConnected();
    void _testLinkActiveStableAcrossReconnect();

private:
    SharedLinkConfigurationPtr _addMockConfig(const QString &name, bool dynamic, bool autoConnect);
    void _reconnect();
};
