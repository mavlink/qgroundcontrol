#include "LinkManagerTest.h"

#include "LinkManager.h"
#include "MockLink.h"

#include <QtTest/QTest>

SharedLinkConfigurationPtr LinkManagerTest::_addMockConfig(const QString &name, bool dynamic, bool autoConnect)
{
    MockConfiguration *const mockConfig = new MockConfiguration(name);
    mockConfig->setDynamic(dynamic);
    mockConfig->setAutoConnect(autoConnect);

    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(mockConfig);
    config->setAutoConnectStarted(true);
    if (!linkManager()->createConnectedLink(config)) {
        linkManager()->removeConfiguration(config.get());
        return nullptr;
    }
    return config;
}

void LinkManagerTest::_reconnect()
{
    linkManager()->_reconnectAutoConnectLinks();
}

void LinkManagerTest::_testReconnectsDroppedAutoConnectLink()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("ReconnectMock"), false /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());

    _reconnect();
    QVERIFY(config->link());

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testSuppressedLinkNotReconnected()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("SuppressMock"), false /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    // Manual disconnect sets suppressAutoReconnect so the timer leaves it alone.
    linkManager()->disconnectLink(config->link());
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    QVERIFY(config->suppressAutoReconnect());

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testDynamicLinkNotReconnected()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("DynamicMock"), true /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testNonAutoConnectLinkNotReconnected()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("ManualMock"), false /*dynamic*/, false /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->link());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testNeverStartedLinkNotConnected()
{
    MockConfiguration *const mockConfig = new MockConfiguration(QStringLiteral("NeverStartedMock"));
    mockConfig->setDynamic(false);
    mockConfig->setAutoConnect(true);
    SharedLinkConfigurationPtr config = linkManager()->addConfiguration(mockConfig);
    QVERIFY(!config->autoConnectStarted());
    QVERIFY(config->link() == nullptr);

    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

void LinkManagerTest::_testLinkActiveStableAcrossReconnect()
{
    SharedLinkConfigurationPtr config = _addMockConfig(QStringLiteral("ActiveMock"), false /*dynamic*/, true /*autoConnect*/);
    QVERIFY(config);
    QVERIFY(config->linkActive());

    config->link()->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(config->link() == nullptr, TestTimeout::mediumMs());
    QVERIFY(config->linkActive());

    linkManager()->disconnectLinkConfiguration(config.get());
    QVERIFY(!config->linkActive());
    _reconnect();
    QVERIFY(config->link() == nullptr);

    linkManager()->removeConfiguration(config.get());
}

UT_REGISTER_TEST(LinkManagerTest, TestLabel::Integration, TestLabel::Comms)
