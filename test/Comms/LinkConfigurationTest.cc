#include "LinkConfigurationTest.h"
#include "LinkConfiguration.h"
#include "UDPLink.h"
#include "TCPLink.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void LinkConfigurationTest::_namePropertyTest()
{
    UDPConfiguration config("TestLink");
    QSignalSpy spy(&config, &LinkConfiguration::nameChanged);

    QCOMPARE(config.name(), QStringLiteral("TestLink"));

    config.setName("NewName");
    QCOMPARE(config.name(), QStringLiteral("NewName"));
    QCOMPARE(spy.count(), 1);

    // Setting same name should not emit signal
    config.setName("NewName");
    QCOMPARE(spy.count(), 1);
}

void LinkConfigurationTest::_dynamicPropertyTest()
{
    UDPConfiguration config("TestLink");
    QSignalSpy spy(&config, &LinkConfiguration::dynamicChanged);

    QVERIFY(!config.isDynamic());

    config.setDynamic(true);
    QVERIFY(config.isDynamic());
    QCOMPARE(spy.count(), 1);

    // Setting same value should not emit signal
    config.setDynamic(true);
    QCOMPARE(spy.count(), 1);

    config.setDynamic(false);
    QVERIFY(!config.isDynamic());
    QCOMPARE(spy.count(), 2);
}

void LinkConfigurationTest::_autoConnectPropertyTest()
{
    UDPConfiguration config("TestLink");
    QSignalSpy spy(&config, &LinkConfiguration::autoConnectChanged);

    QVERIFY(!config.isAutoConnect());

    config.setAutoConnect(true);
    QVERIFY(config.isAutoConnect());
    QCOMPARE(spy.count(), 1);

    // Setting same value should not emit signal
    config.setAutoConnect(true);
    QCOMPARE(spy.count(), 1);
}

void LinkConfigurationTest::_highLatencyPropertyTest()
{
    UDPConfiguration config("TestLink");
    QSignalSpy spy(&config, &LinkConfiguration::highLatencyChanged);

    QVERIFY(!config.isHighLatency());

    config.setHighLatency(true);
    QVERIFY(config.isHighLatency());
    QCOMPARE(spy.count(), 1);

    // Setting same value should not emit signal
    config.setHighLatency(true);
    QCOMPARE(spy.count(), 1);
}

void LinkConfigurationTest::_createSettingsTest()
{
    // Test UDP creation
    LinkConfiguration* udpConfig = LinkConfiguration::createSettings(
        LinkConfiguration::TypeUdp, "UDP Test");
    QVERIFY(udpConfig != nullptr);
    QCOMPARE(udpConfig->type(), LinkConfiguration::TypeUdp);
    QCOMPARE(udpConfig->name(), QStringLiteral("UDP Test"));
    delete udpConfig;

    // Test TCP creation
    LinkConfiguration* tcpConfig = LinkConfiguration::createSettings(
        LinkConfiguration::TypeTcp, "TCP Test");
    QVERIFY(tcpConfig != nullptr);
    QCOMPARE(tcpConfig->type(), LinkConfiguration::TypeTcp);
    delete tcpConfig;

    // Test invalid type returns nullptr
    LinkConfiguration* invalidConfig = LinkConfiguration::createSettings(
        LinkConfiguration::TypeLast, "Invalid");
    QVERIFY(invalidConfig == nullptr);
}

void LinkConfigurationTest::_duplicateSettingsTest()
{
    UDPConfiguration original("OriginalLink");
    original.setDynamic(true);
    original.setAutoConnect(true);
    original.setHighLatency(true);
    original.setLocalPort(14550);

    LinkConfiguration* duplicate = LinkConfiguration::duplicateSettings(&original);
    QVERIFY(duplicate != nullptr);
    QCOMPARE(duplicate->type(), LinkConfiguration::TypeUdp);
    QCOMPARE(duplicate->name(), original.name());
    QCOMPARE(duplicate->isDynamic(), original.isDynamic());
    QCOMPARE(duplicate->isAutoConnect(), original.isAutoConnect());
    QCOMPARE(duplicate->isHighLatency(), original.isHighLatency());

    // Verify it's a UDPConfiguration with correct port
    UDPConfiguration* udpDupe = qobject_cast<UDPConfiguration*>(duplicate);
    QVERIFY(udpDupe != nullptr);
    QCOMPARE(udpDupe->localPort(), original.localPort());

    delete duplicate;
}

void LinkConfigurationTest::_copyFromTest()
{
    UDPConfiguration source("SourceLink");
    source.setDynamic(true);
    source.setAutoConnect(true);
    source.setHighLatency(true);

    UDPConfiguration target("TargetLink");
    target.copyFrom(&source);

    QCOMPARE(target.name(), source.name());
    QCOMPARE(target.isDynamic(), source.isDynamic());
    QCOMPARE(target.isAutoConnect(), source.isAutoConnect());
    QCOMPARE(target.isHighLatency(), source.isHighLatency());
}

void LinkConfigurationTest::_copyConstructorTest()
{
    UDPConfiguration original("OriginalLink");
    original.setDynamic(true);
    original.setAutoConnect(true);
    original.setLocalPort(14551);

    UDPConfiguration copy(&original);

    QCOMPARE(copy.name(), original.name());
    QCOMPARE(copy.isDynamic(), original.isDynamic());
    QCOMPARE(copy.isAutoConnect(), original.isAutoConnect());
    QCOMPARE(copy.localPort(), original.localPort());
}

void LinkConfigurationTest::_settingsRootTest()
{
    QCOMPARE(LinkConfiguration::settingsRoot(), QStringLiteral("LinkConfigurations"));
}
