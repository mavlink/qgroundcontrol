#include "GPSManagerTest.h"

#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "ColoredSvgImageProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "LinkManager.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "RTKAutoConnect.h"
#include "SerialPortManager.h"
#endif

namespace {
void saveNetworkSettings(TestFixtures::SettingsFixture& saved, const QString& host, int port, int type)
{
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->networkBaseHost(), host);
    saved.setFactValue(settings->networkBasePort(), port);
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
            connect(peer, &QTcpSocket::readyRead, peer, [socket = peer]() {
                while (socket->canReadLine()) {
                    const QByteArray command = socket->readLine().trimmed();
                    const QByteArray reply = '<' + command.split(' ').first() + " OK" + char(0);
                    socket->write(reply);
                }
            });
        });
    }

    QTcpSocket* peer = nullptr;
    int connections = 0;
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
    QVERIFY(active.isEmpty());
}

void GPSManagerTest::_networkRecoveryAndDisconnect()
{
    ReceiverServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral(" 127.0.0.1 "), port, 3);
    GPSManager manager;
    QSignalSpy active(&manager, &GPSManager::networkRtkActiveChanged);
    auto* receiver = manager.gpsRtk();
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver->gpsRtkFactGroup());
    QVERIFY(facts);

    expectLogMessage("GPS.TcpGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    QVERIFY(manager.connectNetworkRtk());
    QVERIFY(manager.networkRtkActive());
    QVERIFY(!receiver->connected());
    QVERIFY(!manager.connectNetworkRtk());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::OpenFailed));

    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateNetworkRtk();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(server.connections, 1);
    QCOMPARE(facts->lastError()->rawValue().toInt(), static_cast<int>(GPSConnectionError::None));

    expectLogMessage("GPS.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    server.peer->abort();
    QTRY_VERIFY_WITH_TIMEOUT(!receiver->hasReceiver(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!receiver->connected());
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateNetworkRtk();
                                 return receiver->connected();
                             })(),
                             TestTimeout::mediumMs());
    QCOMPARE(server.connections, 2);

    manager.disconnectNetworkRtk();
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!receiver->hasReceiver());
    QVERIFY(!receiver->connected());
    manager._updateNetworkRtk();
    QVERIFY(!receiver->hasReceiver());
    QCOMPARE(active.size(), 2);
}

void GPSManagerTest::_suspendedConnections()
{
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QStringLiteral("localhost"), 2101, 0);
    auto* links = LinkManager::instance();
    QVERIFY(!links->connectionsSuspended());
    const auto restore = qScopeGuard([links]() { links->setConnectionsAllowed(); });
    links->setConnectionsSuspended(QStringLiteral("test"));
    GPSManager manager;
    QVERIFY(!manager.connectNetworkRtk());
    QVERIFY(!manager.networkRtkActive());
    QVERIFY(!manager.gpsRtk()->hasReceiver());
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
    GPSManager manager;
    manager.init();
    delete manager._rtkAutoConnect;
    manager._rtkAutoConnect = new RTKAutoConnect(settings, manager.gpsRtk(), &ports, &manager);
    // Observe discovery requests without opening the synthetic serial device.
    QSignalSpy serialConnects(manager._rtkAutoConnect, &RTKAutoConnect::connectRequested);
    QSignalSpy serialDisconnects(manager._rtkAutoConnect, &RTKAutoConnect::disconnectRequested);
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateConnections();
                                 return serialConnects.size() == 1;
                             })(),
                             TestTimeout::longMs());
    QVERIFY(manager.connectNetworkRtk());
    QCOMPARE(serialDisconnects.size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(manager.gpsRtk()->connected(), TestTimeout::mediumMs());
    manager._updateConnections();
    QCOMPARE(serialConnects.size(), 1);
    QVERIFY(settings->autoConnectRTKGPS()->rawValue().toBool());
    manager.disconnectNetworkRtk();
    QTRY_VERIFY_WITH_TIMEOUT(([&]() {
                                 manager._updateConnections();
                                 return serialConnects.size() == 2;
                             })(),
                             TestTimeout::longMs());
#else
    QSKIP("Serial discovery is unavailable in this build");
#endif
}

void GPSManagerTest::_networkSettingsPanel()
{
    TestFixtures::SettingsFixture saved;
    saveNetworkSettings(saved, QString(), 2101, 0);
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine,
                            QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NetworkRtkSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    auto* button = root->findChild<QObject*>(QStringLiteral("networkRtkConnectButton"));
    auto* host = root->findChild<QObject*>(QStringLiteral("networkRtkHost"));
    QVERIFY(button);
    QVERIFY(host);
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
    auto* settings = SettingsManager::instance()->rtkSettings();
    settings->networkBaseHost()->setRawValue(QStringLiteral("localhost"));
    settings->networkBasePort()->setRawValue(server.serverPort());
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
}

UT_REGISTER_TEST(GPSManagerTest, TestLabel::Unit)
