#include "NmeaSourceManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "NmeaSourceManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "UdpIODevice.h"

namespace {
const QByteArray kFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";
}

void NmeaSourceManagerTest::init()
{
    UnitTest::init();
    ignoreLogMessage("GPS.PositionManager.QGCPositionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

void NmeaSourceManagerTest::_udpSwitchAndDisable()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 firstPort = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), firstPort);
    QGCPositionManager position;
    NmeaSourceManager source(settings, &position);
    source.update();
    QVERIFY(source._sourceInstalled);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(position.gcsPosition().latitude() - 53.361337) < 0.0001);
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 secondPort = spare.localPort();
    spare.close();
    settings->nmeaUdpPort()->setRawValue(secondPort);
    source.update();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(source._sourceInstalled);
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QVERIFY(spare.bind(QHostAddress::LocalHost, firstPort, QUdpSocket::DontShareAddress));
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/missing-nmea"));
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
    source.update();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceInstalled);
    QVERIFY(!source._udp);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    source.update();
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    source.update();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceInstalled);
    QVERIFY(!source._udp);
}

void NmeaSourceManagerTest::_bindFailureAndTeardown()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket occupied;
    QVERIFY(occupied.bind(QHostAddress::AnyIPv4, 0, QUdpSocket::DontShareAddress));
    const quint16 port = occupied.localPort();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    QGCPositionManager position;
    {
        NmeaSourceManager source(settings, &position);
        source.update();
        QVERIFY(!source._sourceInstalled);
        QVERIFY(!source._udp);
        occupied.close();
        source.update();
        QVERIFY(source._sourceInstalled);
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, port), kFix.size());
        QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    }
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(occupied.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));
}

UT_REGISTER_TEST(NmeaSourceManagerTest, TestLabel::Unit)

void NmeaSourceManagerTest::_configuredSerialRoutingSurvivesReconnect()
{
#ifndef QGC_NO_SERIAL_LINK
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* ports = SerialPortManager::instance();
    const QString first = QStringLiteral("/test/nmea-first");
    const QString second = QStringLiteral("/test/nmea-second");
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), first);
    QGCPositionManager position;
    {
        NmeaSourceManager source(settings, &position);
        QVERIFY(!ports->canAutoConnectPort(first));
        QVERIFY(!ports->isPortReserved(first));
        source.update();
        QVERIFY(!source._sourceInstalled);
        source.stop();
        QVERIFY(!ports->canAutoConnectPort(first));
        settings->autoConnectNmeaPort()->setRawValue(second);
        QVERIFY(ports->canAutoConnectPort(first));
        QVERIFY(!ports->canAutoConnectPort(second));
        settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
        QVERIFY(ports->canAutoConnectPort(second));
        settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
        QVERIFY(!ports->canAutoConnectPort(second));
    }
    QVERIFY(ports->canAutoConnectPort(second));
#else
    QSKIP("Serial routing is unavailable in this build");
#endif
}

void NmeaSourceManagerTest::_settingsUseSharedSerialInventory()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->autoConnectNmeaBaud(), 123457);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/saved-nmea"));
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NmeaGpsSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    auto* portCombo = root->findChild<QObject*>(QStringLiteral("nmeaPortCombo"));
    QVERIFY(portCombo);
    auto* baudCombo = root->findChild<QObject*>(QStringLiteral("nmeaBaudCombo"));
    QVERIFY(baudCombo);
    auto* customBaud = root->findChild<QObject*>(QStringLiteral("customNmeaBaudField"));
    QVERIFY(customBaud);
    QVERIFY(baudCombo->property("isCustomBaud").toBool());
    QCOMPARE(customBaud->property("text").toString(), QStringLiteral("123457"));
    QCOMPARE(settings->autoConnectNmeaBaud()->rawValue().toInt(), 123457);
    QCOMPARE(settings->autoConnectNmeaPort()->rawValue().toString(), QStringLiteral("/test/saved-nmea"));

    QQmlExpression portCount(qmlContext(root.get()), root.get(), QStringLiteral("_serialPorts.length"));
    const int count = portCount.evaluate().toInt();
    QVERIFY(!portCount.hasError());
    QCOMPARE(portCombo->property("enabled").toBool(), count > 0);
    auto* manager = root->property("_serialPortManager").value<QObject*>();
#ifndef QGC_NO_SERIAL_LINK
    QCOMPARE(manager, SerialPortManager::instance());
    QCOMPARE(count, SerialPortManager::instance()->serialPorts().size());
#else
    QVERIFY(!manager);
#endif
}

void NmeaSourceManagerTest::_tcpRecoveryAndSourceSwitch()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->nmeaAutoConnect(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->nmeaTcpPort(), server.serverPort());
    QGCPositionManager position;
    NmeaSourceManager source(settings, &position);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    peer->write(kFix.first(20));
    QVERIFY(peer->waitForBytesWritten());
    QVERIFY(!position.gcsPosition().isValid());
    peer->write(kFix.mid(20));
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(position.gcsPosition().latitude() - 53.361337) < 0.0001);
    peer->disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(!source._tcp, TestTimeout::mediumMs());
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(source.active());
    source.update();
    QVERIFY(!source._tcp);
    source._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    peer.reset(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    peer->write(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 port = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    source.update();
    QVERIFY(!source._tcp);
    QVERIFY(!position.gcsPosition().isValid());
    QUdpSocket sender;
    sender.writeDatagram(kFix, QHostAddress::LocalHost, port);
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    source.disconnectSource();
    QVERIFY(!position.gcsPosition().isValid());
    source.update();
    QVERIFY(!source.active());
    QVERIFY(!source._udp);
    QVERIFY(settings->nmeaAutoConnect()->rawValue().toBool());
}

void NmeaSourceManagerTest::_tcpManualAndAutoConnect()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->nmeaAutoConnect(), false);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("tcp://invalid"));
    saved.setFactValue(settings->nmeaTcpPort(), server.serverPort());
    QGCPositionManager position;
    NmeaSourceManager source(settings, &position);
    source.update();
    QVERIFY(!source.active());
    QVERIFY(!source.connectSource());
    QVERIFY(!source._tcp);
    settings->nmeaTcpHost()->setRawValue(QStringLiteral("localhost"));
    QVERIFY(source.connectSource());
    source.disconnectSource();
    source.update();
    QVERIFY(!source._tcp);
    QVERIFY(!source.active());
    settings->nmeaAutoConnect()->setRawValue(true);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    QVERIFY(source.active());
    source.disconnectSource();
    source.update();
    QVERIFY(!source.active());
    QVERIFY(source.connectSource());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    settings->nmeaAutoConnect()->setRawValue(false);
    QVERIFY(!source.active());
    QVERIFY(!source._tcp);
    source.update();
    QVERIFY(!source.active());
    QVERIFY(source.connectSource());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    QVERIFY(!settings->nmeaAutoConnect()->rawValue().toBool());
    source.update();
    QVERIFY(source.active());
}

void NmeaSourceManagerTest::_tcpRefusalBackoff()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    saved.setFactValue(settings->nmeaAutoConnect(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->nmeaTcpPort(), port);
    QGCPositionManager position;
    NmeaSourceManager source(settings, &position);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._tcp, TestTimeout::mediumMs());
    QVERIFY(!source._retryDeadline.isForever());
    QCOMPARE(source._retryDelayMs, 2000);
    source.update();
    QVERIFY(!source._tcp);
    source._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._tcp, TestTimeout::mediumMs());
    QCOMPARE(source._retryDelayMs, 4000);
    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    source._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    QCOMPARE(source._retryDelayMs, 1000);
}
