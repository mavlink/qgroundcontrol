#include "GPSCorrectionManagerTest.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <unistd.h>
#endif

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSReceiver.h"
#include "GPSSettingsBindings.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QGCFormat.h"
#include "RTCMFramer.h"
#include "RTCMMessageCountModel.h"
#include "RTCMUdpInput.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
#include "Transport/Support/UnusedUdpPort.h"

using namespace GPSTest;

namespace {
#ifdef Q_OS_LINUX
/// Points this process's open descriptors of @a file at /dev/full, so that what is written to the open file from now
/// on fails as on a full disk. Only that file is affected. @return whether a descriptor was found.
bool failWritesTo(const QString& file)
{
    constexpr int DESCRIPTORS = 4096;
    const QString path = QFileInfo(file).canonicalFilePath();
    const int full = ::open("/dev/full", O_WRONLY | O_CLOEXEC);
    if (path.isEmpty() || full < 0) {
        return false;
    }
    bool found = false;
    for (int descriptor = 0; descriptor < DESCRIPTORS; ++descriptor) {
        if (descriptor != full && QFile::symLinkTarget(QStringLiteral("/proc/self/fd/%1").arg(descriptor)) == path) {
            found = ::dup2(full, descriptor) == descriptor || found;
        }
    }
    (void) ::close(full);
    return found;
}
#endif

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

void GPSCorrectionManagerTest::_messageFilter_data()
{
    QTest::addColumn<QString>("filter");
    // Of the message IDs 1, 1005, 1077, 1087, 1230 and 4095, each offered once.
    QTest::addColumn<QList<int>>("taken");
    QTest::addColumn<QString>("invalidEntry");
    const QList<int> all{1, 1005, 1077, 1087, 1230, 4095};
    QTest::newRow("empty") << QString() << all << QString();
    QTest::newRow("single") << QStringLiteral("1005") << QList<int>{1005} << QString();
    QTest::newRow("multiple") << QStringLiteral("1087,1005,1077") << QList<int>{1005, 1077, 1087} << QString();
    QTest::newRow("blank-entries") << QStringLiteral(" 1005,, ,1077,") << QList<int>{1005, 1077} << QString();
    QTest::newRow("12-bit-bounds") << QStringLiteral("1,4095") << QList<int>{1, 4095} << QString();
    // Skipping a mistyped entry could leave an empty filter, which takes every message.
    QTest::newRow("not-a-number") << QStringLiteral("1005,abc,,1077") << QList<int>{} << QStringLiteral("abc");
    QTest::newRow("semicolon-separator") << QStringLiteral("1005;1077") << QList<int>{} << QStringLiteral("1005;1077");
    QTest::newRow("space-separator") << QStringLiteral("1005,1077 1087") << QList<int>{} << QStringLiteral("1077 1087");
    QTest::newRow("zero") << QStringLiteral("1005,0") << QList<int>{} << QStringLiteral("0");
    QTest::newRow("negative") << QStringLiteral("-1,1005") << QList<int>{} << QStringLiteral("-1");
    QTest::newRow("above-12-bits") << QStringLiteral("4096") << QList<int>{} << QStringLiteral("4096");
}

void GPSCorrectionManagerTest::_messageFilter()
{
    QFETCH(QString, filter);
    QFETCH(QList<int>, taken);
    QFETCH(QString, invalidEntry);
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    if (!invalidEntry.isEmpty()) {
        expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Invalid RTCM message filter entry")));
    }
    corrections.setConfiguration({.udpInput = {}, .udpOutput = {}, .messageFilter = filter});
    const QString error = corrections.messageFilterError();
    if (invalidEntry.isEmpty()) {
        QVERIFY2(error.isEmpty(), qPrintable(error));
    } else {
        verifyExpectedLogMessage();
        // The message quotes the entry to correct.
        QVERIFY2(error.contains(QStringLiteral("\"%1\"").arg(invalidEntry)), qPrintable(error));
    }

    const auto source = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    for (const int messageId : {1, 1005, 1077, 1087, 1230, 4095}) {
        source->submit(GPSTest::rtcmMessage(static_cast<uint16_t>(messageId)), scheduler.nowMs());
    }
    QList<int> routed;
    for (const QByteArray& frame : std::as_const(sent)) {
        routed.append(RTCMFramer::frameMessageId(frame));
    }
    QCOMPARE(routed, taken);
}

void GPSCorrectionManagerTest::_messageFilterGovernsRouting()
{
    using State = GPSCorrectionManager::State;
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    saved.setFactValue(settings->rtcmUdpInputEnabled(), false);
    saved.setFactValue(settings->correctionSource(), GPSCorrectionSettings::HighestPriority);
    saved.setFactValue(settings->ntripWhitelist(), QStringLiteral("1005"));
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, 0));
    configureUdpOutput(saved, settings, QStringLiteral("127.0.0.1"), destination.localPort());
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    GPSSettingsBindings::bindCorrections(settings, &corrections);
    const QByteArray taken = GPSTest::rtcmMessage(1005, 20);
    const QByteArray dropped = GPSTest::rtcmMessage(1077, 30);
    const auto submit = [&scheduler](const auto& source, const QByteArray& frame) {
        source->submit(frame, scheduler.nowMs());
        return advanceDiagnostics(scheduler);
    };

    // Whatever its source, a stream delivers only the messages the filter takes: the others neither reach vehicles
    // nor make the stream a fresh one.
    for (const auto category :
         {GPSCorrectionSettings::LocalReceiver, GPSCorrectionSettings::Ntrip, GPSCorrectionSettings::Udp}) {
        const auto source = corrections.openSource(category, {});
        QVERIFY(submit(source, dropped));
        QCOMPARE(corrections.state(), State::Waiting);
        QVERIFY(submit(source, taken));
        QCOMPARE(corrections.state(), State::Fresh);
        QCOMPARE(corrections.selectedStream().source, static_cast<int>(category));
        QVERIFY(submit(source, dropped));
        // The dropped messages are not counted either.
        const RTCMMessageCountModel* const counts = corrections.messageCounts();
        QCOMPARE(counts->rowCount(), 1);
        QCOMPARE(counts->data(counts->index(0), RTCMMessageCountModel::MessageIdRole).toInt(), 1005);
        QCOMPARE(counts->data(counts->index(0), RTCMMessageCountModel::CountRole).toInt(), 1);
    }
    QCOMPARE(sent, (QList<QByteArray>{taken, taken, taken}));

    // An entry that is not a message ID stops routing instead of sending every message, and is reported.
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    QSignalSpy errorChanges(&corrections, &GPSCorrectionManager::messageFilterChanged);
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM message filter entry")));
    settings->ntripWhitelist()->setRawValue(QStringLiteral("1005;1077"));
    verifyExpectedLogMessage();
    QVERIFY(corrections.messageFilterError().contains(QStringLiteral("\"1005;1077\"")));
    QCOMPARE(errorChanges.size(), 1);
    QVERIFY(submit(ntrip, taken));
    QCOMPARE(sent.size(), 3);
    QCOMPARE(corrections.state(), State::Waiting);

    // A corrected or emptied filter applies to the next message.
    settings->ntripWhitelist()->setRawValue(QStringLiteral("1077"));
    QVERIFY(corrections.messageFilterError().isEmpty());
    QCOMPARE(errorChanges.size(), 2);
    QVERIFY(submit(ntrip, dropped));
    QVERIFY(submit(ntrip, taken));
    settings->ntripWhitelist()->setRawValue(QString());
    QVERIFY(submit(ntrip, taken));
    QCOMPARE(errorChanges.size(), 2);
    const QList<QByteArray> routed{taken, taken, taken, dropped, taken};
    QCOMPARE(sent, routed);

    // UDP forwarding sent what vehicles received: loopback delivers in order.
    QList<QByteArray> forwarded;
    while (forwarded.size() < routed.size() && destination.waitForReadyRead(TestTimeout::mediumMs())) {
        while (destination.hasPendingDatagrams()) {
            forwarded.append(destination.receiveDatagram().data());
        }
    }
    QCOMPARE(forwarded, routed);
}

void GPSCorrectionManagerTest::_messageFilterLeavesNoStream()
{
    using State = GPSCorrectionManager::State;
    using Configuration = GPSCorrectionManager::Configuration;
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    const auto submit = [&scheduler](const auto& source, uint16_t messageId) {
        source->submit(GPSTest::rtcmMessage(messageId, 20), scheduler.nowMs());
        return advanceDiagnostics(scheduler);
    };
    // A filter saved for an NTRIP stream, and a local base station that sends none of its messages.
    const Configuration automatic{.udpInput = {}, .udpOutput = {}, .messageFilter = QStringLiteral("1005,1077")};
    corrections.setConfiguration(automatic);
    const auto local = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    for (const uint16_t messageId : {1084, 1230, 1074, 1084}) {
        QVERIFY(submit(local, messageId));
    }
    QCOMPARE(corrections.state(), State::Waiting);
    QVERIFY(sent.isEmpty());
    // The message names what arrives, so the filter can be completed.
    QVERIFY2(corrections.messageFilterError().contains(QStringLiteral("(1074, 1084, 1230)")),
             qPrintable(corrections.messageFilterError()));

    // A stream the filter takes messages of is routed, so nothing is wrong.
    auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    QVERIFY(submit(ntrip, 1005));
    QCOMPARE(corrections.state(), State::Fresh);
    QVERIFY(corrections.messageFilterError().isEmpty());
    QVERIFY(submit(local, 1074));
    QVERIFY(corrections.messageFilterError().isEmpty());

    // The messages of a source the routing does not select from are not what keeps corrections from vehicles.
    Configuration ntripOnly = automatic;
    ntripOnly.source = GPSCorrectionSettings::Ntrip;
    corrections.setConfiguration(ntripOnly);
    ntrip.reset();
    QVERIFY(advanceDiagnostics(scheduler));
    QCOMPARE(corrections.state(), State::Waiting);
    QVERIFY(submit(local, 1074));
    QVERIFY(corrections.messageFilterError().isEmpty());

    // Once such messages stop arriving, the filter is no longer what stands in the way.
    corrections.setConfiguration(automatic);
    QVERIFY(submit(local, 1074));
    QVERIFY(corrections.messageFilterError().contains(QStringLiteral("(1074)")));
    // The state is refreshed every second.
    QVERIFY(scheduler.advanceBy(GPSCorrectionSelector::FRESHNESS_TIMEOUT + std::chrono::seconds(1)));
    QVERIFY(corrections.messageFilterError().isEmpty());
}

void GPSCorrectionManagerTest::_logsRoutedStream()
{
    TestFixtures::TempDirFixture directory;
    QVERIFY(directory.isValid());
    QString folder = QDir(directory.path()).filePath(QStringLiteral("first"));
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler, {.logDirectory = [&folder] { return folder; }});
    corrections.setConfiguration({.udpInput = {}, .udpOutput = {}, .messageFilter = QStringLiteral("1005,1077")});
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    QSignalSpy changes(&corrections, &GPSCorrectionManager::logChanged);
    const auto local = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    const auto submit = [&scheduler](const auto& source, uint16_t messageId) {
        source->submit(GPSTest::rtcmMessage(messageId, messageId % 50), scheduler.nowMs());
    };

    // Off unless turned on.
    QVERIFY(!corrections.logToFile());
    QVERIFY(corrections.logFile().isEmpty());
    submit(local, 1005);
    QCOMPARE(sent.size(), 1);

    corrections.setLogToFile(true);
    QVERIFY(corrections.logToFile());
    QCOMPARE(changes.size(), 1);
    const QString file = corrections.logFile();
    QCOMPARE(QFileInfo(file).absolutePath(), folder);
    QVERIFY2(QRegularExpression(QStringLiteral("^\\d{8}T\\d{6}\\.\\d{3}Z\\.rtcm3$"))
                 .match(QFileInfo(file).fileName())
                 .hasMatch(),
             qPrintable(file));

    // The log is the stream as vehicles receive it from then on: the frames of the selected source that the message
    // filter takes, byte for byte.
    sent.clear();
    submit(local, 1077);
    submit(ntrip, 1005);
    submit(local, 1087);
    submit(local, 1005);
    QCOMPARE(sent.size(), 2);
    // Each tick writes the buffered frames out, so the file can be read while the log goes on.
    QVERIFY(advanceTick(scheduler));
    const QByteArray logged = sent.join();
    QCOMPARE(readFile(file), logged);

    corrections.setLogToFile(false);
    QVERIFY(!corrections.logToFile());
    QVERIFY(corrections.logFile().isEmpty());
    QCOMPARE(changes.size(), 2);
    submit(local, 1005);
    QCOMPARE(readFile(file), logged);

    // The next log is a file of its own, in the folder as it is by then, and shutting down closes it.
    folder = QDir(directory.path()).filePath(QStringLiteral("second"));
    corrections.setLogToFile(true);
    const QString next = corrections.logFile();
    QCOMPARE(QFileInfo(next).absolutePath(), folder);
    sent.clear();
    submit(local, 1077);
    corrections.shutdown();
    QCOMPARE(readFile(next), sent.join());
    QCOMPARE(readFile(file), logged);
}

void GPSCorrectionManagerTest::_logThatCannotStart_data()
{
    QTest::addColumn<bool>("folderKnown");
    QTest::addColumn<QString>("category");
    QTest::addColumn<QString>("warning");
    QTest::newRow("no-folder") << false << QStringLiteral("GPS.Corrections.GPSCorrectionManager")
                               << QStringLiteral("No folder");
    // A regular file is where the folder should be.
    QTest::newRow("folder-unavailable") << true << QStringLiteral("GPS.Corrections.RTCMLogFile")
                                        << QStringLiteral("file error");
}

void GPSCorrectionManagerTest::_logThatCannotStart()
{
    QFETCH(bool, folderKnown);
    QFETCH(QString, category);
    QFETCH(QString, warning);
    TestFixtures::TempDirFixture directory;
    const QString folder =
        folderKnown ? directory.createFile(QStringLiteral("GPSCorrections"), "not a folder") : QString();
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler, {.logDirectory = [&folder] { return folder; }});
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    QSignalSpy changes(&corrections, &GPSCorrectionManager::logChanged);

    QVERIFY(corrections.logError().isEmpty());
    expectLogMessage(category.toLatin1().constData(), QtWarningMsg, QRegularExpression(warning));
    corrections.setLogToFile(true);
    verifyExpectedLogMessage();
    QVERIFY(!corrections.logToFile());
    QVERIFY(corrections.logFile().isEmpty());
    // Notified although the log stays off, so that a switch bound to the property goes back to off.
    QCOMPARE(changes.size(), 1);
    // The user is told why, not only the application log.
    QCOMPARE(corrections.logError(),
             folderKnown
                 ? GPSCorrectionManager::tr("Cannot log the corrections: the folder %1 cannot be created.").arg(folder)
                 : GPSCorrectionManager::tr("Cannot log the corrections: there is no folder to save the log to."));

    // The corrections are routed all the same.
    const auto source = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    const QByteArray frame = GPSTest::rtcmMessage(1005, 20);
    source->submit(frame, scheduler.nowMs());
    QCOMPARE(sent, QList<QByteArray>{frame});
}

void GPSCorrectionManagerTest::_logKeepsExistingFile()
{
    // Two logs can be named alike after the clock was set back.
    TestFixtures::TempDirFixture directory;
    const QString earlier = directory.createFile(QStringLiteral("20260102T030405.006Z.rtcm3"), "earlier log");
    RTCMLogFile log;
    expectLogMessage("GPS.Corrections.RTCMLogFile", QtWarningMsg, QRegularExpression(QStringLiteral("file error")));
    QVERIFY(!log.open(earlier));
    verifyExpectedLogMessage();
    QVERIFY(log.file().isEmpty());
    // The system's reason is kept for the user.
    QCOMPARE(log.end(), GPSBoundedFile::End::FileError);
    QVERIFY(!log.errorString().isEmpty());
    QCOMPARE(readFile(earlier), QByteArray("earlier log"));
}

void GPSCorrectionManagerTest::_logsStartedTogetherGetOwnFiles()
{
    TestFixtures::TempDirFixture directory;
    const QString folder = directory.path();
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler, {.logDirectory = [&folder] { return folder; }});

    // A log is named by the millisecond it starts in. These start far more often than that, as a log that is turned
    // on again from a signal of the one before does, and none is refused as an earlier log of its name.
    QStringList files;
    for (int start = 0; start < 100; ++start) {
        corrections.setLogToFile(true);
        QVERIFY2(corrections.logToFile(), qPrintable(corrections.logError()));
        files.append(QFileInfo(corrections.logFile()).fileName());
        corrections.setLogToFile(false);
    }
    // Their names sort in the order they started.
    QCOMPARE(QDir(folder).entryList(QDir::Files, QDir::Name), files);
}

void GPSCorrectionManagerTest::_logEndsAtSizeLimit()
{
    TestFixtures::TempDirFixture directory;
    const QString folder = directory.path();
    const QByteArray frame = GPSTest::rtcmMessage(1077, 100);
    ManualScheduler scheduler;
    // Room for two frames and part of a third.
    GPSCorrectionManager corrections(
        nullptr, &scheduler, {.logDirectory = [&folder] { return folder; }, .maxLogFileBytes = 2 * frame.size() + 10});
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    corrections.setLogToFile(true);
    const QString file = corrections.logFile();
    QVERIFY(!file.isEmpty());
    QSignalSpy changes(&corrections, &GPSCorrectionManager::logChanged);
    const auto source = corrections.openSource(GPSCorrectionSettings::Ntrip, {});

    source->submit(frame, scheduler.nowMs());
    source->submit(frame, scheduler.nowMs());
    QVERIFY(corrections.logToFile());
    expectLogMessage("GPS.Corrections.RTCMLogFile", QtWarningMsg, QRegularExpression(QStringLiteral("size limit")));
    source->submit(frame, scheduler.nowMs());
    verifyExpectedLogMessage();

    // The log ended, says why and no longer names its file; it holds whole frames, and the routing went on.
    QVERIFY(!corrections.logToFile());
    QVERIFY(corrections.logFile().isEmpty());
    QCOMPARE(corrections.logError(), GPSCorrectionManager::tr("The log reached its size limit of %1 and stopped.")
                                         .arg(QGC::bigSizeToString(2 * frame.size() + 10)));
    QCOMPARE(changes.size(), 1);
    source->submit(frame, scheduler.nowMs());
    QCOMPARE(sent.size(), 4);
    QCOMPARE(readFile(file), frame + frame);

    // The reason stays until the log is turned on again, which starts a file of its own in the same folder.
    QVERIFY(advanceTick(scheduler));
    QVERIFY(!corrections.logError().isEmpty());
    corrections.setLogToFile(true);
    QVERIFY(corrections.logToFile());
    QVERIFY(corrections.logError().isEmpty());
    QCOMPARE(changes.size(), 2);
}

void GPSCorrectionManagerTest::_logStopsAfterWriteError_data()
{
    QTest::addColumn<bool>("bufferFills");
    // The frames are written out every second, and sooner once they fill the file's buffer.
    QTest::newRow("at-the-tick") << false;
    QTest::newRow("as-the-buffer-fills") << true;
}

void GPSCorrectionManagerTest::_logStopsAfterWriteError()
{
#ifdef Q_OS_LINUX
    QFETCH(bool, bufferFills);
    TestFixtures::TempDirFixture directory;
    const QString folder = directory.path();
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler, {.logDirectory = [&folder] { return folder; }});
    QList<QByteArray> sent;
    captureVehicleFrames(corrections, sent);
    const auto source = corrections.openSource(GPSCorrectionSettings::Ntrip, {});
    const QByteArray frame = GPSTest::rtcmMessage(1077, 100);
    corrections.setLogToFile(true);
    const QString file = corrections.logFile();
    source->submit(frame, scheduler.nowMs());
    QVERIFY(advanceTick(scheduler));
    QCOMPARE(readFile(file), frame);
    QSignalSpy changes(&corrections, &GPSCorrectionManager::logChanged);

    // The log is open, and then its disk is full.
    QVERIFY(failWritesTo(file));
    expectLogMessage("GPS.Corrections.RTCMLogFile", QtWarningMsg, QRegularExpression(QStringLiteral("file error")));
    if (bufferFills) {
        // Several times what QFile buffers before it writes.
        for (int frames = 0; corrections.logToFile() && frames < 1000; ++frames) {
            source->submit(frame, scheduler.nowMs());
        }
    } else {
        source->submit(frame, scheduler.nowMs());
        QVERIFY(corrections.logToFile());
        QVERIFY(advanceTick(scheduler));
    }
    verifyExpectedLogMessage();

    // The log ended, says why with the system's reason and no longer names its file. What it wrote before stays,
    // and the routing goes on.
    QVERIFY(!corrections.logToFile());
    QVERIFY(corrections.logFile().isEmpty());
    QCOMPARE(corrections.logError(), GPSCorrectionManager::tr("The log stopped after a file error: %1")
                                         .arg(QCoreApplication::translate("QIODevice", "No space left on device")));
    QCOMPARE(changes.size(), 1);
    const qsizetype routed = sent.size();
    source->submit(frame, scheduler.nowMs());
    QCOMPARE(sent.size(), routed + 1);
    QCOMPARE(readFile(file), frame);
#else
    QSKIP("Needs /proc/self/fd and /dev/full to fail the writes to an open file");
#endif
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

void GPSCorrectionManagerTest::_udpOutputReportsSendFailure()
{
    QUdpSocket destination;
    QVERIFY(destination.bind(QHostAddress::LocalHost, 0));
    const quint16 port = destination.localPort();
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    corrections.setConfiguration(
        {.udpInput = {}, .udpOutput = {.enabled = true, .address = QStringLiteral("127.0.0.1"), .port = port}});
    QVERIFY(corrections.udpOutputError().isEmpty());
    QSignalSpy changes(&corrections, &GPSCorrectionManager::udpOutputChanged);
    const auto source = corrections.openSource(GPSCorrectionSettings::Ntrip, {});

    // No UDP datagram carries this much, so the socket refuses to send it.
    const QByteArray oversized(70000, '\0');
    expectLogMessage("Utilities.UdpForwarder", QtWarningMsg, QRegularExpression(QStringLiteral("UDP forward failed")));
    source->submit(oversized, scheduler.nowMs());
    verifyExpectedLogMessage();
    // A repeated failure keeps its reason without warning or notifying again.
    source->submit(oversized, scheduler.nowMs());
    const QString reason = udpForwarding(corrections)->errorString();
    QVERIFY(!reason.isEmpty());
    const QString error = corrections.udpOutputError();
    QVERIFY2(error.contains(QString::number(port)) && error.endsWith(reason), qPrintable(error));
    QCOMPARE(changes.size(), 1);

    const QByteArray frame = GPSTest::rtcmMessage(1005, 20);
    source->submit(frame, scheduler.nowMs());
    QVERIFY(corrections.udpOutputError().isEmpty());
    QCOMPARE(changes.size(), 2);
    QVERIFY(destination.waitForReadyRead(TestTimeout::mediumMs()));
    QCOMPARE(destination.receiveDatagram().data(), frame);
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
    // A numeric zone ID looks like a place marker of the error message.
    QTest::newRow("zoned-loopback") << QStringLiteral("::1%1");
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
    const QString error = corrections.udpOutputError();
    QVERIFY2(error.contains(target) && error.contains(QString::number(port)), qPrintable(error));
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
