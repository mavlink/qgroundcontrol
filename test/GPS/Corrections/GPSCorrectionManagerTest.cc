#include "GPSCorrectionManagerTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include <memory>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverSession.h"
#include "GPSReceiverTestProfile.h"
#include "GPSTransport.h"
#include "GpsTestHelpers.h"
#include "MockNTRIPStream.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

namespace {
quint16 unusedPort()
{
    QUdpSocket socket;
    return socket.bind(QHostAddress::LocalHost, 0) ? socket.localPort() : 0;
}

void configureUdp(TestFixtures::SettingsFixture& saved, GPSCorrectionSettings* settings, quint16 port)
{
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    saved.setFactValue(settings->rtcmUdpInputEnabled(), true);
    saved.setFactValue(settings->rtcmUdpInputPort(), port);
    saved.setFactValue(settings->rtcmUdpValidate(), true);
}
}  // namespace

void GPSCorrectionManagerTest::_sourcesShareForwarder()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    const quint16 port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    auto* ntripSettings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(ntripSettings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(ntripSettings->ntripMountpoint(), QStringLiteral("TEST"));
    NTRIPManager ntrip;
    auto* stream = new MockNTRIPStream(&ntrip);
    stream->autoConnect = false;
    ntrip.setTransportForTest(stream);
    GPSManager gps(*SettingsManager::instance(), nullptr, []() { return true; });
    gps.init(&ntrip);
    auto* corrections = gps.corrections();
    corrections->setRoutingPolicy(GPSCorrectionManager::RoutingPolicy::All);
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto* receiverSession = gps.receiverSession();
    auto releaseOpen = std::make_shared<QSemaphore>();
    const auto cleanup = qScopeGuard([&]() {
        receiverSession->stop();
        releaseOpen->release();
        gps.shutdown();
    });
    receiverSession->start(gpsReceiverTestProfile({}, GPSType::u_blox), [releaseOpen](const std::atomic_bool&) {
        releaseOpen->acquire();
        return std::unique_ptr<GPSTransport>{};
    });
    QVERIFY(receiverSession->hasReceiver());
    const quint64 receiverSessionId = receiverSession->sessionId();
    QVERIFY(receiverSessionId > 0);
    auto* worker = receiverSession->findChild<GPSProvider*>();
    QVERIFY(worker);
    QSignalSpy configurationStarted(receiverSession, &GPSReceiverSession::configurationStarted);
    emit worker->transportOpened();
    QTRY_COMPARE_WITH_TIMEOUT(configurationStarted.size(), 1, TestTimeout::mediumMs());
    corrections->init(settings);
    auto* forwarder = corrections->rtcmMavlink();
    QCOMPARE(forwarder->parent(), corrections);
    QVERIFY(corrections->_udpInput.isRunning());
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1077, 500);
    quint64 expected = 0;
    // Events from retired or missing receiver attempts cannot enter the current source session.
    emit receiverSession->rtcmFrameReceived(frame, GPSCorrectionFrame::monotonicNowMs(), 0);
    emit receiverSession->rtcmFrameReceived(frame, GPSCorrectionFrame::monotonicNowMs(), receiverSessionId + 1);
    QCOMPARE(forwarder->totalBytesSent(), expected);
    // Local RTK and UDP work before opening a caster connection.
    emit receiverSession->rtcmFrameReceived(frame, GPSCorrectionFrame::monotonicNowMs(), receiverSessionId);
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    ntrip.startNTRIP();
    const auto attemptId = ntrip.correctionAttemptId();
    QVERIFY(attemptId > 0);
    stream->simulateRtcmData(frame, 1077);
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    ntrip.stopNTRIP();
    QVERIFY(corrections->_udpInput.isRunning());
    emit receiverSession->rtcmFrameReceived(frame, GPSCorrectionFrame::monotonicNowMs(), receiverSessionId);
    expected += frame.size();
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    QCOMPARE(ntrip.connectionStats()->bytesReceived(), quint64(frame.size()));
    receiverSession->stop();
    releaseOpen->release();
    gps.shutdown();
    QVERIFY(!corrections->_udpInput.isRunning());
    emit ntrip.correctionReceivedAt(frame, 1077, false, GPSCorrectionFrame::monotonicNowMs(), attemptId);
    emit receiverSession->rtcmFrameReceived(frame, GPSCorrectionFrame::monotonicNowMs(), receiverSessionId);
    QCOMPARE(forwarder->totalBytesSent(), expected);
}

void GPSCorrectionManagerTest::_mavlinkDestinationAdmissions()
{
    GPSCorrectionManager corrections;
    int calls = 0;
    corrections.rtcmMavlink()->setOutputProvider([&]() {
        return QList<RTCMMavlink::Output>{
            {QStringLiteral("mavlink/full"), 1, [](const GpsRtcmPacket&) { return true; }},
            {QStringLiteral("mavlink/partial"), 2, [&](const GpsRtcmPacket&) { return ++calls == 1; }}};
    });
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1077, 500);
    corrections.acceptIngress(source.token().event(bytes, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    const auto sourceStats = corrections.sources()[static_cast<int>(GPSCorrectionSource::Ntrip)].toMap();
    QCOMPARE(sourceStats.value(QStringLiteral("queuedFrames")).toULongLong(), 1ULL);
    QCOMPARE(sourceStats.value(QStringLiteral("queuedBytes")).toULongLong(), quint64(bytes.size()));
    QCOMPARE(sourceStats.value(QStringLiteral("droppedFrames")).toULongLong(), 0ULL);
    int outputs = 0;
    for (const auto& value : corrections.destinations()) {
        const auto destination = value.toMap();
        const auto id = destination.value(QStringLiteral("destinationId")).toString();
        if (id == QStringLiteral("mavlink/full")) {
            ++outputs;
            QCOMPARE(destination.value(QStringLiteral("queuedFrames")).toULongLong(), 1ULL);
        } else if (id == QStringLiteral("mavlink/partial")) {
            ++outputs;
            QCOMPARE(destination.value(QStringLiteral("queuedFrames")).toULongLong(), 0ULL);
            QCOMPARE(destination.value(QStringLiteral("queuedBytes")).toULongLong(), 180ULL);
            QCOMPARE(destination.value(QStringLiteral("droppedBytes")).toULongLong(), quint64(bytes.size() - 180));
        }
        QCOMPARE(destination.value(QStringLiteral("writtenBytes")).toULongLong(), 0ULL);
        QVERIFY(!destination.value(QStringLiteral("reportsWrites")).toBool());
    }
    QCOMPARE(outputs, 2);
}

void GPSCorrectionManagerTest::_udpSettingsAndShutdown()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
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
    corrections.beginSourceSession(GPSCorrectionSource::Ntrip);
    corrections.acceptFrame({GPSCorrectionSource::Ntrip, corrections.sourceSession(GPSCorrectionSource::Ntrip),
                             GPSCorrectionFrame::monotonicNowMs(), frame, 1005, true});
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
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    const quint16 port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    settings->rtcmUdpValidate()->setRawValue(validate);
    GPSCorrectionManager corrections;
    corrections.init(settings);
    connect(&corrections._udpInput, &RTCMUdpInput::frameReceived, &corrections, &GPSCorrectionManager::shutdown);
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
            readonly property var receiverFacts: QGroundControl.gpsReceiver
            readonly property var baseFacts: QGroundControl.gpsReceiver.rtk
            readonly property var latitude: QGroundControl.gpsReceiver.lat
            readonly property var legacyBaseFacts: QGroundControl.gpsRtk
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QCOMPARE(root->property("forwarder").value<RTCMMavlink*>(), GPSManager::instance()->corrections()->rtcmMavlink());
    auto* receiverFacts = root->property("receiverFacts").value<GPSReceiverFactGroup*>();
    auto* baseFacts = root->property("baseFacts").value<GPSBaseStationFactGroup*>();
    QCOMPARE(receiverFacts, GPSManager::instance()->receiver()->facts());
    QCOMPARE(baseFacts, receiverFacts->rtk());
    QCOMPARE(root->property("latitude").value<Fact*>(), receiverFacts->lat());
    QCOMPARE(root->property("legacyBaseFacts").value<GPSBaseStationFactGroup*>(), baseFacts);
    QVERIFY(receiverFacts->metaObject()->indexOfProperty("connected") >= 0);
    QVERIFY(receiverFacts->metaObject()->indexOfProperty("lastError") >= 0);
    QCOMPARE(baseFacts->metaObject()->indexOfProperty("connected"), -1);
    QCOMPARE(baseFacts->metaObject()->indexOfProperty("lastError"), -1);
    QVERIFY(baseFacts->metaObject()->indexOfProperty("currentLatitude") >= 0);
}

void GPSCorrectionManagerTest::_sourceSelectionAndSessions()
{
    GPSCorrectionManager corrections;
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    const QByteArray data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto initial = corrections.sources().at(static_cast<int>(GPSCorrectionSource::LocalReceiver)).toMap();
    QVERIFY(!initial.value(QStringLiteral("active")).toBool());
    corrections.acceptFrame({GPSCorrectionSource::LocalReceiver,
                             corrections.sourceSession(GPSCorrectionSource::LocalReceiver),
                             GPSCorrectionFrame::monotonicNowMs(), data, 1005, true});
    QVERIFY(routed.isEmpty());
    corrections.beginSourceSession(GPSCorrectionSource::LocalReceiver);
    const quint64 oldSession = corrections.beginSourceSession(GPSCorrectionSource::Ntrip);
    GPSCorrectionFrame frame{
        GPSCorrectionSource::Ntrip, oldSession, GPSCorrectionFrame::monotonicNowMs(), data, 0, true, false};
    corrections.acceptFrame(frame);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(routed.first().first()).messageId, 1005);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSubmitted(), quint64(0));
    corrections.setSelectedSource(GPSCorrectionSource::LocalReceiver);
    corrections.acceptFrame(frame);
    QCOMPARE(routed.size(), 1);
    corrections.acceptFrame({GPSCorrectionSource::LocalReceiver,
                             corrections.sourceSession(GPSCorrectionSource::LocalReceiver),
                             GPSCorrectionFrame::monotonicNowMs(), data, 1005, true});
    QCOMPARE(routed.size(), 2);
    corrections.setSelectedSource(GPSCorrectionSource::Unknown);
    corrections.endSourceSession(GPSCorrectionSource::Ntrip);
    corrections.acceptFrame(frame);
    QCOMPARE(routed.size(), 2);
    const quint64 newSession = corrections.beginSourceSession(GPSCorrectionSource::Ntrip);
    QVERIFY(newSession != oldSession);
    corrections.acceptFrame(frame);
    QCOMPARE(routed.size(), 2);
    frame.session = newSession;
    corrections.acceptFrame(frame);
    QCOMPARE(routed.size(), 3);
    const auto stats = corrections.sources().at(static_cast<int>(GPSCorrectionSource::Ntrip)).toMap();
    QCOMPARE(stats.value(QStringLiteral("validatedFrames")).toULongLong(), quint64(1));
    QCOMPARE(stats.value(QStringLiteral("routedFrames")).toULongLong(), quint64(1));
    QVERIFY(stats.value(QStringLiteral("usable")).toBool());
}

void GPSCorrectionManagerTest::_filteredAndExpiredFrames()
{
    GPSCorrectionManager corrections;
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    const quint64 session = corrections.beginSourceSession(GPSCorrectionSource::Ntrip);
    GPSCorrectionFrame frame{GPSCorrectionSource::Ntrip,
                             session,
                             GPSCorrectionFrame::monotonicNowMs(),
                             GpsTestHelpers::buildRtcmFrame(1005, 20),
                             1005,
                             true,
                             true};
    corrections.acceptFrame(frame);
    auto stats = corrections.sources().at(static_cast<int>(GPSCorrectionSource::Ntrip)).toMap();
    QVERIFY(stats.value(QStringLiteral("usable")).toBool());
    QCOMPARE(stats.value(QStringLiteral("filteredFrames")).toULongLong(), quint64(1));
    QVERIFY(routed.isEmpty());
    frame.filtered = false;
    frame.receivedAtMs -= 6000;
    corrections.acceptFrame(frame);
    stats = corrections.sources().at(static_cast<int>(GPSCorrectionSource::Ntrip)).toMap();
    // Late delivery of an old frame must not make a newer valid observation stale.
    QVERIFY(stats.value(QStringLiteral("usable")).toBool());
    QCOMPARE(stats.value(QStringLiteral("filteredFrames")).toULongLong(), quint64(2));
    QVERIFY(routed.isEmpty());
    frame.receivedAtMs = GPSCorrectionFrame::monotonicNowMs() + 60000;
    corrections.acceptFrame(frame);
    stats = corrections.sources().at(static_cast<int>(GPSCorrectionSource::Ntrip)).toMap();
    QCOMPARE(stats.value(QStringLiteral("filteredFrames")).toULongLong(), quint64(3));
    QCOMPARE(stats.value(QStringLiteral("validatedFrames")).toULongLong(), quint64(2));
    QVERIFY(routed.isEmpty());
}

void GPSCorrectionManagerTest::_outputsEnabledAfterLinkHistoryChurn()
{
    GPSCorrectionManager corrections;
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    quint64 linkSession = 0;
    corrections.rtcmMavlink()->setOutputProvider([&]() {
        return QList<RTCMMavlink::Output>{
            {QStringLiteral("mavlink/%1").arg(linkSession), linkSession, [](const GpsRtcmPacket&) { return true; }}};
    });
    for (int index = 0; index < GPSCorrectionRouter::MAX_DESTINATION_HISTORY * 2; ++index) {
        ++linkSession;
        corrections.acceptIngress(source.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    }
    QCOMPARE(corrections.destinations().size(), GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 1);

    GPSCorrectionFrame receiverFrame;
    corrections.addDetailedSink(QStringLiteral("localReceiver"), [&](const GPSCorrectionFrame& frame) {
        receiverFrame = frame;
        return GPSCorrectionRouter::Submission{quint64(frame.data.size()), 1, GPSCorrectionReason::None};
    });
    QUdpSocket udpDestination;
    QVERIFY(udpDestination.bind(QHostAddress::LocalHost, 0));
    corrections.configureNtripUdpOutput(true, QStringLiteral("127.0.0.1"), udpDestination.localPort());
    corrections.acceptIngress(source.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(receiverFrame.data, data);
    QTRY_VERIFY_WITH_TIMEOUT(udpDestination.hasPendingDatagrams(), TestTimeout::mediumMs());
    QByteArray received(data.size(), Qt::Uninitialized);
    QCOMPARE(udpDestination.readDatagram(received.data(), received.size()), qint64(data.size()));
    QCOMPARE(received, data);
    for (const auto& value : corrections.destinations()) {
        const auto destination = value.toMap();
        const auto id = destination.value(QStringLiteral("destinationId")).toString();
        if (id == QStringLiteral("localReceiver") || id == QStringLiteral("ntripUdp")) {
            QCOMPARE(destination.value(QStringLiteral("queuedBytes")).toULongLong(), quint64(data.size()));
        }
    }
    QCOMPARE(corrections.destinations().size(), GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 3);
}

void GPSCorrectionManagerTest::_ntripUdpOutputIsSourceSpecific()
{
    GPSCorrectionManager corrections;
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, 0));
    corrections.configureNtripUdpOutput(true, QStringLiteral("127.0.0.1"), destination.localPort());
    const auto local = corrections.beginSourceSession(GPSCorrectionSource::LocalReceiver);
    const auto ntrip = corrections.beginSourceSession(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto now = GPSCorrectionFrame::monotonicNowMs();
    corrections.acceptFrame({GPSCorrectionSource::LocalReceiver, local, now, data, 1005, true});
    corrections.acceptFrame({GPSCorrectionSource::Ntrip, ntrip, now, data, 1005, true});
    QTRY_VERIFY_WITH_TIMEOUT(destination.hasPendingDatagrams(), TestTimeout::mediumMs());
    QByteArray received(data.size(), Qt::Uninitialized);
    QCOMPARE(destination.readDatagram(received.data(), received.size()), qint64(data.size()));
    QCOMPARE(received, data);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(data.size()));
    const auto outputStats = [&]() {
        for (const auto& value : corrections.destinations()) {
            const auto map = value.toMap();
            if (map.value(QStringLiteral("destinationId")).toString() == QStringLiteral("ntripUdp")) {
                return map;
            }
        }
        return QVariantMap();
    };
    QCOMPARE(outputStats().value(QStringLiteral("queuedBytes")).toULongLong(), quint64(data.size()));
    QVERIFY(!outputStats().value(QStringLiteral("reportsWrites")).toBool());
    corrections.acceptFrame({GPSCorrectionSource::Ntrip, ntrip, now, data, 1005, true, true});
    corrections.endSourceSession(GPSCorrectionSource::Ntrip);
    corrections.acceptFrame({GPSCorrectionSource::Ntrip, ntrip, now, data, 1005, true});
    QCOMPARE(outputStats().value(QStringLiteral("queuedBytes")).toULongLong(), quint64(data.size()));
    corrections.configureNtripUdpOutput(false, {}, 0);
    const auto next = corrections.beginSourceSession(GPSCorrectionSource::Ntrip);
    corrections.acceptFrame({GPSCorrectionSource::Ntrip, next, now, data, 1005, true});
    QCOMPARE(outputStats().value(QStringLiteral("queuedBytes")).toULongLong(), quint64(data.size()));
    corrections.shutdown();
    QVERIFY(!corrections._ntripUdpOutput.isEnabled());
}

void GPSCorrectionManagerTest::_sourceTopologyDoesNotNotifyOnCounters()
{
    GPSCorrectionManager corrections;
    QSignalSpy topology(&corrections, &GPSCorrectionManager::sourceInstancesChanged);
    QSignalSpy counters(&corrections, &GPSCorrectionManager::sourcesChanged);
    const auto session = corrections.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    GPSCorrectionFrame frame{GPSCorrectionSource::Ntrip,
                             session,
                             GPSCorrectionFrame::monotonicNowMs(),
                             GpsTestHelpers::buildRtcmFrame(1005, 20),
                             1005,
                             true};
    corrections.acceptFrame(frame);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    corrections.acceptFrame(frame);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 2);
    corrections.endSourceSession(GPSCorrectionSource::Ntrip);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 2);
    QVERIFY(corrections.sourceInstances().isEmpty());
}

UT_REGISTER_TEST(GPSCorrectionManagerTest, TestLabel::Unit)
