#include "GPSDriverTest.h"

#include <cstring>
#include <gps_helper.h>  // px4: GPSCallbackType, SurveyInStatus — this is a driver-bridge test
#include <limits>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtPositioning/QGeoCoordinate>

#include "GPSBaseStationConfig.h"
#include "GPSDriver.h"
#include "GPSTransport.h"
#include "ScriptedUBXReceiver.h"

Q_DECLARE_METATYPE(GPSBaseStationConfig)
Q_DECLARE_METATYPE(GPSReceiverConfig)
Q_DECLARE_METATYPE(ScriptedUBXReceiver::Model)
Q_DECLARE_METATYPE(ScriptedUBXReceiver::DisableReply)
Q_DECLARE_METATYPE(ScriptedUBXReceiver::ReadbackReply)

namespace {

static_assert(static_cast<int>(GPSType::ublox) == 0);
static_assert(static_cast<int>(GPSType::trimble) == 1);
static_assert(static_cast<int>(GPSType::septentrio) == 2);
static_assert(static_cast<int>(GPSType::femto) == 3);

const std::atomic_bool neverStop{false};

class FakeGPSTransport : public GPSTransport
{
public:
    FakeGPSTransport()
        : GPSTransport(neverStop)
    {}

    GPSOpenResult open() override { return {GPSOpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override
    {
        lastReadLength = length;
        lastReadTimeoutMs = timeoutMs;
        if (readOverride) {
            return *readOverride;
        }
        const int n = qMin(static_cast<int>(scriptedRead.size()), length);
        (void) memcpy(buffer, scriptedRead.constData(), static_cast<size_t>(n));
        return {GPSReadStatus::Data, n};
    }

    GPSWriteResult write(const uint8_t* buffer, int length) override
    {
        lastWrite = QByteArray(reinterpret_cast<const char*>(buffer), length);
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
    int lastReadLength = -1;
    int lastReadTimeoutMs = -1;
    QByteArray lastWrite;
    unsigned lastBaudrate = 0;
    bool baudrateOk = true;
    bool writeOk = true;
};

class SinkCapture
{
public:
    GPSDriverSinks sinks()
    {
        GPSDriverSinks s;
        s.onRTCM = [this](std::span<const uint8_t> message) {
            ++rtcmCount;
            rtcm = QByteArray(reinterpret_cast<const char*>(message.data()), static_cast<qsizetype>(message.size()));
        };
        s.onSurveyIn = [this](const GPSSurveyReport& status) {
            ++surveyInCount;
            surveyIn = status;
        };
        return s;
    }

    int rtcmCount = 0;
    QByteArray rtcm;
    int surveyInCount = 0;
    GPSSurveyReport surveyIn;
};

int callback(GPSDriver& driver, GPSCallbackType type, void* data1, int data2)
{
    return driver.handleCallback(static_cast<int>(type), data1, data2);
}

}  // namespace

void GPSDriverTest::_ubloxRoleTransition_data()
{
    QTest::addColumn<ScriptedUBXReceiver::Model>("model");
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("corruptVersion");
    QTest::newRow("M8P-fixed") << ScriptedUBXReceiver::Model::M8PBase << true << false;
    QTest::newRow("M8P-survey") << ScriptedUBXReceiver::Model::M8PBase << false << false;
    QTest::newRow("F9P-fixed") << ScriptedUBXReceiver::Model::F9P << true << false;
    QTest::newRow("F9P-survey") << ScriptedUBXReceiver::Model::F9P << false << false;
    QTest::newRow("M8P-corrupt-version") << ScriptedUBXReceiver::Model::M8PBase << true << true;
    QTest::newRow("F9P-corrupt-version") << ScriptedUBXReceiver::Model::F9P << true << true;
}

void GPSDriverTest::_ubloxRoleTransition()
{
    QFETCH(ScriptedUBXReceiver::Model, model);
    QFETCH(bool, fixed);
    QFETCH(bool, corruptVersion);
    std::atomic_bool stopRequested{false};
    ScriptedUBXReceiver receiver(model, stopRequested);
    receiver.corruptVersionReplies = corruptVersion;
    {
        GPSReceiverConfig config;
        config.base = {.useFixedBase = fixed,
                       .surveyInAccMeters = 2.0,
                       .surveyInDurationSecs = 180,
                       .fixedBaseLatitude = 47.0,
                       .fixedBaseLongitude = 8.0,
                       .fixedBaseAltitudeMeters = 500.0f};
        GPSDriver base(GPSType::ublox, receiver, config, {});
        QVERIFY(base.configure());
        QCOMPARE(receiver.timeMode, fixed ? 2u : 1u);
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
    const bool readFailure =
        reply == ScriptedUBXReceiver::DisableReply::ReadError || reply == ScriptedUBXReceiver::DisableReply::Cancelled;
    if (readFailure) {
        expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("ubx poll_or_read err")));
    }
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    QVERIFY(!position.configure());
    if (readFailure) {
        verifyExpectedLogMessage();
        QCOMPARE(receiver.failedReads, 1);
    }
    verifyExpectedLogMessage();
    QCOMPARE(position.receive(10), -1);
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
        const GPSReceiverConfig config{.base = {.useFixedBase = true,
                                                .fixedBaseLatitude = 47,
                                                .fixedBaseLongitude = 8,
                                                .fixedBaseAltitudeMeters = 500}};
        GPSDriver base(GPSType::ublox, receiver, config, {});
        QVERIFY(base.configure());
        QCOMPARE(receiver.timeMode, 2u);
    }
    const int previousDisables = receiver.disableCommands;
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
        QCOMPARE(sbas ? receiver.sbasReads : receiver.timeModeReads, coalesce ? 0 : 1);
    }
    verifyExpectedLogMessage();
    QCOMPARE(receiver.timeMode, 2u);
    QCOMPARE(position.receive(10), -1);
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
    const bool readFailure = reply == ScriptedUBXReceiver::ReadbackReply::ReadError ||
                             reply == ScriptedUBXReceiver::ReadbackReply::Cancelled;
    if (readFailure) {
        expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("ubx poll_or_read err")));
    }
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    QVERIFY(!position.configure());
    if (readFailure) {
        verifyExpectedLogMessage();
        QCOMPARE(receiver.failedReads, 1);
    }
    verifyExpectedLogMessage();
    QCOMPARE(position.receive(10), -1);
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
    const bool readFailure = settingReply == Setting::ReadError || settingReply == Setting::Cancelled ||
                             readbackReply == Readback::ReadError || readbackReply == Readback::Cancelled;
    if (!success) {
        if (readFailure) {
            expectLogMessage("GPS.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("ubx poll_or_read err")));
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
        QCOMPARE(position.receive(10), -1);
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
    QCOMPARE(driver.receive(10), -1);
}

void GPSDriverTest::_testReadDeviceDataRoutesToTransport()
{
    FakeGPSTransport transport;
    transport.scriptedRead = QByteArray::fromHex("b5620102");
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    uint8_t buffer[64] = {};
    const int timeoutMs = 250;  // px4 packs the timeout into the first sizeof(int) bytes
    memcpy(buffer, &timeoutMs, sizeof(timeoutMs));
    const int ret = callback(driver, GPSCallbackType::readDeviceData, buffer, sizeof(buffer));

    QCOMPARE(ret, static_cast<int>(transport.scriptedRead.size()));
    QCOMPARE(transport.lastReadTimeoutMs, 250);
    QCOMPARE(transport.lastReadLength, static_cast<int>(sizeof(buffer)));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(buffer), ret), transport.scriptedRead);
}

void GPSDriverTest::_testWriteDeviceDataRoutesToTransport()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const int ret = callback(driver, GPSCallbackType::writeDeviceData, const_cast<char*>(payload.constData()),
                             static_cast<int>(payload.size()));

    QCOMPARE(ret, static_cast<int>(payload.size()));
    QCOMPARE(transport.lastWrite, payload);
}

void GPSDriverTest::_testSetBaudrateRoutesToTransport()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    QCOMPARE(callback(driver, GPSCallbackType::setBaudrate, nullptr, 115200), 0);
    QCOMPARE(transport.lastBaudrate, 115200u);

    transport.baudrateOk = false;
    QCOMPARE(callback(driver, GPSCallbackType::setBaudrate, nullptr, 9600), -1);
}

void GPSDriverTest::_testRtcmMessageForwardedToSink()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::septentrio, transport, GPSReceiverConfig{}, capture.sinks());

    const QByteArray rtcm = QByteArray::fromHex("d3aabbccddeeff00");
    callback(driver, GPSCallbackType::gotRTCMMessage, const_cast<char*>(rtcm.constData()),
             static_cast<int>(rtcm.size()));

    QCOMPARE(capture.rtcmCount, 1);
    QCOMPARE(capture.rtcm, rtcm);
}

void GPSDriverTest::_testSurveyInStatusTranslatedAndFlagsDecoded()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

    SurveyInStatus status{};
    status.latitude = 47.1;
    status.longitude = 8.2;
    status.altitude = 500.0f;
    status.mean_accuracy = 1234;
    status.duration = 56;

    const struct
    {
        uint8_t flags;
        bool valid;
        bool active;
    } cases[] = {
        {0x00, false, false},
        {0x01, true, false},
        {0x02, false, true},
        {0x03, true, true},
    };

    int expectedCount = 0;
    for (const auto& c : cases) {
        status.flags = c.flags;
        callback(driver, GPSCallbackType::surveyInStatus, &status, 0);
        ++expectedCount;
        QCOMPARE(capture.surveyInCount, expectedCount);
        QCOMPARE(capture.surveyIn.valid, c.valid);
        QCOMPARE(capture.surveyIn.active, c.active);
    }

    QCOMPARE(capture.surveyIn.latitudeDegrees, 47.1);
    QCOMPARE(capture.surveyIn.longitudeDegrees, 8.2);
    QCOMPARE(capture.surveyIn.altitudeEllipsoidMeters, 500.0f);
    QCOMPARE(capture.surveyIn.meanAccuracyMeters.value(), 1.234);
    QCOMPARE(capture.surveyIn.duration.count(), 56u);
}

void GPSDriverTest::_testWriteDeviceDataErrorPropagates()
{
    FakeGPSTransport transport;
    transport.writeOk = false;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const int ret = callback(driver, GPSCallbackType::writeDeviceData, const_cast<char*>(payload.constData()),
                             static_cast<int>(payload.size()));

    QCOMPARE(ret, -1);
}

void GPSDriverTest::_testSurveyInStatusPreservesLargeValues()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

    SurveyInStatus status{};
    status.flags = 0x01;
    status.mean_accuracy = 4000000001u;
    status.duration = 3000000001u;
    callback(driver, GPSCallbackType::surveyInStatus, &status, 0);

    QCOMPARE(capture.surveyInCount, 1);
    QCOMPARE(capture.surveyIn.meanAccuracyMeters.value(), 4000000.001);
    QCOMPARE(capture.surveyIn.duration.count(), 3000000001u);
}

void GPSDriverTest::_testSurveyInStatusNullDataIgnored()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::surveyInStatus, nullptr, 0), 0);
    QCOMPARE(capture.surveyInCount, 0);
}

void GPSDriverTest::_testSurveyInCoordinates_data()
{
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::newRow("unknown") << qQNaN() << qQNaN();
    QTest::newRow("missing-longitude") << 47.0 << qQNaN();
    QTest::newRow("out-of-range") << 91.0 << 8.0;
    QTest::newRow("origin") << 0.0 << 0.0;
    QTest::newRow("surveying") << 47.0 << 8.0;
}

void GPSDriverTest::_testSurveyInCoordinates()
{
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    FakeGPSTransport transport;
    SinkCapture capture;
    QVERIFY(qIsNaN(capture.surveyIn.latitudeDegrees));
    QVERIFY(qIsNaN(capture.surveyIn.longitudeDegrees));
    QVERIFY(qIsNaN(capture.surveyIn.altitudeEllipsoidMeters));
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, capture.sinks());
    SurveyInStatus status{};
    status.latitude = latitude;
    status.longitude = longitude;
    status.altitude = -25.0f;
    status.flags = 0x02;
    callback(driver, GPSCallbackType::surveyInStatus, &status, 0);
    if (qIsNaN(latitude)) {
        QVERIFY(qIsNaN(capture.surveyIn.latitudeDegrees));
    } else {
        QCOMPARE(capture.surveyIn.latitudeDegrees, latitude);
    }
    if (qIsNaN(longitude)) {
        QVERIFY(qIsNaN(capture.surveyIn.longitudeDegrees));
    } else {
        QCOMPARE(capture.surveyIn.longitudeDegrees, longitude);
    }
    QCOMPARE(capture.surveyIn.altitudeEllipsoidMeters, -25.0f);
    QVERIFY(capture.surveyIn.active);
    QVERIFY(!capture.surveyIn.valid);

    status.altitude = qQNaN();
    callback(driver, GPSCallbackType::surveyInStatus, &status, 0);
    QVERIFY(qIsNaN(capture.surveyIn.altitudeEllipsoidMeters));
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
                                      .fixedBaseLatitude = coordinate.latitude(),
                                      .fixedBaseLongitude = coordinate.longitude(),
                                      .fixedBaseAltitudeMeters = static_cast<float>(altitude)};
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{.base = config}, GPSDriverSinks{});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Enter a valid fixed base position and accuracy")));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(transport.lastWrite.isEmpty());
    QCOMPARE(transport.lastBaudrate, 0u);
    QCOMPARE(transport.lastReadLength, -1);
    QCOMPARE(driver.receive(10), -1);
}

void GPSDriverTest::_testCallbacksWithoutSinksAreSafe()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray rtcm = QByteArray::fromHex("d3aabbcc");
    QCOMPARE(callback(driver, GPSCallbackType::gotRTCMMessage, const_cast<char*>(rtcm.constData()),
                      static_cast<int>(rtcm.size())),
             0);

    SurveyInStatus status{};
    status.flags = 0x03;
    QCOMPARE(callback(driver, GPSCallbackType::surveyInStatus, &status, 0), 0);
}

void GPSDriverTest::_testUnknownCallbackIgnored()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::setClock, nullptr, 0), 0);
    QCOMPARE(capture.rtcmCount, 0);
    QCOMPARE(capture.surveyInCount, 0);
}

UT_REGISTER_TEST(GPSDriverTest, TestLabel::Unit)

void GPSDriverTest::_testSurveyInAccuracy_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<quint32>("accuracy");
    QTest::addColumn<bool>("known");
    for (const auto type : {GPSType::ublox, GPSType::septentrio, GPSType::trimble, GPSType::femto}) {
        const auto name = QByteArray::number(int(type));
        QTest::newRow((name + "-zero").constData())
            << int(type) << quint32(0) << (type == GPSType::ublox || type == GPSType::septentrio);
        QTest::newRow((name + "-nonzero").constData()) << int(type) << quint32(42) << true;
    }
}

void GPSDriverTest::_testSurveyInAccuracy()
{
    QFETCH(int, type);
    QFETCH(quint32, accuracy);
    QFETCH(bool, known);
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType(type), transport, GPSReceiverConfig{}, capture.sinks());
    SurveyInStatus status{};
    status.mean_accuracy = accuracy;
    callback(driver, GPSCallbackType::surveyInStatus, &status, 0);
    QCOMPARE(capture.surveyInCount, 1);
    QCOMPARE(capture.surveyIn.meanAccuracyMeters.has_value(), known);
    if (known) {
        QCOMPARE(*capture.surveyIn.meanAccuracyMeters, static_cast<double>(accuracy) / 1000.0);
    }
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
        << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLongitude = 8, .fixedBaseAltitudeMeters = 500}
        << fixedMessage;
    QTest::newRow("missing-fixed-longitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLatitude = 47, .fixedBaseAltitudeMeters = 500}
        << fixedMessage;
    QTest::newRow("missing-fixed-altitude")
        << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLatitude = 47, .fixedBaseLongitude = 8} << fixedMessage;
    const auto fixed = [&](const char* name, float altitude, float accuracy) {
        QTest::newRow(name) << GPSBaseStationConfig{.useFixedBase = true,
                                                    .fixedBaseLatitude = 47,
                                                    .fixedBaseLongitude = 8,
                                                    .fixedBaseAltitudeMeters = altitude,
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
    QCOMPARE(driver.receive(10), -1);
}

void GPSDriverTest::_transportResultsMapToLegacyCallbacks()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    uint8_t buffer[16]{};
    for (const auto status : {GPSReadStatus::Cancelled, GPSReadStatus::Closed, GPSReadStatus::Error,
                              GPSReadStatus::Overflow, GPSReadStatus::InvalidData}) {
        transport.readOverride = GPSReadResult{status, 4};
        QCOMPARE(callback(driver, GPSCallbackType::readDeviceData, buffer, sizeof(buffer)), -1);
    }
    transport.readOverride = GPSReadResult{GPSReadStatus::TimedOut, 4};
    QCOMPARE(callback(driver, GPSCallbackType::readDeviceData, buffer, sizeof(buffer)), 0);
    transport.readOverride = GPSReadResult{GPSReadStatus::Data, 17};
    QCOMPARE(callback(driver, GPSCallbackType::readDeviceData, buffer, sizeof(buffer)), -1);

    for (const auto& result :
         {GPSWriteResult{GPSWriteStatus::TimedOut, 16, 16}, GPSWriteResult{GPSWriteStatus::Completed, 16, 12},
          GPSWriteResult{GPSWriteStatus::Completed, 12, 12}, GPSWriteResult{GPSWriteStatus::Cancelled, 0, 0},
          GPSWriteResult{GPSWriteStatus::Completed, -1, 16}, GPSWriteResult{GPSWriteStatus::Completed, 16, -1},
          GPSWriteResult{GPSWriteStatus::Completed, 16, 17}, GPSWriteResult{GPSWriteStatus::Completed, 17, 16},
          GPSWriteResult{GPSWriteStatus::Error, 16, 16}, GPSWriteResult{GPSWriteStatus::Unsupported, 16, 16},
          GPSWriteResult{GPSWriteStatus::InvalidData, 16, 16}}) {
        transport.writeOverride = result;
        QCOMPARE(callback(driver, GPSCallbackType::writeDeviceData, buffer, sizeof(buffer)), -1);
    }
    transport.writeOverride = GPSWriteResult{GPSWriteStatus::Completed, 16, 16};
    QCOMPARE(callback(driver, GPSCallbackType::writeDeviceData, buffer, sizeof(buffer)), 16);
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
}

void GPSDriverTest::_nativeConfigurationRejectedBeforeIo()
{
    QFETCH(int, type);
    QFETCH(GPSReceiverConfig, config);
    QFETCH(QString, message);
    FakeGPSTransport transport;
    GPSDriver driver(static_cast<GPSType>(type), transport, config, {});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg, QRegularExpression(QRegularExpression::escape(message)));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    QVERIFY(transport.lastWrite.isEmpty());
    QCOMPARE(transport.lastBaudrate, 0u);
    QCOMPARE(transport.lastReadLength, -1);
}

void GPSDriverTest::_invalidRtcmPayload_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("nullData");
    QTest::newRow("missing-data") << 1 << true;
    QTest::newRow("empty-data") << 0 << false;
    QTest::newRow("negative-size") << -1 << false;
}

void GPSDriverTest::_invalidRtcmPayload()
{
    QFETCH(int, size);
    QFETCH(bool, nullData);
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::ublox, transport, {}, capture.sinks());
    uint8_t byte = 0;
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM callback payload")));
    QCOMPARE(callback(driver, GPSCallbackType::gotRTCMMessage, nullData ? nullptr : &byte, size), -1);
    verifyExpectedLogMessage();
    QCOMPARE(capture.rtcmCount, 0);
}
