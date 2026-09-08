#include "NMEASourceManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtPositioning/QNmeaSatelliteInfoSource>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "NMEASourceManager.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
#include "RTKPositionSource.h"
#include "SettingsManager.h"
#include "UdpIODevice.h"

namespace {
const QByteArray kFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

QByteArray satelliteSentences(int signalStrength = 40)
{
    const QByteArray gsv = "$GPGSV,1,1,02,01,45,100," + QByteArray::number(signalStrength) + ",02,30,200,35";
    return NMEAUtils::repairChecksum(gsv) + NMEAUtils::repairChecksum("$GPGSA,A,3,01,02,,,,,,,,,,,1.0,0.8,0.6");
}
}  // namespace

void NMEASourceManagerTest::init()
{
    UnitTest::init();
    ignoreLogMessage("GPS.PositionManager.QGCPositionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

void NMEASourceManagerTest::_udpSwitchAndDisable()
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
    NMEASourceManager source(settings, &position);
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

void NMEASourceManagerTest::_udpActivityStatus()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->nmeaAutoConnect(), false);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 port = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    const QString listening = QStringLiteral("Listening on UDP port %1").arg(port);
    const QString receiving = QStringLiteral("Receiving UDP data on port %1").arg(port);
    QCOMPARE(source.status(), listening);

    QSignalSpy stateSpy(&source, &NMEASourceManager::stateChanged);
    QUdpSocket sender;
    const QByteArray invalidData("Not a GPS fix\r\n");
    QCOMPARE(sender.writeDatagram(invalidData, QHostAddress::LocalHost, port), invalidData.size());
    QTRY_COMPARE_WITH_TIMEOUT(source.status(), receiving, TestTimeout::mediumMs());
    QVERIFY(!stateSpy.isEmpty());
    QVERIFY(!position.gcsPosition().isValid());

    source._udpActivityTimer.start(1);
    QTRY_COMPARE_WITH_TIMEOUT(source.status(), listening, TestTimeout::mediumMs());
    QVERIFY(source.active());
    source._udpActivityTimer.setInterval(5000);
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, port), kFix.size());
    QTRY_COMPARE_WITH_TIMEOUT(source.status(), receiving, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    source.update();
    QCOMPARE(source.status(), receiving);

    source.disconnectSource();
    QCOMPARE(source.status(), QStringLiteral("Disconnected"));
    QVERIFY(!source._udpActivityTimer.isActive());
    QVERIFY(source.connectSource());
    QCOMPARE(source.status(), listening);
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, port), kFix.size());
    QTRY_COMPARE_WITH_TIMEOUT(source.status(), receiving, TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    QCOMPARE(source.status(), QStringLiteral("Disconnected"));
    QVERIFY(!source._udpActivityTimer.isActive());
}

void NMEASourceManagerTest::_bindFailureAndTeardown()
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
        NMEASourceManager source(settings, &position);
        source.update();
        QVERIFY(!source._sourceInstalled);
        QVERIFY(!source._udp);
        QCOMPARE(source.connectionState(), GPSConnectionState::Retrying);
        occupied.close();
        QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                     source.update();
                                     return source._sourceInstalled;
                                 })(),
                                 TestTimeout::mediumMs());
        QCOMPARE(source.connectionState(), GPSConnectionState::Ready);
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, port), kFix.size());
        QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    }
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(occupied.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));
}

UT_REGISTER_TEST(NMEASourceManagerTest, TestLabel::Unit)

void NMEASourceManagerTest::_satellitesShareUdpAndStayFresh()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 port = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    RTKPositionSource receiver;
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    QCOMPARE(source.satellitesInViewCount(), -1);
    QCOMPARE(source.satellitesInUseCount(), -1);
    source._satellitePollTimer.setInterval(50);
    source._satelliteStaleTimer.setInterval(200);
    QUdpSocket sender;
    const auto send = [&](const QByteArray& data) {
        QCOMPARE(sender.writeDatagram(data, QHostAddress::LocalHost, port), data.size());
    };
    const QByteArray payload = kFix + satelliteSentences();
    const qsizetype split = kFix.size() + 15;
    send(payload.first(split));
    send(payload.sliced(split));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 2, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    const auto satellite = source.satellitesInView().first();
    QCOMPARE(satellite.satelliteIdentifier(), 1);
    QCOMPARE(satellite.satelliteSystem(), QGeoSatelliteInfo::GPS);
    QCOMPARE(satellite.signalStrength(), 40);
    QCOMPARE(satellite.attribute(QGeoSatelliteInfo::Elevation), 45);
    QCOMPARE(satellite.attribute(QGeoSatelliteInfo::Azimuth), 100);

    QSignalSpy responses(source._satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    QTimer timer;
    connect(&timer, &QTimer::timeout, this, [&]() { send(satelliteSentences()); });
    timer.start(40);
    QTRY_VERIFY_WITH_TIMEOUT(responses.size() >= 6, TestTimeout::mediumMs());
    QCOMPARE(source.satellitesInViewCount(), 2);
    timer.stop();
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), -1, TestTimeout::mediumMs());
    QCOMPARE(source.satellitesInUseCount(), -1);
    QVERIFY(source.satellitesInView().isEmpty());
    source._satelliteStaleTimer.setInterval(5000);
    send(kFix);
    QVERIFY(source.satellitesInView().isEmpty());
    send(satelliteSentences());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());

    position.setReceiverPositionSource(&receiver);
    source._satelliteSource->requestUpdate(5000);
    send(satelliteSentences(45));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInView().first().signalStrength(), 45, TestTimeout::mediumMs());
    position.clearReceiverPositionSource(&receiver);
    QVERIFY(!position.gcsPosition().isValid());
    send(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QCOMPARE(source.satellitesInViewCount(), 2);

    source.disconnectSource();
    QCOMPARE(source.satellitesInViewCount(), -1);
    QCOMPARE(source.satellitesInUseCount(), -1);
    QVERIFY(!source._satellitePollTimer.isActive());
    QVERIFY(!source._satelliteStaleTimer.isActive());
    QVERIFY(!source._satelliteSource);
    QVERIFY(!source._stream);
    QVERIFY(source.connectSource());
    QCOMPARE(source.satellitesInViewCount(), -1);
    send(payload);
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    QCOMPARE(source.satellitesInViewCount(), -1);
}

void NMEASourceManagerTest::_satellitesShareTcpConnection()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QSignalSpy connections(&server, &QTcpServer::newConnection);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->nmeaTcpPort(), server.serverPort());
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    QCOMPARE(source.connectionState(), GPSConnectionState::Connecting);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    peer->write(kFix + satelliteSentences());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 2, TestTimeout::mediumMs());
    QCOMPARE(connections.size(), 1);
    source._satelliteSource->requestUpdate(5000);
    peer->write(NMEAUtils::repairChecksum("$GPGSV,1,1,00") +
                NMEAUtils::repairChecksum("$GPGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9"));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 0, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 0, TestTimeout::mediumMs());
    peer->disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(!source._sourceInstalled, TestTimeout::mediumMs());
    QCOMPARE(source.satellitesInViewCount(), -1);
    QVERIFY(!position.gcsPosition().isValid());
}

void NMEASourceManagerTest::_disconnectDuringSatelliteUpdate()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 port = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    connect(&source, &NMEASourceManager::satellitesChanged, this, [&]() {
        if (source.satellitesInViewCount() >= 0 || source.satellitesInUseCount() >= 0) {
            source.disconnectSource();
        }
    });
    QUdpSocket sender;
    sender.writeDatagram(satelliteSentences(), QHostAddress::LocalHost, port);
    QTRY_VERIFY_WITH_TIMEOUT(!source.active(), TestTimeout::mediumMs());
    // Both parser signals can be queued together; the retired source must not restore either list.
    QCoreApplication::sendPostedEvents(&source, QEvent::MetaCall);
    QCOMPARE(source.satellitesInViewCount(), -1);
    QCOMPARE(source.satellitesInUseCount(), -1);
}

void NMEASourceManagerTest::_configuredSerialRoutingSurvivesReconnect()
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
        NMEASourceManager source(settings, &position);
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

void NMEASourceManagerTest::_settingsUseSharedSerialInventory()
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

void NMEASourceManagerTest::_tcpRecoveryAndSourceSwitch()
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
    NMEASourceManager source(settings, &position);
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
    source._connection._retryDeadline.setRemainingTime(0);
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

void NMEASourceManagerTest::_tcpManualAndAutoConnect()
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
    NMEASourceManager source(settings, &position);
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

void NMEASourceManagerTest::_tcpRefusalBackoff()
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
    NMEASourceManager source(settings, &position);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._tcp, TestTimeout::mediumMs());
    QVERIFY(!source._connection._retryDeadline.isForever());
    QCOMPARE(source._connection._retryDelayMs, 1000);
    source.update();
    QVERIFY(!source._tcp);
    source._connection._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._tcp, TestTimeout::mediumMs());
    QCOMPARE(source._connection._retryDelayMs, 2000);
    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    source._connection._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceInstalled, TestTimeout::mediumMs());
    QCOMPARE(source._connection._retryDelayMs, 1000);
}

void NMEASourceManagerTest::_disconnectDuringPositionUpdate()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->nmeaUdpPort(), 0);
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    QPointer<QGeoPositionInfoSource> decoder = source.positionSource();
    QVERIFY(decoder);
    connect(&position, &QGCPositionManager::positionInfoUpdated, this, [&](const QGeoPositionInfo& update) {
        if (update.isValid()) {
            source.disconnectSource();
        }
    });
    QUdpSocket sender;
    sender.writeDatagram(kFix + kFix, QHostAddress::LocalHost, source._udp->localPort());
    QTRY_VERIFY_WITH_TIMEOUT(!source.active(), TestTimeout::mediumMs());
    QVERIFY(!decoder);
    QVERIFY(!source.positionSource());
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!position.gcsPositionTimestamp().isValid());
}
