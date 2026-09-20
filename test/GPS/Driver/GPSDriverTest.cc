#include "GPSDriverTest.h"

#include <cerrno>
#include <cstring>
#include <limits>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QThread>
#include <QtCore/QtEndian>
#include <QtPositioning/QGeoCoordinate>

#include "../RTK/ScriptedSBFReceiver.h"
#include "GPSBaseStationConfig.h"
#include "GPSDriver.h"
#include "GPSTransport.h"
#include "ScriptedUBXReceiver.h"

Q_DECLARE_METATYPE(GPSBaseStationConfig)
Q_DECLARE_METATYPE(GPSReceiverConfig)
Q_DECLARE_METATYPE(GPSWriteResult)
Q_DECLARE_METATYPE(ScriptedUBXReceiver::Model)
Q_DECLARE_METATYPE(ScriptedUBXReceiver::DisableReply)
Q_DECLARE_METATYPE(ScriptedUBXReceiver::ReadbackReply)

namespace {

static_assert(static_cast<int>(GPSType::ublox) == 0);
static_assert(static_cast<int>(GPSType::trimble) == 1);
static_assert(static_cast<int>(GPSType::septentrio) == 2);
static_assert(static_cast<int>(GPSType::femto) == 3);
static_assert(static_cast<int>(GPSType::unicore) == 4);
static_assert(static_cast<int>(GPSType::quectel) == 5);
static_assert(static_cast<int>(GPSType::passive) == 6);

const std::atomic_bool neverStop{false};

QByteArray nmeaFrame(const QByteArray& body)
{
    uint8_t checksum = 0;
    for (const char byte : body) {
        checksum ^= static_cast<uint8_t>(byte);
    }
    return '$' + body + '*' + QByteArray::number(checksum, 16).rightJustified(2, '0').toUpper() + "\r\n";
}

class FakeGPSTransport : public GPSTransport
{
public:
    FakeGPSTransport()
        : GPSTransport(neverStop)
    {}

    GPSOpenResult open() override
    {
        ++openCalls;
        return {GPSOpenStatus::Opened};
    }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override
    {
        lastReadLength = length;
        lastReadTimeoutMs = timeoutMs;
        if (readOverride) {
            return *readOverride;
        }
        if (acknowledgeAshtech && scriptedRead.isEmpty() && timeoutMs > 0) {
            QThread::msleep(static_cast<unsigned long>(timeoutMs));
            return {GPSReadStatus::TimedOut};
        }
        const int n = qMin(static_cast<int>(scriptedRead.size()), length);
        (void) memcpy(buffer, scriptedRead.constData(), static_cast<size_t>(n));
        if (acknowledgeFemto || acknowledgeAshtech) {
            scriptedRead.remove(0, n);
        }
        return {GPSReadStatus::Data, n};
    }

    GPSWriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override
    {
        if (deadline.hasExpired()) {
            return {GPSWriteStatus::TimedOut};
        }
        lastWrite = QByteArray(reinterpret_cast<const char*>(buffer), length);
        if (acknowledgeFemto) {
            scriptedRead = '<' + lastWrite.split(' ').first().trimmed() + " OK" + char(0);
        } else if (acknowledgeAshtech) {
            scriptedRead = nmeaFrame(lastWrite.startsWith("$PASHQ,PRT")   ? "PASHR,PRT,A,115200"
                                     : lastWrite.startsWith("$PASHQ,RID") ? "PASHR,RID,MB2"
                                                                          : "PASHR,ACK");
        }
        if (writeOverride) {
            return *writeOverride;
        }
        return writeOk ? GPSWriteResult{GPSWriteStatus::Completed, length, length}
                       : GPSWriteResult{GPSWriteStatus::Error};
    }

    bool setBaudrate(unsigned baudrate) override
    {
        lastBaudrate = baudrate;
        return baudrateOk;
    }

    std::optional<GPSReadResult> readOverride;
    std::optional<GPSWriteResult> writeOverride;
    QByteArray scriptedRead;
    int openCalls = 0;
    int lastReadLength = -1;
    int lastReadTimeoutMs = -1;
    QByteArray lastWrite;
    unsigned lastBaudrate = 0;
    bool baudrateOk = true;
    bool writeOk = true;
    bool acknowledgeFemto = false;
    bool acknowledgeAshtech = false;
};

class ConfigurationProbeTransport : public GPSTransport
{
public:
    ConfigurationProbeTransport()
        : GPSTransport(neverStop)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    bool setBaudrate(unsigned) override { return true; }

    GPSReadResult read(uint8_t*, int, int) override { return {GPSReadStatus::TimedOut}; }

    std::chrono::milliseconds configurationWriteTimeout() const override { return cap; }

    GPSWriteResult writeBounded(const uint8_t*, int length, QDeadlineTimer deadline) override
    {
        ++boundedCalls;
        budgetMs = deadline.remainingTime();
        if (delayReturn) {
            // Completion occurred, but its return was descheduled beyond the command's budget.
            QThread::msleep(static_cast<unsigned long>(budgetMs + 10));
            return {GPSWriteStatus::Completed, length, length};
        }
        return {GPSWriteStatus::Unsupported};
    }

    std::chrono::milliseconds cap{500};
    bool delayReturn = false;
    int boundedCalls = 0;
    qint64 budgetMs = 0;
};

}  // namespace

void GPSDriverTest::_ashtechSatelliteSnapshots()
{
    FakeGPSTransport transport;
    transport.acknowledgeAshtech = true;
    std::vector<GPSSatelliteReport> snapshots;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const auto& snapshot) { snapshots.push_back(snapshot); };
    GPSDriver driver(GPSType::trimble, transport,
                     {.base = {.useFixedBase = true,
                               .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}},
                     sinks);
    QVERIFY(driver.configure());
    unsigned epoch = 120000;
    const auto feed = [&](const QByteArray& body) {
        snapshots.clear();
        transport.scriptedRead =
            nmeaFrame(body) + nmeaFrame("GNRMC," + QByteArray::number(++epoch) + ".00,V,,,,,,,090926,,,N");
        return driver.receiveOutcome(20);
    };
    QCOMPARE(feed("GPGSV,1,1,01,01,10,20,30").status, GPSReceiveStatus::Data);
    QCOMPARE(snapshots.size(), size_t(2));
    QCOMPARE(snapshots[0].count, 1);
    QCOMPARE(snapshots[1].count, 1);  // Empty SBAS scope must not clear GPS.
    QCOMPARE(feed("GLGSV,1,1,01,65,20,30,40").updates, 2);
    QCOMPARE(snapshots.back().count, 2);
    QCOMPARE(feed("GPGSV,1,1,02,01,10,20,30,33,15,25,35").status, GPSReceiveStatus::Data);
    QCOMPARE(snapshots.back().count, 3);
    QCOMPARE(feed("GLGSV,1,1,00").status, GPSReceiveStatus::Data);
    QCOMPARE(snapshots.back().count, 2);
    QCOMPARE(feed("GPGSV,1,1,00").status, GPSReceiveStatus::Data);
    QCOMPARE(snapshots.size(), size_t(2));
    QCOMPARE(snapshots[0].count, 1);
    QCOMPARE(snapshots[1].count, 0);
}

void GPSDriverTest::_nativeIntegrityProvenance()
{
    std::atomic_bool stop = false;
    ScriptedUBXReceiver receiver(ScriptedUBXReceiver::Model::F9P, stop);
    std::vector<GPSPositionReport> positions;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const auto& report) { positions.push_back(report); };
    GPSDriver driver(GPSType::ublox, receiver, {.role = GPSReceiverConfig::Role::Position}, sinks);
    QVERIFY(driver.configure());
    receiver.coalesceReplies = true;
    const auto navigation = [&](uint32_t tow) {
        QByteArray pvt(92, '\0');
        qToLittleEndian(tow, pvt.data());
        pvt[20] = 3;
        pvt[21] = 1;
        receiver.queueFrame(0x01, 0x07, pvt);
        QByteArray end(4, '\0');
        qToLittleEndian(tow, end.data());
        receiver.queueFrame(0x01, 0x61, end);
        for (int attempt = 0; attempt < 4; ++attempt) {
            QVERIFY(!driver.receiveOutcome(0).terminal());
        }
    };
    QByteArray rf(28, '\0');
    rf[1] = 1;
    rf[5] = 3;
    receiver.queueFrame(0x0a, 0x38, rf);
    navigation(1000);
    QVERIFY(!positions.empty());
    const auto first = positions.back();
    QCOMPARE(first.integrity.jamming, GPSIntegrityReport::JammingState::Critical);
    QVERIFY(first.integrity.jammingTimestampUs > 0);

    QByteArray status(16, '\0');
    status[7] = 2 << 3;
    receiver.queueFrame(0x01, 0x03, status);
    navigation(2000);
    QCOMPARE(positions.size(), size_t{2});
    const auto second = positions.back();
    QCOMPARE(second.integrity.jammingTimestampUs, first.integrity.jammingTimestampUs);
    QCOMPARE(second.integrity.rfTimestampUs, first.integrity.rfTimestampUs);
    QVERIFY(second.integrity.spoofingTimestampUs > first.integrity.jammingTimestampUs);
    QCOMPARE(second.integrity.spoofing, GPSIntegrityReport::SpoofingState::Indicated);
    const auto fresh = second.integrity.freshAt(first.integrity.jammingTimestampUs + 5'000'000);
    QCOMPARE(fresh.jamming, GPSIntegrityReport::JammingState::Unknown);
    QVERIFY(!fresh.noisePerMillisecond);
    QCOMPARE(fresh.spoofing, GPSIntegrityReport::SpoofingState::Indicated);
    QCOMPARE(first.integrity.jamming, GPSIntegrityReport::JammingState::Critical);
}

void GPSDriverTest::_femtoSatelliteUsage()
{
    FakeGPSTransport transport;
    transport.acknowledgeFemto = true;
    std::vector<GPSSatelliteUsageReport> usage;
    int snapshots = 0;
    GPSDriverSinks sinks;
    sinks.onSatelliteInfo = [&](const auto&) { ++snapshots; };
    sinks.onSatelliteUsage = [&](const auto& report) { usage.push_back(report); };
    GPSDriver driver(GPSType::femto, transport,
                     {.base = {.useFixedBase = true,
                               .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}},
                     sinks);
    QVERIFY(driver.configure());
    for (const auto& count : {QByteArray("12"), QByteArray("00"), QByteArray()}) {
        transport.scriptedRead = nmeaFrame("GPGGA,123519,4807.038,N,01131.000,E,1," + count + ",0.9,545.4,M,46.9,M,,");
        const auto result = driver.receiveOutcome(20);
        QCOMPARE(result.status, GPSReceiveStatus::Data);
        QCOMPARE(result.updates & 2, 2);
    }
    QCOMPARE(usage.size(), size_t(3));
    QCOMPARE(usage[0].usedCount, std::optional<int>{12});
    QCOMPARE(usage[1].usedCount, std::optional<int>{0});
    QVERIFY(!usage[2].usedCount);
    QVERIFY(usage[0].timestampUs > 0);
    QCOMPARE(snapshots, 0);
}

void GPSDriverTest::_receiveOutcomes()
{
    FakeGPSTransport transport;
    transport.acknowledgeFemto = true;
    GPSDriver driver(GPSType::femto, transport,
                     {.base = {.useFixedBase = true,
                               .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}},
                     {});
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::NotConfigured);
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
    transport.scriptedRead.clear();
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Idle);
    transport.scriptedRead = nmeaFrame("GPTXT,01,01,02,diagnostic");
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::Activity);
    const QString detail = QStringLiteral("Receiver disconnected: Gerät");
    transport.readOverride = GPSReadResult{GPSReadStatus::Error, 0, detail};
    expectLogMessage("GPS.Drivers", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed \\(status %1, code %2\\): %3")
                                            .arg(static_cast<int>(GPSReadStatus::Error))
                                            .arg(-EIO)
                                            .arg(QRegularExpression::escape(detail))));
    const auto failed = driver.receiveOutcome(0);
    verifyExpectedLogMessage();
    QCOMPARE(failed.status, GPSReceiveStatus::TransportError);
    QCOMPARE(failed.detail, detail);
    QVERIFY(!transport.fatalError());
    const auto latched = driver.receiveOutcome(0);
    QCOMPARE(latched.status, GPSReceiveStatus::TransportError);
    QCOMPARE(latched.detail, detail);
    transport.readOverride.reset();
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
    transport.readOverride = GPSReadResult{GPSReadStatus::Cancelled, 0, QStringLiteral("Receiver stopped")};
    const auto cancelled = driver.receiveOutcome(0);
    QCOMPARE(cancelled.status, GPSReceiveStatus::Cancelled);
    QCOMPARE(cancelled.detail, transport.readOverride->detail);
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.configurationError(), transport.readOverride->detail);
    transport.readOverride.reset();
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationError().isEmpty());
    QVERIFY(driver.receiveOutcome(0).detail.isEmpty());
}

void GPSDriverTest::_sbfSatelliteUsage()
{
    ScriptedSBFReceiver receiver(neverStop);
    std::vector<GPSSatelliteUsageReport> usage;
    int snapshots = 0;
    GPSDriverSinks sinks;
    sinks.onSatelliteUsage = [&](const auto& report) { usage.push_back(report); };
    sinks.onSatelliteInfo = [&](const auto&) { ++snapshots; };
    GPSDriver driver(GPSType::septentrio, receiver,
                     {.base = {.useFixedBase = true,
                               .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}},
                     sinks);
    QVERIFY(driver.configure());
    uint32_t tow = 0;
    for (const uint8_t used : {12, 0, 255}) {
        receiver.reply = ScriptedSBFReceiver::pvt(used, ++tow);
        const auto result = driver.receiveOutcome(20);
        QCOMPARE(result.status, GPSReceiveStatus::Data);
        QCOMPARE(result.updates & 2, 2);
    }
    QCOMPARE(usage.size(), size_t(3));
    QCOMPARE(usage[0].usedCount, std::optional<int>{12});
    QCOMPARE(usage[1].usedCount, std::optional<int>{0});
    QVERIFY(!usage[2].usedCount);
    QCOMPARE(snapshots, 0);
}

void GPSDriverTest::_rtcmActivationRejected()
{
    std::atomic_bool stop = false;
    ScriptedUBXReceiver receiver(ScriptedUBXReceiver::Model::F9P, stop);
    GPSDriver driver(GPSType::ublox, receiver, {.base = {.surveyInAccMeters = 2, .surveyInDurationSecs = 1}}, {});
    QVERIFY(driver.configure());
    receiver.rejectRtcmActivation = true;
    QByteArray survey(40, '\0');
    qToLittleEndian<quint32>(5, survey.data() + 8);
    survey[36] = 1;
    receiver.queueFrame(0x01, 0x3b, survey);
    const auto result = driver.receiveOutcome(20);
    QCOMPARE(result.status, GPSReceiveStatus::ProtocolError);
    QVERIFY(result.terminal());
    QVERIFY(!receiver.fatalError());
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::ProtocolError);
    stop = true;
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::ProtocolError);
}

void GPSDriverTest::_configurationDeadline_data()
{
    QTest::addColumn<int>("capMs");
    QTest::addColumn<bool>("delayReturn");
    QTest::newRow("desktop-default") << 500 << false;
    QTest::newRow("short-transport-cap") << 20 << false;
    QTest::newRow("tcp-cap-command-deadline") << 5000 << false;
    QTest::newRow("expired-multipart") << 500 << true;
}

void GPSDriverTest::_configurationDeadline()
{
    QFETCH(int, capMs);
    QFETCH(bool, delayReturn);
    ConfigurationProbeTransport transport;
    transport.cap = std::chrono::milliseconds(capMs);
    transport.delayReturn = delayReturn;
    GPSDriver driver(GPSType::ublox, transport, {.role = GPSReceiverConfig::Role::Position}, {});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(transport.budgetMs > 0);
    QVERIFY(transport.budgetMs <= std::min(capMs, 250));
    if (delayReturn) {
        QCOMPARE(transport.boundedCalls, 1);
        QVERIFY(!driver.configurationEvidence().empty());
        QCOMPARE(driver.configurationEvidence().back().acceptedBytes, 6);
        QCOMPARE(driver.configurationEvidence().back().writtenBytes, 6);
        QCOMPARE(driver.configurationEvidence().back().uncertainBytes, 0);
    }
}

void GPSDriverTest::_femtoConfigurationSurvey()
{
    FakeGPSTransport transport;
    transport.acknowledgeFemto = true;
    int reports = 0;
    GPSSurveyReport survey;
    GPSDriverSinks sinks;
    sinks.onSurveyIn = [&](const auto& report) {
        ++reports;
        survey = report;
    };
    GPSDriver driver(
        GPSType::femto, transport,
        {.base = {.useFixedBase = true,
                  .fixedPosition = {.latitudeDegrees = -47.1, .longitudeDegrees = -8.2, .altitudeMeters = -25}}},
        sinks);
    QVERIFY(driver.configure());
    QCOMPARE(reports, 1);
    QCOMPARE(survey.position.latitudeDegrees, -47.1);
    QCOMPARE(survey.position.longitudeDegrees, -8.2);
    QCOMPARE(survey.position.altitudeMeters, -25);
    QVERIFY(survey.valid);
    QVERIFY(!survey.active);
    QVERIFY(!survey.meanAccuracyMeters);
    QCOMPARE(survey.duration.count(), 0);
    QVERIFY(!driver.configurationEvidence().empty());
    for (const auto& command : driver.configurationEvidence()) {
        QCOMPARE(command.outcome, GPSConfigurationOutcome::Acknowledged);
    }
}

void GPSDriverTest::_configurationWriteEvidence_data()
{
    QTest::addColumn<GPSWriteResult>("result");
    QTest::addColumn<int>("uncertain");
    QTest::newRow("partial-completed") << GPSWriteResult{GPSWriteStatus::Completed, 6, 2} << 4;
    QTest::newRow("short-completed") << GPSWriteResult{GPSWriteStatus::Completed, 2, 2} << 0;
    QTest::newRow("empty-completed") << GPSWriteResult{GPSWriteStatus::Completed, 0, 0} << 0;
    QTest::newRow("negative-accepted") << GPSWriteResult{GPSWriteStatus::Completed, -1, 6} << -1;
    QTest::newRow("negative-written") << GPSWriteResult{GPSWriteStatus::Completed, 6, -1} << -1;
    QTest::newRow("written-beyond-accepted") << GPSWriteResult{GPSWriteStatus::Completed, 6, 7} << -1;
    QTest::newRow("accepted-beyond-buffer") << GPSWriteResult{GPSWriteStatus::Completed, 7, 6} << 1;
    QTest::newRow("timeout-after-write") << GPSWriteResult{GPSWriteStatus::TimedOut, 6, 6} << 0;
    QTest::newRow("cancelled") << GPSWriteResult{GPSWriteStatus::Cancelled, 0, 0} << 0;
    QTest::newRow("error-after-write") << GPSWriteResult{GPSWriteStatus::Error, 6, 6} << 0;
    QTest::newRow("unsupported-after-write") << GPSWriteResult{GPSWriteStatus::Unsupported, 6, 6} << 0;
    QTest::newRow("invalid-after-write") << GPSWriteResult{GPSWriteStatus::InvalidData, 6, 6} << 0;
}

void GPSDriverTest::_configurationWriteEvidence()
{
    QFETCH(GPSWriteResult, result);
    QFETCH(int, uncertain);
    result.detail = QStringLiteral("Configuration write failed: Gerät");
    FakeGPSTransport transport;
    transport.writeOverride = result;
    GPSDriver driver(GPSType::ublox, transport, {.role = GPSReceiverConfig::Role::Position}, {});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.configurationError(), result.detail);
    QVERIFY(!driver.configurationEvidence().empty());
    for (const auto& command : driver.configurationEvidence()) {
        QCOMPARE(command.outcome, result.status == GPSWriteStatus::Cancelled ? GPSConfigurationOutcome::Cancelled
                                                                             : GPSConfigurationOutcome::TransportError);
        QCOMPARE(command.acceptedBytes, result.acceptedBytes);
        QCOMPARE(command.writtenBytes, result.writtenBytes);
        QCOMPARE(command.uncertainBytes, uncertain);
    }
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::NotConfigured);
}

void GPSDriverTest::_ashtechFixedSurvey()
{
    FakeGPSTransport transport;
    transport.acknowledgeAshtech = true;
    std::vector<GPSSurveyReport> surveys;
    GPSDriverSinks sinks;
    sinks.onSurveyIn = [&](const auto& report) { surveys.push_back(report); };
    GPSDriver driver(GPSType::trimble, transport,
                     {.base = {.useFixedBase = true,
                               .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}},
                     sinks);
    QVERIFY(driver.configure());
    surveys.clear();
    transport.scriptedRead = nmeaFrame(
        "PASHR,POS,2,10,125410.00,5525.8138702,N,03833.9587380,E,131.555,1.0,0.0,0.007,-0.001,2.0,1.0,1.7,1.0,");
    QCOMPARE(driver.receiveOutcome(50).status, GPSReceiveStatus::Data);
    QCOMPARE(surveys.size(), size_t(1));
    const auto& survey = surveys.front();
    QCOMPARE(survey.position.latitudeDegrees, 47);
    QCOMPARE(survey.position.longitudeDegrees, 8);
    QCOMPARE(survey.position.altitudeMeters, 500);
    QVERIFY(survey.valid);
    QVERIFY(!survey.active);
    QVERIFY(!survey.meanAccuracyMeters);
    QCOMPARE(survey.duration.count(), 0);
}

void GPSDriverTest::_freshSurveyAndEvidence_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::addColumn<bool>("stuck");
    for (const auto model : {ScriptedUBXReceiver::Model::M8PBase, ScriptedUBXReceiver::Model::F9P}) {
        QTest::newRow(qPrintable(QString::number(int(model)) + "-fresh")) << model << false;
        QTest::newRow(qPrintable(QString::number(int(model)) + "-stuck")) << model << true;
    }
}

void GPSDriverTest::_freshSurveyAndEvidence()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    QFETCH(bool, stuck);
    std::atomic_bool stop{false};
    ScriptedUBXReceiver receiver(model, stop);
    receiver.timeMode = 1;
    receiver.retainedSurveyDuration = 329000;
    receiver.surveyStopStuck = stuck;
    GPSDriver driver(GPSType::ublox, receiver, {.base = {.surveyInAccMeters = 2, .surveyInDurationSecs = 180}}, {});
    if (stuck) {
        expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression("Time mode did not stop"));
        expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression("Driver configuration failed"));
    }
    QCOMPARE(driver.configure(), !stuck);
    if (stuck) {
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        QCOMPARE(receiver.timeMode, 0u);
        QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::NotConfigured);
    } else {
        QCOMPARE(receiver.timeMode, 1u);
        QCOMPARE(receiver.retainedSurveyDuration, 0u);
        QCOMPARE(receiver.surveyDuration, 180u);
    }
    QCOMPARE(receiver.disableCommands, 1);
    QCOMPARE(receiver.timeModeReads, 1);
    QVERIFY(receiver.surveyStopReads > 0);
    const auto& evidence = driver.configurationEvidence();
    QVERIFY(!evidence.empty());
    bool acknowledged = false;
    bool verified = false;
    for (const auto& command : evidence) {
        acknowledged |= command.outcome == GPSConfigurationOutcome::Acknowledged;
        verified |= command.outcome == GPSConfigurationOutcome::ReadbackVerified;
        QVERIFY(command.finishedAtUs >= command.startedAtUs);
        QCOMPARE(command.uncertainBytes, 0);
    }
    QVERIFY(acknowledged);
    QVERIFY(verified);
}

void GPSDriverTest::_ubloxRoleTransition_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("corruptVersion");
    QTest::addColumn<float>("fixedAccuracyMeters");
    QTest::addColumn<quint32>("fixedAccuracyUnits");
    QTest::newRow("M8P-fixed") << ScriptedUBXReceiver::Model::M8PBase << true << false << 0.0f << quint32{0};
    QTest::newRow("M8P-survey") << ScriptedUBXReceiver::Model::M8PBase << false << false << 0.0f << quint32{0};
    QTest::newRow("F9P-fixed") << ScriptedUBXReceiver::Model::F9P << true << false << 0.0f << quint32{0};
    QTest::newRow("F9P-survey") << ScriptedUBXReceiver::Model::F9P << false << false << 0.0f << quint32{0};
    QTest::newRow("M8P-corrupt-version") << ScriptedUBXReceiver::Model::M8PBase << true << true << 0.0f << quint32{0};
    QTest::newRow("F9P-corrupt-version") << ScriptedUBXReceiver::Model::F9P << true << true << 0.0f << quint32{0};
    QTest::newRow("M8P-maximum-fixed-accuracy")
        << ScriptedUBXReceiver::Model::M8PBase << true << false << 429496.71875f << quint32{4294967040};
    QTest::newRow("F9P-maximum-fixed-accuracy")
        << ScriptedUBXReceiver::Model::F9P << true << false << 429496.71875f << quint32{4294967040};
}

void GPSDriverTest::_ubloxRoleTransition()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    QFETCH(bool, fixed);
    QFETCH(bool, corruptVersion);
    QFETCH(float, fixedAccuracyMeters);
    QFETCH(quint32, fixedAccuracyUnits);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    receiver.corruptVersionReplies = corruptVersion;
    {
        GPSReceiverConfig config;
        config.base = {.useFixedBase = fixed,
                       .surveyInAccMeters = 2.0,
                       .surveyInDurationSecs = 180,
                       .fixedPosition = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                       .fixedBaseAccuracyMeters = fixedAccuracyMeters};
        GPSDriver base(GPSType::ublox, receiver, config, {});
        QVERIFY(base.configure());
        QCOMPARE(receiver.timeMode, fixed ? 2u : 1u);
        if (fixed) {
            QCOMPARE(receiver.fixedAccuracy, fixedAccuracyUnits);
        }
    }
    QCOMPARE(receiver.timeMode, fixed ? 2u : 1u);
    const int previousDisables = receiver.disableCommands;
    const int previousAcks = receiver.disableAcksRead;
    const int previousReads = receiver.timeModeReads;

    GPSDriver position(GPSType::ublox, receiver, {.role = GPSReceiverConfig::Role::Position}, {});
    QVERIFY(position.configure());
    QCOMPARE(receiver.timeMode, 0u);
    QCOMPARE(receiver.disableCommands, previousDisables + 1);
    QCOMPARE(receiver.disableAcksRead, previousAcks + 1);
    QCOMPARE(receiver.timeModeReads, previousReads + 1);
    // VALSET changes just TMODE_MODE in RAM; TMODE3 sets its disabled mode and reserved fields to zero.
    QCOMPARE(receiver.lastDisablePayload,
             receiver.modern() ? QByteArray::fromHex("000100000100032000") : QByteArray(40, '\0'));
    QCOMPARE(receiver.dynamicModel, 7u);
    QCOMPARE(receiver.resetCommands, 0);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_ubloxDisableFailure_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::addColumn<ScriptedUBXReceiver::DisableReply>("reply");
    using Reply = ScriptedUBXReceiver::DisableReply;
    const std::pair<const char*, Reply> failures[] = {
        {"NAK", Reply::Nak},
        {"timeout", Reply::Timeout},
        {"wrong-ACK", Reply::WrongAck},
        {"bad-ACK", Reply::CorruptAck},
        {"write", Reply::WriteError},
        {"read", Reply::ReadError},
        {"cancel", Reply::Cancelled},
        {"ack-without-change", Reply::AckWithoutChange},
    };
    for (const auto model : {ScriptedUBXReceiver::Model::M8PBase, ScriptedUBXReceiver::Model::F9P,
                             ScriptedUBXReceiver::Model::Unidentified}) {
        for (const auto& [name, reply] : failures) {
            const QByteArray row = QByteArray::number(static_cast<int>(model)) + '-' + name;
            QTest::newRow(row.constData()) << model << reply;
        }
    }
}

void GPSDriverTest::_ubloxDisableFailure()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    QFETCH(ScriptedUBXReceiver::DisableReply, reply);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    receiver.timeMode = 2;
    receiver.disableReply = reply;
    int positions = 0;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const auto&) { ++positions; };
    GPSDriver position(GPSType::ublox, receiver, {.role = GPSReceiverConfig::Role::Position}, sinks);
    const bool readFailure = reply == ScriptedUBXReceiver::DisableReply::ReadError;
    if (readFailure) {
        expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("Receiver read failed")));
    }
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    QVERIFY(!position.configure());
    if (readFailure) {
        verifyExpectedLogMessage();
        QCOMPARE(receiver.failedReads, 1);
    }
    verifyExpectedLogMessage();
    QCOMPARE(position.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
    QCOMPARE(positions, 0);
    QCOMPARE(receiver.disableCommands, 1);
    QCOMPARE(receiver.disableAcksRead, reply == ScriptedUBXReceiver::DisableReply::AckWithoutChange ? 1 : 0);
    QCOMPARE(receiver.resetCommands, 0);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_ubloxPositionNonBase_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::newRow("M8N") << ScriptedUBXReceiver::Model::M8N;
    QTest::newRow("M9N") << ScriptedUBXReceiver::Model::M9N;
    QTest::newRow("M10") << ScriptedUBXReceiver::Model::M10;
    QTest::newRow("M8P-rover-only") << ScriptedUBXReceiver::Model::M8PRover;
    QTest::newRow("F9R-sensor-fusion") << ScriptedUBXReceiver::Model::F9R;
    QTest::newRow("u-blox6-no-extensions") << ScriptedUBXReceiver::Model::U6;
    QTest::newRow("M8N-protocol15-no-firmware-type") << ScriptedUBXReceiver::Model::M8NEarly;
}

void GPSDriverTest::_ubloxPositionNonBase()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    GPSDriver position(GPSType::ublox, receiver, {.role = GPSReceiverConfig::Role::Position}, {});
    QVERIFY(position.configure());
    QCOMPARE(receiver.disableCommands, 0);
    QCOMPARE(receiver.timeModeReads, 0);
    QCOMPARE(receiver.dynamicModel, 7u);
    QCOMPARE(receiver.resetCommands, 0);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_ubloxAmbiguousAcknowledgements_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::addColumn<bool>("sbas");
    QTest::addColumn<bool>("delayOptional");
    QTest::addColumn<bool>("alreadyMatching");
    QTest::addColumn<bool>("coalesce");
    using Model = ScriptedUBXReceiver::Model;
    QTest::newRow("time-mode-delayed-optional-ACK") << Model::F9P << false << true << false << false;
    QTest::newRow("time-mode-stale-ACK-then-NAK") << Model::F9P << false << false << false << false;
    QTest::newRow("legacy-stale-ACK-then-NAK") << Model::M8PBase << false << false << false << false;
    QTest::newRow("SBAS-delayed-optional-ACK") << Model::F9P << true << true << false << false;
    QTest::newRow("SBAS-stale-ACK-then-NAK") << Model::F9P << true << false << false << false;
    QTest::newRow("SBAS-matching-state-does-not-excuse-NAK") << Model::F9P << true << false << true << false;
    QTest::newRow("SBAS-coalesced-stale-ACK-NAK") << Model::F9P << true << false << true << true;
    QTest::newRow("time-mode-coalesced-stale-ACK-NAK") << Model::F9P << false << false << false << true;
}

void GPSDriverTest::_ubloxAmbiguousAcknowledgements()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    QFETCH(bool, sbas);
    QFETCH(bool, delayOptional);
    QFETCH(bool, alreadyMatching);
    QFETCH(bool, coalesce);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    {
        const GPSReceiverConfig config{
            .base = {.useFixedBase = true,
                     .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}};
        GPSDriver base(GPSType::ublox, receiver, config, {});
        QVERIFY(base.configure());
        QCOMPARE(receiver.timeMode, 2u);
    }
    const int previousDisables = receiver.disableCommands;
    const int previousReads = receiver.timeModeReads;
    receiver.delayOptionalAck = delayOptional;
    receiver.coalesceReplies = coalesce;
    if (sbas) {
        receiver.sbasReply = ScriptedUBXReceiver::DisableReply::Nak;
        receiver.staleSbasAck = !delayOptional;
        receiver.sbasEnabled = receiver.sbasL1caEnabled = alreadyMatching ? 1 : 0;
    } else {
        receiver.disableReply = ScriptedUBXReceiver::DisableReply::Nak;
        receiver.staleDisableAck = !delayOptional;
    }
    const GPSReceiverConfig config{.role = GPSReceiverConfig::Role::Position, .constellationMask = sbas ? 3u : 0u};
    GPSDriver position(GPSType::ublox, receiver, config, {});
    if (delayOptional) {
        expectLogMessage("GPS.Drivers", QtWarningMsg,
                         QRegularExpression(QStringLiteral("CFG-SEC-JAMDET_SENSITIVITY_HI not supported")));
    }
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    QVERIFY(!position.configure());
    if (delayOptional) {
        verifyExpectedLogMessage();
        QCOMPARE(receiver.optionalAckDelays, 1);
        QCOMPARE(receiver.disableCommands, previousDisables);
        QCOMPARE(receiver.sbasCommands, 0);
    } else {
        QCOMPARE(sbas ? receiver.sbasReads : receiver.timeModeReads - previousReads, coalesce ? 0 : 1);
    }
    verifyExpectedLogMessage();
    QCOMPARE(receiver.timeMode, 2u);
    QCOMPARE(position.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_ubloxReadbackFailure_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::addColumn<ScriptedUBXReceiver::ReadbackReply>("reply");
    using Reply = ScriptedUBXReceiver::ReadbackReply;
    const std::pair<const char*, Reply> failures[] = {
        {"NAK", Reply::Nak},
        {"timeout", Reply::Timeout},
        {"ACK-not-value", Reply::AckOnly},
        {"wrong-message", Reply::WrongMessage},
        {"wrong-value", Reply::WrongValue},
        {"wrong-version", Reply::WrongVersion},
        {"corrupt", Reply::Corrupt},
        {"truncated", Reply::Truncated},
        {"oversized", Reply::Oversized},
        {"write", Reply::WriteError},
        {"read", Reply::ReadError},
        {"cancel", Reply::Cancelled},
    };
    for (const auto model : {ScriptedUBXReceiver::Model::M8PBase, ScriptedUBXReceiver::Model::F9P}) {
        for (const auto& [name, reply] : failures) {
            const QByteArray row = QByteArray::number(static_cast<int>(model)) + '-' + name;
            QTest::newRow(row.constData()) << model << reply;
        }
    }
    QTest::newRow("VALGET-wrong-key") << ScriptedUBXReceiver::Model::F9P << Reply::WrongKey;
    QTest::newRow("VALGET-wrong-layer") << ScriptedUBXReceiver::Model::F9P << Reply::WrongLayer;
    QTest::newRow("VALGET-wrong-position") << ScriptedUBXReceiver::Model::F9P << Reply::WrongPosition;
}

void GPSDriverTest::_ubloxReadbackFailure()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    QFETCH(ScriptedUBXReceiver::ReadbackReply, reply);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    receiver.timeMode = 2;
    receiver.readbackReply = reply;
    GPSDriver position(GPSType::ublox, receiver, {.role = GPSReceiverConfig::Role::Position}, {});
    const bool readFailure = reply == ScriptedUBXReceiver::ReadbackReply::ReadError;
    if (readFailure) {
        expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("Receiver read failed")));
    }
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    QVERIFY(!position.configure());
    if (readFailure) {
        verifyExpectedLogMessage();
        QCOMPARE(receiver.failedReads, 1);
    }
    verifyExpectedLogMessage();
    QCOMPARE(position.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
    QCOMPARE(receiver.disableAcksRead, 1);
    QCOMPARE(receiver.timeModeReads, 1);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_ubloxSbasConfiguration_data()
{
    QTest::addColumn<bool>("enable");
    QTest::addColumn<ScriptedUBXReceiver::DisableReply>("settingReply");
    QTest::addColumn<ScriptedUBXReceiver::ReadbackReply>("readbackReply");
    QTest::addColumn<quint32>("faultKey");
    using Setting = ScriptedUBXReceiver::DisableReply;
    using Readback = ScriptedUBXReceiver::ReadbackReply;
    const std::pair<const char*, Setting> settings[] = {
        {"accepted", Setting::Ack},
        {"NAK", Setting::Nak},
        {"timeout", Setting::Timeout},
        {"wrong-ACK", Setting::WrongAck},
        {"corrupt-ACK", Setting::CorruptAck},
        {"write", Setting::WriteError},
        {"read", Setting::ReadError},
        {"cancel", Setting::Cancelled},
        {"unchanged", Setting::AckWithoutChange},
    };
    for (const bool enable : {false, true}) {
        for (const auto& [name, reply] : settings) {
            const QByteArray row = QByteArray(enable ? "enable-" : "disable-") + name;
            QTest::newRow(row.constData()) << enable << reply << Readback::Value << quint32(0);
        }
    }
    QTest::newRow("readback-timeout") << true << Setting::Ack << Readback::Timeout << quint32(0x10310020);
    QTest::newRow("readback-NAK") << true << Setting::Ack << Readback::Nak << quint32(0x10310020);
    QTest::newRow("L1CA-mismatch") << true << Setting::Ack << Readback::WrongValue << quint32(0x10310005);
    QTest::newRow("L1CA-timeout") << true << Setting::Ack << Readback::Timeout << quint32(0x10310005);
    QTest::newRow("L1CA-read-error") << true << Setting::Ack << Readback::ReadError << quint32(0x10310005);
    QTest::newRow("L1CA-cancel") << true << Setting::Ack << Readback::Cancelled << quint32(0x10310005);
}

void GPSDriverTest::_ubloxSbasConfiguration()
{
    QFETCH(bool, enable);
    QFETCH(ScriptedUBXReceiver::DisableReply, settingReply);
    QFETCH(ScriptedUBXReceiver::ReadbackReply, readbackReply);
    QFETCH(quint32, faultKey);
    using Setting = ScriptedUBXReceiver::DisableReply;
    using Readback = ScriptedUBXReceiver::ReadbackReply;
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(ScriptedUBXReceiver::Model::F9P, stopRequested);
    receiver.sbasEnabled = enable ? 0 : 1;
    receiver.sbasReply = settingReply;
    receiver.readbackReply = readbackReply;
    receiver.faultReadbackKey = faultKey;
    const GPSReceiverConfig config{.role = GPSReceiverConfig::Role::Position, .constellationMask = enable ? 3u : 1u};
    GPSDriver position(GPSType::ublox, receiver, config, {});
    const bool success = settingReply == Setting::Ack && readbackReply == Readback::Value;
    const bool readFailure = settingReply == Setting::ReadError || readbackReply == Readback::ReadError;
    if (!success) {
        if (readFailure) {
            expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("Receiver read failed")));
        }
        expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    }
    QCOMPARE(position.configure(), success);
    if (success) {
        QCOMPARE(receiver.sbasEnabled, enable ? 1u : 0u);
        QCOMPARE(receiver.sbasReads, enable ? 2 : 1);
        if (enable) {
            QCOMPARE(receiver.sbasL1caEnabled, 1u);
        }
    } else {
        if (readFailure) {
            verifyExpectedLogMessage();
        }
        verifyExpectedLogMessage();
        QCOMPARE(position.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
        QCOMPARE(receiver.timeModeReads, 0);
    }
    QCOMPARE(receiver.sbasCommands, 1);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_ubloxBaseRoleDefaults_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::newRow("M8P") << ScriptedUBXReceiver::Model::M8PBase;
    QTest::newRow("F9P") << ScriptedUBXReceiver::Model::F9P;
}

void GPSDriverTest::_ubloxBaseRoleDefaults()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    const GPSReceiverConfig config{.base = {.surveyInAccMeters = 2.0, .surveyInDurationSecs = 180}};
    GPSDriver base(GPSType::ublox, receiver, config, {});
    QVERIFY(base.configure());
    QCOMPARE(receiver.timeMode, 1u);
    QCOMPARE(receiver.dynamicModel, 2u);
    QCOMPARE(receiver.surveyDuration, 180u);
    QCOMPARE(receiver.surveyAccuracy, 20000u);
    QCOMPARE(receiver.resetCommands, 0);
    QVERIFY(receiver.wireValid);
}

void GPSDriverTest::_testReceiveUnconfiguredReturnsError()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    QCOMPARE(driver.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
}

void GPSDriverTest::_testInvalidFixedBaseRejected_data()
{
    QTest::addColumn<QGeoCoordinate>("coordinate");
    QTest::addColumn<double>("altitude");
    QTest::newRow("missing-coordinate") << QGeoCoordinate() << 500.0;
    QTest::newRow("invalid-latitude") << QGeoCoordinate(91, 8) << 500.0;
    QTest::newRow("invalid-longitude") << QGeoCoordinate(47, 181) << 500.0;
    QTest::newRow("missing-altitude") << QGeoCoordinate(47, 8) << qQNaN();
    QTest::newRow("infinite-altitude") << QGeoCoordinate(47, 8) << qInf();
    QTest::newRow("msl-is-not-ellipsoid-height") << QGeoCoordinate(47, 8, 500) << qQNaN();
}

void GPSDriverTest::_testInvalidFixedBaseRejected()
{
    QFETCH(QGeoCoordinate, coordinate);
    QFETCH(double, altitude);
    FakeGPSTransport transport;
    const GPSBaseStationConfig config{.useFixedBase = true,
                                      .fixedPosition = {.latitudeDegrees = coordinate.latitude(),
                                                        .longitudeDegrees = coordinate.longitude(),
                                                        .altitudeMeters = static_cast<float>(altitude)}};
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{.base = config}, GPSDriverSinks{});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Enter a valid fixed base position and accuracy")));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(transport.lastWrite.isEmpty());
    QCOMPARE(transport.lastBaudrate, 0u);
    QCOMPARE(transport.lastReadLength, -1);
    QCOMPARE(driver.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
}

void GPSDriverTest::_testInvalidConfiguration_data()
{
    QTest::addColumn<GPSBaseStationConfig>("config");
    QTest::addColumn<QString>("message");
    const QString surveyMessage = QStringLiteral("Enter a valid survey-in accuracy and duration");
    const auto survey = [&](const char* name, double accuracy, int64_t duration) {
        QTest::newRow(name) << GPSBaseStationConfig{.surveyInAccMeters = accuracy, .surveyInDurationSecs = duration}
                            << surveyMessage;
    };
    survey("zero-survey-accuracy", 0, 180);
    survey("negative-survey-accuracy", -1, 180);
    survey("nan-survey-accuracy", qQNaN(), 180);
    survey("infinite-survey-accuracy", qInf(), 180);
    survey("unrepresentable-survey-accuracy", 0.00001, 180);
    survey("overflowing-survey-accuracy", 429496.7296, 180);
    survey("zero-survey-duration", 2, 0);
    survey("negative-survey-duration", 2, -1);
    survey("overflowing-survey-duration", 2, 4294967296LL);
    const QString fixedMessage = QStringLiteral("Enter a valid fixed base position and accuracy");
    QTest::newRow("missing-fixed-position") << GPSBaseStationConfig{.useFixedBase = true} << fixedMessage;
    QTest::newRow("missing-fixed-latitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedPosition = {.longitudeDegrees = 8, .altitudeMeters = 500}}
        << fixedMessage;
    QTest::newRow("missing-fixed-longitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedPosition = {.latitudeDegrees = 47, .altitudeMeters = 500}}
        << fixedMessage;
    QTest::newRow("missing-fixed-altitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8}}
        << fixedMessage;
    const auto fixed = [&](const char* name, float altitude, float accuracy) {
        QTest::newRow(name) << GPSBaseStationConfig{.useFixedBase = true,
                                                    .fixedPosition = {.latitudeDegrees = 47,
                                                                      .longitudeDegrees = 8,
                                                                      .altitudeMeters = altitude},
                                                    .fixedBaseAccuracyMeters = accuracy}
                            << fixedMessage;
    };
    fixed("positive-altitude-overflow", (std::numeric_limits<float>::max)(), 1);
    fixed("negative-altitude-overflow", -(std::numeric_limits<float>::max)(), 1);
    fixed("negative-fixed-accuracy", 500, -1);
    fixed("nan-fixed-accuracy", 500, std::numeric_limits<float>::quiet_NaN());
    fixed("infinite-fixed-accuracy", 500, std::numeric_limits<float>::infinity());
    fixed("overflowing-fixed-accuracy", 500, (std::numeric_limits<float>::max)());
    fixed("fixed-wire-accuracy-overflow", 500, 429496.75f);
    fixed("fixed-altitude-wire-overflow", 21474838.0f, 1);
}

void GPSDriverTest::_testInvalidConfiguration()
{
    QFETCH(GPSBaseStationConfig, config);
    QFETCH(QString, message);
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{.base = config}, GPSDriverSinks{});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(message)));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(transport.lastWrite.isEmpty());
    QCOMPARE(transport.lastBaudrate, 0u);
    QCOMPARE(transport.lastReadLength, -1);
    QCOMPARE(driver.receiveOutcome(10).status, GPSReceiveStatus::NotConfigured);
}

void GPSDriverTest::_nativeConfigurationRejectedBeforeIo_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<QString>("message");
    const GPSReceiverConfig position{.role = GPSReceiverConfig::Role::Position};
    const QString unsupportedRole = QStringLiteral("This receiver does not support the requested role");
    for (const auto type : {GPSType::trimble, GPSType::septentrio, GPSType::femto}) {
        const QByteArray name = "unsupported-position-" + QByteArray::number(int(type));
        QTest::newRow(name.constData()) << int(type) << position << unsupportedRole;
    }
    QTest::newRow("unknown-receiver") << 255 << position << QStringLiteral("Unsupported GPS receiver type");
    QTest::newRow("invalid-role") << int(GPSType::ublox)
                                  << GPSReceiverConfig{.role = static_cast<GPSReceiverConfig::Role>(255)}
                                  << QStringLiteral("Unsupported GPS receiver role");
    QTest::newRow("femto-position-before-constellations")
        << int(GPSType::femto) << GPSReceiverConfig{.role = GPSReceiverConfig::Role::Position, .constellationMask = 1}
        << unsupportedRole;
    QTest::newRow("invalid-model") << int(GPSType::ublox)
                                   << GPSReceiverConfig{.role = GPSReceiverConfig::Role::Position, .dynamicModel = 1}
                                   << QStringLiteral("Unsupported receiver dynamic model");
    QTest::newRow("base-cannot-honor-model")
        << int(GPSType::ublox)
        << GPSReceiverConfig{.base = {.surveyInAccMeters = 2, .surveyInDurationSecs = 180}, .dynamicModel = 0}
        << QStringLiteral("This receiver role cannot configure a dynamic model");
    QTest::newRow("ublox-position-cannot-honor-heading")
        << int(GPSType::ublox)
        << GPSReceiverConfig{.role = GPSReceiverConfig::Role::Position, .headingOffsetRadians = 0.5f}
        << QStringLiteral("This receiver role cannot configure a heading offset");
    QTest::newRow("ublox-position-cannot-honor-zero-heading")
        << int(GPSType::ublox)
        << GPSReceiverConfig{.role = GPSReceiverConfig::Role::Position, .headingOffsetRadians = 0.0f}
        << QStringLiteral("This receiver role cannot configure a heading offset");
    QTest::newRow("septentrio-position-before-heading")
        << int(GPSType::septentrio)
        << GPSReceiverConfig{.role = GPSReceiverConfig::Role::Position, .headingOffsetRadians = qQNaN()}
        << unsupportedRole;
    QTest::newRow("unicore-survey-semantics")
        << int(GPSType::unicore) << GPSReceiverConfig{.base = {.surveyInAccMeters = 2, .surveyInDurationSecs = 180}}
        << QStringLiteral("This receiver does not support the selected base mode");
    QTest::newRow("passive-missing-baud")
        << int(GPSType::passive) << GPSReceiverConfig{.role = GPSReceiverConfig::Role::Passive}
        << QStringLiteral("Select a valid serial baud rate; passive input requires an explicit rate");
}

void GPSDriverTest::_nativeConfigurationRejectedBeforeIo()
{
    QFETCH(int, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(QString, message);
    FakeGPSTransport transport;
    GPSDriver driver(static_cast<GPSType>(type), transport, config, {});
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::NotConfigured);
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(message)));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QCOMPARE(driver.receiveOutcome(0).status, GPSReceiveStatus::NotConfigured);
    QCOMPARE(transport.openCalls, 0);
    QVERIFY(transport.lastWrite.isEmpty());
    QCOMPARE(transport.lastBaudrate, 0u);
    QCOMPARE(transport.lastReadLength, -1);
}

void GPSDriverTest::_passiveInput()
{
    FakeGPSTransport transport;
    int surveys = 0;
    std::vector<GPSPositionReport> positions;
    GPSDriverSinks sinks;
    sinks.onPosition = [&](const auto& position) { positions.push_back(position); };
    sinks.onSurveyIn = [&](const auto&) { ++surveys; };
    GPSDriver driver(GPSType::passive, transport, {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200},
                     sinks);
    QVERIFY(driver.configure());
    QVERIFY(driver.configurationEvidence().empty());
    QCOMPARE(transport.lastBaudrate, 115200u);
    QVERIFY(transport.lastWrite.isEmpty());
    transport.scriptedRead = nmeaFrame("GNGGA,123519,4807.038,N,01131.000,E,4,08,0.9,0.0,M,,M,,");
    QCOMPARE(driver.receiveOutcome(10).status, GPSReceiveStatus::Data);
    QCOMPARE(positions.size(), size_t{1});
    QCOMPARE(positions.front().fixType, GPSPositionReport::FixType::RTKFixed);
    QCOMPARE(surveys, 0);
    QVERIFY(transport.lastWrite.isEmpty());
    transport.readOverride = GPSReadResult{GPSReadStatus::Cancelled};
    QCOMPARE(driver.receiveOutcome(10).status, GPSReceiveStatus::Cancelled);
    QVERIFY(transport.lastWrite.isEmpty());
}

UT_REGISTER_TEST(GPSDriverTest, TestLabel::Unit)
