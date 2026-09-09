#include "NMEASourceManagerTest.h"

#include <QtCore/QBuffer>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtPositioning/QNmeaSatelliteInfoSource>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSReceiverPositionSource.h"
#include "GPSTransport.h"
#include "NMEASourceManager.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
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

void NMEASourceManagerTest::_satelliteSnapshotsPreserveProvenance()
{
    QBuffer device;
    QVERIFY(device.open(QIODevice::ReadOnly));
    NMEADecoderSession decoder;
    QVERIFY(decoder.start(&device));
    const quint64 session = decoder.sessionId();
    QVERIFY(session != 0);
    QGeoSatelliteInfo gps;
    gps.setSatelliteSystem(QGeoSatelliteInfo::GPS);
    gps.setSatelliteIdentifier(3);
    QGeoSatelliteInfo galileo = gps;
    galileo.setSatelliteSystem(QGeoSatelliteInfo::GALILEO);
    const quint64 receipt = GPSObservation::monotonicNowUs() - 100000;
    decoder._viewSnapshot.satellites = {gps, galileo};
    decoder._viewSnapshot.constellationReceipts = {{"GP", receipt}, {"GA", receipt}};
    decoder._useSnapshot.satellites = {gps};
    decoder._useSnapshot.constellationReceipts = {{"GP", receipt}};
    QSignalSpy snapshots(&decoder, &NMEADecoderSession::satellitesChanged);
    decoder._expireSatellites();
    QCOMPARE(decoder.satellitesReceivedAtUs(), receipt);
    QVERIFY(decoder.satellitesUsedKnown());
    QCOMPARE(decoder.satelliteUseSystems(), QSet<int>{QGeoSatelliteInfo::GPS});
    QCOMPARE(decoder.satellitesInView().size(), 2);
    QCOMPARE(snapshots.size(), 1);
    decoder._viewSnapshot.satellites[0].setSignalStrength(42);
    decoder._expireSatellites();
    QCOMPARE(snapshots.size(), 2);
    QCOMPARE(decoder.satellitesReceivedAtUs(), receipt);
    QCOMPARE(decoder.satellitesInView()[0].signalStrength(), 42);
    decoder._useSnapshot.constellationReceipts["GP"] = receipt - 6000000;
    decoder._expireSatellites();
    QVERIFY(!decoder.satellitesUsedKnown());
    QVERIFY(decoder.satelliteUseSystems().isEmpty());
    QCOMPARE(decoder.satellitesInView().size(), 2);
    decoder._health._freshnessTimeoutMs = 200;
    const quint64 freshReceipt = GPSObservation::monotonicNowUs() - 10000;
    decoder._viewSnapshot.constellationReceipts = {{"GP", freshReceipt - 400000}, {"GA", freshReceipt}};
    decoder._useSnapshot.satellites = {gps, galileo};
    decoder._useSnapshot.constellationReceipts = decoder._viewSnapshot.constellationReceipts;
    decoder._expireSatellites();
    QCOMPARE(decoder.satellitesInView().size(), 1);
    QCOMPARE(decoder.satellitesInView().first().satelliteSystem(), QGeoSatelliteInfo::GALILEO);
    QCOMPARE(decoder.satelliteUseSystems(), QSet<int>{QGeoSatelliteInfo::GALILEO});
    QCOMPARE(decoder.satellitesReceivedAtUs(), freshReceipt);
    decoder._health._freshnessTimeoutMs = 5000;
    decoder._viewSnapshot.satellites.append(gps);
    decoder._useSnapshot.satellites.append(gps);
    decoder._viewSnapshot.constellationReceipts["GP"] = freshReceipt - 400000;
    decoder._useSnapshot.constellationReceipts["GP"] = freshReceipt - 400000;
    decoder._expireSatellites();
    QCOMPARE(decoder.satellitesInView().size(), 1);
    QCOMPARE(decoder.satelliteUseSystems(), QSet<int>{QGeoSatelliteInfo::GALILEO});

    decoder._viewSnapshot = {};
    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.0, 8.0, 100.0), QDateTime::currentDateTimeUtc());
    fix.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    fix.satellitesUsed = 8;
    decoder._health.updateObservation(fix);
    decoder._expireSatellites();
    QVERIFY(decoder.satellitesInView().isEmpty());
    QVERIFY(decoder.satellitesUsedKnown());
    QCOMPARE(decoder.health()->satellitesInUseCount(), 8);
    emit decoder._satelliteSource->errorOccurred(QGeoSatelliteInfoSource::UpdateTimeoutError);
    QTRY_VERIFY_WITH_TIMEOUT(!decoder.satellitesUsedKnown(), TestTimeout::shortMs());
    QVERIFY(decoder.satelliteUseSystems().isEmpty());
    QVERIFY(decoder.satellitesInUse().isEmpty());
    QCOMPARE(decoder.satellitesReceivedAtUs(), 0ULL);
    QCOMPARE(decoder.health()->satellitesInUseCount(), 8);
    decoder._useSnapshot.satellites = {galileo};
    decoder._useSnapshot.constellationReceipts = {{"GA", freshReceipt}};
    decoder._expireSatellites();
    QVERIFY(!decoder.satellitesUsedKnown());
    QVERIFY(decoder.satelliteUseSystems().isEmpty());
    decoder.stop();
    QVERIFY(decoder.sessionId() > session);
    QCOMPARE(decoder.satellitesReceivedAtUs(), 0ULL);
    QVERIFY(decoder.satellitesInView().isEmpty());
    QVERIFY(decoder.satelliteUseSystems().isEmpty());
}

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
    QVERIFY(!source._attempt);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    source.update();
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    source.update();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceInstalled);
    QVERIFY(!source._attempt);
}

void NMEASourceManagerTest::_inactiveSettingsKeepConnection()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->nmeaTcpPort(), server.serverPort());
    saved.setFactValue(settings->nmeaAutoConnect(), false);
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    const auto peer = std::unique_ptr<QTcpSocket>(server.nextPendingConnection());
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), GPSConnectionState::Ready, TestTimeout::mediumMs());
    QPointer<QGeoPositionInfoSource> decoder = source.positionSource();
    QVERIFY(decoder);

    saved.setFactValue(settings->nmeaUdpPort(), 3200);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/unused"));
    source.update();
    QCOMPARE(source.positionSource(), decoder.data());
    QCOMPARE(source.connectionState(), GPSConnectionState::Ready);
    QCOMPARE(peer->write(kFix), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());

    settings->nmeaTcpHost()->setRawValue(QString());
    QVERIFY(!source.connectSource());
    QVERIFY(!decoder);
    QVERIFY(!source.positionSource());
    QVERIFY(source.status().contains(QStringLiteral("valid TCP")));
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
        QVERIFY(!source._attempt);
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
    GPSReceiverPositionSource receiver;
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    QVERIFY(source.connectSource());
    QCOMPARE(source.satellitesInViewCount(), -1);
    QCOMPARE(source.satellitesInUseCount(), -1);
    source._decoder._satellitePollTimer.setInterval(50);
    source._decoder._health._freshnessTimeoutMs = 200;
    QUdpSocket sender;
    const auto send = [&](const QByteArray& data) {
        QCOMPARE(sender.writeDatagram(data, QHostAddress::LocalHost, port), data.size());
    };
    const QByteArray payload = kFix + satelliteSentences();
    const qsizetype split = kFix.size() + 15;
    send(payload.first(split));
    send(payload.sliced(split));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 8, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    const auto satellite = source.satellitesInView().first();
    QCOMPARE(satellite.satelliteIdentifier(), 1);
    QCOMPARE(satellite.satelliteSystem(), QGeoSatelliteInfo::GPS);
    QCOMPARE(satellite.signalStrength(), 40);
    QCOMPARE(satellite.attribute(QGeoSatelliteInfo::Elevation), 45);
    QCOMPARE(satellite.attribute(QGeoSatelliteInfo::Azimuth), 100);

    QSignalSpy responses(source._decoder._satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    QTimer timer;
    connect(&timer, &QTimer::timeout, this, [&]() { send(satelliteSentences()); });
    timer.start(40);
    QTRY_VERIFY_WITH_TIMEOUT(responses.size() >= 6, TestTimeout::mediumMs());
    QCOMPARE(source.satellitesInViewCount(), 2);
    QCOMPARE(position.sourceHealth(), source.health());
    QTRY_COMPARE_WITH_TIMEOUT(source.health()->state(), GPSSourceHealth::Stale, TestTimeout::mediumMs());
    QVERIFY(!position.gcsPosition().isValid());
    QCOMPARE(source.connectionState(), GPSConnectionState::Ready);
    timer.stop();
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), -1, TestTimeout::mediumMs());
    QCOMPARE(source.satellitesInUseCount(), -1);
    QVERIFY(source.satellitesInView().isEmpty());
    QVERIFY(source.satellitesInUse().isEmpty());
    QCOMPARE(source.satellitesReceivedAtUs(), 0ULL);
    QVERIFY(!source.satellitesUsedKnown());
    QVERIFY(source.satelliteUseSystems().isEmpty());
    source._decoder._health._freshnessTimeoutMs = 5000;
    send(kFix);
    QVERIFY(source.satellitesInView().isEmpty());
    send(satelliteSentences());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());

    position.setReceiverPositionSource(&receiver);
    source._decoder._satelliteSource->requestUpdate(5000);
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
    QVERIFY(!source._decoder._satellitePollTimer.isActive());
    QVERIFY(!source._decoder._health._satellitesInViewTimer.isActive());
    QVERIFY(!source._decoder._satelliteSource);
    QVERIFY(!source._decoder._stream);
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
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 8, TestTimeout::mediumMs());
    QCOMPARE(connections.size(), 1);
    source._decoder._satelliteSource->requestUpdate(5000);
    peer->write(NMEAUtils::repairChecksum("$GPGSV,1,1,00") +
                NMEAUtils::repairChecksum("$GPGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9"));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 0, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(source.satellitesInUse().isEmpty(), TestTimeout::mediumMs());
    // The fresh GGA total remains authoritative while the GSA satellite list changes.
    QCOMPARE(source.satellitesInUseCount(), 8);
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
    auto* receiverMode = root->findChild<QObject*>(QStringLiteral("nmeaReceiverMode"));
    QVERIFY(receiverMode);
    QVERIFY(!receiverMode->property("visible").toBool());
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverPassive);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
    QVERIFY(receiverMode->property("visible").toBool());
    QVERIFY(baudCombo->property("visible").toBool());
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverUblox);
    QVERIFY(!baudCombo->property("visible").toBool());
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverPassive);
    QVERIFY(baudCombo->property("visible").toBool());

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
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(source.active());
    source.update();
    QVERIFY(!source._attempt);
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
    QVERIFY(source._attempt);
    QCOMPARE(source._attempt->localPort(), port);
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    QVERIFY(!position.gcsPosition().isValid());
    QUdpSocket sender;
    sender.writeDatagram(kFix, QHostAddress::LocalHost, port);
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    source.disconnectSource();
    QVERIFY(!position.gcsPosition().isValid());
    source.update();
    QVERIFY(!source.active());
    QVERIFY(!source._attempt);
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
    QVERIFY(!source._attempt);
    settings->nmeaTcpHost()->setRawValue(QStringLiteral("localhost"));
    QVERIFY(source.connectSource());
    source.disconnectSource();
    source.update();
    QVERIFY(!source._attempt);
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
    QVERIFY(!source._attempt);
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
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
    QVERIFY(!source._connection._retryDeadline.isForever());
    QCOMPARE(source._connection._retryDelayMs, 1000);
    source.update();
    QVERIFY(!source._attempt);
    source._connection._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
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
    source.update();
    QCOMPARE(source.positionSource(), decoder.data());
    connect(&position, &QGCPositionManager::positionInfoUpdated, this, [&](const QGeoPositionInfo& update) {
        if (update.isValid()) {
            source.disconnectSource();
        }
    });
    QUdpSocket sender;
    sender.writeDatagram(kFix + kFix, QHostAddress::LocalHost, source._attempt->localPort());
    QTRY_VERIFY_WITH_TIMEOUT(!source.active(), TestTimeout::mediumMs());
    QVERIFY(!decoder);
    QVERIFY(!source.positionSource());
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!position.gcsPositionTimestamp().isValid());
}

void NMEASourceManagerTest::_managedReceiverFailureAndCancellation()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->nmeaAutoConnect(), false);
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverUblox);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/injected-nmea"));
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    auto lease = std::make_shared<int>(1);
    const std::weak_ptr<int> weakLease = lease;
    auto attempts = std::make_shared<std::atomic_int>(0);
    source._receiverFactory = [lease, attempts](const std::atomic_bool&) -> std::unique_ptr<GPSTransport> {
        ++*attempts;
        return {};
    };
    QVERIFY(source.connectSource());
    lease.reset();
    QCOMPARE(source.connectionState(), GPSConnectionState::Connecting);
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), GPSConnectionState::Retrying, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
    QCOMPARE(source.status(),
             QStringLiteral("Cannot configure receiver for NMEA: Cannot open the receiver connection"));
    QVERIFY(!source.positionSource());
    QCOMPARE(attempts->load(), 1);
    QCOMPARE(source._connection._retryDelayMs, 1000);
    QVERIFY(!weakLease.expired());

    source.update();
    QCOMPARE(attempts->load(), 1);
    source._connection._retryDeadline.setRemainingTime(0);
    source.update();
    QTRY_COMPARE_WITH_TIMEOUT(attempts->load(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), GPSConnectionState::Retrying, TestTimeout::mediumMs());
    QCOMPARE(source._connection._retryDelayMs, 2000);
    QVERIFY(!weakLease.expired());

    source.disconnectSource();
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(weakLease.expired(), TestTimeout::mediumMs());
    QCOMPARE(source.connectionState(), GPSConnectionState::Disconnected);
    QCOMPARE(source.status(), QStringLiteral("Disconnected"));
    QVERIFY(!source.active());
    QVERIFY(!source.positionSource());
    QVERIFY(!source._receiverFactory);
    source.update();
    QCOMPARE(attempts->load(), 2);
}

void NMEASourceManagerTest::_managedReceiverSettingsPreservePassiveBaud()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverUblox);
    saved.setFactValue(settings->autoConnectNmeaBaud(), 4800);
    QGCPositionManager position;
    {
        NMEASourceManager source(settings, &position);
        QCOMPARE(source._config.receiverMode, NMEAConnectionConfig::Ublox);
        source.disconnectSource();
        settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverPassive);
        QCOMPARE(source._config.baud, 4800);
    }
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverUblox);
    NMEASourceManager restarted(settings, &position);
    QCOMPARE(restarted._config.receiverMode, NMEAConnectionConfig::Ublox);
    QCOMPARE(settings->autoConnectNmeaBaud()->rawValue().toInt(), 4800);
}

void NMEASourceManagerTest::_managedModeChangeCancelsRetry()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->nmeaAutoConnect(), true);
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverUblox);
    saved.setFactValue(settings->autoConnectNmeaBaud(), 4800);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/injected-nmea"));
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    auto lease = std::make_shared<int>(1);
    const std::weak_ptr<int> weakLease = lease;
    source._receiverFactory = [lease](const std::atomic_bool&) -> std::unique_ptr<GPSTransport> { return {}; };
    QVERIFY(source.connectSource());
    lease.reset();
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), GPSConnectionState::Retrying, TestTimeout::mediumMs());
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverPassive);
    QTRY_VERIFY_WITH_TIMEOUT(weakLease.expired(), TestTimeout::mediumMs());
    QVERIFY(!source._receiverFactory);
    QVERIFY(source._shouldConnect());
    QCOMPARE(source._config.baud, 4800);
    source.disconnectSource();
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverUblox);
    QVERIFY(!source._shouldConnect());
    QVERIFY(!source._receiverFactory);
    QCOMPARE(source.status(), QStringLiteral("Automatic connection paused"));
}

void NMEASourceManagerTest::_reentrantConnectionCommands_data()
{
    QTest::addColumn<int>("sourceType");
    QTest::addColumn<QString>("command");
    for (const auto& command : {QStringLiteral("disconnect"), QStringLiteral("disable"), QStringLiteral("destroy")}) {
        QTest::newRow(qPrintable(QStringLiteral("tcp-") + command))
            << int(AutoConnectSettings::NmeaSourceTcp) << command;
        QTest::newRow(qPrintable(QStringLiteral("udp-") + command))
            << int(AutoConnectSettings::NmeaSourceUdp) << command;
    }
}

void NMEASourceManagerTest::_reentrantConnectionCommands()
{
    QFETCH(int, sourceType);
    QFETCH(QString, command);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->nmeaSource(), sourceType);
    saved.setFactValue(settings->nmeaAutoConnect(), false);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->nmeaTcpPort(), server.serverPort());
    saved.setFactValue(settings->nmeaUdpPort(), 0);
    QGCPositionManager position;
    auto source = std::make_unique<NMEASourceManager>(settings, &position);
    bool applied = false;
    connect(source.get(), &NMEASourceManager::stateChanged, &position, [&]() {
        if (applied || !source || !source->active()) {
            return;
        }
        applied = true;
        if (command == QStringLiteral("disconnect")) {
            source->disconnectSource();
        } else if (command == QStringLiteral("disable")) {
            settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
        } else {
            source.reset();
        }
    });
    source->connectSource();
    QVERIFY(applied);
    if (source) {
        QVERIFY(!source->active());
        QVERIFY(!source->positionSource());
        QVERIFY(!source->_attempt);
        QCOMPARE(source->connectionState(), GPSConnectionState::Disconnected);
    }
    QVERIFY(!position.gcsPosition().isValid());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void NMEASourceManagerTest::_managedAttemptRetainsReservationWhileStopping()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/injected-nmea"));
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverUblox);
    saved.setFactValue(settings->nmeaAutoConnect(), false);
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    auto lease = std::make_shared<int>(1);
    const std::weak_ptr<int> reservation = lease;
    auto entered = std::make_shared<QSemaphore>();
    auto release = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([release]() { release->release(); });
    source._receiverFactory = [lease = std::move(lease), entered, release](const std::atomic_bool&) {
        entered->release();
        release->acquire();
        return std::unique_ptr<GPSTransport>();
    };
    QVERIFY(source.connectSource());
    QTRY_VERIFY_WITH_TIMEOUT(entered->available() > 0, TestTimeout::mediumMs());
    source.disconnectSource();
    QVERIFY(!source.active());
    QCOMPARE(source.connectionState(), GPSConnectionState::Stopping);
    QVERIFY(!reservation.expired());
    source.update();
    QCOMPARE(entered->available(), 1);
    release->release();
    QTRY_VERIFY_WITH_TIMEOUT(reservation.expired(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), GPSConnectionState::Disconnected, TestTimeout::mediumMs());
    QVERIFY(!source._attempt);
}

void NMEASourceManagerTest::_injectedSerialDiscovery()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Serial discovery is unavailable in this build");
#else
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    const QString device = QStringLiteral("/test/injected-nmea");
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), device);
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverPassive);
    saved.setFactValue(settings->autoConnectNmeaBaud(), 9600);
    saved.setFactValue(settings->nmeaAutoConnect(), true);
    int enumerations = 0;
    SerialPortManager inventory(nullptr, [&]() {
        ++enumerations;
        return QList<SerialPortManager::Port>();
    });
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    source.setSerialDiscovery(&inventory);
    QVERIFY(inventory.isAutoConnectExcluded(device));
    source.update();
    QVERIFY(enumerations > 0);
    QCOMPARE(source.status(), QStringLiteral("Waiting for serial device"));
    source.setSerialDiscovery(nullptr);
    QVERIFY(!inventory.isAutoConnectExcluded(device));
    const int previousEnumerations = enumerations;
    source.update();
    QCOMPARE(enumerations, previousEnumerations);
    QVERIFY(!source._attempt);
    QCOMPARE(source.status(), QStringLiteral("Serial discovery is unavailable"));
    source.setSerialDiscovery(&inventory);
    QVERIFY(inventory.isAutoConnectExcluded(device));
    source.disconnectSource();
    QVERIFY(!inventory.isAutoConnectExcluded(device));
#endif
}
