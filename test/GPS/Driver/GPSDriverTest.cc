#include "GPSDriverTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QtMath>

#include <cstring>
#include <limits>

#include "GPSDriver.h"
#include "GPSDriverBackend.h"
#include "GPSDriverData.h"
#include "GPSProtocol.h"
#include "GPSProtocolTestIO.h"
#include "GPSRelativeReport.h"
#include "GPSTransport.h"
#include "GPSType.h"

namespace {

class FakeGPSTransport : public GPSTransport
{
public:
    explicit FakeGPSTransport(std::atomic_bool& requestStop)
        : GPSTransport(requestStop)
        , cancelled(requestStop)
    {}

    OpenResult open() override { return {OpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    unsigned fixedBaudrate() const override { return fixedRate; }

    ReadResult read(uint8_t* buffer, int length, int timeoutMs) override
    {
        lastReadLength = length;
        lastReadTimeoutMs = timeoutMs;
        cancelled = cancelDuringRead;
        if (readError) {
            return {ReadStatus::Error};
        }
        const int n = qMin(static_cast<int>(scriptedRead.size()), length);
        (void) memcpy(buffer, scriptedRead.constData(), static_cast<size_t>(n));
        return {n > 0 ? ReadStatus::Data : ReadStatus::TimedOut, n};
    }

    WriteResult write(const uint8_t* buffer, int length) override
    {
        lastWrite = QByteArray(reinterpret_cast<const char*>(buffer), length);
        if (scriptedWriteResult) {
            return *scriptedWriteResult;
        }
        return writeOk ? WriteResult{WriteStatus::Completed, length, length} : WriteResult{WriteStatus::Error};
    }

    WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer) override
    {
        return write(buffer, length);
    }

    bool setBaudrate(unsigned baudrate) override
    {
        lastBaudrate = baudrate;
        requestedBaudrates.append(baudrate);
        return baudrateOk;
    }

    QByteArray scriptedRead;
    int lastReadLength = -1;
    int lastReadTimeoutMs = -1;
    QByteArray lastWrite;
    unsigned lastBaudrate = 0;
    unsigned fixedRate = 0;
    QList<unsigned> requestedBaudrates;
    bool baudrateOk = true;
    bool writeOk = true;
    std::optional<WriteResult> scriptedWriteResult;
    std::atomic_bool& cancelled;
    bool cancelDuringRead = false;
    int readError = 0;
};

class SinkCapture
{
public:
    GPSDriverSinks sinks()
    {
        GPSDriverSinks s;
        s.onRTCM = [this](const QByteArray& message) {
            ++rtcmCount;
            rtcm = message;
        };
        s.onSurveyIn = [this](const GPSSurveyInStatus& status) {
            ++surveyInCount;
            surveyIn = status;
        };
        return s;
    }

    int rtcmCount = 0;
    QByteArray rtcm;
    int surveyInCount = 0;
    GPSSurveyInStatus surveyIn;
};

}  // namespace

void GPSDriverTest::_testReceiveUnconfiguredReturnsError()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    QCOMPARE(driver.receiveResult(10).status, GPSDriver::ReceiveStatus::NotConfigured);
    QCOMPARE(driver.receiveResult(10).status, GPSDriver::ReceiveStatus::NotConfigured);
}

void GPSDriverTest::_testReadDeviceDataRoutesToTransport()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.scriptedRead = QByteArray::fromHex("b5620102");
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    uint8_t buffer[64] = {};
    const auto io = driver._protocolIO();
    const auto result = io.read(buffer, {io.nowUs() + 250000});
    QCOMPARE(result.status, GPSReadStatus::Data);
    const int ret = result.bytesRead;

    QCOMPARE(ret, static_cast<int>(transport.scriptedRead.size()));
    QCOMPARE(transport.lastReadTimeoutMs, 250);
    QCOMPARE(transport.lastReadLength, static_cast<int>(sizeof(buffer)));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(buffer), ret), transport.scriptedRead);
}

void GPSDriverTest::_testReadCancellation_data()
{
    QTest::addColumn<bool>("beforeRead");
    QTest::addColumn<bool>("duringRead");
    QTest::newRow("ordinary-error") << false << false;
    QTest::newRow("cancel-before-read") << true << false;
    QTest::newRow("cancel-during-read") << false << true;
}

void GPSDriverTest::_testReadCancellation()
{
    QFETCH(bool, beforeRead);
    QFETCH(bool, duringRead);
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.cancelled = beforeRead;
    transport.cancelDuringRead = duringRead;
    transport.readError = -1;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    uint8_t buffer[16]{};
    const auto result = driver._protocolIO().read(buffer, {});
    QCOMPARE(result.status, beforeRead || duringRead ? GPSReadStatus::Cancelled : GPSReadStatus::Error);
    QCOMPARE(transport.lastReadLength, beforeRead ? -1 : static_cast<int>(sizeof(buffer)));
}

void GPSDriverTest::_testCancelledConfigurationDoesNotWarn()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.cancelled = true;
    transport.fixedRate = 115200;
    transport.writeOk = false;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    QVERIFY(!driver.configure());
    QCOMPARE(driver.configurationResult().status, GPSDriver::ConfigurationStatus::Cancelled);
    QVERIFY(driver.configurationResult().error.isEmpty());
}

void GPSDriverTest::_testWriteDeviceDataRoutesToTransport()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const auto result =
        driver._protocolIO().write({reinterpret_cast<const uint8_t*>(payload.constData()), size_t(payload.size())}, {});

    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QCOMPARE(result.acceptedBytes, payload.size());
    QCOMPARE(result.writtenBytes, payload.size());
    QCOMPARE(result.uncertainBytes, 0);
    QCOMPARE(transport.lastWrite, payload);
}

void GPSDriverTest::_testSetBaudrateRoutesToTransport()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    QCOMPARE(driver._protocolIO().setBaudrate(115200), GPSBaudStatus::Configured);
    QCOMPARE(transport.lastBaudrate, 115200u);

    transport.baudrateOk = false;
    QCOMPARE(driver._protocolIO().setBaudrate(9600), GPSBaudStatus::Unsupported);
}

void GPSDriverTest::_testFixedTransportBaudrate()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.fixedRate = 115200;
    transport.writeOk = false;
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    GPSDriver driver(GPSType::u_blox, transport, config, GPSDriverSinks{});
    expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    QVERIFY(!driver.configure());
    verifyExpectedLogMessage();
    // Even a failed attempt must not probe other rates and reconfigure a bridged UART.
    QCOMPARE(transport.requestedBaudrates, QList<unsigned>{115200});
}

void GPSDriverTest::_testRtcmMessageForwardedToSink()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    SinkCapture capture;
    GPSDriver driver(GPSType::septentrio, transport, GPSReceiverConfig{}, capture.sinks());

    const QByteArray rtcm = QByteArray::fromHex("d3aabbccddeeff00");
    GPSRTCMReport report;
    std::copy_n(reinterpret_cast<const uint8_t*>(rtcm.constData()), rtcm.size(), report.bytes.begin());
    report.size = rtcm.size();
    driver._protocolIO().decoded({{report}, 0});

    QCOMPARE(capture.rtcmCount, 1);
    QCOMPARE(capture.rtcm, rtcm);
}

void GPSDriverTest::_testSurveyInStatusTranslatedAndFlagsDecoded()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    SinkCapture capture;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, capture.sinks());

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
        driver._protocolIO().decoded({{status}, 0});
        ++expectedCount;
        QCOMPARE(capture.surveyInCount, expectedCount);
        QCOMPARE(capture.surveyIn.valid, c.valid);
        QCOMPARE(capture.surveyIn.active, c.active);
    }

    QCOMPARE(capture.surveyIn.latitude, 47.1);
    QCOMPARE(capture.surveyIn.longitude, 8.2);
    QCOMPARE(capture.surveyIn.altitude, 500.0f);
    QCOMPARE(capture.surveyIn.meanAccuracyMM, 1234u);
    QCOMPARE(capture.surveyIn.durationSecs, 56u);
}

void GPSDriverTest::_testWriteDeviceDataErrorPropagates()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.writeOk = false;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const auto result =
        driver._protocolIO().write({reinterpret_cast<const uint8_t*>(payload.constData()), size_t(payload.size())}, {});

    QCOMPARE(result.status, GPSWriteStatus::Error);
}

void GPSDriverTest::_testSurveyInStatusPreservesLargeValues()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    SinkCapture capture;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, capture.sinks());

    SurveyInStatus status{};
    status.flags = 0x01;
    status.mean_accuracy = 4000000001u;
    status.duration = 3000000001u;
    driver._protocolIO().decoded({{status}, 0});

    QCOMPARE(capture.surveyInCount, 1);
    QCOMPARE(capture.surveyIn.meanAccuracyMM, 4000000001u);
    QCOMPARE(capture.surveyIn.durationSecs, 3000000001u);
}

void GPSDriverTest::_testCallbacksWithoutSinksAreSafe()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray rtcm = QByteArray::fromHex("d3aabbcc");
    GPSRTCMReport report;
    std::copy_n(reinterpret_cast<const uint8_t*>(rtcm.constData()), rtcm.size(), report.bytes.begin());
    report.size = rtcm.size();
    driver._protocolIO().decoded({{report}, 0});

    SurveyInStatus status{};
    status.flags = 0x03;
    driver._protocolIO().decoded({{status}, 0});
}

void GPSDriverTest::_testDefaultConfigHeadingOffsetMatchesSeptentrioPreset()
{
    QCOMPARE(GPSReceiverConfig{}.headingOffsetDeg, 5.0f);
}

UT_REGISTER_TEST(GPSDriverTest, TestLabel::Unit)

void GPSDriverTest::_positionRoleDoesNotForwardBaseData()
{
    std::atomic_bool stop{false};
    FakeGPSTransport transport(stop);
    int corrections = 0;
    int surveys = 0;
    GPSDriverSinks sinks;
    sinks.onRTCM = [&](const QByteArray&) { ++corrections; };
    sinks.onSurveyIn = [&](const GPSSurveyInStatus&) { ++surveys; };
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    GPSDriver driver(GPSType::u_blox, transport, config, sinks);
    GPSRTCMReport correction;
    const QByteArray bytes("RTCM");
    std::copy_n(bytes.constData(), bytes.size(), correction.bytes.begin());
    correction.size = bytes.size();
    driver._protocolIO().decoded({{correction, SurveyInStatus{}}, 0});
    QCOMPARE(corrections, 0);
    QCOMPARE(surveys, 0);
}

void GPSDriverTest::_receiverRoleCommands_data()
{
    QTest::addColumn<bool>("septentrio");
    QTest::addColumn<bool>("position");
    QTest::addColumn<float>("headingOffset");
    QTest::newRow("femto-base") << false << false << 5.0f;
    QTest::newRow("femto-position") << false << true << 5.0f;
    QTest::newRow("septentrio-base") << true << false << 5.0f;
    QTest::newRow("septentrio-position") << true << true << 5.0f;
    QTest::newRow("septentrio-position-heading-offset") << true << true << 12.0f;
}

void GPSDriverTest::_receiverRoleCommands()
{
    QFETCH(bool, septentrio);
    QFETCH(bool, position);
    QFETCH(float, headingOffset);
    std::atomic_bool stop{false};

    class CommandTransport : public GPSTransport
    {
    public:
        CommandTransport(const std::atomic_bool& stop, bool septentrio)
            : GPSTransport(stop)
            , _septentrio(septentrio)
        {}

        OpenResult open() override { return {OpenStatus::Opened}; }

        bool fatalError() const override { return false; }

        bool setBaudrate(unsigned) override { return true; }

        unsigned fixedBaudrate() const override { return 115200; }

        ReadResult read(uint8_t* data, int size, int) override
        {
            const int count = qMin(size, int(_reply.size()));
            memcpy(data, _reply.constData(), count);
            _reply.remove(0, count);
            return {count > 0 ? ReadStatus::Data : ReadStatus::TimedOut, count};
        }

        WriteResult write(const uint8_t* data, int size) override
        {
            const QByteArray command(reinterpret_cast<const char*>(data), size);
            commands.append(command);
            if (_septentrio) {
                _reply = command.trimmed().isEmpty() ? "USB1>" : "$R: " + command;
            } else {
                _reply = '<' + command.split(' ').first().trimmed() + " OK";
                _reply.append(char(0));
            }
            return {WriteStatus::Completed, size, size};
        }

        WriteResult writeBounded(const uint8_t* data, int size, QDeadlineTimer deadline) override
        {
            if (deadline.hasExpired()) {
                return {WriteStatus::TimedOut};
            }
            return write(data, size);
        }

        QList<QByteArray> commands;

    private:
        bool _septentrio;
        QByteArray _reply;
    } transport(stop, septentrio);

    GPSReceiverConfig config;
    config.role = position ? GPSReceiverConfig::Role::Position : GPSReceiverConfig::Role::RTKBase;
    config.headingOffsetDeg = headingOffset;
    config.base.useFixedBase = true;
    config.base.fixedBaseLatitude = 10.0;
    config.base.fixedBaseLongitude = 20.0;
    GPSDriver driver(septentrio ? GPSType::septentrio : GPSType::femto, transport, config, {});
    QVERIFY(driver.configure());
    const QByteArray correction = QByteArray::fromHex("d30000000000");
    const auto beforeInjection = transport.commands.size();
    const auto injected = driver.injectCorrections(correction);
    const bool supportsInjection = position && septentrio;
    QCOMPARE(driver.readyForCorrections(), supportsInjection);
    QCOMPARE(injected.status,
             supportsInjection ? GPSDriver::CorrectionStatus::Submitted : GPSDriver::CorrectionStatus::Unsupported);
    QCOMPARE(injected.bytesWritten, supportsInjection ? correction.size() : 0);
    QCOMPARE(transport.commands.size(), beforeInjection + (supportsInjection ? 1 : 0));
    if (supportsInjection) {
        QCOMPARE(transport.commands.last(), correction);
    }
    QCOMPARE(driver.injectCorrections({}).status, GPSDriver::CorrectionStatus::InvalidData);
    QCOMPARE(driver.injectCorrections(QByteArray(1030, 'x')).status, GPSDriver::CorrectionStatus::InvalidData);
    const QByteArray commands = transport.commands.join("");
    if (septentrio) {
        QCOMPARE(commands.contains("setPVTMode, Rover, All, auto"), position);
        QCOMPARE(commands.contains("setPVTMode, Static"), !position);
        const QByteArray headingCommand =
            QStringLiteral("setAttitudeOffset, %1, 0.000").arg(headingOffset, 0, 'f', 3).toLatin1();
        QCOMPARE(commands.contains(headingCommand), position);
        QCOMPARE(commands.contains("setDataInOut, USB1, Auto, RTCMv3+SBF"), !position);
    } else {
        QCOMPARE(commands.contains("POSAVE OFF"), position);
        QCOMPARE(commands.contains("FIX NONE"), position);
        QCOMPARE(commands.contains("LOG UAVGPSB"), position);
        QCOMPARE(commands.contains("FIX POSITION 10.00000000 20.00000000"), !position);
    }
}

void GPSDriverTest::_unsupportedOutputProtocol()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    for (const auto type : {GPSType::u_blox, GPSType::septentrio, GPSType::trimble, GPSType::femto}) {
        GPSReceiverConfig config;
        config.role = type == GPSType::u_blox ? GPSReceiverConfig::Role::RTKBase : GPSReceiverConfig::Role::Position;
        config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
        GPSDriver driver(type, transport, config, {});
        expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Unsupported receiver output protocol")));
        QVERIFY(!driver.configure());
        verifyExpectedLogMessage();
        QCOMPARE(driver.configurationResult().status, GPSDriver::ConfigurationStatus::Unsupported);
        QVERIFY(!driver.configurationResult().error.isEmpty());
        QVERIFY(transport.lastWrite.isEmpty());
        QCOMPARE(driver.baudrate(), 0u);
    }
}

void GPSDriverTest::_observationMetadata()
{
    GPSPositionReport fix;
    auto observation = GPSDriverData::position(fix);
    QVERIFY(!observation.position.isValid());
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Unknown);
    QVERIFY(!observation.satellitesUsed);
    QVERIFY(!observation.horizontalDop);
    QVERIFY(!observation.altitudeEllipsoidMeters);
    QVERIFY(!observation.trueHeadingDegrees);
    QVERIFY(!observation.integrity.jammingState);
    fix.heading = 0;
    observation = GPSDriverData::position(fix);
    QCOMPARE(observation.trueHeadingDegrees.value(), 0.0);
    QVERIFY(!observation.trueHeadingAccuracyDegrees);

    fix.fix_type = GPSPositionReport::FIX_TYPE_RTK_FIXED;
    fix.latitude_deg = 47;
    fix.longitude_deg = 8;
    fix.altitude_msl_m = 500;
    fix.altitude_ellipsoid_m = 540;
    fix.eph = 0.1f;
    fix.epv = 0.2f;
    fix.timestamp = GPSObservation::monotonicNowUs() - 2000000;
    fix.time_utc_usec = 1000000;
    fix.satellites_used = 0;
    fix.hdop = 0.7f;
    fix.heading = qDegreesToRadians(-90.0f);
    fix.heading_accuracy = qDegreesToRadians(0.5f);
    fix.vel_ned_valid = true;
    fix.vel_m_s = 0;
    fix.cog_rad = 0;
    fix.jamming_state = GPSPositionReport::JAMMING_STATE_DETECTED;
    fix.jamming_state_timestamp = fix.timestamp - 6000000;
    fix.corrections_msg_used = GPSPositionReport::CORRECTIONS_MSG_USED_USED;
    observation = GPSDriverData::position(fix);
    QVERIFY(observation.usable());
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::RTKFixed);
    QCOMPARE(observation.altitudeDatum, GPSObservation::AltitudeDatum::MeanSeaLevel);
    QCOMPARE(observation.position.coordinate().altitude(), 500.0);
    QCOMPARE(observation.altitudeEllipsoidMeters.value(), 540.0);
    QCOMPARE(observation.satellitesUsed.value(), 0);
    QCOMPARE(observation.horizontalDop.value(), static_cast<double>(fix.hdop));
    QVERIFY(observation.ageMilliseconds() >= 2000);
    QCOMPARE(observation.position.timestamp().toMSecsSinceEpoch(), 1000);
    QVERIFY(observation.receivedAt > observation.position.timestamp());
    QVERIFY(qAbs(observation.trueHeadingDegrees.value() - 270) < 0.001);
    QVERIFY(qAbs(observation.trueHeadingAccuracyDegrees.value() - 0.5) < 0.001);
    QVERIFY(qIsNaN(observation.heading()));  // stationary course must not become antenna orientation
    QCOMPARE(observation.integrity.jammingState.value(), static_cast<int>(GPSPositionReport::JAMMING_STATE_DETECTED));
    QVERIFY(observation.integrity.provenance);
    QCOMPARE(observation.integrity.provenance->jammingTimestampUs, fix.jamming_state_timestamp);
    QCOMPARE(observation.integrity.provenance->correctionsTimestampUs, 0ULL);
    QCOMPARE(observation.integrity.correctionsUsed.value(),
             static_cast<int>(GPSPositionReport::CORRECTIONS_MSG_USED_USED));
    GPSExecutionContext context;
    context.nowUs = [] { return uint64_t{9000000}; };
    context.utcNowUs = [] { return uint64_t{1704067209000000}; };
    fix.dop_timestamp = fix.heading_timestamp = fix.accuracy_timestamp = 3000000;
    const auto stale = GPSDriverData::position(fix, context);
    QVERIFY(!stale.horizontalDop);
    QVERIFY(!stale.trueHeadingDegrees);
    QVERIFY(!stale.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QCOMPARE(stale.dopTimestampUs, quint64(3000000));
    fix.dop_timestamp = fix.heading_timestamp = fix.accuracy_timestamp = 8000000;
    const auto fresh = GPSDriverData::position(fix, context);
    QVERIFY(fresh.horizontalDop);
    QVERIFY(fresh.trueHeadingDegrees);
    QVERIFY(fresh.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    fix.dop_timestamp = fix.heading_timestamp = fix.accuracy_timestamp = 0;
    fix.eph = qQNaN();
    observation = GPSDriverData::position(fix);
    QVERIFY(observation.position.isValid());
    QVERIFY(!observation.position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QVERIFY(!observation.usable());
    QVERIFY(!observation.acceptedPosition(GPSObservation::PositionUse::GroundStation).isValid());
    QVERIFY(observation.acceptedPosition(GPSObservation::PositionUse::Diagnostics).isValid());
    QVERIFY(observation.acceptedPosition(GPSObservation::PositionUse::Gga).isValid());
    fix.eph = 0;
    observation = GPSDriverData::position(fix);
    QVERIFY(!observation.usable());
    QVERIFY(observation.position.isValid());
    fix.eph = 0.5f;
    fix.fix_type = GPSPositionReport::FIX_TYPE_NONE;
    observation = GPSDriverData::position(fix);
    QVERIFY(observation.acceptedPosition(GPSObservation::PositionUse::Diagnostics).isValid());
    QVERIFY(!observation.acceptedPosition(GPSObservation::PositionUse::Gga).isValid());
    QVERIFY(!observation.usable());
    fix.fix_type = GPSPositionReport::FIX_TYPE_EXTRAPOLATED;
    observation = GPSDriverData::position(fix);
    QVERIFY(observation.position.isValid());
    QVERIFY(!observation.usable());
}

void GPSDriverTest::_relativePositionCallback()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSRelativeObservation captured;
    int received = 0;
    GPSDriverSinks sinks;
    sinks.onRelativePosition = [&](const GPSRelativeObservation& observation) {
        captured = observation;
        ++received;
    };
    GPSDriver driver(GPSType::u_blox, transport, {}, sinks);
    GPSRelativeReport relative{};
    relative.timestamp = GPSObservation::monotonicNowUs();
    relative.reference_station_id = 42;
    relative.position[0] = 1.5f;
    relative.position_accuracy[2] = 0.2f;
    relative.heading = qDegreesToRadians(-90.0f);
    relative.heading_accuracy = qDegreesToRadians(1.0f);
    relative.relative_position_valid = true;
    relative.carrier_solution_fixed = true;
    relative.heading_valid = true;
    driver._protocolIO().decoded({{relative}, 0});
    QCOMPARE(received, 1);
    QCOMPARE(captured.referenceStationId, 42);
    QCOMPARE(captured.positionNedMeters[0], 1.5);
    QCOMPARE(captured.accuracyNedMeters[2], static_cast<double>(relative.position_accuracy[2]));
    QVERIFY(captured.positionValid);
    QVERIFY(captured.carrierFixed);
    QVERIFY(qAbs(captured.headingDegrees.value() - 270) < 0.001);
    relative.heading_valid = false;
    driver._protocolIO().decoded({{relative}, 0});
    QVERIFY(!captured.headingDegrees);
}

void GPSDriverTest::_satelliteAzimuthEncoding_data()
{
    QTest::addColumn<double>("degrees");
    QTest::newRow("north") << 0.0;
    QTest::newRow("west") << 270.0;
    QTest::newRow("full-resolution") << 359.0;
    QTest::newRow("invalid") << -1.0;
}

void GPSDriverTest::_satelliteAzimuthEncoding()
{
    QFETCH(double, degrees);
    GPSSatelliteReport report{};
    report.timestamp = GPSObservation::monotonicNowUs();
    report.count = 1;
    report.entries[0].id = 1;
    report.entries[0].elevation = -10;
    report.entries[0].azimuth = degrees;
    const auto observation = GPSDriverData::satellites(report);
    QCOMPARE(observation.monotonicTimestampUs, report.timestamp);
    QCOMPARE(observation.satellites.size(), 1);
    const auto& satellite = observation.satellites.front();
    QVERIFY(!satellite.used.has_value());
    QCOMPARE(satellite.elevationDegrees.value(), -10.0);
    QCOMPARE(satellite.azimuthDegrees().has_value(), degrees >= 0);
    if (degrees >= 0) {
        QCOMPARE(satellite.azimuthDegrees().value(), degrees);
    }
    const auto countOnly = GPSDriverData::satellites(GPSSatelliteUsageReport{report.timestamp, 12});
    QVERIFY(countOnly.satellites.isEmpty());
    QCOMPARE(countOnly.provenance.front().satellitesUsed.value(), 12);
}

void GPSDriverTest::_invalidConfiguration_data()
{
    QTest::addColumn<int>("invalidField");
    for (int field = 0; field < 8; ++field) {
        QTest::newRow(qPrintable(QString::number(field))) << field;
    }
}

void GPSDriverTest::_invalidConfiguration()
{
    QFETCH(int, invalidField);
    GPSReceiverConfig config;
    config.base.useFixedBase = true;
    switch (invalidField) {
        case 0:
            config.base.fixedBaseLatitude = 91.0;
            break;
        case 1:
            config.base.fixedBaseLongitude = -181.0;
            break;
        case 2:
            config.base.fixedBaseAltitudeMeters = std::numeric_limits<float>::infinity();
            break;
        case 3:
            config.base.fixedBaseAccuracyMeters = -1.0f;
            break;
        case 4:
            config.base.fixedBaseAccuracyMeters = (std::numeric_limits<float>::max)();
            break;
        case 5:
            config.headingOffsetDeg = std::numeric_limits<float>::quiet_NaN();
            break;
        case 6:
            config.base.useFixedBase = false;
            config.base.surveyInAccMeters = -1.0;
            config.base.surveyInDurationSecs = 60;
            break;
        case 7:
            config.base.useFixedBase = false;
            config.base.surveyInAccMeters = 1.0;
            config.base.surveyInDurationSecs = -1;
            break;
    }
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    for (const auto type : {GPSType::u_blox, GPSType::trimble, GPSType::septentrio, GPSType::femto}) {
        GPSDriver driver(type, transport, config, {});
        QVERIFY(!driver.configure());
        QCOMPARE(driver.configurationResult().status, GPSDriver::ConfigurationStatus::Unsupported);
        QVERIFY(!driver.configurationResult().error.isEmpty());
        QVERIFY(transport.requestedBaudrates.isEmpty());
        QVERIFY(transport.lastWrite.isEmpty());
        QCOMPARE(transport.lastReadLength, -1);
    }
}

void GPSDriverTest::_familyCancellation_data()
{
    QTest::addColumn<int>("family");
    QTest::addColumn<bool>("cancelled");
    for (int family = 0; family != 4; ++family) {
        QTest::newRow(qPrintable(QStringLiteral("cancel-%1").arg(family))) << family << true;
        QTest::newRow(qPrintable(QStringLiteral("error-%1").arg(family))) << family << false;
    }
}

void GPSDriverTest::_familyCancellation()
{
    QFETCH(int, family);
    QFETCH(bool, cancelled);
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.fixedRate = 115200;
    transport.readError = -EIO;
    transport.cancelDuringRead = cancelled;
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    GPSDriver driver(static_cast<GPSType>(family), transport, config, {});
    if (!cancelled) {
        expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Driver configuration failed for type")));
        if (family == 2) {
            ignoreLogMessage("GPS.Driver.Drivers", QtWarningMsg, QRegularExpression(QStringLiteral("sbf read err")));
        } else if (family == 0) {
            ignoreLogMessage("GPS.Driver.Drivers", QtWarningMsg,
                             QRegularExpression(QStringLiteral("ubx poll_or_read err")));
        }
    }
    QVERIFY(!driver.configure());
    QCOMPARE(driver.configurationResult().status,
             cancelled ? GPSDriver::ConfigurationStatus::Cancelled : GPSDriver::ConfigurationStatus::TransportError);
    if (!cancelled) {
        verifyExpectedLogMessage();
    }
}

void GPSDriverTest::_positionBackendWithoutBaseSupport()
{
    class PositionDriver final : public GPSProtocol
    {
    public:
        PositionDriver()
            : GPSProtocol(GPSProtocolIO{})
        {}

        int configure(unsigned& baudrate, const GPSConfig& config) override
        {
            baudrate = 9600;
            return config.output_mode == OutputMode::GPS ? 0 : -1;
        }

        int receive(unsigned) override { return 1; }

        int consume(std::span<const uint8_t>) override { return 0; }
    };

    class PositionBackend final : public GPSDriverBackend
    {
    public:
        PositionBackend() { setDriver(std::make_unique<PositionDriver>()); }
    } backend;

    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    unsigned baudrate = 0;
    QCOMPARE(backend.configure(baudrate, config), 0);
    QCOMPARE(baudrate, 9600u);
    QCOMPARE(backend.receive(0), 1);
    config.role = GPSReceiverConfig::Role::RTKBase;
    config.base.useFixedBase = true;
    QVERIFY(backend.configure(baudrate, config) < 0);
}

void GPSDriverTest::_configurationWriteEvidence_data()
{
    QTest::addColumn<bool>("cancelled");
    QTest::newRow("partial-timeout") << false;
    QTest::newRow("partial-cancel") << true;
}

void GPSDriverTest::_configurationWriteEvidence()
{
    QFETCH(bool, cancelled);
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.fixedRate = 115200;
    transport.scriptedWriteResult = GPSTransport::WriteResult{
        cancelled ? GPSTransport::WriteStatus::Cancelled : GPSTransport::WriteStatus::TimedOut, 4, 1, 3};
    GPSReceiverConfig config;
    config.role = GPSReceiverConfig::Role::Position;
    GPSDriver driver(GPSType::u_blox, transport, config, {});
    if (!cancelled) {
        expectLogMessage("GPS.Driver.GPSDriver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Driver configuration failed for type")));
    }
    QVERIFY(!driver.configure());
    const auto& result = driver.configurationResult();
    QCOMPARE(result.status,
             cancelled ? GPSDriver::ConfigurationStatus::Cancelled : GPSDriver::ConfigurationStatus::TransportError);
    QVERIFY(result.transportWrite.has_value());
    QCOMPARE(result.transportWrite->status, transport.scriptedWriteResult->status);
    QCOMPARE(result.transportWrite->acceptedBytes, 4);
    QCOMPARE(result.transportWrite->writtenBytes, 1);
    QCOMPARE(result.transportWrite->uncertainBytes, 3);
    if (!cancelled) {
        QVERIFY(result.error.contains(QStringLiteral("timed out")));
        verifyExpectedLogMessage();
    }
}
