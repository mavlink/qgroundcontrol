#include "GPSCorrectionManagerTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "FactGroup.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
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
    NTRIPManager ntrip(SettingsManager::instance()->ntripSettings());
    auto* stream = new MockNTRIPTransport(&ntrip);
    stream->autoConnect = false;
    ntrip.setTransportForTest(stream);
    ntrip.setCorrectionManager(&corrections);
    ntrip.init();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::LocalReceiver);
    corrections.init(settings);
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    const auto localToken = local.token();
    auto* forwarder = corrections.rtcmMavlink();
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
    QVERIFY(corrections._udpInput.isRunning());
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::LocalReceiver);
    corrections.acceptIngress(localToken.event(frame, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
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
            routingAppliedBeforeIngress = corrections._router.policy() == expectedPolicy &&
                                          corrections.selectedSource() == expectedSource &&
                                          corrections._router.configuration().instance == QStringLiteral("initial");
        }
    });
    corrections.init(settings);
    QVERIFY(routingAppliedBeforeIngress);
    QSignalSpy routed(&corrections._router, &GPSCorrectionRouter::frameRouted);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    QCOMPARE(corrections._router.policy(), GPSCorrectionManager::RoutingPolicy::Manual);
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
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Automatic);
    QCOMPARE(corrections._router.policy(), GPSCorrectionManager::RoutingPolicy::Manual);
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
    QVERIFY(corrections->rtcmMavlink()->submitToOutputs(bytes).isEmpty());
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
            readonly property var baseFacts: QGroundControl.gpsManager.gpsRtk.facts
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QCOMPARE(root->property("forwarder").value<RTCMMavlink*>(), GPSManager::instance()->corrections()->rtcmMavlink());
    QCOMPARE(root->property("baseFacts").value<FactGroup*>(), GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
}

void GPSCorrectionManagerTest::_sourceMessageCounts()
{
    GPSCorrectionManager corrections;
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto messageCounts = [&corrections]() {
        return corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip)).messageCounts;
    };
    const qint64 now = GPSCorrectionFrame::monotonicNowMs();
    corrections.acceptIngress(ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1077, 20), now, 1077, true));
    corrections.acceptIngress(ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true));
    // Validated frames without a caller-supplied ID are identified from the RTCM header.
    corrections.acceptIngress(ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1077, 20), now, 0, true));
    corrections.acceptIngress(ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1230, 20), now, 1230, false));
    const QList<RTCMMessageCount> expected{{1005, 1}, {1077, 2}};
    QCOMPARE(messageCounts(), expected);

    corrections._refreshDiagnostics();
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

void GPSCorrectionManagerTest::_sourceSelectionAndSessions()
{
    GPSCorrectionManager corrections;
    QSignalSpy routed(&corrections._router, &GPSCorrectionRouter::frameRouted);
    const QByteArray data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto initial = corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::LocalReceiver));
    QVERIFY(!initial.active);
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
    corrections.applyRoutingConfiguration(
        {GPSCorrectionManager::RoutingPolicy::Manual, GPSCorrectionSource::LocalReceiver, {}});
    corrections.acceptIngress(frame);
    QCOMPARE(routed.size(), 1);
    corrections.acceptIngress(local.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(routed.size(), 2);
    corrections.applyRoutingConfiguration(
        {GPSCorrectionManager::RoutingPolicy::Manual, GPSCorrectionSource::Ntrip, {}});
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
    const auto stats = corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(stats.validatedFrames, quint64(1));
    QCOMPARE(stats.selectedFrames, quint64(1));
    QVERIFY(stats.usable);
}

void GPSCorrectionManagerTest::_filteredAndExpiredFrames()
{
    GPSCorrectionManager corrections;
    QSignalSpy routed(&corrections._router, &GPSCorrectionRouter::frameRouted);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto now = GPSCorrectionFrame::monotonicNowMs();
    auto frame = ntrip.token().event(data, now, 1005, true, true);
    corrections.acceptIngress(frame);
    auto stats = corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QVERIFY(stats.usable);
    QCOMPARE(stats.droppedFrames, quint64(1));
    QVERIFY(routed.isEmpty());
    frame = ntrip.token().event(data, now - 6000, 1005, true);
    corrections.acceptIngress(frame);
    stats = corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    // Older deliveries cannot expire newer observations.
    QVERIFY(stats.usable);
    QCOMPARE(stats.droppedFrames, quint64(2));
    QVERIFY(routed.isEmpty());
    frame = ntrip.token().event(data, now + 60000, 1005, true);
    corrections.acceptIngress(frame);
    stats = corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(stats.droppedFrames, quint64(3));
    QCOMPARE(stats.validatedFrames, quint64(2));
    QVERIFY(routed.isEmpty());
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
        corrections.acceptIngress(source.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
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
    corrections.init(settings);
    corrections.acceptIngress(source.token().event(data, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
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
    corrections.init(settings);
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto localData = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto ntripData = GpsTestHelpers::buildRtcmFrame(1077, 30);
    const auto receive = [&destination]() {
        return destination.waitForReadyRead(TestTimeout::mediumMs()) ? destination.receiveDatagram().data()
                                                                     : QByteArray();
    };

    // Automatic routing prefers the local base, so UDP forwards the same stream as vehicles receive.
    corrections.acceptIngress(local.token().event(localData, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    corrections.acceptIngress(ntrip.token().event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(receive(), localData);
    QCOMPARE(udpOutputStats(corrections).queuedBytes, quint64(localData.size()));

    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    corrections.acceptIngress(ntrip.token().event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(receive(), ntripData);
    const quint64 forwarded = localData.size() + ntripData.size();
    QCOMPARE(udpOutputStats(corrections).queuedBytes, forwarded);

    // Filtered messages are not forwarded, and disabling the output stops forwarding.
    corrections.acceptIngress(ntrip.token().event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true, true));
    settings->rtcmUdpOutputEnabled()->setRawValue(false);
    corrections.acceptIngress(ntrip.token().event(ntripData, GPSCorrectionFrame::monotonicNowMs(), 1077, true));
    QCOMPARE(udpOutputStats(corrections).queuedBytes, forwarded);
    QVERIFY(!destination.waitForReadyRead(TestTimeout::shortMs()));
    QVERIFY(!corrections._udpOutput.isEnabled());
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
    corrections.init(settings);
    verifyExpectedLogMessage();
    QVERIFY(!corrections._udpOutput.isEnabled());
    // Disabling the input removes the loop, so forwarding starts.
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QVERIFY(corrections._udpOutput.isEnabled());
    QCOMPARE(corrections._udpOutput.port(), port);
}

void GPSCorrectionManagerTest::_udpOutputEndpointChanges_data()
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

void GPSCorrectionManagerTest::_udpOutputEndpointChanges()
{
    QFETCH(QString, initialAddress);
    QFETCH(QString, address);
    QFETCH(quint16, port);
    QFETCH(bool, unchanged);
    QFETCH(bool, expectedEnabled);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    configureUdpOutput(saved, settings, initialAddress, 13320);
    GPSCorrectionManager corrections;
    corrections.init(settings);
    QVERIFY(corrections._udpOutput.isEnabled());
    auto* socket = corrections._udpOutput.findChild<QUdpSocket*>();
    QVERIFY(socket);
    // IPv4 binding exposes socket retirement without requiring IPv6.
    QVERIFY(socket->bind(QHostAddress::LocalHost, 0));
    const quint16 boundPort = socket->localPort();
    if (!expectedEnabled) {
        expectLogMessage("Utilities.UdpForwarder", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Invalid UDP forward config:")));
    }
    settings->rtcmUdpOutputAddress()->setRawValue(address);
    settings->rtcmUdpOutputPort()->setRawValue(port);
    if (!expectedEnabled) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(corrections._udpOutput.isEnabled(), expectedEnabled);
    QCOMPARE(corrections._udpOutput.port(), expectedEnabled ? port : quint16(0));
    QCOMPARE(corrections._udpOutput.address(), expectedEnabled ? QHostAddress(address).toString() : QString());
    QCOMPARE(socket->state(), unchanged ? QAbstractSocket::BoundState : QAbstractSocket::UnconnectedState);
    if (unchanged) {
        QCOMPARE(socket->localPort(), boundPort);
    }
}

void GPSCorrectionManagerTest::_sourceTopologyDoesNotNotifyOnCounters()
{
    GPSCorrectionManager corrections;
    QSignalSpy topology(&corrections, &GPSCorrectionManager::sourceInstancesChanged);
    QSignalSpy counters(corrections.sourceModel(), &QAbstractItemModel::dataChanged);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    const auto frame =
        ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), GPSCorrectionFrame::monotonicNowMs(), 1005, true);
    corrections.acceptIngress(frame);
    QCOMPARE(corrections.sourceDiagnostics()[2].receivedFrames, 1ULL);
    QCOMPARE(corrections.sourceInstances().size(), 1);
    QCOMPARE(topology.size(), 0);
    QCOMPARE(counters.size(), 0);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    corrections.acceptIngress(frame);
    QCOMPARE(corrections.sourceDiagnostics()[2].receivedFrames, 2ULL);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 2);
    ntrip.reset();
    QVERIFY(corrections.sourceInstances().isEmpty());
    QCOMPARE(topology.size(), 1);
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 2);
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void GPSCorrectionManagerTest::_diagnosticsNotifyOnlyOnChange()
{
    GPSCorrectionManager corrections;
    QSignalSpy topology(&corrections, &GPSCorrectionManager::sourceInstancesChanged);
    QSignalSpy counters(corrections.sourceModel(), &QAbstractItemModel::dataChanged);
    QSignalSpy destinations(corrections.destinationModel(), &QAbstractItemModel::dataChanged);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    corrections.acceptIngress(ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20),
                                                  GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 1);
    QCOMPARE(destinations.size(), 1);
    QVERIFY(!corrections.destinationDiagnostics().isEmpty());
    corrections._refreshDiagnostics();
    QCOMPARE(topology.size(), 1);
    QCOMPARE(counters.size(), 1);
    QCOMPARE(destinations.size(), 1);
}

void GPSCorrectionManagerTest::_receivedByteRates()
{
    GPSCorrectionManager corrections;
    QSignalSpy counters(corrections.sourceModel(), &QAbstractItemModel::dataChanged);
    auto ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto rate = [&corrections]() {
        return corrections.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip)).receivedBytesPerSecond;
    };
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    corrections._router.sampleReceivedByteRates(1000);
    corrections.acceptIngress(ntrip.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QCOMPARE(rate(), 0ULL);
    corrections._router.sampleReceivedByteRates(1500);
    QCOMPARE(rate(), quint64(frame.size() * 2));
    corrections._refreshDiagnostics();
    const qsizetype published = counters.size();
    corrections._router.sampleReceivedByteRates(2500);
    QCOMPARE(rate(), 0ULL);
    corrections._refreshDiagnostics();
    QCOMPARE(counters.size(), published + 1);
    ntrip.reset();
    ntrip = corrections.registerSource(GPSCorrectionSource::Ntrip);
    corrections.acceptIngress(ntrip.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    corrections._router.sampleReceivedByteRates(3500);
    QCOMPARE(rate(), quint64(frame.size()));
}

void GPSCorrectionManagerTest::_correctionsStatusShowsSelectedStream()
{
    GPSCorrectionManager corrections;
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    const QUrl url =
        QUrl::fromLocalFile(QFileInfo(QString::fromUtf8(__FILE__))
                                .dir()
                                .filePath(QStringLiteral("../../../src/AppSettings/CorrectionsStatus.qml")));
    QQmlComponent component(&engine, url);
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> status(
        component.createWithInitialProperties({{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}}));
    QVERIFY2(status, qPrintable(component.errorString()));
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
    corrections._router.sampleReceivedByteRates(1000);
    corrections.acceptIngress(ntrip.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    corrections._router.sampleReceivedByteRates(2000);
    corrections._refreshDiagnostics();
    QCOMPARE(corrections.selectedStream().source, static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(corrections.selectedBytesPerSecond(), quint64(frame.size()));
    QCOMPARE(source->property("labelText").toString(),
             GPSCorrectionManager::sourceName(static_cast<int>(GPSCorrectionSource::Ntrip)));
    QVERIFY(stream->property("visible").toBool());
    QCOMPARE(stream->property("text").toString(), instance);
    QVERIFY(rate->property("visible").toBool());
    QCOMPARE(rate->property("labelText").toString(), QStringLiteral("%1 B/s").arg(frame.size()));

    ntrip.reset();
    corrections._refreshDiagnostics();
    QVERIFY(!corrections.selectedStream().selected);
    QCOMPARE(corrections.selectedBytesPerSecond(), 0ULL);
    QCOMPARE(source->property("labelText").toString(), QStringLiteral("None"));
}

UT_REGISTER_TEST(GPSCorrectionManagerTest, TestLabel::Unit)
