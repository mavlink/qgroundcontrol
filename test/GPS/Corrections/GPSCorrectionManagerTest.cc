#include "GPSCorrectionManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSReceiver.h"
#include "GPSSettingsBindings.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCMUdpInput.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
#include "Transport/Support/UnusedUdpPort.h"

using namespace GPSTest;

namespace {
void configureUdp(TestFixtures::SettingsFixture& saved, GPSCorrectionSettings* settings, quint16 port)
{
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerConnectEnabled(), false);
    saved.setFactValue(settings->rtcmUdpInputEnabled(), true);
    saved.setFactValue(settings->rtcmUdpInputPort(), port);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::HighestPriority);
}

void configureUdpOutput(TestFixtures::SettingsFixture& saved, GPSCorrectionSettings* settings, const QString& address,
                        quint16 port)
{
    saved.setFactValue(settings->rtcmUdpOutputAddress(), address);
    saved.setFactValue(settings->rtcmUdpOutputPort(), port);
    saved.setFactValue(settings->rtcmUdpOutputEnabled(), true);
}

const UdpForwarder* udpForwarding(const GPSCorrectionManager& corrections)
{
    return corrections.findChild<UdpForwarder*>();
}

/// The port the UDP input listens on, or 0 while it holds no bound socket.
quint16 udpInputPort(const GPSCorrectionManager& corrections)
{
    const auto* input = corrections.findChild<RTCMUdpInput*>();
    if (!input) {
        return 0;
    }
    for (const auto* socket : input->findChildren<QUdpSocket*>()) {
        if (socket->state() == QAbstractSocket::BoundState) {
            return socket->localPort();
        }
    }
    return 0;
}

bool advanceDiagnostics(ManualScheduler& scheduler)
{
    return scheduler.advanceBy(std::chrono::milliseconds(100));
}

bool advanceTick(ManualScheduler& scheduler)
{
    return scheduler.advanceBy(std::chrono::seconds(1));
}

/// Offers @a data, received now, from @a source.
void submitNow(const std::unique_ptr<GPSCorrectionSourceHandle>& source, const QByteArray& data)
{
    source->submit(data, GPSTest::nowMs());
}
}  // namespace

quint16 GPSCorrectionManagerTest::_listenOnUnusedPort(GPSCorrectionSettings* settings,
                                                      const GPSCorrectionManager& corrections)
{
    int failures = 0;
    const quint16 port = bindUnusedUdpPort(
        [&](quint16 candidate) {
            settings->rtcmUdpInputPort()->setRawValue(candidate);
            return udpInputPort(corrections) == candidate;
        },
        failures);
    if (failures > 0) {
        // Another process took a probed port before the input bound it; the next port was tried.
        ignoreLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^UDP correction input unavailable")));
    }
    return port;
}

void GPSCorrectionManagerTest::_sourcesShareForwarder()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    configureUdp(saved, settings, 0);
    auto* ntripSettings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(ntripSettings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(ntripSettings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->rtcmUdpOutputEnabled(), false);
    GPSCorrectionManager corrections;
    NTRIPManager ntrip(nullptr, nullptr, {.corrections = &corrections});
    GPSSettingsBindings::bindNtrip(ntripSettings, &ntrip);
    auto* stream = injectMockTransport(ntrip);
    ntrip.init();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::LocalReceiver);
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    // Port 0 listens on a port the system chooses.
    const quint16 port = udpInputPort(corrections);
    QVERIFY(port);
    auto local = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    QSignalSpy submitted(&corrections, &GPSCorrectionManager::vehicleBytesSubmittedChanged);
    QList<uint8_t> sequences;
    corrections.rtcmMavlink()->setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{[&](const GPSRTCMPacket& packet) {
            if (((packet.flags >> 1) & 0x03U) == 0) {
                sequences.append(packet.flags >> 3);
            }
            return true;
        }};
    });
    const QByteArray frame = GPSTest::rtcmMessage(1077, 500);
    quint64 expected = 0;
    submitNow(local, frame);
    expected += frame.size();
    QCOMPARE(corrections.vehicleBytesSubmitted(), expected);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.vehicleBytesSubmitted(), expected, TestTimeout::mediumMs());
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    ntripSettings->ntripServerConnectEnabled()->setRawValue(true);
    QTRY_COMPARE_WITH_TIMEOUT(stream->startCount, 1, TestTimeout::mediumMs());
    stream->simulateRtcmData(frame, 1077);
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.vehicleBytesSubmitted(), expected, TestTimeout::mediumMs());
    ntripSettings->ntripServerConnectEnabled()->setRawValue(false);
    QTRY_COMPARE_WITH_TIMEOUT(ntrip.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected,
                              TestTimeout::mediumMs());
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::LocalReceiver);
    submitNow(local, frame);
    expected += frame.size();
    QCOMPARE(corrections.vehicleBytesSubmitted(), expected);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.vehicleBytesSubmitted(), expected, TestTimeout::mediumMs());
    QCOMPARE(ntrip.connectionStats()->bytesReceived(), quint64(frame.size()));
    QCOMPARE(sequences, QList<uint8_t>({0, 1, 2, 3, 4}));
    QCOMPARE(submitted.size(), sequences.size());
    QCOMPARE(udpInputPort(corrections), port);
    corrections.shutdown();
    QCOMPARE(udpInputPort(corrections), 0);
    submitNow(local, frame);
    QCOMPARE(corrections.vehicleBytesSubmitted(), expected);
    QCOMPARE(submitted.size(), sequences.size());
}

void GPSCorrectionManagerTest::_udpSettingsAndShutdown()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    configureUdp(saved, settings, 0);
    GPSCorrectionManager corrections;
    corrections.rtcmMavlink()->setOutputProvider(
        []() { return QList<RTCMMAVLink::Output>{[](const GPSRTCMPacket&) { return true; }}; });
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    const quint16 port = udpInputPort(corrections);
    QVERIFY(port);
    QUdpSocket sender;
    const QByteArray frame = GPSTest::rtcmMessage(1005, 30);
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    quint64 expected = frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.vehicleBytesSubmitted(), expected, TestTimeout::mediumMs());

    // Another port rebinds the input.
    const quint16 nextPort = _listenOnUnusedPort(settings, corrections);
    QVERIFY(nextPort);
    QVERIFY(nextPort != port);

    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, nextPort), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.vehicleBytesSubmitted(), expected, TestTimeout::mediumMs());

    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QCOMPARE(udpInputPort(corrections), 0);
    // Other sources remain available when UDP input is disabled.
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    submitNow(ntrip, frame);
    expected += frame.size();
    QCOMPARE(corrections.vehicleBytesSubmitted(), expected);

    settings->rtcmUdpInputEnabled()->setRawValue(true);
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Udp);
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, nextPort), frame.size());
    expected += frame.size();
    QTRY_COMPARE_WITH_TIMEOUT(corrections.vehicleBytesSubmitted(), expected, TestTimeout::mediumMs());

    corrections.shutdown();
    corrections.shutdown();
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    settings->rtcmUdpInputEnabled()->setRawValue(true);
    QCOMPARE(udpInputPort(corrections), 0);
}

void GPSCorrectionManagerTest::_udpInputRetriesUnavailablePort()
{
    QUdpSocket holder;
    QVERIFY(holder.bind(QHostAddress::Any, 0, QUdpSocket::DontShareAddress));
    const quint16 port = holder.localPort();
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QSignalSpy changes(&corrections, &GPSCorrectionManager::udpInputChanged);

    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UDP correction input unavailable:.*%1").arg(port)));
    corrections.setConfiguration({.udpInput = {.enabled = true, .port = port}, .udpOutput = {}});
    verifyExpectedLogMessage();
    QVERIFY(corrections.udpInputError().contains(QString::number(port)));
    QCOMPARE(udpInputPort(corrections), 0);
    QCOMPARE(changes.size(), 1);

    // Each tick retries; a repeated failure neither warns nor notifies again.
    QVERIFY(advanceTick(scheduler));
    QCOMPARE(changes.size(), 1);

    holder.close();
    QVERIFY(advanceTick(scheduler));
    QVERIFY(corrections.udpInputError().isEmpty());
    QCOMPARE(udpInputPort(corrections), port);
    QCOMPARE(changes.size(), 2);

    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    const QByteArray frame = GPSTest::rtcmMessage(1005, 20);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame, QHostAddress::LocalHost, port), frame.size());
    QTRY_COMPARE_WITH_TIMEOUT(sent, QList<QByteArray>{frame}, TestTimeout::mediumMs());

    corrections.setConfiguration({.udpInput = {.enabled = false, .port = port}, .udpOutput = {}});
    QCOMPARE(udpInputPort(corrections), 0);
    QCOMPARE(changes.size(), 3);
}

void GPSCorrectionManagerTest::_settingsOwnRouting_data()
{
    QTest::addColumn<int>("configuredSource");
    // Unknown when every source category is routed.
    QTest::addColumn<GPSCorrectionSettings::CorrectionSource>("routedSource");
    QTest::newRow("automatic") << int(GPSCorrectionSettings::HighestPriority) << GPSCorrectionSettings::HighestPriority;
    QTest::newRow("local") << int(GPSCorrectionSettings::LocalReceiver) << GPSCorrectionSettings::LocalReceiver;
    QTest::newRow("ntrip") << int(GPSCorrectionSettings::Ntrip) << GPSCorrectionSettings::Ntrip;
    QTest::newRow("udp") << int(GPSCorrectionSettings::Udp) << GPSCorrectionSettings::Udp;
}

void GPSCorrectionManagerTest::_settingsOwnRouting()
{
    QFETCH(int, configuredSource);
    QFETCH(GPSCorrectionSettings::CorrectionSource, routedSource);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->rtcmUdpOutputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), configuredSource);
    GPSCorrectionManager corrections;
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    for (const auto source :
         {GPSCorrectionSettings::LocalReceiver, GPSCorrectionSettings::Ntrip, GPSCorrectionSettings::Udp}) {
        const auto handle = corrections.openSource(source, {});
        const auto frame = GPSTest::rtcmMessage(1005, 10 * static_cast<int>(source));
        submitNow(handle, frame);
        QCOMPARE(sent.contains(frame),
                 routedSource == GPSCorrectionSettings::HighestPriority || routedSource == source);
    }

    // A changed setting applies at once.
    sent.clear();
    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    const auto localSource = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    const auto ntripSource = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    const auto local = GPSTest::rtcmMessage(1005, 20);
    const auto ntrip = GPSTest::rtcmMessage(1077, 20);
    submitNow(localSource, local);
    submitNow(ntripSource, ntrip);
    QCOMPARE(sent, QList<QByteArray>{ntrip});
}

void GPSCorrectionManagerTest::_udpOutputForwardsSelectedStream()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::HighestPriority);
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, 0));
    configureUdpOutput(saved, settings, QStringLiteral("127.0.0.1"), destination.localPort());
    GPSCorrectionManager corrections;
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    QVERIFY(corrections.udpOutputError().isEmpty());
    const auto local = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    const auto localData = GPSTest::rtcmMessage(1005, 20);
    const auto ntripData = GPSTest::rtcmMessage(1077, 30);
    const auto receive = [&destination]() {
        return destination.waitForReadyRead(TestTimeout::mediumMs()) ? destination.receiveDatagram().data()
                                                                     : QByteArray();
    };

    // Automatic routing prefers the local base, so UDP forwards the same stream as vehicles receive.
    submitNow(local, localData);
    submitNow(ntrip, ntripData);
    QCOMPARE(receive(), localData);

    settings->correctionSource()->setRawValue(GPSCorrectionSettings::Ntrip);
    submitNow(ntrip, ntripData);
    QCOMPARE(receive(), ntripData);

    // Disabling the output stops forwarding. Loopback delivers in order, so the marker arriving next shows nothing was
    // sent before it.
    const auto marker = GPSTest::rtcmMessage(1230, 8);
    settings->rtcmUdpOutputEnabled()->setRawValue(false);
    submitNow(ntrip, ntripData);
    settings->rtcmUdpOutputEnabled()->setRawValue(true);
    submitNow(ntrip, marker);
    QCOMPARE(receive(), marker);
}

void GPSCorrectionManagerTest::_udpOutputSkipsOwnInput()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    configureUdp(saved, settings, 0);
    GPSCorrectionManager corrections;
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    const quint16 port = _listenOnUnusedPort(settings, corrections);
    QVERIFY(port);
    QSignalSpy outputChanges(&corrections, &GPSCorrectionManager::udpOutputChanged);
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("own UDP input port")));
    configureUdpOutput(saved, settings, QStringLiteral("127.0.0.1"), port);
    verifyExpectedLogMessage();
    QVERIFY(!udpForwarding(corrections)->isEnabled());
    QVERIFY(corrections.udpOutputError().contains(QString::number(port)));
    QCOMPARE(outputChanges.size(), 1);
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    const auto frame = GPSTest::rtcmMessage(1005, 20);

    // Disabling the input removes the loop, so forwarding starts.
    settings->rtcmUdpInputEnabled()->setRawValue(false);
    QVERIFY(corrections.udpOutputError().isEmpty());
    QCOMPARE(outputChanges.size(), 2);
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, port, QUdpSocket::DontShareAddress));
    submitNow(ntrip, frame);
    QTRY_VERIFY_WITH_TIMEOUT(destination.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(destination.receiveDatagram().data(), frame);
    destination.close();

    // Re-enabling the input restores the loop, so active forwarding stops.
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("own UDP input port")));
    settings->rtcmUdpInputEnabled()->setRawValue(true);
    verifyExpectedLogMessage();
    QVERIFY(!udpForwarding(corrections)->isEnabled());
    QVERIFY(!corrections.udpOutputError().isEmpty());
}

void GPSCorrectionManagerTest::_udpOutputSkipsHostTargets_data()
{
    QTest::addColumn<QString>("target");
    QNetworkAddressEntry local;
    for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces()) {
        for (const QNetworkAddressEntry& entry : interface.addressEntries()) {
            if (local.ip().isNull() && entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                !entry.ip().isLoopback() && !entry.broadcast().isNull()) {
                local = entry;
            }
        }
    }
    // Rows without a non-loopback IPv4 interface are empty and skipped.
    const bool hasLocal = !local.ip().isNull();
    // The IPv4-mapped form of a local address reaches the dual-stack input as the address itself does.
    QTest::newRow("mapped-local") << (hasLocal ? QStringLiteral("::ffff:") + local.ip().toString() : QString());
    QTest::newRow("subnet-broadcast") << (hasLocal ? local.broadcast().toString() : QString());
    QTest::newRow("broadcast") << QStringLiteral("255.255.255.255");
    QTest::newRow("unspecified-ipv4") << QStringLiteral("0.0.0.0");
    QTest::newRow("unspecified-ipv6") << QStringLiteral("::");
    QTest::newRow("multicast-ipv4") << QStringLiteral("239.255.0.1");
    QTest::newRow("multicast-ipv6") << QStringLiteral("ff02::1");
}

void GPSCorrectionManagerTest::_udpOutputSkipsHostTargets()
{
    QFETCH(QString, target);
    if (target.isEmpty()) {
        QSKIP("This host has no non-loopback IPv4 interface");
    }
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    configureUdp(saved, settings, 0);
    GPSCorrectionManager corrections;
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    const quint16 port = _listenOnUnusedPort(settings, corrections);
    QVERIFY(port);
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("own UDP input port")));
    configureUdpOutput(saved, settings, target, port);
    verifyExpectedLogMessage();
    QVERIFY(!udpForwarding(corrections)->isEnabled());
    QVERIFY(!corrections.udpOutputError().isEmpty());
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
    const UdpForwarder* const forwarding = udpForwarding(corrections);
    QVERIFY(forwarding->isEnabled());

    if (!expectedEnabled) {
        expectLogMessage("Utilities.UdpForwarder", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Invalid UDP forward config:")));
    }
    settings->rtcmUdpOutputAddress()->setRawValue(address);
    settings->rtcmUdpOutputPort()->setRawValue(port);
    if (!expectedEnabled) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(forwarding->isEnabled(), expectedEnabled);
    QCOMPARE(corrections.udpOutputError().isEmpty(), expectedEnabled);
    if (expectedEnabled) {
        QCOMPARE(QHostAddress(forwarding->address()), QHostAddress(address));
        QCOMPARE(forwarding->port(), port);
    }
}

void GPSCorrectionManagerTest::_sourceTopologyDoesNotNotifyOnCounters()
{
    using State = GPSCorrectionManager::State;
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QSignalSpy topology(&corrections, &GPSCorrectionManager::stateChanged);
    auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster/mount"));
    const auto frame = GPSTest::rtcmMessage(1005, 20);
    ntrip->submit(frame, scheduler.nowMs());
    // The state publishes in batches, together with its notification.
    QCOMPARE(corrections.state(), State::Inactive);
    QCOMPARE(topology.size(), 0);
    QVERIFY(advanceDiagnostics(scheduler));
    QCOMPARE(topology.size(), 1);
    QCOMPARE(corrections.state(), State::Fresh);
    QCOMPARE(corrections.selectedStream().instanceId, QStringLiteral("caster/mount"));
    ntrip->submit(frame, scheduler.nowMs());
    QVERIFY(advanceDiagnostics(scheduler));
    QCOMPARE(topology.size(), 1);
    ntrip.reset();
    QCOMPARE(corrections.state(), State::Fresh);
    QCOMPARE(topology.size(), 1);
    QVERIFY(advanceTick(scheduler));
    QCOMPARE(topology.size(), 2);
    QCOMPARE(corrections.state(), State::Inactive);
    QCOMPARE(corrections.selectedStream(), GPSCorrectionStream{});
}

UT_REGISTER_TEST(GPSCorrectionManagerTest, TestLabel::Unit)
