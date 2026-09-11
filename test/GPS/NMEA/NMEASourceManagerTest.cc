#ifndef QGC_NO_SERIAL_LINK
#include "GPSSerialPortRegistry.h"
#endif
#include <QtCore/QBuffer>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QTime>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSSettings.h"
#include "GPSTransport.h"
#include "ManualScheduler.h"
#include "NMEASourceManager.h"
#include "NMEASourceManagerTest.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "TestGPSPositionSource.h"
#include "UdpIODevice.h"

namespace {

void configureSource(NMEASourceManager& source, AutoConnectSettings* settings)
{
    const auto apply = [&source, settings]() {
        source.setProfile(GPSSettings::nmea(*settings).profile);
        source.setAutoConnect(settings->nmeaAutoConnect()->rawValue().toBool());
    };
    for (Fact* fact : {settings->nmeaSource(), settings->autoConnectNmeaPort(), settings->autoConnectNmeaBaud(),
                       settings->nmeaUdpPort(), settings->nmeaTcpHost(), settings->nmeaTcpPort(),
                       settings->nmeaReceiverMode(), settings->nmeaAutoConnect()}) {
        QObject::connect(fact, &Fact::rawValueChanged, &source, apply);
    }
    apply();
}

void attachPosition(NMEASourceManager& source, QGCPositionManager& position)
{
    auto registration = std::make_shared<GPSPositionSourceRegistration>();
    QObject::connect(
        &source, &NMEASourceManager::positionSourceChanged, &position, [&source, &position, registration]() {
            if (source.positionSource()) {
                *registration = position.registerPositionSource(QGCPositionManager::SelectedSource::Nmea, &source,
                                                                source.health(), source.sessionId());
            } else {
                registration->reset();
            }
        });
}

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
    const quint64 receipt = GPSObservation::monotonicNowUs() - 100000;
    GPSSatelliteObservation report;
    report.monotonicTimestampUs = receipt;
    GPSSatellite gps;
    gps.id = 3;
    gps.constellation = GPSSatellite::Constellation::GPS;
    gps.used = true;
    gps.elevationDegrees = 0;
    gps.signalStrength = 0;
    gps.normalizedAzimuthDegrees = 275;
    GPSSatellite galileo;
    galileo.id = 3;
    galileo.constellation = GPSSatellite::Constellation::Galileo;
    report.satellites = {gps, galileo};
    report.provenance = {{GPSSatellite::Constellation::GPS, receipt, receipt, 1, QList<int>{3}},
                         {GPSSatellite::Constellation::Galileo, receipt, 0, std::nullopt, std::nullopt}};
    QSignalSpy snapshots(&decoder, &NMEADecoderSession::satellitesReceived);
    decoder._updateSatellites(report);
    auto observation = decoder.satelliteObservation();
    QCOMPARE(observation.sourceId, QStringLiteral("nmeaReceiver"));
    QCOMPARE(observation.satellitesInViewCount(), 2);
    QCOMPARE(observation.satellitesInUseCount(), 1);
    QCOMPARE(observation.satellites[0].used, std::optional<bool>(true));
    QVERIFY(!observation.satellites[1].used.has_value());
    QCOMPARE(observation.satellites[0].azimuthDegrees(), std::optional<double>(275));
    QCOMPARE(observation.satellites[0].elevationDegrees, std::optional<double>(0));
    QCOMPARE(observation.satellites[0].signalStrength, std::optional<int>(0));
    QCOMPARE(observation.provenance[0].inViewTimestampUs, receipt);
    QCOMPARE(observation.provenance[0].inUseTimestampUs, receipt);
    QCOMPARE(snapshots.size(), 1);
    report.satellites[0].signalStrength = 42;
    decoder._updateSatellites(report);
    QCOMPARE(snapshots.size(), 2);
    QCOMPARE(decoder.satelliteObservation().satellites[0].signalStrength, std::optional<int>(42));

    GPSObservation fix;
    fix.position = QGeoPositionInfo(QGeoCoordinate(47.0, 8.0, 500.0), QDateTime::currentDateTimeUtc());
    fix.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
    fix.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    fix.satellitesUsed = 8;
    decoder.health()->updateObservation(fix);
    QCOMPARE(decoder.health()->satellitesInUseCount(), 8);
    decoder._satellites.clear();
    QTRY_COMPARE_WITH_TIMEOUT(decoder.satelliteObservation().satellitesInViewCount(), -1, TestTimeout::shortMs());
    QCOMPARE(decoder.satelliteObservation().satellitesInUseCount(), -1);
    QCOMPARE(decoder.health()->satellitesInUseCount(), 8);
    decoder._updateSatellites(report);
    QCOMPARE(decoder.satelliteObservation().satellitesInViewCount(), -1);
    decoder.stop();
    QVERIFY(decoder.sessionId() > session);
    QCOMPARE(decoder.satelliteObservation().satellitesInViewCount(), -1);
}

void NMEASourceManagerTest::init()
{
    UnitTest::init();
    ignoreLogMessage("GPS.PositionManager.GPSPositionService", QtWarningMsg,
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
    source.update();
    QVERIFY(source._sourceAvailable);
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
    QVERIFY(source._sourceAvailable);
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QVERIFY(spare.bind(QHostAddress::LocalHost, firstPort, QUdpSocket::DontShareAddress));
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/missing-nmea"));
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
    source.update();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceAvailable);
    QVERIFY(!source._attempt);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    source.update();
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    source.update();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceAvailable);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
    ManualScheduler scheduler;
    NMEASourceManager source(nullptr, &scheduler);
    configureSource(source, settings);
    attachPosition(source, position);
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

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(4999)));
    QCOMPARE(source.status(), receiving);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(source.status(), listening);
    QVERIFY(source.active());
    QCOMPARE(sender.writeDatagram(invalidData, QHostAddress::LocalHost, port), invalidData.size());
    QTRY_COMPARE_WITH_TIMEOUT(source.status(), receiving, TestTimeout::mediumMs());
    source.update();
    QCOMPARE(source.status(), receiving);

    source.disconnectSource();
    QCOMPARE(source.status(), QStringLiteral("Disconnected"));
    QVERIFY(!source._udpActivity.active());
    QVERIFY(source.connectSource());
    QCOMPARE(source.status(), listening);
    QCOMPARE(sender.writeDatagram(invalidData, QHostAddress::LocalHost, port), invalidData.size());
    QTRY_COMPARE_WITH_TIMEOUT(source.status(), receiving, TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    QCOMPARE(source.status(), QStringLiteral("Disconnected"));
    QVERIFY(!source._udpActivity.active());
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
        NMEASourceManager source;
        configureSource(source, settings);
        attachPosition(source, position);
        source.update();
        QVERIFY(!source._sourceAvailable);
        QVERIFY(!source._attempt);
        QCOMPARE(source.connectionState(), GPSConnectionState::Retrying);
        occupied.close();
        QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                     source.update();
                                     return source._sourceAvailable;
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
    GPSPositionSourceRegistration positionReceiverRegistration;

    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 port = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    TestGPSPositionSource receiver;
    QGCPositionManager position;
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
    QVERIFY(source.connectSource());
    QCOMPARE(source.satellitesInViewCount(), -1);
    QCOMPARE(source.satellitesInUseCount(), -1);
    source._decoder.setFreshnessTimeoutMs(200);
    QUdpSocket sender;
    const auto send = [&](const QByteArray& data) {
        QCOMPARE(sender.writeDatagram(data, QHostAddress::LocalHost, port), data.size());
    };
    int fixSequence = 0;
    const auto freshFix = [&]() {
        const QByteArray epoch =
            QTime(9, 27, 50).addSecs(++fixSequence).toString(QStringLiteral("hhmmss.zzz")).toLatin1();
        QByteArray result;
        for (QByteArray sentence : kFix.split('\n')) {
            if (!sentence.trimmed().isEmpty()) {
                sentence.replace("092750.000", epoch);
                result += NMEAUtils::repairChecksum(sentence);
            }
        }
        return result;
    };
    const QByteArray payload = freshFix() + satelliteSentences();
    const qsizetype split = kFix.size() + 15;
    send(payload.first(split));
    send(payload.sliced(split));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 8, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    const auto satellite = source.satelliteObservation().satellites.first();
    QCOMPARE(satellite.id, 1);
    QCOMPARE(satellite.constellation, GPSSatellite::Constellation::GPS);
    QCOMPARE(satellite.signalStrength, std::optional<int>(40));
    QCOMPARE(satellite.elevationDegrees, std::optional<double>(45));
    QCOMPARE(satellite.azimuthDegrees(), std::optional<double>(100));

    QSignalSpy responses(source._decoder._satelliteAdapter.get(), &NMEASatelliteAdapter::observationReceived);
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
    QVERIFY(source.satelliteObservation().satellites.isEmpty());
    QCOMPARE(source.satelliteObservation().satellitesInUseCount(), -1);
    QCOMPARE(source.satelliteObservation().satellitesInViewCount(), -1);
    source._decoder.setFreshnessTimeoutMs(5000);
    send(freshFix());
    QVERIFY(source.satelliteObservation().satellites.isEmpty());
    send(satelliteSentences());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());

    positionReceiverRegistration =
        position.registerPositionSource(GPSPositionService::SelectedSource::Receiver, &receiver, nullptr);
    send(satelliteSentences(45));
    QTRY_COMPARE_WITH_TIMEOUT(source.satelliteObservation().satellites.first().signalStrength, std::optional<int>(45),
                              TestTimeout::mediumMs());
    const quint64 previousReceipt = source.health()->observation().monotonicTimestampUs;
    positionReceiverRegistration.reset();
    QVERIFY(!position.gcsPosition().isValid());
    send(freshFix());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(source.health()->observation().monotonicTimestampUs > previousReceipt);
    QCOMPARE(source.satellitesInViewCount(), 2);

    source.disconnectSource();
    QCOMPARE(source.satellitesInViewCount(), -1);
    QCOMPARE(source.satellitesInUseCount(), -1);
    QVERIFY(!source._decoder._satelliteAdapter);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
    QVERIFY(source.connectSource());
    QCOMPARE(source.connectionState(), GPSConnectionState::Connecting);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
    peer->write(kFix + satelliteSentences());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInUseCount(), 8, TestTimeout::mediumMs());
    QCOMPARE(connections.size(), 1);
    peer->write(NMEAUtils::repairChecksum("$GPGSV,1,1,00") +
                NMEAUtils::repairChecksum("$GPGSA,A,1,,,,,,,,,,,,,99.9,99.9,99.9"));
    QTRY_COMPARE_WITH_TIMEOUT(source.satellitesInViewCount(), 0, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.satelliteObservation().satellitesInUseCount(), 0, TestTimeout::mediumMs());
    // An explicit GSA no-fix report retires the earlier GGA fix and satellite total.
    QCOMPARE(source.satellitesInUseCount(), 0);
    QVERIFY(!source.health()->usable());
    peer->disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(!source._sourceAvailable, TestTimeout::mediumMs());
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
        NMEASourceManager source;
        source.setSerialDiscovery(new GPSSerialPortRegistry(ports, &source));
        configureSource(source, settings);
        attachPosition(source, position);
        QVERIFY(!ports->canAutoConnectPort(first));
        QVERIFY(!ports->isPortReserved(first));
        source.update();
        QVERIFY(!source._sourceAvailable);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
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
    source._control.connection()._retryDeadlineMs = 0;
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    peer.reset(server.nextPendingConnection());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
    QVERIFY(source.active());
    source.disconnectSource();
    source.update();
    QVERIFY(!source.active());
    QVERIFY(source.connectSource());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
    settings->nmeaAutoConnect()->setRawValue(false);
    QVERIFY(!source.active());
    QVERIFY(!source._attempt);
    source.update();
    QVERIFY(!source.active());
    QVERIFY(source.connectSource());
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
    QVERIFY(source._control.connection()._retryDeadlineMs >= 0);
    QCOMPARE(source._control.connection()._retryDelayMs, 1000);
    source.update();
    QVERIFY(!source._attempt);
    source._control.connection()._retryDeadlineMs = 0;
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(!source._attempt, TestTimeout::mediumMs());
    QCOMPARE(source._control.connection()._retryDelayMs, 2000);
    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    source._control.connection()._retryDeadlineMs = 0;
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(source._sourceAvailable, TestTimeout::mediumMs());
    QCOMPARE(source._control.connection()._retryDelayMs, 1000);
}

void NMEASourceManagerTest::_disconnectDuringPositionUpdate()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->nmeaUdpPort(), 0);
    QGCPositionManager position;
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
    QCOMPARE(source._control.connection()._retryDelayMs, 1000);
    QVERIFY(!weakLease.expired());

    source.update();
    QCOMPARE(attempts->load(), 1);
    source._control.connection()._retryDeadlineMs = 0;
    source.update();
    QTRY_COMPARE_WITH_TIMEOUT(attempts->load(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), GPSConnectionState::Retrying, TestTimeout::mediumMs());
    QCOMPARE(source._control.connection()._retryDelayMs, 2000);
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
        NMEASourceManager source;
        configureSource(source, settings);
        attachPosition(source, position);
        QCOMPARE(source._control.profile().configurationPolicy, GPSReceiverProfile::ConfigurationPolicy::Configure);
        source.disconnectSource();
        settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverPassive);
        QCOMPARE(source._control.profile().endpoint.baud, 4800);
    }
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverUblox);
    NMEASourceManager restarted(&position);
    configureSource(restarted, settings);
    QCOMPARE(restarted._control.profile().configurationPolicy, GPSReceiverProfile::ConfigurationPolicy::Configure);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
    QCOMPARE(source._control.profile().endpoint.baud, 4800);
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
    auto source = std::make_unique<NMEASourceManager>();
    configureSource(*source, settings);
    attachPosition(*source, position);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
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
    NMEASourceManager source;
    configureSource(source, settings);
    attachPosition(source, position);
    source.setSerialDiscovery(new GPSSerialPortRegistry(&inventory, &source));
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
    source.setSerialDiscovery(new GPSSerialPortRegistry(&inventory, &source));
    QVERIFY(inventory.isAutoConnectExcluded(device));
    source.disconnectSource();
    QVERIFY(!inventory.isAutoConnectExcluded(device));
#endif
}

void NMEASourceManagerTest::_decodesWithoutPositionManager()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->nmeaUdpPort(), 0);
    NMEASourceManager source;
    configureSource(source, settings);
    QVERIFY(source.connectSource());
    QVERIFY(source.positionSource());
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, source._attempt->localPort()), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(
        source.health()->acceptedObservation(GPSObservation::PositionUse::GroundStation).has_value(),
        TestTimeout::mediumMs());
    source.disconnectSource();
    QVERIFY(!source.positionSource());
}

void NMEASourceManagerTest::_scheduledRetryAndSuspension()
{
    ManualScheduler scheduler;
    QUdpSocket occupied;
    QVERIFY(occupied.bind(QHostAddress::AnyIPv4, 0, QAbstractSocket::DontShareAddress));
    const quint16 port = occupied.localPort();
    NMEASourceManager source(nullptr, &scheduler);
    source.setProfile({.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::UdpListener, .port = port}});
    source.setSuspended(true);
    source.setAutoConnect(true);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(source.connectionState(), GPSConnectionState::Disconnected);
    source.setSuspended(false);
    QVERIFY(scheduler.advanceBy(std::chrono::microseconds(0)));
    QCOMPARE(source.connectionState(), GPSConnectionState::Retrying);
    occupied.close();
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(source.connectionState(), GPSConnectionState::Retrying);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(source.connectionState(), GPSConnectionState::Ready);
    const quint64 session = source.sessionId();
    source.setSuspended(true);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(source.connectionState(), GPSConnectionState::Ready);
    QCOMPARE(source.sessionId(), session);
    source.disconnectSource();
    const quint64 retired = source.sessionId();
    source.update();
    source.stop();
    source.update();
    QCOMPARE(source.sessionId(), retired);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void NMEASourceManagerTest::_passiveConnectTimeoutUsesScheduler()
{
    ManualScheduler scheduler;
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GPSReceiverProfile profile;
    profile.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Tcp,
                        .host = QStringLiteral("127.0.0.1"),
                        .port = server.serverPort()};
    NMEAConnectionAttempt attempt(profile, nullptr, 1, &scheduler);
    QSignalSpy failures(&attempt, &NMEAConnectionAttempt::failed);
    attempt.start();
    QCOMPARE(attempt.attempt().phase, GPSReceiverAttempt::Phase::Connecting);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(9999)));
    QCOMPARE(failures.size(), 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(failures.size(), 1);
    QCOMPARE(failures.first().first().toString(), QStringLiteral("Connection timed out"));
    attempt.stop();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(failures.size(), 1);
    NMEAConnectionAttempt cancelled(profile, nullptr, 2, &scheduler);
    QSignalSpy cancelledFailures(&cancelled, &NMEAConnectionAttempt::failed);
    cancelled.start();
    cancelled.stop();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(cancelledFailures.size(), 0);
}
