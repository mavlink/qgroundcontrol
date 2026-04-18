#include "LinkConfigurationTest.h"

#include "LinkConfiguration.h"
#include "TCPLink.h"
#include "UDPLink.h"

#include "Fixtures/RAIIFixtures.h"

#include <QtCore/QSettings>

// ============================================================================
// LinkConfiguration base tests (exercised via TCPConfiguration)
// ============================================================================

void LinkConfigurationTest::_testBaseSetNameEmitsSignal()
{
    TCPConfiguration config(QStringLiteral("OriginalName"));
    QSignalSpy spy(&config, &LinkConfiguration::nameChanged);

    config.setName(QStringLiteral("NewName"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(config.name(), QStringLiteral("NewName"));

    // Same name must not re-emit
    config.setName(QStringLiteral("NewName"));
    QCOMPARE(spy.count(), 1);
}

void LinkConfigurationTest::_testBaseSetDynamicEmitsSignal()
{
    TCPConfiguration config(QStringLiteral("DynTest"));
    QSignalSpy spy(&config, &LinkConfiguration::dynamicChanged);

    config.setDynamic(true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(config.isDynamic());

    // Same value must not re-emit
    config.setDynamic(true);
    QCOMPARE(spy.count(), 1);

    config.setDynamic(false);
    QCOMPARE(spy.count(), 2);
    QVERIFY(!config.isDynamic());
}

void LinkConfigurationTest::_testBaseSetAutoConnectEmitsSignal()
{
    TCPConfiguration config(QStringLiteral("AutoConnTest"));
    QSignalSpy spy(&config, &LinkConfiguration::autoConnectChanged);

    config.setAutoConnect(true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(config.isAutoConnect());

    config.setAutoConnect(true);
    QCOMPARE(spy.count(), 1);

    config.setAutoConnect(false);
    QCOMPARE(spy.count(), 2);
}

void LinkConfigurationTest::_testBaseSetHighLatencyEmitsSignal()
{
    TCPConfiguration config(QStringLiteral("HLTest"));
    QSignalSpy spy(&config, &LinkConfiguration::highLatencyChanged);

    config.setHighLatency(true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(config.isHighLatency());

    config.setHighLatency(true);
    QCOMPARE(spy.count(), 1);

    config.setHighLatency(false);
    QCOMPARE(spy.count(), 2);
}

void LinkConfigurationTest::_testBaseDefaults()
{
    TCPConfiguration config(QStringLiteral("DefaultsTest"));

    QVERIFY(!config.isDynamic());
    QVERIFY(!config.isAutoConnect());
    QVERIFY(!config.isHighLatency());
    QVERIFY(!config.isForwarding());
    QVERIFY(config.link() == nullptr);
}

void LinkConfigurationTest::_testBaseSettingsRoot()
{
    QCOMPARE(LinkConfiguration::settingsRoot(), QStringLiteral("LinkConfigurations"));
}

// ============================================================================
// TCPConfiguration tests
// ============================================================================

void LinkConfigurationTest::_testTcpConstruction()
{
    TCPConfiguration config(QStringLiteral("TestTCP"));

    QCOMPARE(config.name(), QStringLiteral("TestTCP"));
    QCOMPARE(config.type(), LinkConfiguration::TypeTcp);
    QCOMPARE(config.port(), quint16(5760));
    QVERIFY(config.host().isEmpty());
}

void LinkConfigurationTest::_testTcpSetHostEmitsSignal()
{
    TCPConfiguration config(QStringLiteral("TCPHostSignal"));
    QSignalSpy spy(&config, &TCPConfiguration::hostChanged);

    config.setHost(QStringLiteral("192.168.1.1"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(config.host(), QStringLiteral("192.168.1.1"));

    // Same value must not re-emit
    config.setHost(QStringLiteral("192.168.1.1"));
    QCOMPARE(spy.count(), 1);

    // setHost trims whitespace
    config.setHost(QStringLiteral("  10.0.0.1  "));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(config.host(), QStringLiteral("10.0.0.1"));
}

void LinkConfigurationTest::_testTcpSetPortEmitsSignal()
{
    TCPConfiguration config(QStringLiteral("TCPPortSignal"));
    QSignalSpy spy(&config, &TCPConfiguration::portChanged);

    config.setPort(14550);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(config.port(), quint16(14550));

    // Same value must not re-emit
    config.setPort(14550);
    QCOMPARE(spy.count(), 1);
}

void LinkConfigurationTest::_testTcpCopyConstruction()
{
    TCPConfiguration original(QStringLiteral("TCPCopyOrig"));
    original.setHost(QStringLiteral("192.168.0.10"));
    original.setPort(5700);
    original.setDynamic(true);
    original.setHighLatency(true);

    TCPConfiguration copy(&original);

    QCOMPARE(copy.name(), QStringLiteral("TCPCopyOrig"));
    QCOMPARE(copy.host(), QStringLiteral("192.168.0.10"));
    QCOMPARE(copy.port(), quint16(5700));
    QVERIFY(copy.isDynamic());
    QVERIFY(copy.isHighLatency());
}

void LinkConfigurationTest::_testTcpCopyFrom()
{
    TCPConfiguration source(QStringLiteral("TCPSource"));
    source.setHost(QStringLiteral("10.0.0.5"));
    source.setPort(14550);

    TCPConfiguration dest(QStringLiteral("TCPDest"));
    dest.copyFrom(&source);

    QCOMPARE(dest.name(), QStringLiteral("TCPSource"));
    QCOMPARE(dest.host(), QStringLiteral("10.0.0.5"));
    QCOMPARE(dest.port(), quint16(14550));
}

void LinkConfigurationTest::_testTcpSettingsRoundtrip()
{
    // Isolate the test from the real application QSettings store: use an
    // IniFormat file inside a TempDirFixture so a crash/QFAIL cannot leave
    // LinkConfigTest_* keys in the user's config.
    TestFixtures::TempDirFixture tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString iniPath = tmpDir.path() + QStringLiteral("/settings.ini");

    const QString root = QStringLiteral("LinkConfigTest_TCP");
    QSettings settings(iniPath, QSettings::IniFormat);

    {
        TCPConfiguration config(QStringLiteral("TCPSave"));
        config.setHost(QStringLiteral("172.16.0.1"));
        config.setPort(5762);
        config.saveSettings(settings, root);
    }

    {
        TCPConfiguration config(QStringLiteral("TCPLoad"));
        config.loadSettings(settings, root);
        QCOMPARE(config.host(), QStringLiteral("172.16.0.1"));
        QCOMPARE(config.port(), quint16(5762));
    }
}

// ============================================================================
// UDPConfiguration tests
// ============================================================================

void LinkConfigurationTest::_testUdpConstruction()
{
    UDPConfiguration config(QStringLiteral("TestUDP"));

    QCOMPARE(config.name(), QStringLiteral("TestUDP"));
    QCOMPARE(config.type(), LinkConfiguration::TypeUdp);
    QCOMPARE(config.localPort(), quint16(0));
    QVERIFY(config.hostList().isEmpty());
    QVERIFY(config.targetHosts().isEmpty());
}

void LinkConfigurationTest::_testUdpAddRemoveHost()
{
    UDPConfiguration config(QStringLiteral("UDPHosts"));
    QSignalSpy spy(&config, &UDPConfiguration::hostListChanged);

    // addHost(host, port) with a numeric IP resolves immediately without DNS
    config.addHost(QStringLiteral("127.0.0.1"), 14550);
    QCOMPARE(config.targetHosts().size(), 1);
    QCOMPARE(spy.count(), 1);

    // Adding the same host/port must be a no-op
    config.addHost(QStringLiteral("127.0.0.1"), 14550);
    QCOMPARE(config.targetHosts().size(), 1);
    QCOMPARE(spy.count(), 1);

    // addHost with "host:port" string format
    config.addHost(QStringLiteral("192.168.1.2:5760"));
    QCOMPARE(config.targetHosts().size(), 2);
    QCOMPARE(spy.count(), 2);

    // hostList reflects the two entries
    QCOMPARE(config.hostList().size(), 2);

    // removeHost(host, port)
    config.removeHost(QStringLiteral("127.0.0.1"), 14550);
    QCOMPARE(config.targetHosts().size(), 1);
    QCOMPARE(spy.count(), 3);

    // removeHost with "host:port" string format
    config.removeHost(QStringLiteral("192.168.1.2:5760"));
    QCOMPARE(config.targetHosts().size(), 0);
    QCOMPARE(spy.count(), 4);
    QVERIFY(config.hostList().isEmpty());
}

void LinkConfigurationTest::_testUdpSetLocalPortEmitsSignal()
{
    UDPConfiguration config(QStringLiteral("UDPPort"));
    QSignalSpy spy(&config, &UDPConfiguration::localPortChanged);

    config.setLocalPort(14550);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(config.localPort(), quint16(14550));

    // Same value must not re-emit
    config.setLocalPort(14550);
    QCOMPARE(spy.count(), 1);

    config.setLocalPort(5760);
    QCOMPARE(spy.count(), 2);
}

void LinkConfigurationTest::_testUdpCopyConstruction()
{
    UDPConfiguration original(QStringLiteral("UDPCopyOrig"));
    original.setLocalPort(14550);
    original.addHost(QStringLiteral("10.0.0.1"), 14550);
    original.addHost(QStringLiteral("10.0.0.2"), 5760);

    UDPConfiguration copy(&original);

    QCOMPARE(copy.name(), QStringLiteral("UDPCopyOrig"));
    QCOMPARE(copy.localPort(), quint16(14550));
    QCOMPARE(copy.targetHosts().size(), 2);
    QCOMPARE(copy.hostList().size(), 2);
}

void LinkConfigurationTest::_testUdpCopyFrom()
{
    UDPConfiguration source(QStringLiteral("UDPSource"));
    source.setLocalPort(5760);
    source.addHost(QStringLiteral("192.168.1.1"), 14550);

    UDPConfiguration dest(QStringLiteral("UDPDest"));
    dest.copyFrom(&source);

    QCOMPARE(dest.name(), QStringLiteral("UDPSource"));
    QCOMPARE(dest.localPort(), quint16(5760));
    QCOMPARE(dest.targetHosts().size(), 1);

    const auto targets = dest.targetHosts();
    const auto &target = targets.constFirst();
    QCOMPARE(target->address.toString(), QStringLiteral("192.168.1.1"));
    QCOMPARE(target->port, quint16(14550));
}

void LinkConfigurationTest::_testUdpSettingsRoundtrip()
{
    // See _testTcpSettingsRoundtrip for rationale on the IniFormat isolation.
    TestFixtures::TempDirFixture tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString iniPath = tmpDir.path() + QStringLiteral("/settings.ini");

    const QString root = QStringLiteral("LinkConfigTest_UDP");
    QSettings settings(iniPath, QSettings::IniFormat);

    {
        UDPConfiguration config(QStringLiteral("UDPSave"));
        config.setLocalPort(14550);
        config.addHost(QStringLiteral("192.168.10.1"), 14550);
        config.addHost(QStringLiteral("192.168.10.2"), 5760);
        config.saveSettings(settings, root);
    }

    {
        UDPConfiguration config(QStringLiteral("UDPLoad"));
        // Pre-populate the port key so loadSettings doesn't fall back to
        // SettingsManager (which is not available in unit tests). saveSettings
        // writes "port" unconditionally, so the value is already present.
        config.loadSettings(settings, root);

        QCOMPARE(config.localPort(), quint16(14550));
        QCOMPARE(config.targetHosts().size(), 2);
    }
}

UT_REGISTER_TEST(LinkConfigurationTest, TestLabel::Unit, TestLabel::Comms)
