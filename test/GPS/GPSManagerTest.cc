#include "GPSManagerTest.h"

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "ColoredSvgImageProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSPositionSettings.h"
#include "GPSProvider.h"
#include "GPSReceiver.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverSession.h"
#include "GPSReceiverSettingsPresentation.h"
#include "GPSTransport.h"
#include "GpsTestHelpers.h"
#include "LinkManager.h"
#include "MockNTRIPStream.h"
#include "NMEAUtils.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "RTCMMavlink.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

namespace {
QQuickItem* findVisualItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* found = findVisualItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

void saveNetworkSettings(TestFixtures::SettingsFixture& saved, const QString& host, int port, int type)
{
    saved.setFactValue(SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS(), false);
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->receiverRole(), RTKSettings::RTKBase);
    saved.setFactValue(settings->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(settings->networkBaseHost(), host);
    saved.setFactValue(settings->networkBasePort(), port);
    saved.setFactValue(settings->udpLocalPort(), 0);
    saved.setFactValue(settings->networkReceiverType(), type);
    saved.setFactValue(settings->baseReceiverManufacturers(), settings->baseReceiverManufacturers()->rawValue());
}

// A loopback receiver that acknowledges the pinned Femto driver's configuration commands.
class ReceiverServer : public QTcpServer
{
public:
    ReceiverServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this]() {
            peer = nextPendingConnection();
            ++connections;
            connect(peer, &QTcpSocket::readyRead, peer, [this, socket = peer]() {
                while (socket->canReadLine()) {
                    const QByteArray command = socket->readLine().trimmed();
                    commands.append(command);
                    const QByteArray reply = '<' + command.split(' ').first() + " OK" + char(0);
                    socket->write(reply);
                }
            });
        });
    }

    QTcpSocket* peer = nullptr;
    int connections = 0;
    QList<QByteArray> commands;
};

class UdpReceiverServer : public QUdpSocket
{
public:
    UdpReceiverServer()
    {
        connect(this, &QUdpSocket::readyRead, this, [this]() {
            while (hasPendingDatagrams()) {
                const QNetworkDatagram datagram = receiveDatagram();
                if (!respond) {
                    continue;
                }
                if (_peerPort != datagram.senderPort()) {
                    _commands.clear();
                    _peerPort = datagram.senderPort();
                }
                _peerAddress = datagram.senderAddress();
                _commands += datagram.data();
                while (_commands.contains('\n')) {
                    const qsizetype end = _commands.indexOf('\n');
                    const QByteArray command = _commands.left(end).trimmed();
                    _commands.remove(0, end + 1);
                    const QByteArray reply = '<' + command.split(' ').first() + " OK" + char(0);
                    writeDatagram(reply, datagram.senderAddress(), datagram.senderPort());
                    ++acknowledgedCommands;
                }
            }
        });
    }

    bool respond = true;
    int acknowledgedCommands = 0;

    qint64 send(const QByteArray& data) { return writeDatagram(data, _peerAddress, _peerPort); }

private:
    QByteArray _commands;
    QHostAddress _peerAddress;
    quint16 _peerPort = 0;
};
}  // namespace

void GPSManagerTest::_positionSourceSettings()
{
    auto* settings = SettingsManager::instance();
    auto* mode = settings->gpsPositionSettings()->sourceMode();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(mode, static_cast<int>(QGCPositionManager::SourceMode::ReceiverOnly));
    QGCPositionManager positions;
    GPSManager manager(*settings, &positions, []() { return true; });
    QCOMPARE(positions.sourceMode(), QGCPositionManager::SourceMode::ReceiverOnly);
    mode->setRawValue(static_cast<int>(QGCPositionManager::SourceMode::Automatic));
    QCOMPARE(positions.sourceMode(), QGCPositionManager::SourceMode::Automatic);
    QCOMPARE(QSettings().value(QStringLiteral("GPSPosition/sourceMode")).toInt(),
             static_cast<int>(QGCPositionManager::SourceMode::Automatic));
    manager.shutdown();
    mode->setRawValue(static_cast<int>(QGCPositionManager::SourceMode::NmeaOnly));
    QCOMPARE(positions.sourceMode(), QGCPositionManager::SourceMode::Automatic);
}

void GPSManagerTest::_positionSourceReentrantDisable()
{
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), server.serverPort(), 3);
    auto* settings = SettingsManager::instance();
    auto* useReceiver = settings->rtkSettings()->useReceiverPosition();
    saved.setFactValue(useReceiver, false);
    saved.setFactValue(settings->gpsPositionSettings()->sourceMode(),
                       static_cast<int>(QGCPositionManager::SourceMode::ReceiverOnly));
    QGCPositionManager positions;
    GPSManager manager(*settings, &positions, []() { return false; });
    QVERIFY(manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    connect(&positions, &QGCPositionManager::sourceHealthChanged, &manager, [&]() {
        if (positions.sourceHealth() == manager.receiver()->health()) {
            useReceiver->setRawValue(false);
        }
    });
    useReceiver->setRawValue(true);
    QVERIFY(!useReceiver->rawValue().toBool());
    QVERIFY(!manager._receiverBinding.registration);
    QVERIFY(!manager._receiverBinding.source);
    QVERIFY(positions.sourceHealth() != manager.receiver()->health());
}

void GPSManagerTest::_nmeaSourceRegistration_data()
{
    QTest::addColumn<bool>("publishPosition");
    QTest::addColumn<bool>("deleteOnShutdown");
    QTest::newRow("registered-position") << true << false;
    QTest::newRow("standalone-diagnostics") << false << false;
    QTest::newRow("delete-during-retirement") << true << true;
}

void GPSManagerTest::_nmeaSourceRegistration()
{
    QFETCH(bool, publishPosition);
    QFETCH(bool, deleteOnShutdown);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto* settings = SettingsManager::instance();
    auto* connections = settings->autoConnectSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(connections->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(connections->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverPassive);
    saved.setFactValue(connections->nmeaAutoConnect(), false);
    saved.setFactValue(connections->nmeaTcpHost(), QStringLiteral("localhost"));
    saved.setFactValue(connections->nmeaTcpPort(), server.serverPort());
    saved.setFactValue(settings->gpsPositionSettings()->sourceMode(),
                       static_cast<int>(QGCPositionManager::SourceMode::NmeaOnly));
    QGCPositionManager positions;
    auto owner =
        std::make_unique<GPSManager>(*settings, publishPosition ? &positions : nullptr, []() { return false; });
    auto& manager = *owner;
    QVERIFY(manager.recordingController()->start());
    QVERIFY(manager.connectNmea());
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(manager.nmeaConnection()->positionSource(), TestTimeout::mediumMs());
    if (publishPosition) {
        QCOMPARE(positions.sourceHealth(), manager.nmeaConnection()->health());
        QVERIFY(manager._nmeaBinding.registration);
    }
    const QByteArray sentences =
        NMEAUtils::repairChecksum("$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
        NMEAUtils::repairChecksum("$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,") +
        NMEAUtils::repairChecksum("$GPGSV,1,1,02,01,45,100,40,02,30,200,35");
    QCOMPARE(peer->write(sentences), sentences.size());
    QTRY_VERIFY_WITH_TIMEOUT(manager.nmeaConnection()->health()->usable(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(manager.nmeaSatelliteModel()->count(), 2, TestTimeout::mediumMs());
    QVERIFY(manager.recordingController()->eventCount() > 0);
    if (deleteOnShutdown) {
        auto* health = manager.nmeaConnection()->health();
        const auto connection = connect(&positions, &QGCPositionManager::sourceHealthChanged, &positions, [&]() {
            if (positions.sourceHealth() != health) {
                owner.reset();
            }
        });
        manager.shutdown();
        QObject::disconnect(connection);
        QVERIFY(!owner);
        return;
    }
    manager.disconnectNmea();
    QVERIFY(!manager._nmeaBinding.registration);
    QVERIFY(!manager._nmeaBinding.source);
    QVERIFY(!manager.nmeaConnection()->positionSource());
    QCOMPARE(manager.nmeaSatelliteModel()->count(), 0);
    if (publishPosition) {
        QVERIFY(positions.sourceHealth() != manager.nmeaConnection()->health());
    }
    manager.shutdown();
}

void GPSManagerTest::_ntripUdpOutputSettings()
{
    QUdpSocket output;
    QVERIFY(output.bind(QHostAddress(QHostAddress::LocalHost), 0));
    auto* settings = SettingsManager::instance();
    auto* ntripSettings = settings->ntripSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings->gpsCorrectionSettings()->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->gpsCorrectionSettings()->correctionSource(), GPSCorrectionSettings::LocalReceiver);
    saved.setFactValue(settings->gpsCorrectionSettings()->correctionSourceInstance(), QString());
    saved.setFactValue(ntripSettings->ntripServerConnectEnabled(), true);
    saved.setFactValue(ntripSettings->ntripServerHostAddress(), QStringLiteral("caster.example.test"));
    saved.setFactValue(ntripSettings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(ntripSettings->ntripUdpForwardEnabled(), true);
    saved.setFactValue(ntripSettings->ntripUdpTargetAddress(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(ntripSettings->ntripUdpTargetPort(), output.localPort());
    NTRIPManager ntrip;
    auto* stream = new MockNTRIPStream(&ntrip);
    ntrip.setTransportForTest(stream);
    GPSManager manager(*settings, nullptr, []() { return false; });
    manager.init(&ntrip);
    const int connections = stream->startCount;
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    stream->simulateRtcmData(frame, 1005);
    QTRY_VERIFY_WITH_TIMEOUT(output.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(output.receiveDatagram().data(), frame);
    ntripSettings->ntripUdpForwardEnabled()->setRawValue(false);
    stream->simulateRtcmData(frame, 1005);
    QCOMPARE(stream->startCount, connections);
    const QByteArray nextFrame = GpsTestHelpers::buildRtcmFrame(1006);
    ntripSettings->ntripUdpForwardEnabled()->setRawValue(true);
    stream->simulateRtcmData(nextFrame, 1006);
    QTRY_VERIFY_WITH_TIMEOUT(output.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(output.receiveDatagram().data(), nextFrame);
    QCOMPARE(stream->startCount, connections);
    manager.shutdown();
}

void GPSManagerTest::_invalidEndpoint_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<int>("type");
    QTest::newRow("empty-host") << QStringLiteral("  ") << 2101 << 0;
    QTest::newRow("url-in-host") << QStringLiteral("tcp://localhost") << 2101 << 0;
    QTest::newRow("host-with-port") << QStringLiteral("localhost:2101") << 2101 << 0;
    QTest::newRow("zero-port") << QStringLiteral("localhost") << 0 << 0;
    QTest::newRow("unknown-receiver") << QStringLiteral("localhost") << 2101 << 4;
}

void GPSManagerTest::_invalidEndpoint()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(int, type);
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, host, port, type);
    GPSManager manager;
    manager.init();
    QSignalSpy active(&manager, &GPSManager::networkRtkActiveChanged);
    QVERIFY(!manager.connectNetworkRtk());
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.receiver()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(active.isEmpty());
    SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS()->setRawValue(true);
    manager._receiverAutoConnect->update();
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.receiver()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
}

void GPSManagerTest::_networkRecoveryAndDisconnect()
{
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    expectLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral(" 127.0.0.1 "), port, 3);
    SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS()->setRawValue(true);
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), 1);
    saved.setFactValue(settings->fixedBasePositionLatitude(), 10.0);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 20.0);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 30.0);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 0.0);
    GPSManager manager;
    QSignalSpy active(&manager, &GPSManager::networkRtkActiveChanged);
    manager.init();
    auto* receiver = manager.receiver();
    auto* facts = receiver->facts();
    QVERIFY(facts);

    expectLogMessage("GPS.Driver.Transport.TcpGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    manager._receiverAutoConnect->update();
    QVERIFY(manager.networkRtkActive());
    QVERIFY(!receiver->connected());
    QVERIFY(!manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::OpenFailed));

    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._receiverAutoConnect->update();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(server.connections, 1);
    const QByteArray originalPosition("FIX POSITION 10.00000000 20.00000000 30.00000");
    QCOMPARE(server.commands.count(originalPosition), 1);
    ReceiverServer replacement;
    QVERIFY(replacement.listen(QHostAddress::LocalHost));
    settings->networkBasePort()->setRawValue(replacement.serverPort());
    settings->fixedBasePositionLatitude()->setRawValue(11.0);
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::None));

    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    server.peer->abort();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!receiver->connected());
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._receiverAutoConnect->update();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(server.connections, 2);
    QCOMPARE(server.commands.count(originalPosition), 2);
    QCOMPARE(replacement.connections, 0);

    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!receiver->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
    QVERIFY(!receiver->connected());
    manager._receiverAutoConnect->update();
    QVERIFY(!receiver->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
    QCOMPARE(active.size(), 2);

    QVERIFY(manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(receiver->connected(), TestTimeout::mediumMs());
    QCOMPARE(replacement.connections, 1);
    QVERIFY(replacement.commands.contains("FIX POSITION 11.00000000 20.00000000 30.00000"));
    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
}

void GPSManagerTest::_udpRecoveryAndSelection()
{
    UdpReceiverServer server;
    QVERIFY(server.bind(QHostAddress::LocalHost, 0));
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("127.0.0.1"), server.localPort(), 3);
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* automatic = SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS();
    settings->connectionType()->setRawValue(RTKSettings::Udp);
    automatic->setRawValue(true);
    GPSManager manager;
    manager.init();
    auto* forwarder = manager.corrections()->rtcmMavlink();
    forwarder->setOutputProvider([]() {
        return QList<RTCMMavlink::Output>{{QStringLiteral("test"), 1, [](const GpsRtcmPacket&) { return true; }}};
    });
    auto* receiver = manager.receiver();

    manager._receiverAutoConnect->update();
    QVERIFY(manager.rtkConnection()->active());
    QVERIFY(!receiver->connected());
    QTRY_VERIFY_WITH_TIMEOUT(receiver->connected(), TestTimeout::mediumMs());
    const int initialCommands = server.acknowledgedCommands;
    QVERIFY(initialCommands > 0);

    const QByteArray corrections = GpsTestHelpers::buildRtcmFrame(1077, 500);
    QCOMPARE(server.send(corrections.left(4)), 4);
    QCOMPARE(server.send(corrections.mid(4)), corrections.size() - 4);
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), quint64(corrections.size()), TestTimeout::shortMs());

    // A silent UDP peer has no disconnect event; the driver's idle deadline must retire the session.
    server.respond = false;
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::longMs());
    verifyExpectedLogMessage();
    QVERIFY(!receiver->connected());
    QVERIFY(manager.rtkConnection()->active());
    server.respond = true;
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._receiverAutoConnect->update();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QVERIFY(server.acknowledgedCommands > initialCommands);

    manager.disconnectRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.rtkConnection()->autoConnectPaused());
    manager._receiverAutoConnect->update();
    QVERIFY(!receiver->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
    automatic->setRawValue(false);
    QVERIFY(manager.connectRtk());
    QTRY_VERIFY_WITH_TIMEOUT(receiver->connected(), TestTimeout::mediumMs());

    ReceiverServer tcp;
    QVERIFY(tcp.listen(QHostAddress::LocalHost));
    settings->connectionType()->setRawValue(RTKSettings::Tcp);
    QVERIFY(!manager.rtkConnection()->active());
    QVERIFY(!receiver->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
    settings->networkBasePort()->setRawValue(tcp.serverPort());
    automatic->setRawValue(true);
    manager._receiverAutoConnect->update();
    QTRY_VERIFY_WITH_TIMEOUT(receiver->connected(), TestTimeout::mediumMs());
    QCOMPARE(tcp.connections, 1);
    automatic->setRawValue(false);
    QVERIFY(!receiver->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
}

void GPSManagerTest::_networkStartupAndPause()
{
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), server.serverPort(), 3);
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), false);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    // SettingsFact ignores saved values during unit tests; verify the stored preference directly.
    QVERIFY(QSettings().value(QStringLiteral("AutoConnect/autoConnectNetworkRTKGPS")).toBool());

    GPSManager manager;
    QCoreApplication::processEvents();
    QVERIFY(!manager.networkRtkActive());
    QCOMPARE(server.connections, 0);
    manager.init();
    QVERIFY(manager.networkRtkActive());
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 1);

    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.networkRtkAutoConnectPaused());
    QVERIFY(settings->autoConnectNetworkRTKGPS()->rawValue().toBool());
    manager._updateConnections();
    manager._updateConnections();
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.receiver()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 1);

    // A fresh manager reads the saved option without requiring a setting-change signal.
    {
        GPSManager restarted;
        restarted.init();
        QVERIFY(!restarted.networkRtkAutoConnectPaused());
        restarted._updateConnections();
        QTRY_VERIFY_WITH_TIMEOUT(restarted.receiver()->connected(), TestTimeout::mediumMs());
        QCOMPARE(server.connections, 2);
    }
    QVERIFY(manager.networkRtkAutoConnectPaused());
    QVERIFY(manager.connectNetworkRtk());
    QVERIFY(!manager.networkRtkAutoConnectPaused());
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 3);
    settings->autoConnectNetworkRTKGPS()->setRawValue(false);
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.receiver()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    manager._updateConnections();
    QVERIFY(!manager.networkRtkActive());

    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    manager._updateConnections();
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.networkRtkAutoConnectPaused());
    settings->autoConnectNetworkRTKGPS()->setRawValue(false);
    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    QVERIFY(!manager.networkRtkAutoConnectPaused());
    manager._updateConnections();
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 5);
}

void GPSManagerTest::_suspendedConnections()
{
    TestFixtures::SettingsFixture saved;
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saveNetworkSettings(saved, QStringLiteral("localhost"), server.serverPort(), 3);
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), false);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    auto* links = LinkManager::instance();
    QVERIFY(!links->connectionsSuspended());
    const auto restore = qScopeGuard([links]() { links->setConnectionsAllowed(); });
    links->setConnectionsSuspended(QStringLiteral("test"));
    GPSManager manager;
    manager.init();
    QVERIFY(!manager.connectNetworkRtk());
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.receiver()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    links->setConnectionsAllowed();
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    const quint64 session = manager.receiverSession()->sessionId();
    links->setConnectionsSuspended(QStringLiteral("keep established receiver"));
    QVERIFY(manager.receiver()->connected());
    QCOMPARE(manager.receiverSession()->sessionId(), session);
}

void GPSManagerTest::_serialDiscoveryPausesForNetwork()
{
#ifndef QGC_NO_SERIAL_LINK
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), server.serverPort(), 3);
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->autoConnectRTKGPS(), true);
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceDisabled);
    SerialPortManager ports(nullptr, []() {
        return QList<SerialPortManager::Port>{{QStringLiteral("/test/rtk"), QStringLiteral("rtk"),
                                               QGCSerialPortInfo::BoardTypeRTKGPS, QStringLiteral("u-blox")}};
    });
    auto* rtkSettings = SettingsManager::instance()->rtkSettings();
    rtkSettings->connectionType()->setRawValue(RTKSettings::Serial);
    GPSManager manager;
    manager.init();
    manager._receiverAutoConnect->setSerialDiscovery(&ports);
    manager._receiverAutoConnect->setSerialTransportFactory([](const QString&) {
        return [](const std::atomic_bool& stop) -> std::unique_ptr<GPSTransport> {
            while (!stop) {
                QThread::msleep(1);
            }
            return {};
        };
    });
    QSignalSpy serialConnects(manager._receiverAutoConnect, &GPSReceiverAutoConnect::connectRequested);
    QSignalSpy serialDisconnects(manager._receiverAutoConnect, &GPSReceiverAutoConnect::disconnectRequested);
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateConnections();
                                 return serialConnects.size() == 1;
                             })(),
                             TestTimeout::longMs());
    rtkSettings->connectionType()->setRawValue(RTKSettings::Tcp);
    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    manager._updateConnections();
    QVERIFY(manager.networkRtkActive());
    QCOMPARE(serialDisconnects.size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateConnections();
                                 return manager.receiver()->connected();
                             })(),
                             TestTimeout::mediumMs());
    manager._updateConnections();
    QCOMPARE(serialConnects.size(), 1);
    QVERIFY(settings->autoConnectRTKGPS()->rawValue().toBool());
    manager.disconnectRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    manager._updateConnections();
    QCOMPARE(serialConnects.size(), 1);
    rtkSettings->connectionType()->setRawValue(RTKSettings::Serial);
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateConnections();
                                 return serialConnects.size() == 2;
                             })(),
                             TestTimeout::longMs());
#else
    QSKIP("Serial discovery is unavailable in this build");
#endif
}

void GPSManagerTest::_networkSettingsPanel_data()
{
    QTest::addColumn<int>("connection");
    QTest::addColumn<int>("role");
    QTest::newRow("tcp-base") << int(RTKSettings::Tcp) << int(RTKSettings::RTKBase);
    QTest::newRow("udp-base") << int(RTKSettings::Udp) << int(RTKSettings::RTKBase);
    QTest::newRow("tcp-position") << int(RTKSettings::Tcp) << int(RTKSettings::Position);
    QTest::newRow("udp-position") << int(RTKSettings::Udp) << int(RTKSettings::Position);
}

void GPSManagerTest::_networkSettingsPanel()
{
    QFETCH(int, connection);
    QFETCH(int, role);
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QString(), 2101, 0);
    SettingsManager::instance()->rtkSettings()->connectionType()->setRawValue(connection);
    SettingsManager::instance()->rtkSettings()->receiverRole()->setRawValue(role);
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/LocalRtkSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QQmlComponent indicatorComponent(&engine,
                                     QUrl(QStringLiteral("qrc:/qml/QGroundControl/Toolbar/GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!indicatorComponent.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(indicatorComponent.isReady(), qPrintable(indicatorComponent.errorString()));
    std::unique_ptr<QObject> indicator(indicatorComponent.create());
    QVERIFY2(indicator, qPrintable(indicatorComponent.errorString()));
    QCOMPARE(indicator->property("showExpand").toBool(), role == RTKSettings::RTKBase);

    auto* button = root->findChild<QObject*>(QStringLiteral("networkRtkConnectButton"));
    auto* host = root->findChild<QObject*>(QStringLiteral("networkRtkHost"));
    auto* roleControl = root->findChild<QObject*>(QStringLiteral("gpsReceiverRole"));
    QVERIFY(roleControl);
    QCOMPARE(roleControl->property("fact").value<Fact*>(), SettingsManager::instance()->rtkSettings()->receiverRole());
    QVERIFY(roleControl->property("enabled").toBool());
    auto* usePosition = root->findChild<QObject*>(QStringLiteral("rtkUseReceiverPosition"));
    QVERIFY(usePosition);
    QCOMPARE(usePosition->property("fact").value<Fact*>(),
             SettingsManager::instance()->rtkSettings()->useReceiverPosition());
    auto* localPort = root->findChild<QObject*>(QStringLiteral("networkRtkLocalPort"));
    QVERIFY(button);
    QVERIFY(host);
    QVERIFY(localPort);
    QCOMPARE(localPort->property("visible").toBool(), connection == RTKSettings::Udp);
    QVERIFY(!button->property("enabled").toBool());
    SettingsManager::instance()->rtkSettings()->networkBaseHost()->setRawValue(QStringLiteral("localhost"));
    QTRY_VERIFY_WITH_TIMEOUT(button->property("enabled").toBool(), TestTimeout::shortMs());

    // Invalid host input is rejected without starting a worker from the QML button.
    SettingsManager::instance()->rtkSettings()->networkBaseHost()->setRawValue(QStringLiteral("tcp://localhost"));
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    auto* status = root->findChild<QObject*>(QStringLiteral("networkRtkStatus"));
    QVERIFY(status);
    QVERIFY(!GPSManager::instance()->rtkConnection()->validationError().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(status->property("text").toString(),
                              GPSManager::instance()->rtkConnection()->validationError(), TestTimeout::shortMs());
    QVERIFY(!GPSManager::instance()->networkRtkActive());

    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    UdpReceiverServer udpServer;
    QVERIFY(udpServer.bind(QHostAddress::LocalHost, 0));
    auto* settings = SettingsManager::instance()->rtkSettings();
    settings->networkBaseHost()->setRawValue(QStringLiteral("127.0.0.1"));
    settings->networkBasePort()->setRawValue(connection == RTKSettings::Udp ? udpServer.localPort()
                                                                            : server.serverPort());
    settings->networkReceiverType()->setRawValue(3);
    QVERIFY(GPSManager::instance()->rtkConnection()->validationError().isEmpty());
    GPSManager::instance()->init();
    if (connection == RTKSettings::Tcp && role == RTKSettings::Position) {
        auto* automatic = SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS();
        GPSManager::instance()->init();
        automatic->setRawValue(true);
        QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                     GPSManager::instance()->_updateConnections();
                                     return GPSManager::instance()->receiver()->connected();
                                 })(),
                                 TestTimeout::mediumMs());
        QTRY_COMPARE_WITH_TIMEOUT(status->property("text").toString(), QStringLiteral("Connected"),
                                  TestTimeout::mediumMs());
        automatic->setRawValue(false);
        QTRY_VERIFY_WITH_TIMEOUT(!GPSManager::instance()->receiver()->stopping(), TestTimeout::mediumMs());
    }
    const auto disconnect = qScopeGuard([]() { GPSManager::instance()->disconnectNetworkRtk(); });
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    QVERIFY(GPSManager::instance()->networkRtkActive());
    QVERIFY(!host->property("enabled").toBool());
    QVERIFY(!roleControl->property("enabled").toBool());
    QCOMPARE(button->property("text").toString(), QStringLiteral("Disconnect"));
    QTRY_VERIFY_WITH_TIMEOUT(GPSManager::instance()->receiver()->connected(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(status->property("text").toString(), QStringLiteral("Connected"),
                              TestTimeout::mediumMs());
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    QVERIFY(!GPSManager::instance()->networkRtkActive());
    QVERIFY(host->property("enabled").toBool());
    QVERIFY(roleControl->property("enabled").toBool());
    QCOMPARE(button->property("text").toString(), QStringLiteral("Connect"));
    QVERIFY(!button->property("enabled").toBool());
    QCOMPARE(status->property("text").toString(), QStringLiteral("Stopping"));
    auto* automatic = root->findChild<QObject*>(QStringLiteral("networkRtkAutoConnect"));
    QVERIFY(automatic);
    QVERIFY(!automatic->property("checked").toBool());
    automatic->setProperty("checked", true);
    QVERIFY(QMetaObject::invokeMethod(automatic, "clicked"));
    QVERIFY(SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS()->rawValue().toBool());
    auto* manager = GPSManager::instance();
    manager->_receiverAutoConnect->update();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager->_updateConnections();
                                 return manager->receiver()->connected();
                             })(),
                             TestTimeout::mediumMs());
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    manager->_receiverAutoConnect->update();
    QVERIFY(!manager->networkRtkActive());
    QTRY_COMPARE_WITH_TIMEOUT(status->property("text").toString(), QStringLiteral("Automatic connection paused"),
                              TestTimeout::mediumMs());
    automatic->setProperty("checked", false);
    QVERIFY(QMetaObject::invokeMethod(automatic, "clicked"));
    QVERIFY(!SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS()->rawValue().toBool());
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)

void GPSManagerTest::_nmeaAndRtkIndependent()
{
    TestFixtures::SettingsFixture saved;
    ReceiverServer rtkServer;
    QTcpServer nmeaServer;
    QVERIFY(rtkServer.listen(QHostAddress::LocalHost));
    QVERIFY(nmeaServer.listen(QHostAddress::LocalHost));
    saveNetworkSettings(saved, QStringLiteral("localhost"), rtkServer.serverPort(), 3);
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaAutoConnect(), true);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("localhost"));
    saved.setFactValue(settings->nmeaTcpPort(), nmeaServer.serverPort());
    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    GPSManager manager;
    manager.init();
    manager._updateConnections();
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(manager.nmeaConnection()->status(), QStringLiteral("Connected"), TestTimeout::mediumMs());
    QVERIFY(manager.nmeaConnection()->active());
    GPSSatelliteObservation satellites;
    satellites.sessionId = manager.receiverSession()->sessionId();
    satellites.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    GPSSatellite satellite;
    satellite.id = 17;
    satellite.used = true;
    satellites.satellites.append(satellite);
    emit manager.receiverSession()->satellitesReceived(satellites);
    QVERIFY(manager.satelliteModel()->fresh());
    QCOMPARE(manager.satelliteModel()->count(), 1);
    GPSRelativeObservation relative;
    relative.sessionId = satellites.sessionId;
    relative.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    relative.fixValid = true;
    relative.positionValid = true;
    relative.lengthMeters = 1.5;
    emit manager.receiverSession()->relativePositionReceived(relative);
    QVERIFY(manager.relativePositionModel()->fresh());
    QCOMPARE(manager.relativePositionModel()->length(), 1.5);
    QTRY_VERIFY_WITH_TIMEOUT(nmeaServer.hasPendingConnections(), TestTimeout::mediumMs());
    auto* nmeaPeer = nmeaServer.nextPendingConnection();
    QVERIFY(nmeaPeer);
    const auto nmea = NMEAUtils::repairChecksum("$GPGSV,1,1,02,01,45,100,40,02,30,200,35") +
                      NMEAUtils::repairChecksum("$GPGSA,A,3,01,02,,,,,,,,,,,1.0,0.8,0.6");
    QCOMPARE(nmeaPeer->write(nmea), nmea.size());
    QTRY_COMPARE_WITH_TIMEOUT(manager.nmeaSatelliteModel()->count(), 2, TestTimeout::mediumMs());
    QCOMPARE(manager.satelliteModel()->count(), 1);
    manager.disconnectNmea();
    manager._updateConnections();
    QVERIFY(!manager.nmeaConnection()->active());
    QVERIFY(!manager.nmeaSatelliteModel()->fresh());
    QCOMPARE(manager.nmeaSatelliteModel()->count(), 0);
    QVERIFY(manager.receiver()->connected());
    QVERIFY(manager.connectNmea());
    QTRY_COMPARE_WITH_TIMEOUT(manager.nmeaConnection()->status(), QStringLiteral("Connected"), TestTimeout::mediumMs());
    manager.disconnectRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    manager._updateConnections();
    QVERIFY(!manager.receiver()->hasReceiver());
    QVERIFY(!manager.satelliteModel()->fresh());
    QVERIFY(!manager.relativePositionModel()->fresh());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.nmeaConnection()->active());
    QCOMPARE(manager.nmeaConnection()->status(), QStringLiteral("Connected"));
}

void GPSManagerTest::_rtkConnectionSelectionMigration()
{
    QSettings storage;
    const QString key = QStringLiteral("RTK/connectionType");
    const bool existed = storage.contains(key);
    const QVariant previous = storage.value(key);
    const auto restore = qScopeGuard([&]() {
        if (existed) {
            storage.setValue(key, previous);
        } else {
            storage.remove(key);
        }
    });
    TestFixtures::SettingsFixture saved;
    auto* automatic = SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS();
    saved.setFactValue(automatic, false);
    storage.remove(key);
    {
        const RTKSettings migrated;
    }
    QVERIFY(!storage.contains(key));
    automatic->setRawValue(true);
    {
        const RTKSettings migrated;
    }
    QCOMPARE(storage.value(key).toInt(), static_cast<int>(RTKSettings::Tcp));
    storage.setValue(key, static_cast<int>(RTKSettings::Serial));
    {
        const RTKSettings migrated;
    }
    QCOMPARE(storage.value(key).toInt(), static_cast<int>(RTKSettings::Serial));
}

void GPSManagerTest::_correctionRoutingSettings_data()
{
    QTest::addColumn<int>("selection");
    QTest::addColumn<int>("policy");
    QTest::addColumn<int>("category");
    using Policy = GPSCorrectionManager::RoutingPolicy;
    QTest::newRow("automatic") << int(GPSCorrectionSettings::Automatic) << int(Policy::Automatic) << 0;
    QTest::newRow("local") << int(GPSCorrectionSettings::LocalReceiver) << int(Policy::Manual)
                           << int(GPSCorrectionSource::LocalReceiver);
    QTest::newRow("ntrip") << int(GPSCorrectionSettings::Ntrip) << int(Policy::Manual)
                           << int(GPSCorrectionSource::Ntrip);
    QTest::newRow("udp") << int(GPSCorrectionSettings::Udp) << int(Policy::Manual) << int(GPSCorrectionSource::Udp);
    QTest::newRow("all") << int(GPSCorrectionSettings::All) << int(Policy::All) << 0;
}

void GPSManagerTest::_correctionRoutingSettings()
{
    QFETCH(int, selection);
    QFETCH(int, policy);
    QFETCH(int, category);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Automatic);
    saved.setFactValue(settings->correctionSourceInstance(), QString());
    saved.setFactValue(settings->injectLocalReceiver(), false);
    GPSManager manager(*SettingsManager::instance(), nullptr, []() { return false; });
    manager.init();
    settings->correctionSource()->setRawValue(selection);
    QCOMPARE(int(manager.corrections()->routingPolicy()), policy);
    if (policy == int(GPSCorrectionManager::RoutingPolicy::Manual)) {
        QCOMPARE(int(manager.corrections()->selectedSource()), category);
    }
    manager.shutdown();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::All);
    QCOMPARE(int(manager.corrections()->routingPolicy()), policy);
    QVERIFY(!manager.connectRtk());
    QVERIFY(!manager.connectNmea());
}

void GPSManagerTest::_correctionDeliveryFlushReentrancy_data()
{
    QTest::addColumn<bool>("destroyManager");
    QTest::newRow("replace-during-delivery-flush") << false;
    QTest::newRow("delete-during-delivery-flush") << true;
}

void GPSManagerTest::_correctionDeliveryFlushReentrancy()
{
    QFETCH(bool, destroyManager);
    TestFixtures::SettingsFixture saved;
    auto* allSettings = SettingsManager::instance();
    auto* settings = allSettings->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Automatic);
    saved.setFactValue(settings->correctionSourceInstance(), QString());
    saved.setFactValue(settings->injectLocalReceiver(), false);
    auto* manufacturer = allSettings->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto manager = std::make_unique<GPSManager>(*allSettings, nullptr, []() { return true; });
    manager->init();
    auto releaseOpen = std::make_shared<QSemaphore>();
    QPointer<GPSProvider> worker;
    const auto cleanup = qScopeGuard([&]() {
        if (worker) {
            worker->stop();
        }
        releaseOpen->release();
        if (manager) {
            manager->shutdown();
        }
    });
    GPSReceiverProfile profile;
    profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
    profile.endpoint.host = QStringLiteral("unused.example.test");
    profile.endpoint.port = 2101;
    profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
    profile.receiver.role = GPSReceiverConfig::Role::Position;
    profile.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::Native;
    auto* session = manager->receiverSession();
    session->start(profile, [releaseOpen](const std::atomic_bool&) {
        releaseOpen->acquire();
        return std::unique_ptr<GPSTransport>{};
    });
    worker = session->findChild<GPSProvider*>();
    QVERIFY(worker);
    const auto mailbox = worker->mailbox();
    mailbox->setCorrectionsEnabled(true);
    const auto now = GPSCorrectionFrame::monotonicNowMs();
    const GPSCorrectionFrame frame{GPSCorrectionSource::Ntrip,           1,    now,
                                   GpsTestHelpers::buildRtcmFrame(1005), 1005, true};
    QVERIFY(mailbox->submitCorrection(frame, session->sessionId(), now).accepted);
    QCOMPARE(mailbox->stats().pendingCommands, 1);
    bool flushed = false;
    connect(session, &GPSReceiverSession::correctionDeliveriesReady, this,
            [&](const QList<GPSCorrectionDelivery>& deliveries) {
                if (flushed) {
                    return;
                }
                flushed = true;
                QCOMPARE(deliveries.size(), 1);
                QCOMPARE(deliveries.first().outcome, GPSCorrectionOutcome::Cleared);
                QCOMPARE(deliveries.first().requestedBytes, quint64(frame.data.size()));
                // The outer operation has captured NTRIP, but must not apply it after this callback supersedes it.
                QCOMPARE(manager->corrections()->routingPolicy(), GPSCorrectionManager::RoutingPolicy::Automatic);
                if (destroyManager) {
                    worker->stop();
                    releaseOpen->release();
                    manager.reset();
                } else {
                    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
                }
            });
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    QVERIFY(flushed);
    if (destroyManager) {
        QVERIFY(!manager);
    } else {
        QCOMPARE(manager->corrections()->selectedSource(), GPSCorrectionSource::Udp);
        QCOMPARE(manager->corrections()->routingPolicy(), GPSCorrectionManager::RoutingPolicy::Manual);
        QCOMPARE(mailbox->stats().pendingCommands, 0);
    }
}

void GPSManagerTest::_correctionSettingsReentrantChange_data()
{
    QTest::addColumn<bool>("destroyManager");
    QTest::newRow("replace-selection") << false;
    QTest::newRow("destroy-manager") << true;
}

void GPSManagerTest::_correctionSettingsReentrantChange()
{
    QFETCH(bool, destroyManager);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Automatic);
    saved.setFactValue(settings->correctionSourceInstance(), QString());
    saved.setFactValue(settings->injectLocalReceiver(), false);
    auto manager = std::make_unique<GPSManager>(*SettingsManager::instance(), nullptr, []() { return true; });
    manager->init();
    manager->corrections()->addSink(QStringLiteral("test"), [](const GPSCorrectionFrame&) { return true; });
    auto source = manager->corrections()->registerSource(GPSCorrectionSource::LocalReceiver);
    manager->corrections()->acceptIngress(
        source.token().event(GpsTestHelpers::buildRtcmFrame(1005), GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    bool notified = false;
    connect(manager->corrections(), &GPSCorrectionManager::selectedSourceChanged, this, [&]() {
        if (notified) {
            return;
        }
        notified = true;
        QCOMPARE(manager->corrections()->routingPolicy(), GPSCorrectionManager::RoutingPolicy::Manual);
        QCOMPARE(manager->corrections()->selectedSource(), GPSCorrectionSource::Ntrip);
        if (destroyManager) {
            manager.reset();
        } else {
            settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
        }
    });
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    QVERIFY(notified);
    if (destroyManager) {
        QVERIFY(!manager);
    } else {
        QCOMPARE(manager->corrections()->selectedSource(), GPSCorrectionSource::Udp);
        QCOMPARE(manager->corrections()->routingPolicy(), GPSCorrectionManager::RoutingPolicy::Manual);
    }
}

void GPSManagerTest::_correctionRuntimeLifecycle()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Ntrip);
    saved.setFactValue(settings->correctionSourceInstance(), QString());
    auto* ntripSettings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(ntripSettings->ntripServerConnectEnabled(), true);
    saved.setFactValue(ntripSettings->ntripServerHostAddress(), QStringLiteral("caster.example.test"));
    saved.setFactValue(ntripSettings->ntripMountpoint(), QStringLiteral("TEST"));
    NTRIPManager ntrip;
    auto* stream = new MockNTRIPStream(&ntrip);
    ntrip.setTransportForTest(stream);
    GPSManager manager(*SettingsManager::instance(), nullptr, []() { return false; });
    QSignalSpy routed(manager.corrections(), &GPSCorrectionManager::correctionRouted);
    manager.init(&ntrip);
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    stream->simulateRtcmData(frame, 1005);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(routed.at(0).at(0)).source, GPSCorrectionSource::Ntrip);
    manager.shutdown();
    QCOMPARE(ntrip.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    ntrip.correctionReceivedAt(frame, 1005, false, GPSCorrectionFrame::monotonicNowMs(), ntrip.correctionAttemptId());
    QCOMPARE(routed.size(), 1);
}

void GPSManagerTest::_correctionSettingsPanel()
{
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "CorrectionRoutingSettings");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    const std::unique_ptr<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));
    QVERIFY(panel->findChild<QObject*>(QStringLiteral("correctionSource")));
    QVERIFY(panel->findChild<QObject*>(QStringLiteral("correctionStream")));
    QVERIFY(panel->findChild<QObject*>(QStringLiteral("injectLocalReceiver")));
}

void GPSManagerTest::_receiverConfigurationPanel_data()
{
    QTest::addColumn<int>("family");
    QTest::addColumn<bool>("heading");
    QTest::newRow("ublox") << int(GPSType::u_blox) << false;
    QTest::newRow("septentrio") << int(GPSType::septentrio) << true;
}

void GPSManagerTest::_receiverConfigurationPanel()
{
    QFETCH(int, family);
    QFETCH(bool, heading);
    auto* settings = SettingsManager::instance()->rtkSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings->constellationMask(), 0);
    saved.setFactValue(settings->dynamicModel(), heading ? 4 : 0);
    saved.setFactValue(settings->outputRateHz(), 0);
    saved.setFactValue(settings->headingOffsetDeg(), 5.0);
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "ReceiverConfigurationSettings");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    const auto descriptors = GPSReceiverSettingsPresentation::settingDescriptors(
        GPSReceiverCapabilities::forType(static_cast<GPSType>(family)), false);
    std::unique_ptr<QObject> panel(component.createWithInitialProperties(
        {{QStringLiteral("descriptors"), descriptors}, {QStringLiteral("receiverActive"), false}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* visualRoot = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(visualRoot);
    const auto findControl = [visualRoot](const QString& name) { return findVisualItem(visualRoot, name); };
    if (heading) {
        QTRY_VERIFY_WITH_TIMEOUT(findControl(QStringLiteral("receiverSetting_headingOffsetDeg")),
                                 TestTimeout::mediumMs());
    } else {
        QVERIFY(!findControl(QStringLiteral("receiverSetting_headingOffsetDeg")));
    }
    if (heading) {
        auto* reset = findControl(QStringLiteral("receiverSetting_dynamicModel_reset"));
        QVERIFY(reset);
        QVERIFY(QMetaObject::invokeMethod(reset, "clicked"));
        QCOMPARE(settings->dynamicModel()->rawValue().toInt(), 0);
        QTRY_VERIFY_WITH_TIMEOUT(findControl(QStringLiteral("receiverSetting_headingOffsetDeg")),
                                 TestTimeout::mediumMs());
        auto* headingControl = findControl(QStringLiteral("receiverSetting_headingOffsetDeg"));
        QVERIFY(headingControl->property("enabled").toBool());
        panel->setProperty("receiverActive", true);
        QVERIFY(!headingControl->property("enabled").toBool());
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(findControl(QStringLiteral("receiverSetting_dynamicModel")), TestTimeout::mediumMs());
        auto* dynamic = findControl(QStringLiteral("receiverSetting_dynamicModel"));
        QVERIFY(QMetaObject::invokeMethod(dynamic, "activated", Q_ARG(int, 1)));
        QCOMPARE(settings->dynamicModel()->rawValue().toInt(), 2);
        auto* sbas = findControl(QStringLiteral("receiverSetting_constellationMask_2"));
        QVERIFY(sbas);
        sbas->setProperty("checked", true);
        QVERIFY(QMetaObject::invokeMethod(sbas, "clicked"));
        QCOMPARE(settings->constellationMask()->rawValue().toInt(), 3);
        panel->setProperty("receiverActive", true);
        QVERIFY(!dynamic->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(dynamic, "activated", Q_ARG(int, 2)));
        QCOMPARE(settings->dynamicModel()->rawValue().toInt(), 2);
    }
}

void GPSManagerTest::_correctionDiagnosticsPanel()
{
    GPSCorrectionManager corrections;
    auto source = corrections.registerSource(GPSCorrectionSource::Udp, QStringLiteral("test-source"));
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "CorrectionDiagnostics");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(
        component.createWithInitialProperties({{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    corrections.acceptIngress(source.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QTRY_VERIFY_WITH_TIMEOUT(corrections.events()->rowCount() > 0, TestTimeout::mediumMs());
    auto* toggle = panel->findChild<QObject*>(QStringLiteral("correctionHistoryToggle"));
    QVERIFY(toggle);
    QVERIFY(toggle->setProperty("checked", true));
    QTRY_VERIFY_WITH_TIMEOUT(panel->findChild<QObject*>(QStringLiteral("correctionEventHistory")),
                             TestTimeout::mediumMs());
    corrections.shutdown();
}

void GPSManagerTest::_receiverSettingsReentrantTransportChange()
{
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), 2101, 0);
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* automatic = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(automatic->autoConnectRTKGPS(), false);
    saved.setFactValue(automatic->autoConnectNetworkRTKGPS(), true);
    settings->connectionType()->setRawValue(RTKSettings::Serial);
    GPSManager manager;
    manager.init();
    bool switched = false;
    connect(manager.rtkConnection(), &GPSReceiverAutoConnect::stateChanged, &manager, [&]() {
        if (!switched) {
            switched = true;
            settings->connectionType()->setRawValue(RTKSettings::Serial);
        }
    });
    settings->connectionType()->setRawValue(RTKSettings::Tcp);
    QVERIFY(switched);
    QCOMPARE(settings->connectionType()->rawValue().toInt(), int(RTKSettings::Serial));
    QVERIFY(!manager.rtkConnection()->active());
    QVERIFY(!manager.rtkConnection()->networkActive());
}

void GPSManagerTest::_receiverSettingsNotificationCanDestroyManager()
{
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), 2101, 0);
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), false);
    auto manager = std::make_unique<GPSManager>();
    connect(manager->rtkConnection(), &GPSReceiverAutoConnect::stateChanged, this, [&]() { manager.reset(); });
    settings->networkBasePort()->setRawValue(2102);
    QVERIFY(!manager);
}

void GPSManagerTest::_saveBaseReference()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), server.serverPort(), 3);
    auto* settings = SettingsManager::instance();
    auto* base = settings->rtkSettings();
    saved.setFactValue(base->useFixedBasePosition(), true);
    saved.setFactValue(base->fixedBasePositionLatitude(), 1.0);
    saved.setFactValue(base->fixedBasePositionLongitude(), 2.0);
    saved.setFactValue(base->fixedBasePositionAltitude(), 3.0);
    saved.setFactValue(base->fixedBasePositionAccuracy(), 0.0);
    GPSManager manager(*settings, nullptr, []() { return false; });
    QVERIFY(!manager.canSaveBaseReference());
    QVERIFY(manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    auto* session = manager.receiverSession();
    GPSSurveyInStatus reference;
    reference.latitude = 47.5;
    reference.longitude = 8.5;
    reference.altitude = 500;
    reference.valid = true;
    reference.sessionId = session->sessionId();
    reference.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    reference.altitudeDatum = GPSObservation::AltitudeDatum::Unknown;
    emit session->surveyInReceived(reference);
    QVERIFY(!manager.canSaveBaseReference());
    QVERIFY(!manager.baseReferenceSaveError().isEmpty());
    QCOMPARE(base->fixedBasePositionLatitude()->rawValue().toDouble(), 1.0);
    QCOMPARE(base->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.0);

    reference.altitudeDatum = GPSObservation::AltitudeDatum::Ellipsoid;
    emit session->surveyInReceived(reference);
    QVERIFY(manager.canSaveBaseReference());
    QVERIFY(manager.baseReferenceSaveError().isEmpty());
    const quint64 savedSession = session->sessionId();
    QSignalSpy settingsChanged(manager.rtkConnection(), &GPSReceiverAutoConnect::stateChanged);
    QVERIFY(manager.saveBaseReference());
    QCOMPARE(settingsChanged.count(), 1);
    QCOMPARE(session->sessionId(), savedSession);
    QCOMPARE(session->config().base.fixedBaseLatitude, 1.0);
    QCOMPARE(base->fixedBasePositionLatitude()->rawValue().toDouble(), 47.5);
    QCOMPARE(base->fixedBasePositionLongitude()->rawValue().toDouble(), 8.5);
    QCOMPARE(base->fixedBasePositionAltitude()->rawValue().toDouble(), 500.0);
    QCOMPARE(base->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.0);
    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.receiver()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(manager.receiver()->connected(), TestTimeout::mediumMs());
    QCOMPARE(session->config().base.fixedBaseLatitude, 47.5);
    QCOMPARE(session->config().base.fixedBaseLongitude, 8.5);
    QCOMPARE(session->config().base.fixedBaseAltitudeMeters, 500.0f);
    QCOMPARE(session->config().base.fixedBaseAccuracyMeters, 0.0f);
    QVERIFY(session->config().validationError().isEmpty());

    reference.sessionId = session->sessionId();
    reference.latitude = 48.5;
    emit session->surveyInReceived(reference);
    QVERIFY(manager.canSaveBaseReference());
    connect(base->fixedBasePositionLatitude(), &Fact::rawValueChanged, &manager,
            [&]() { base->receiverRole()->setRawValue(RTKSettings::Position); });
    QVERIFY(manager.saveBaseReference());
    QVERIFY(!session->hasReceiver());
    QCOMPARE(base->receiverRole()->rawValue().toInt(), static_cast<int>(RTKSettings::Position));
    QVERIFY(!manager.canSaveBaseReference());
}
