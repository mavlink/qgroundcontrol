#include "GPSCorrectionManagerTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "FactGroup.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSRtk.h"
#include "GpsTestHelpers.h"
#include "MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
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
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Automatic);
    saved.setFactValue(settings->correctionSourceInstance(), QString());
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
    saved.setFactValue(ntripSettings->ntripUdpForwardEnabled(), false);
    GPSCorrectionManager corrections;
    NTRIPManager ntrip;
    auto* stream = new MockNTRIPTransport(&ntrip);
    stream->autoConnect = false;
    ntrip.setTransportForTest(stream);
    ntrip.setCorrectionManager(&corrections);
    ntrip.init();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::All);
    corrections.init(settings);
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    const auto localToken = local.token();
    auto* forwarder = corrections.rtcmMavlink();
    QCOMPARE(ntrip.rtcmMavlink(), forwarder);
    QCOMPARE(forwarder->parent(), &corrections);
    QVERIFY(corrections._udpInput.isRunning());
    QList<uint8_t> sequences;
    forwarder->setOutputProvider([&]() {
        return QList<RTCMMavlink::Output>{{QStringLiteral("link"), 1, [&](const GpsRtcmPacket& packet) {
                                               if (((packet.flags >> 1) & 0x03U) == 0) {
                                                   sequences.append(packet.flags >> 3);
                                               }
                                               return true;
                                           }}};
    });
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1077, 500);
    quint64 expected = 0;
    corrections.acceptIngress(localToken.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    ntrip.startNTRIP();
    stream->simulateRtcmData(frame, 1077);
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    ntrip.stopNTRIP();
    QVERIFY(corrections._udpInput.isRunning());
    corrections.acceptIngress(localToken.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    expected += frame.size();
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    QCOMPARE(ntrip.connectionStats()->bytesReceived(), quint64(frame.size()));
    QCOMPARE(forwarder->totalBytesSubmitted(), expected);
    QCOMPARE(sequences, QList<uint8_t>({0, 1, 2, 3, 4}));
    local.reset();
    corrections.shutdown();
    QVERIFY(!corrections._udpInput.isRunning());
    corrections.acceptIngress(localToken.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
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
    // Validation changes must discard the previous stream's partial frame.
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
    // Other sources remain available when UDP input is disabled.
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    corrections.acceptIngress(ntrip.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
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

void GPSCorrectionManagerTest::_settingsOwnRouting_data()
{
    using Policy = GPSCorrectionManager::RoutingPolicy;
    QTest::addColumn<int>("configuredSource");
    QTest::addColumn<Policy>("expectedPolicy");
    QTest::addColumn<GPSCorrectionSource>("expectedSource");
    QTest::newRow("automatic") << int(GPSCorrectionSettings::Automatic) << Policy::Automatic
                               << GPSCorrectionSource::Unknown;
    QTest::newRow("local") << int(GPSCorrectionSettings::LocalReceiver) << Policy::Manual
                           << GPSCorrectionSource::LocalReceiver;
    QTest::newRow("ntrip") << int(GPSCorrectionSettings::Ntrip) << Policy::Manual << GPSCorrectionSource::Ntrip;
    QTest::newRow("udp") << int(GPSCorrectionSettings::Udp) << Policy::Manual << GPSCorrectionSource::Udp;
    QTest::newRow("all") << int(GPSCorrectionSettings::All) << Policy::All << GPSCorrectionSource::Unknown;
}

void GPSCorrectionManagerTest::_settingsOwnRouting()
{
    QFETCH(int, configuredSource);
    QFETCH(GPSCorrectionManager::RoutingPolicy, expectedPolicy);
    QFETCH(GPSCorrectionSource, expectedSource);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    const auto port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    settings->correctionSource()->setRawValue(configuredSource);
    settings->correctionSourceInstance()->setRawValue(QStringLiteral("initial"));
    GPSCorrectionManager corrections;
    bool routingAppliedBeforeIngress = false;
    connect(&corrections._udpInput, &RTCMUdpInput::runningChanged, this, [&]() {
        if (corrections._udpInput.isRunning()) {
            routingAppliedBeforeIngress = corrections.routingPolicy() == expectedPolicy &&
                                          corrections.selectedSource() == expectedSource &&
                                          corrections._router.selectedInstance() == QStringLiteral("initial");
        }
    });
    corrections.init(settings);
    QVERIFY(routingAppliedBeforeIngress);
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    QCOMPARE(corrections.routingPolicy(), GPSCorrectionManager::RoutingPolicy::Manual);
    QCOMPARE(corrections.selectedSource(), GPSCorrectionSource::Ntrip);
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto ingress = source.token().event(bytes, GPSCorrectionFrame::monotonicNowMs(), 1005, true);
    corrections.acceptIngress(ingress);
    QVERIFY(routed.isEmpty());
    settings->correctionSourceInstance()->setRawValue(QStringLiteral("caster"));
    corrections.acceptIngress(ingress);
    QCOMPARE(routed.size(), 1);
    corrections.shutdown();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::All);
    QCOMPARE(corrections.routingPolicy(), GPSCorrectionManager::RoutingPolicy::Manual);
}

void GPSCorrectionManagerTest::_shutdownDuringAdmission_data()
{
    QTest::addColumn<bool>("retireFromProvider");
    QTest::addColumn<bool>("destroy");
    QTest::newRow("provider-shutdown") << true << false;
    QTest::newRow("packet-shutdown") << false << false;
    QTest::newRow("provider-delete") << true << true;
    QTest::newRow("packet-delete") << false << true;
}

void GPSCorrectionManagerTest::_shutdownDuringAdmission()
{
    QFETCH(bool, retireFromProvider);
    QFETCH(bool, destroy);
    QPointer<GPSCorrectionManager> corrections = new GPSCorrectionManager;
    const auto cleanup = qScopeGuard([&]() { delete corrections.data(); });
    QSignalSpy updates(corrections, &GPSCorrectionManager::sourcesChanged);
    int packetCalls = 0;
    int laterCalls = 0;
    const auto retire = [&]() {
        corrections->shutdown();
        QCoreApplication::sendPostedEvents(corrections.data(), QEvent::MetaCall);
        QCOMPARE(updates.size(), 0);
        if (destroy) {
            delete corrections.data();
        }
    };
    corrections->rtcmMavlink()->setOutputProvider([&]() {
        if (retireFromProvider) {
            retire();
        }
        return QList<RTCMMavlink::Output>{{QStringLiteral("mavlink/first"), 1,
                                           [&](const GpsRtcmPacket&) {
                                               ++packetCalls;
                                               retire();
                                               return true;
                                           }},
                                          {QStringLiteral("mavlink/later"), 2, [&](const GpsRtcmPacket&) {
                                               ++laterCalls;
                                               return true;
                                           }}};
    });
    auto source = corrections->registerSource(GPSCorrectionSource::Ntrip);
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1077, 500);
    corrections->acceptIngress(source.token().event(bytes, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(packetCalls, retireFromProvider ? 0 : 1);
    QCOMPARE(laterCalls, 0);
    QCOMPARE(corrections.isNull(), destroy);
    if (destroy) {
        return;
    }
    QCOMPARE(updates.size(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(corrections->events()->rowCount(), corrections->_router.events().size());
    QCOMPARE(corrections->rtcmMavlink()->totalBytesSubmitted(), retireFromProvider ? 0ULL : 180ULL);
    QCOMPARE(corrections->rtcmMavlink()->submit(bytes), 0ULL);
    corrections->shutdown();
    QCoreApplication::sendPostedEvents(corrections.data(), QEvent::MetaCall);
    QCOMPARE(updates.size(), 1);
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
            readonly property var ntripForwarder: QGroundControl.ntripManager.rtcmMavlink
            readonly property var legacyBaseFacts: QGroundControl.gpsRtk
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QCOMPARE(root->property("forwarder").value<RTCMMavlink*>(), GPSManager::instance()->corrections()->rtcmMavlink());
    QCOMPARE(root->property("ntripForwarder").value<RTCMMavlink*>(),
             GPSManager::instance()->corrections()->rtcmMavlink());
    QCOMPARE(root->property("legacyBaseFacts").value<FactGroup*>(),
             GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
}

void GPSCorrectionManagerTest::_sourceSelectionAndSessions()
{
    GPSCorrectionManager corrections;
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    const QByteArray data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto initial = corrections.sources().at(static_cast<int>(GPSCorrectionSource::LocalReceiver)).toMap();
    QVERIFY(!initial.value(QStringLiteral("active")).toBool());
    corrections.acceptIngress(GPSCorrectionSourceToken().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QVERIFY(routed.isEmpty());
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto oldToken = ntrip.token();
    auto frame = oldToken.event(data, GPSCorrectionFrame::monotonicNowMs(), 0, true);
    corrections.acceptIngress(frame);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(routed.first().first()).messageId, 1005);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSubmitted(), quint64(0));
    corrections.setSelectedSource(GPSCorrectionSource::LocalReceiver);
    corrections.acceptIngress(frame);
    QCOMPARE(routed.size(), 1);
    corrections.acceptIngress(local.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(routed.size(), 2);
    corrections.setSelectedSource(GPSCorrectionSource::Unknown);
    ntrip.reset();
    corrections.acceptIngress(frame);
    QCOMPARE(routed.size(), 2);
    ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    QVERIFY(ntrip.token().session() != oldToken.session());
    corrections.acceptIngress(frame);
    QCOMPARE(routed.size(), 2);
    frame = ntrip.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 0, true);
    corrections.acceptIngress(frame);
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
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto now = GPSCorrectionFrame::monotonicNowMs();
    auto frame = ntrip.token().event(data, now, 1005, true, true);
    corrections.acceptIngress(frame);
    auto stats = corrections.sources().at(static_cast<int>(GPSCorrectionSource::Ntrip)).toMap();
    QVERIFY(stats.value(QStringLiteral("usable")).toBool());
    QCOMPARE(stats.value(QStringLiteral("filteredFrames")).toULongLong(), quint64(1));
    QVERIFY(routed.isEmpty());
    frame = ntrip.token().event(data, now - 6000, 1005, true);
    corrections.acceptIngress(frame);
    stats = corrections.sources().at(static_cast<int>(GPSCorrectionSource::Ntrip)).toMap();
    // Older deliveries cannot expire newer observations.
    QVERIFY(stats.value(QStringLiteral("usable")).toBool());
    QCOMPARE(stats.value(QStringLiteral("filteredFrames")).toULongLong(), quint64(2));
    QVERIFY(routed.isEmpty());
    frame = ntrip.token().event(data, now + 60000, 1005, true);
    corrections.acceptIngress(frame);
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
    corrections.setOutput(
        QStringLiteral("localReceiver"),
        {.completion = GPSCorrectionRouter::Completion::Reported, .admit = [&](const GPSCorrectionFrame& frame) {
             receiverFrame = frame;
             return QList<GPSCorrectionRouter::Admission>{
                 {QStringLiteral("localReceiver"), {quint64(frame.data.size()), 1, GPSCorrectionReason::None}}};
         }});
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
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto now = GPSCorrectionFrame::monotonicNowMs();
    corrections.acceptIngress(local.token().event(data, now, 1005, true));
    corrections.acceptIngress(ntrip.token().event(data, now, 1005, true));
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
    corrections.acceptIngress(ntrip.token().event(data, now, 1005, true, true));
    const auto retired = ntrip.token();
    ntrip.reset();
    corrections.acceptIngress(retired.event(data, now, 1005, true));
    QCOMPARE(outputStats().value(QStringLiteral("queuedBytes")).toULongLong(), quint64(data.size()));
    corrections.configureNtripUdpOutput(false, {}, 0);
    auto next = corrections.registerSource(GPSCorrectionSource::Ntrip);
    corrections.acceptIngress(next.token().event(data, now, 1005, true));
    QCOMPARE(outputStats().value(QStringLiteral("queuedBytes")).toULongLong(), quint64(data.size()));
    corrections.shutdown();
    QVERIFY(!corrections._ntripUdpOutput.isEnabled());
}

void GPSCorrectionManagerTest::_ntripUdpOutputEndpointChanges_data()
{
    QTest::addColumn<QString>("initialAddress");
    QTest::addColumn<QString>("address");
    QTest::addColumn<quint16>("port");
    QTest::addColumn<bool>("unchanged");
    QTest::addColumn<bool>("expectedEnabled");
    const quint16 port = 13320;
    QTest::newRow("same-ipv4") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1") << port << true << true;
    QTest::newRow("equivalent-ipv6") << QStringLiteral("::1") << QStringLiteral("0:0:0:0:0:0:0:1") << port << true
                                     << true;
    QTest::newRow("ipv6-case-and-zeroes")
        << QStringLiteral("2001:db8::a") << QStringLiteral("2001:0DB8:0:0:0:0:0:000A") << port << true << true;
    QTest::newRow("same-scope") << QStringLiteral("fe80::1%1") << QStringLiteral("fe80:0:0:0:0:0:0:1%1") << port << true
                                << true;
    QTest::newRow("changed-address") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.2") << port << false
                                     << true;
    QTest::newRow("changed-port") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1") << quint16(port + 1)
                                  << false << true;
    QTest::newRow("changed-scope") << QStringLiteral("fe80::1%1") << QStringLiteral("fe80::1%2") << port << false
                                   << true;
    QTest::newRow("mapped-ipv4") << QStringLiteral("127.0.0.1") << QStringLiteral("::ffff:127.0.0.1") << port << false
                                 << true;
    QTest::newRow("loopback-protocol") << QStringLiteral("::1") << QStringLiteral("127.0.0.1") << port << false << true;
    QTest::newRow("empty-address") << QStringLiteral("127.0.0.1") << QString() << port << false << false;
    QTest::newRow("hostname") << QStringLiteral("127.0.0.1") << QStringLiteral("localhost") << port << false << false;
    QTest::newRow("zero-port") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1") << quint16(0) << false
                               << false;
}

void GPSCorrectionManagerTest::_ntripUdpOutputEndpointChanges()
{
    QFETCH(QString, initialAddress);
    QFETCH(QString, address);
    QFETCH(quint16, port);
    QFETCH(bool, unchanged);
    QFETCH(bool, expectedEnabled);
    GPSCorrectionManager corrections;
    corrections.configureNtripUdpOutput(true, initialAddress, 13320);
    QVERIFY(corrections._ntripUdpOutput.isEnabled());
    auto* socket = corrections._ntripUdpOutput.findChild<QUdpSocket*>();
    QVERIFY(socket);
    // IPv4 binding exposes socket retirement without requiring IPv6.
    QVERIFY(socket->bind(QHostAddress::LocalHost, 0));
    const quint16 boundPort = socket->localPort();
    if (!expectedEnabled) {
        expectLogMessage("Utilities.UdpForwarder", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Invalid UDP forward config:")));
    }
    corrections.configureNtripUdpOutput(true, address, port);
    if (!expectedEnabled) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(corrections._ntripUdpOutput.isEnabled(), expectedEnabled);
    QCOMPARE(corrections._ntripUdpOutput.port(), expectedEnabled ? port : quint16(0));
    QCOMPARE(corrections._ntripUdpOutput.address(), expectedEnabled ? QHostAddress(address).toString() : QString());
    QCOMPARE(socket->state(), unchanged ? QAbstractSocket::BoundState : QAbstractSocket::UnconnectedState);
    if (unchanged) {
        QCOMPARE(socket->localPort(), boundPort);
    }
}

void GPSCorrectionManagerTest::_sourceTopologyDoesNotNotifyOnCounters()
{
    GPSCorrectionManager corrections;
    QSignalSpy topology(&corrections, &GPSCorrectionManager::sourceInstancesChanged);
    QSignalSpy counters(&corrections, &GPSCorrectionManager::sourcesChanged);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    const auto frame =
        ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true);
    corrections.acceptIngress(frame);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    corrections.acceptIngress(frame);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 2);
    ntrip.reset();
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 2);
    QVERIFY(corrections.sourceInstances().isEmpty());
}

UT_REGISTER_TEST(GPSCorrectionManagerTest, TestLabel::Unit)
