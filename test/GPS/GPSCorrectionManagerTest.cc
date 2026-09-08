#include "GPSCorrectionManagerTest.h"

#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include <memory>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSRtk.h"
#include "GpsTestHelpers.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"

namespace {
quint16 unusedPort()
{
    QUdpSocket socket;
    return socket.bind(QHostAddress::LocalHost, 0) ? socket.localPort() : 0;
}

void configureUdp(TestFixtures::SettingsFixture& saved, NTRIPSettings* settings, quint16 port)
{
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);
    saved.setFactValue(settings->rtcmUdpInputEnabled(), true);
    saved.setFactValue(settings->rtcmUdpInputPort(), port);
    saved.setFactValue(settings->rtcmUdpValidate(), true);
}
}  // namespace

void GPSCorrectionManagerTest::_sourcesShareForwarder()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    const quint16 port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    GPSManager gps;
    auto* corrections = gps.corrections();
    corrections->init(settings);
    auto* forwarder = corrections->rtcmMavlink();
    QCOMPARE(forwarder->parent(), corrections);
    QVERIFY(corrections->_udpInput.isRunning());
    NTRIPManager ntrip;
    connect(&ntrip, &NTRIPManager::rtcmDataReceived, corrections, &GPSCorrectionManager::forwardCorrections);
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1077, 500);
    quint64 expected = 0;
    // Local RTK and UDP work before any NTRIP initialization or caster connection.
    emit gps.gpsRtk()->rtcmDataReceived(frame);
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    emit ntrip.rtcmDataReceived(frame);
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    ntrip.stopNTRIP();
    QVERIFY(corrections->_udpInput.isRunning());
    emit gps.gpsRtk()->rtcmDataReceived(frame);
    expected += frame.size();
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    QCOMPARE(ntrip.connectionStats()->bytesReceived(), quint64(0));
    gps.shutdown();
    QVERIFY(!corrections->_udpInput.isRunning());
    emit ntrip.rtcmDataReceived(frame);
    emit gps.gpsRtk()->rtcmDataReceived(frame);
    QCOMPARE(forwarder->totalBytesSent(), expected);
}

void GPSCorrectionManagerTest::_udpSettingsAndShutdown()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    const quint16 port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    GPSCorrectionManager corrections;
    corrections.init(settings);
    corrections.init(settings);
    QVERIFY(corrections._udpInput.isRunning());
    QUdpSocket sender;
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 30);
    auto* socket = corrections._udpInput.findChild<QUdpSocket*>();
    QVERIFY(socket);
    QSignalSpy reads(socket, &QUdpSocket::readyRead);
    const QByteArray prefix = frame.first(5);
    QCOMPARE(sender.writeDatagram(prefix, QHostAddress::LocalHost, port), prefix.size());
    QTRY_VERIFY_WITH_TIMEOUT(!reads.isEmpty(), TestTimeout::mediumMs());
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(0));
    settings->rtcmUdpValidate()->setRawValue(false);
    const QByteArray raw = QByteArrayLiteral("unvalidated input");
    QCOMPARE(sender.writeDatagram(raw, QHostAddress::LocalHost, port), raw.size());
    QTRY_COMPARE_WITH_TIMEOUT(corrections.rtcmMavlink()->totalBytesSent(), quint64(raw.size()),
                              TestTimeout::mediumMs());
    settings->rtcmUdpValidate()->setRawValue(true);
    // Changing validation starts a new parser session; an old partial frame cannot leak through.
    const QByteArray tailAndFrame = frame.sliced(5) + frame;
    QCOMPARE(sender.writeDatagram(tailAndFrame, QHostAddress::LocalHost, port), tailAndFrame.size());
    const quint64 expected = raw.size() + frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.rtcmMavlink()->totalBytesSent(), expected, TestTimeout::mediumMs());
    const quint16 nextPort = unusedPort();
    QVERIFY(nextPort);
    QVERIFY(nextPort != port);
    settings->rtcmUdpInputPort()->setRawValue(nextPort);
    QCOMPARE(corrections._udpInput.port(), nextPort);
    QUdpSocket released;
    QVERIFY(released.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QVERIFY(!corrections._udpInput.isRunning());
    // Disabling UDP input must not disable the other correction sources.
    corrections.forwardCorrections(frame);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), expected + frame.size());
    settings->rtcmUdpInputEnabled()->setRawValue(true);
    QVERIFY(corrections._udpInput.isRunning());
    corrections.shutdown();
    corrections.shutdown();
    settings->rtcmUdpValidate()->setRawValue(false);
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    settings->rtcmUdpInputEnabled()->setRawValue(true);
    QVERIFY(!corrections._udpInput.isRunning());
    QUdpSocket stopped;
    QVERIFY(stopped.bind(QHostAddress::AnyIPv4, nextPort, QUdpSocket::DontShareAddress));
}

void GPSCorrectionManagerTest::_shutdownDuringDelivery_data()
{
    QTest::addColumn<bool>("validate");
    QTest::newRow("validated-frames") << true;
    QTest::newRow("raw-datagrams") << false;
}

void GPSCorrectionManagerTest::_shutdownDuringDelivery()
{
    QFETCH(bool, validate);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    const quint16 port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    settings->rtcmUdpValidate()->setRawValue(validate);
    GPSCorrectionManager corrections;
    corrections.init(settings);
    connect(&corrections._udpInput, &RTCMUdpInput::rtcmDataReceived, &corrections, &GPSCorrectionManager::shutdown);
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1077, 30);
    const QByteArray datagram = validate ? frame + frame : frame;
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(datagram, QHostAddress::LocalHost, port), datagram.size());
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    QTRY_VERIFY_WITH_TIMEOUT(!corrections._udpInput.isRunning(), TestTimeout::mediumMs());
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(frame.size()));
}

void GPSCorrectionManagerTest::_qmlForwarderAvailableBeforeInit()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var forwarder: QGroundControl.gpsManager.corrections.rtcmMavlink
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QCOMPARE(root->property("forwarder").value<RTCMMavlink*>(), GPSManager::instance()->corrections()->rtcmMavlink());
}

UT_REGISTER_TEST(GPSCorrectionManagerTest, TestLabel::Unit)
