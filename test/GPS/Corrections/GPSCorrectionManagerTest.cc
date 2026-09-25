#include "GPSCorrectionManagerTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "FactGroup.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GPSSettingsBindings.h"
#include "GpsQmlTestHelpers.h"
#include "GpsTestHelpers.h"
#include "ManualScheduler.h"
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

void configureUdpOutput(TestFixtures::SettingsFixture& saved, GPSCorrectionSettings* settings, const QString& address,
                        quint16 port)
{
    saved.setFactValue(settings->rtcmUdpOutputAddress(), address);
    saved.setFactValue(settings->rtcmUdpOutputPort(), port);
    saved.setFactValue(settings->rtcmUdpOutputEnabled(), true);
}

GPSCorrectionDestinationDiagnostic udpOutputStats(const GPSCorrectionManager& corrections)
{
    for (const auto& destination : corrections.destinationDiagnostics()) {
        if (destination.destinationId == QStringLiteral("udpOutput")) {
            return destination;
        }
    }
    return {};
}

bool advanceDiagnostics(ManualScheduler& scheduler)
{
    return scheduler.advanceBy(std::chrono::milliseconds(100));
}

bool advanceHealthSample(ManualScheduler& scheduler)
{
    return scheduler.advanceBy(std::chrono::seconds(1));
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
    saved.setFactValue(settings->rtcmUdpOutputEnabled(), false);
    GPSCorrectionManager corrections;
    NTRIPManager ntrip;
    GPSSettingsBindings::bindNtrip(ntripSettings, &ntrip);
    auto* stream = new MockNTRIPTransport(&ntrip);
    stream->autoConnect = false;
    ntrip.setTransportForTest(stream);
    ntrip.setCorrectionManager(&corrections);
    ntrip.init();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::LocalReceiver);
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    const auto localSource = local.weak();
    auto* forwarder = corrections.rtcmMavlink();
    QCOMPARE(forwarder->parent(), &corrections);
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
    corrections.acceptIngress(localSource.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    ntrip.startNTRIP();
    stream->simulateRtcmData(frame, 1077);
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    ntrip.stopNTRIP();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::LocalReceiver);
    corrections.acceptIngress(localSource.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    expected += frame.size();
    QCOMPARE(forwarder->totalBytesSent(), expected);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(forwarder->totalBytesSent(), expected, TestTimeout::mediumMs());
    QCOMPARE(ntrip.connectionStats()->bytesReceived(), quint64(frame.size()));
    QCOMPARE(forwarder->totalBytesSubmitted(), expected);
    QCOMPARE(sequences, QList<uint8_t>({0, 1, 2, 3, 4}));
    local.reset();
    corrections.shutdown();
    QUdpSocket released;
    QVERIFY(released.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));
    corrections.acceptIngress(localSource.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
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
    corrections.acceptIngress(source.event(bytes, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    const auto sourceStats = corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::Ntrip)];
    QCOMPARE(sourceStats.queuedFrames, 1ULL);
    QCOMPARE(sourceStats.queuedBytes, quint64(bytes.size()));
    QCOMPARE(sourceStats.droppedFrames, 0ULL);
    int outputs = 0;
    for (const auto& value : corrections.destinationDiagnostics()) {
        const auto& destination = value;
        const auto id = destination.destinationId;
        if (id == QStringLiteral("mavlink/full")) {
            ++outputs;
            QCOMPARE(destination.queuedFrames, 1ULL);
        } else if (id == QStringLiteral("mavlink/partial")) {
            ++outputs;
            QCOMPARE(destination.queuedFrames, 0ULL);
            QCOMPARE(destination.queuedBytes, 180ULL);
            QCOMPARE(destination.droppedBytes, quint64(bytes.size() - 180));
        }
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
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    corrections.setUdpInputConfiguration(GPSSettingsBindings::udpInputConfiguration(settings));
    QUdpSocket sender;
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 30);
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    quint64 expected = frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.rtcmMavlink()->totalBytesSent(), expected, TestTimeout::mediumMs());

    const quint16 nextPort = unusedPort();
    QVERIFY(nextPort);
    QVERIFY(nextPort != port);
    settings->rtcmUdpInputPort()->setRawValue(nextPort);
    QUdpSocket released;
    QVERIFY(released.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));

    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, nextPort), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.rtcmMavlink()->totalBytesSent(), expected, TestTimeout::mediumMs());

    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QUdpSocket disabledPort;
    QVERIFY(disabledPort.bind(QHostAddress::AnyIPv4, nextPort, QUdpSocket::DontShareAddress));
    // Other sources remain available when UDP input is disabled.
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    corrections.acceptIngress(ntrip.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    expected += frame.size();
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), expected);
    disabledPort.close();

    settings->rtcmUdpInputEnabled()->setRawValue(true);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, nextPort), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.rtcmMavlink()->totalBytesSent(), expected, TestTimeout::mediumMs());

    corrections.shutdown();
    corrections.shutdown();
    settings->rtcmUdpValidate()->setRawValue(false);
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    settings->rtcmUdpInputEnabled()->setRawValue(true);
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
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    QCOMPARE(corrections.router().policy(), expectedPolicy);
    QCOMPARE(corrections.selectedSource(), expectedSource);
    QCOMPARE(corrections.router().configuration().instance, QStringLiteral("initial"));
    QSignalSpy routed(&corrections.router(), &GPSCorrectionRouter::frameRouted);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    QCOMPARE(corrections.router().policy(), GPSCorrectionManager::RoutingPolicy::Manual);
    QCOMPARE(corrections.selectedSource(), GPSCorrectionSource::Ntrip);
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto ingress = source.event(bytes, GPSCorrectionFrame::monotonicNowMs(), 1005, true);
    corrections.acceptIngress(ingress);
    QVERIFY(routed.isEmpty());
    settings->correctionSourceInstance()->setRawValue(QStringLiteral("caster"));
    corrections.acceptIngress(ingress);
    QCOMPARE(routed.size(), 1);
    corrections.shutdown();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Automatic);
    QCOMPARE(corrections.router().policy(), GPSCorrectionManager::RoutingPolicy::Manual);
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
    QSignalSpy updates(corrections->sourceModel(), &QAbstractItemModel::dataChanged);
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
    corrections->acceptIngress(source.event(bytes, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(packetCalls, retireFromProvider ? 0 : 1);
    QCOMPARE(laterCalls, 0);
    QCOMPARE(corrections.isNull(), destroy);
    if (destroy) {
        return;
    }
    QCOMPARE(updates.size(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(corrections->events()->rowCount(), corrections->router().events().size());
    QCOMPARE(corrections->rtcmMavlink()->totalBytesSubmitted(), retireFromProvider ? 0ULL : 180ULL);
    QVERIFY(corrections->rtcmMavlink()->submitToOutputs(bytes).isEmpty());
    corrections->shutdown();
    QCoreApplication::sendPostedEvents(corrections.data(), QEvent::MetaCall);
    QCOMPARE(updates.size(), 1);
}

void GPSCorrectionManagerTest::_qmlForwarderAvailableBeforeInit()
{
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> root = engine.create(QByteArray(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var forwarder: QGroundControl.gpsManager.corrections.rtcmMavlink
            readonly property var baseFacts: QGroundControl.gpsManager.gpsRtkFacts
        }
    )"));
    QVERIFY2(root, qPrintable(engine.lastError()));
    QCOMPARE(root->property("forwarder").value<RTCMMavlink*>(), GPSManager::instance()->corrections()->rtcmMavlink());
    QCOMPARE(root->property("baseFacts").value<FactGroup*>(), GPSManager::instance()->gpsRtkFacts());
}

void GPSCorrectionManagerTest::_sourceMessageCounts()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto messageCounts = [&corrections]() {
        return corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip)).messageCounts;
    };
    const qint64 now = GPSCorrectionFrame::monotonicNowMs();
    corrections.acceptIngress(ntrip.event(GpsTestHelpers::buildRtcmFrame(1077, 20), now, 1077, true));
    corrections.acceptIngress(ntrip.event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true));
    // Validated frames without a caller-supplied ID are identified from the RTCM header.
    corrections.acceptIngress(ntrip.event(GpsTestHelpers::buildRtcmFrame(1077, 20), now, 0, true));
    corrections.acceptIngress(ntrip.event(GpsTestHelpers::buildRtcmFrame(1230, 20), now, 1230, false));
    const QList<RTCMMessageCount> expected{{1005, 1}, {1077, 2}};
    QCOMPARE(messageCounts(), expected);

    QVERIFY(advanceDiagnostics(scheduler));
    const auto* model = corrections.sourceModel();
    const int role = model->roleNames().key(QByteArrayLiteral("messageCounts"), -1);
    QVERIFY(role >= 0);
    const auto row = model->data(model->index(static_cast<int>(GPSCorrectionSource::Ntrip), 0), role);
    QCOMPARE(row.value<QList<RTCMMessageCount>>(), expected);

    ntrip.reset();
    ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    QVERIFY(messageCounts().isEmpty());
}

void GPSCorrectionManagerTest::_diagnosticsModelUpdatesInPlace()
{
    GPSCorrectionManager::DestinationModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy moved(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
    const auto row = [](const QString& id, quint64 queuedBytes) {
        return GPSCorrectionDestinationDiagnostic{.destinationId = id, .queuedBytes = queuedBytes};
    };
    const auto keys = [&model]() {
        QStringList result;
        for (const auto& entry : model.rows()) {
            result.append(entry.destinationId);
        }
        return result;
    };
    const int destinationRole = model.roleNames().key(QByteArrayLiteral("destinationId"), -1);
    const int queuedRole = model.roleNames().key(QByteArrayLiteral("queuedBytes"), -1);

    model.setRows({row(QStringLiteral("a"), 1), row(QStringLiteral("b"), 1)});
    QCOMPARE(inserted.size(), 1);
    QCOMPARE(model.rowCount(), 2);
    // Every row property is a role, so QML delegates bind to typed values.
    QVERIFY(destinationRole >= 0 && queuedRole >= 0);
    QCOMPARE(model.data(model.index(0, 0), destinationRole).toString(), QStringLiteral("a"));

    model.setRows({row(QStringLiteral("a"), 1), row(QStringLiteral("b"), 2)});
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.first().at(0).toModelIndex().row(), 1);
    QCOMPARE(changed.first().at(1).toModelIndex().row(), 1);
    QCOMPARE(model.data(model.index(1, 0), queuedRole).toULongLong(), 2ULL);
    QCOMPARE(inserted.size(), 1);
    QCOMPARE(removed.size(), 0);

    model.setRows({row(QStringLiteral("a"), 1), row(QStringLiteral("b"), 2)});
    QCOMPARE(changed.size(), 1);

    model.setRows({row(QStringLiteral("b"), 2), row(QStringLiteral("c"), 1)});
    QCOMPARE(removed.size(), 1);
    QCOMPARE(inserted.size(), 2);
    QCOMPARE(keys(), (QStringList{QStringLiteral("b"), QStringLiteral("c")}));

    model.setRows({row(QStringLiteral("c"), 1), row(QStringLiteral("b"), 2)});
    QCOMPARE(moved.size(), 1);
    QCOMPARE(keys(), (QStringList{QStringLiteral("c"), QStringLiteral("b")}));
    QCOMPARE(changed.size(), 1);

    model.setRows({});
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(reset.size(), 0);
    QVERIFY(!model.data(model.index(0, 0), queuedRole).isValid());
}

void GPSCorrectionManagerTest::_outputsEnabledAfterLinkHistoryChurn()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Automatic);
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
        corrections.acceptIngress(source.event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    }
    QCOMPARE(corrections.destinationDiagnostics().size(), GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 2);

    GPSCorrectionFrame receiverFrame;
    corrections.setOutput(
        QStringLiteral("localReceiver"), {.admit = [&](const GPSCorrectionFrame& frame) {
            receiverFrame = frame;
            return QList<GPSCorrectionRouter::Admission>{
                {QStringLiteral("localReceiver"), {quint64(frame.data.size()), 1, GPSCorrectionReason::None}}};
        }});
    QUdpSocket udpDestination;
    QVERIFY(udpDestination.bind(QHostAddress::LocalHost, 0));
    configureUdpOutput(saved, settings, QStringLiteral("127.0.0.1"), udpDestination.localPort());
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    corrections.acceptIngress(source.event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(receiverFrame.data, data);
    QTRY_VERIFY_WITH_TIMEOUT(udpDestination.hasPendingDatagrams(), TestTimeout::mediumMs());
    QByteArray received(data.size(), Qt::Uninitialized);
    QCOMPARE(udpDestination.readDatagram(received.data(), received.size()), qint64(data.size()));
    QCOMPARE(received, data);
    for (const auto& value : corrections.destinationDiagnostics()) {
        const auto& destination = value;
        const auto id = destination.destinationId;
        if (id == QStringLiteral("localReceiver") || id == QStringLiteral("udpOutput")) {
            QCOMPARE(destination.queuedBytes, quint64(data.size()));
        }
    }
    QCOMPARE(corrections.destinationDiagnostics().size(), GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 4);
}

void GPSCorrectionManagerTest::_udpOutputForwardsSelectedStream()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::Automatic);
    saved.setFactValue(settings->correctionSourceInstance(), QString());
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, 0));
    configureUdpOutput(saved, settings, QStringLiteral("127.0.0.1"), destination.localPort());
    GPSCorrectionManager corrections;
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto localData = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto ntripData = GpsTestHelpers::buildRtcmFrame(1077, 30);
    const auto receive = [&destination]() {
        return destination.waitForReadyRead(TestTimeout::mediumMs()) ? destination.receiveDatagram().data()
                                                                     : QByteArray();
    };

    // Automatic routing prefers the local base, so UDP forwards the same stream as vehicles receive.
    corrections.acceptIngress(local.event(localData, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    corrections.acceptIngress(ntrip.event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(receive(), localData);
    QCOMPARE(udpOutputStats(corrections).queuedBytes, quint64(localData.size()));

    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    corrections.acceptIngress(ntrip.event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(receive(), ntripData);
    const quint64 forwarded = localData.size() + ntripData.size();
    QCOMPARE(udpOutputStats(corrections).queuedBytes, forwarded);

    // Filtered messages are not forwarded, and disabling the output stops forwarding.
    corrections.acceptIngress(ntrip.event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true, true));
    settings->rtcmUdpOutputEnabled()->setRawValue(false);
    corrections.acceptIngress(ntrip.event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(udpOutputStats(corrections).queuedBytes, forwarded);
    QVERIFY(!destination.waitForReadyRead(TestTimeout::shortMs()));
}

void GPSCorrectionManagerTest::_udpOutputSkipsOwnInput()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    const quint16 port = unusedPort();
    QVERIFY(port);
    configureUdp(saved, settings, port);
    configureUdpOutput(saved, settings, QStringLiteral("127.0.0.1"), port);
    GPSCorrectionManager corrections;
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("own UDP input port")));
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    verifyExpectedLogMessage();
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    corrections.acceptIngress(source.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(udpOutputStats(corrections).queuedBytes, 0ULL);

    // Disabling the input removes the loop, so forwarding starts.
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, port, QUdpSocket::DontShareAddress));
    corrections.acceptIngress(source.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QTRY_VERIFY_WITH_TIMEOUT(destination.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(destination.receiveDatagram().data(), frame);
    QCOMPARE(udpOutputStats(corrections).queuedBytes, quint64(frame.size()));
    destination.close();

    // Re-enabling the input restores the loop, so active forwarding stops.
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("own UDP input port")));
    settings->rtcmUdpInputEnabled()->setRawValue(true);
    verifyExpectedLogMessage();
    corrections.acceptIngress(source.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(udpOutputStats(corrections).queuedBytes, quint64(frame.size()));
}

void GPSCorrectionManagerTest::_udpOutputEndpointChanges_data()
{
    QTest::addColumn<QString>("initialAddress");
    QTest::addColumn<QString>("address");
    QTest::addColumn<quint16>("port");
    QTest::addColumn<bool>("expectedEnabled");
    const quint16 port = 13320;
    QTest::newRow("same-ipv4") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1") << port << true;
    QTest::newRow("equivalent-ipv6") << QStringLiteral("::1") << QStringLiteral("0:0:0:0:0:0:0:1") << port << true;
    QTest::newRow("changed-address") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.2") << port << true;
    QTest::newRow("changed-port") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1") << quint16(port + 1)
                                  << true;
    QTest::newRow("mapped-ipv4") << QStringLiteral("127.0.0.1") << QStringLiteral("::ffff:127.0.0.1") << port << true;
    QTest::newRow("loopback-protocol") << QStringLiteral("::1") << QStringLiteral("127.0.0.1") << port << true;
    QTest::newRow("empty-address") << QStringLiteral("127.0.0.1") << QString() << port << false;
    QTest::newRow("hostname") << QStringLiteral("127.0.0.1") << QStringLiteral("localhost") << port << false;
    QTest::newRow("zero-port") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1") << quint16(0) << false;
}

void GPSCorrectionManagerTest::_udpOutputEndpointChanges()
{
    QFETCH(QString, initialAddress);
    QFETCH(QString, address);
    QFETCH(quint16, port);
    QFETCH(bool, expectedEnabled);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    configureUdpOutput(saved, settings, initialAddress, 13320);
    GPSCorrectionManager corrections;
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    corrections.acceptIngress(source.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    const quint64 initialForwarded = udpOutputStats(corrections).queuedBytes;
    QCOMPARE(initialForwarded, quint64(frame.size()));

    if (!expectedEnabled) {
        expectLogMessage("Utilities.UdpForwarder", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Invalid UDP forward config:")));
    }
    settings->rtcmUdpOutputAddress()->setRawValue(address);
    settings->rtcmUdpOutputPort()->setRawValue(port);
    if (!expectedEnabled) {
        verifyExpectedLogMessage();
    }
    corrections.acceptIngress(source.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(udpOutputStats(corrections).queuedBytes,
             initialForwarded + (expectedEnabled ? quint64(frame.size()) : 0ULL));
}

void GPSCorrectionManagerTest::_sourceTopologyDoesNotNotifyOnCounters()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QSignalSpy topology(&corrections, &GPSCorrectionManager::sourceInstancesChanged);
    QSignalSpy counters(corrections.sourceModel(), &QAbstractItemModel::dataChanged);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    const auto frame =
        ntrip.event(GpsTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true);
    corrections.acceptIngress(frame);
    QCOMPARE(corrections.sourceDiagnostics()[2].receivedFrames, 1ULL);
    QCOMPARE(corrections.sourceInstances().size(), 1);
    QCOMPARE(topology.size(), 0);
    QCOMPARE(counters.size(), 0);
    QVERIFY(advanceDiagnostics(scheduler));
    QCOMPARE(topology.size(), 1);
    corrections.acceptIngress(frame);
    QCOMPARE(corrections.sourceDiagnostics()[2].receivedFrames, 2ULL);
    QVERIFY(advanceDiagnostics(scheduler));
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 2);
    ntrip.reset();
    QVERIFY(corrections.sourceInstances().isEmpty());
    QCOMPARE(topology.size(), 1);
    QVERIFY(advanceHealthSample(scheduler));
    QCOMPARE(topology.size(), 2);
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void GPSCorrectionManagerTest::_diagnosticsNotifyOnlyOnChange()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QSignalSpy topology(&corrections, &GPSCorrectionManager::sourceInstancesChanged);
    QSignalSpy counters(corrections.sourceModel(), &QAbstractItemModel::dataChanged);
    QSignalSpy destinations(corrections.destinationModel(), &QAbstractItemModel::dataChanged);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    corrections.acceptIngress(
        ntrip.event(GpsTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QVERIFY(advanceDiagnostics(scheduler));
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 1);
    QCOMPARE(destinations.size(), 1);
    QVERIFY(!corrections.destinationDiagnostics().isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(900)));
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 1);
    QCOMPARE(destinations.size(), 1);
}

void GPSCorrectionManagerTest::_healthSampleObserverCanShutDown()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    corrections.acceptIngress(
        ntrip.event(GpsTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QVERIFY(advanceDiagnostics(scheduler));
    ntrip.reset();
    // The next health sample publishes the removed source to an observer that shuts the manager down.
    int notifications = 0;
    connect(&corrections, &GPSCorrectionManager::sourceInstancesChanged, this, [&]() {
        ++notifications;
        corrections.shutdown();
    });
    QVERIFY(advanceHealthSample(scheduler));
    QCOMPARE(notifications, 1);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void GPSCorrectionManagerTest::_correctionsStatusShowsSelectedStream()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(GpsTestHelpers::sourceQmlUrl(QStringLiteral("AppSettings/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* source = status->findChild<QObject*>(QStringLiteral("correctionsSelectedSource"));
    auto* stream = status->findChild<QObject*>(QStringLiteral("correctionsSelectedStream"));
    auto* rate = status->findChild<QObject*>(QStringLiteral("correctionsDataRate"));
    QVERIFY(source && stream && rate);
    QCOMPARE(source->property("labelText").toString(), QStringLiteral("None"));
    QVERIFY(!rate->property("visible").toBool());

    const QString instance = QStringLiteral("ntrip://caster.example.com:2101/MOUNT");
    // A stream is listed once it delivers corrections.
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, instance);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    QVERIFY(advanceHealthSample(scheduler));
    corrections.acceptIngress(ntrip.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QVERIFY(advanceHealthSample(scheduler));
    QCOMPARE(corrections.selectedStream().source, static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(corrections.selectedBytesPerSecond(), quint64(frame.size()));
    QCOMPARE(source->property("labelText").toString(),
             GPSCorrectionManager::sourceName(static_cast<int>(GPSCorrectionSource::Ntrip)));
    QVERIFY(stream->property("visible").toBool());
    QCOMPARE(stream->property("text").toString(), instance);
    QVERIFY(rate->property("visible").toBool());
    QCOMPARE(rate->property("labelText").toString(), QStringLiteral("%1 B/s").arg(frame.size()));

    ntrip.reset();
    QVERIFY(advanceHealthSample(scheduler));
    QVERIFY(!corrections.selectedStream().selected);
    QCOMPARE(corrections.selectedBytesPerSecond(), 0ULL);
    QCOMPARE(source->property("labelText").toString(), QStringLiteral("None"));
}

UT_REGISTER_TEST(GPSCorrectionManagerTest, TestLabel::Unit)
