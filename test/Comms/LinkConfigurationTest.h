#pragma once

#include "UnitTest.h"

class LinkConfigurationTest : public UnitTest
{
    Q_OBJECT

private slots:
    // LinkConfiguration base (via TCPConfiguration)
    void _testBaseSetNameEmitsSignal();
    void _testBaseSetDynamicEmitsSignal();
    void _testBaseSetAutoConnectEmitsSignal();
    void _testBaseSetHighLatencyEmitsSignal();
    void _testBaseDefaults();
    void _testBaseSettingsRoot();
    void _testSuppressAutoReconnectNotPersisted();
    void _testReconnectBackoff();

    // TCPConfiguration
    void _testTcpConstruction();
    void _testTcpSetHostEmitsSignal();
    void _testTcpSetPortEmitsSignal();
    void _testTcpCopyConstruction();
    void _testTcpCopyFrom();
    void _testTcpSettingsRoundtrip();
    void _testTcpHostnameRoundtrip();

    // UDPConfiguration
    void _testUdpConstruction();
    void _testUdpAddRemoveHost();
    void _testUdpSetLocalPortEmitsSignal();
    void _testUdpCopyConstruction();
    void _testUdpCopyFrom();
    void _testUdpSettingsRoundtrip();
    void _testUdpHostnamePreservedWhenUnresolved();
    void _testUdpHostnameRoundtrip();
    void _testUdpRemoveByHostname();
    void _testUdpResolveHostsUpdatesAddress();
};
