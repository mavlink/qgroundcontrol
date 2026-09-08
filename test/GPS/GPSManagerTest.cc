#include "GPSManagerTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QSettings>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "ColoredSvgImageProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GpsTestHelpers.h"
#include "LinkManager.h"
#include "RTCMMavlink.h"
#include "RTKAutoConnect.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

namespace {
void saveNetworkSettings(TestFixtures::SettingsFixture& saved, const QString& host, int port, int type)
{
    saved.setFactValue(SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS(), false);
    auto* settings = SettingsManager::instance()->rtkSettings();
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
    QSignalSpy active(&manager, &GPSManager::networkRtkActiveChanged);
    QVERIFY(!manager.connectNetworkRtk());
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    QVERIFY(active.isEmpty());
    SettingsManager::instance()->autoConnectSettings()->autoConnectNetworkRTKGPS()->setRawValue(true);
    manager._rtkAutoConnect->update();
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
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
    auto* receiver = manager.gpsRtk();
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver->gpsRtkFactGroup());
    QVERIFY(facts);

    expectLogMessage("GPS.RTK.TcpGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    manager._rtkAutoConnect->update();
    QVERIFY(manager.networkRtkActive());
    QVERIFY(!receiver->connected());
    QVERIFY(!manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::OpenFailed));

    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._rtkAutoConnect->update();
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

    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    server.peer->abort();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!receiver->connected());
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._rtkAutoConnect->update();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(server.connections, 2);
    QCOMPARE(server.commands.count(originalPosition), 2);
    QCOMPARE(replacement.connections, 0);

    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!receiver->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->stopping(), TestTimeout::mediumMs());
    QVERIFY(!receiver->connected());
    manager._rtkAutoConnect->update();
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
    auto* forwarder = manager.corrections()->rtcmMavlink();
    auto* receiver = manager.gpsRtk();

    manager._rtkAutoConnect->update();
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
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::longMs());
    verifyExpectedLogMessage();
    QVERIFY(!receiver->connected());
    QVERIFY(manager.rtkConnection()->active());
    server.respond = true;
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._rtkAutoConnect->update();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QVERIFY(server.acknowledgedCommands > initialCommands);

    manager.disconnectRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.rtkConnection()->autoConnectPaused());
    manager._rtkAutoConnect->update();
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
    manager._rtkAutoConnect->update();
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
    manager.init();
    QVERIFY(!manager.networkRtkActive());
    manager._updateConnections();
    QVERIFY(manager.networkRtkActive());
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 1);

    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.networkRtkAutoConnectPaused());
    QVERIFY(settings->autoConnectNetworkRTKGPS()->rawValue().toBool());
    manager._updateConnections();
    manager._updateConnections();
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 1);

    // A fresh manager reads the saved option without requiring a setting-change signal.
    {
        GPSManager restarted;
        restarted.init();
        QVERIFY(!restarted.networkRtkAutoConnectPaused());
        restarted._updateConnections();
        QTRY_VERIFY_WITH_TIMEOUT(restarted.gpsRtk()->connected(), TestTimeout::mediumMs());
        QCOMPARE(server.connections, 2);
    }
    QVERIFY(manager.networkRtkAutoConnectPaused());
    QVERIFY(manager.connectNetworkRtk());
    QVERIFY(!manager.networkRtkAutoConnectPaused());
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
    QCOMPARE(server.connections, 3);
    settings->autoConnectNetworkRTKGPS()->setRawValue(false);
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    manager._updateConnections();
    QVERIFY(!manager.networkRtkActive());

    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    manager._updateConnections();
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    QVERIFY(manager.networkRtkAutoConnectPaused());
    settings->autoConnectNetworkRTKGPS()->setRawValue(false);
    settings->autoConnectNetworkRTKGPS()->setRawValue(true);
    QVERIFY(!manager.networkRtkAutoConnectPaused());
    manager._updateConnections();
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
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
    manager._updateConnections();
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    links->setConnectionsAllowed();
    manager._updateConnections();
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
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
    delete manager._rtkAutoConnect;
    manager._rtkAutoConnect =
        new RTKAutoConnect(manager.gpsRtk(), settings, SettingsManager::instance()->rtkSettings(), &manager);
    manager._rtkAutoConnect->setSerialDiscovery(&ports);
    // Observe discovery requests without opening the synthetic serial device.
    QSignalSpy serialConnects(manager._rtkAutoConnect, &RTKAutoConnect::connectRequested);
    QSignalSpy serialDisconnects(manager._rtkAutoConnect, &RTKAutoConnect::disconnectRequested);
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
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
    manager._updateConnections();
    QCOMPARE(serialConnects.size(), 1);
    QVERIFY(settings->autoConnectRTKGPS()->rawValue().toBool());
    manager.disconnectRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
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
    QTest::newRow("tcp") << int(RTKSettings::Tcp);
    QTest::newRow("udp") << int(RTKSettings::Udp);
}

void GPSManagerTest::_networkSettingsPanel()
{
    QFETCH(int, connection);
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QString(), 2101, 0);
    SettingsManager::instance()->rtkSettings()->connectionType()->setRawValue(connection);
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/LocalRtkSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    auto* button = root->findChild<QObject*>(QStringLiteral("networkRtkConnectButton"));
    auto* host = root->findChild<QObject*>(QStringLiteral("networkRtkHost"));
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
    QVERIFY(root->property("_invalidConnection").toBool());
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
    const auto disconnect = qScopeGuard([]() { GPSManager::instance()->disconnectNetworkRtk(); });
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    QVERIFY(GPSManager::instance()->networkRtkActive());
    QVERIFY(!host->property("enabled").toBool());
    QCOMPARE(button->property("text").toString(), QStringLiteral("Disconnect"));
    QTRY_VERIFY_WITH_TIMEOUT(GPSManager::instance()->gpsRtk()->connected(), TestTimeout::mediumMs());
    auto* status = root->findChild<QObject*>(QStringLiteral("networkRtkStatus"));
    QVERIFY(status);
    QTRY_COMPARE_WITH_TIMEOUT(status->property("text").toString(), QStringLiteral("Connected"),
                              TestTimeout::mediumMs());
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    QVERIFY(!GPSManager::instance()->networkRtkActive());
    QVERIFY(host->property("enabled").toBool());
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
    manager->_rtkAutoConnect->update();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager->_updateConnections();
                                 return manager->gpsRtk()->connected();
                             })(),
                             TestTimeout::mediumMs());
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    manager->_rtkAutoConnect->update();
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
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(manager.nmeaConnection()->status(), QStringLiteral("Connected"), TestTimeout::mediumMs());
    QVERIFY(manager.nmeaConnection()->active());
    manager.disconnectNmea();
    manager._updateConnections();
    QVERIFY(!manager.nmeaConnection()->active());
    QVERIFY(manager.gpsRtk()->connected());
    QVERIFY(manager.connectNmea());
    QTRY_COMPARE_WITH_TIMEOUT(manager.nmeaConnection()->status(), QStringLiteral("Connected"), TestTimeout::mediumMs());
    manager.disconnectRtk();
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
    manager._updateConnections();
    QVERIFY(!manager.gpsRtk()->hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(!manager.gpsRtk()->stopping(), TestTimeout::mediumMs());
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
