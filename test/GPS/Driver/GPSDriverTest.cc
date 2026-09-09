#include "GPSDriverTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QtMath>

#include <cstring>
#include <gps_helper.h>  // px4: GPSCallbackType, SurveyInStatus — this is a driver-bridge test
#include <limits>

#include "GPSDriver.h"
#include "GPSDriverBackend.h"
#include "GPSDriverData.h"
#include "GPSTransport.h"
#include "GPSType.h"
#include "sensor_gnss_relative.h"

namespace {

class FakeGPSTransport : public GPSTransport
{
public:
    explicit FakeGPSTransport(std::atomic_bool& requestStop)
        : GPSTransport(requestStop)
        , cancelled(requestStop)
    {
    }

    bool open() override { return true; }

    bool fatalError() const override { return false; }

    unsigned fixedBaudrate() const override { return fixedRate; }
    int read(uint8_t *buffer, int length, int timeoutMs) override
    {
        lastReadLength = length;
        lastReadTimeoutMs = timeoutMs;
        cancelled = cancelDuringRead;
        if (readError) {
            return readError;
        }
        const int n = qMin(static_cast<int>(scriptedRead.size()), length);
        (void) memcpy(buffer, scriptedRead.constData(), static_cast<size_t>(n));
        return n;
    }

    int write(const uint8_t *buffer, int length) override
    {
        lastWrite = QByteArray(reinterpret_cast<const char *>(buffer), length);
        return writeOk ? length : -1;
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
        s.onRTCM = [this](const QByteArray &message) { ++rtcmCount; rtcm = message; };
        s.onSurveyIn = [this](const GPSSurveyInStatus &status) { ++surveyInCount; surveyIn = status; };
        return s;
    }

    int rtcmCount = 0;
    QByteArray rtcm;
    int surveyInCount = 0;
    GPSSurveyInStatus surveyIn;
};

int callback(GPSDriver &driver, GPSCallbackType type, void *data1, int data2)
{
    return driver.handleCallback(static_cast<int>(type), data1, data2);
}

} // namespace

void GPSDriverTest::_testReceiveUnconfiguredReturnsError()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    QCOMPARE(driver.receive(10), -1);
    QCOMPARE(driver.receiveResult(10).status, GPSDriver::ReceiveStatus::NotConfigured);
}

void GPSDriverTest::_testReadDeviceDataRoutesToTransport()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    transport.scriptedRead = QByteArray::fromHex("b5620102");
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    uint8_t buffer[64] = {};
    const int timeoutMs = 250; // px4 packs the timeout into the first sizeof(int) bytes
    memcpy(buffer, &timeoutMs, sizeof(timeoutMs));
    const int ret = callback(driver, GPSCallbackType::readDeviceData, buffer, sizeof(buffer));

    QCOMPARE(ret, static_cast<int>(transport.scriptedRead.size()));
    QCOMPARE(transport.lastReadTimeoutMs, 250);
    QCOMPARE(transport.lastReadLength, static_cast<int>(sizeof(buffer)));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(buffer), ret), transport.scriptedRead);
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
    const int result = callback(driver, GPSCallbackType::readDeviceData, buffer, sizeof(buffer));
    QCOMPARE(result, beforeRead || duringRead ? GPSHelper::ReadCancelled : -1);
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
    const int ret = callback(driver, GPSCallbackType::writeDeviceData,
                             const_cast<char *>(payload.constData()), static_cast<int>(payload.size()));

    QCOMPARE(ret, static_cast<int>(payload.size()));
    QCOMPARE(transport.lastWrite, payload);
}

void GPSDriverTest::_testSetBaudrateRoutesToTransport()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    QCOMPARE(callback(driver, GPSCallbackType::setBaudrate, nullptr, 115200), 0);
    QCOMPARE(transport.lastBaudrate, 115200u);

    transport.baudrateOk = false;
    QCOMPARE(callback(driver, GPSCallbackType::setBaudrate, nullptr, 9600), -1);
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
    callback(driver, GPSCallbackType::gotRTCMMessage,
             const_cast<char *>(rtcm.constData()), static_cast<int>(rtcm.size()));

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

    const struct { uint8_t flags; bool valid; bool active; } cases[] = {
        { 0x00, false, false },
        { 0x01, true,  false },
        { 0x02, false, true  },
        { 0x03, true,  true  },
    };

    int expectedCount = 0;
    for (const auto &c : cases) {
        status.flags = c.flags;
        callback(driver, GPSCallbackType::surveyInStatus, &status, 0);
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
    const int ret = callback(driver, GPSCallbackType::writeDeviceData,
                             const_cast<char *>(payload.constData()), static_cast<int>(payload.size()));

    QCOMPARE(ret, -1);
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
    callback(driver, GPSCallbackType::surveyInStatus, &status, 0);

    QCOMPARE(capture.surveyInCount, 1);
    QCOMPARE(capture.surveyIn.meanAccuracyMM, 4000000001u);
    QCOMPARE(capture.surveyIn.durationSecs, 3000000001u);
}

void GPSDriverTest::_testSurveyInStatusNullDataIgnored()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    SinkCapture capture;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::surveyInStatus, nullptr, 0), 0);
    QCOMPARE(capture.surveyInCount, 0);
}

void GPSDriverTest::_testCallbacksWithoutSinksAreSafe()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray rtcm = QByteArray::fromHex("d3aabbcc");
    QCOMPARE(callback(driver, GPSCallbackType::gotRTCMMessage,
                      const_cast<char *>(rtcm.constData()), static_cast<int>(rtcm.size())), 0);

    SurveyInStatus status{};
    status.flags = 0x03;
    QCOMPARE(callback(driver, GPSCallbackType::surveyInStatus, &status, 0), 0);
}

void GPSDriverTest::_testDefaultConfigHeadingOffsetMatchesSeptentrioPreset()
{
    QCOMPARE(GPSReceiverConfig{}.headingOffsetDeg, 5.0f);
}

void GPSDriverTest::_testUnknownCallbackIgnored()
{
    std::atomic_bool stop = false;
    FakeGPSTransport transport(stop);
    SinkCapture capture;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::setClock, nullptr, 0), 0);
    QCOMPARE(capture.rtcmCount, 0);
    QCOMPARE(capture.surveyInCount, 0);
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
    QByteArray correction("RTCM");
    driver.handleCallback(int(GPSCallbackType::gotRTCMMessage), correction.data(), correction.size());
    SurveyInStatus status{};
    driver.handleCallback(int(GPSCallbackType::surveyInStatus), &status, 0);
    QCOMPARE(corrections, 0);
    QCOMPARE(surveys, 0);
}

void GPSDriverTest::_receiverRoleCommands_data()
{
    QTest::addColumn<bool>("septentrio");
    QTest::addColumn<bool>("position");
    QTest::newRow("femto-base") << false << false;
    QTest::newRow("femto-position") << false << true;
    QTest::newRow("septentrio-base") << true << false;
    QTest::newRow("septentrio-position") << true << true;
}

void GPSDriverTest::_receiverRoleCommands()
{
    QFETCH(bool, septentrio);
    QFETCH(bool, position);
    std::atomic_bool stop{false};

    class CommandTransport : public GPSTransport
    {
    public:
        CommandTransport(const std::atomic_bool& stop, bool septentrio)
            : GPSTransport(stop)
            , _septentrio(septentrio)
        {}

        bool open() override { return true; }

        bool fatalError() const override { return false; }

        bool setBaudrate(unsigned) override { return true; }

        unsigned fixedBaudrate() const override { return 115200; }

        int read(uint8_t* data, int size, int) override
        {
            const int count = qMin(size, int(_reply.size()));
            memcpy(data, _reply.constData(), count);
            _reply.remove(0, count);
            return count;
        }

        int write(const uint8_t* data, int size) override
        {
            const QByteArray command(reinterpret_cast<const char*>(data), size);
            commands.append(command);
            if (_septentrio) {
                _reply = command.trimmed().isEmpty() ? "USB1>" : "$R: " + command;
            } else {
                _reply = '<' + command.split(' ').first().trimmed() + " OK";
                _reply.append(char(0));
            }
            return size;
        }

        QList<QByteArray> commands;

    private:
        bool _septentrio;
        QByteArray _reply;
    } transport(stop, septentrio);

    GPSReceiverConfig config;
    config.role = position ? GPSReceiverConfig::Role::Position : GPSReceiverConfig::Role::RTKBase;
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
        QCOMPARE(commands.contains("setAttitudeOffset, 5.000, 0.000"), position);
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
    sensor_gps_s fix;
    GPSDriverData::initialize(fix);
    auto observation = GPSDriverData::position(fix);
    QVERIFY(!observation.position.isValid());
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Unknown);
    QVERIFY(!observation.satellitesUsed);
    QVERIFY(!observation.horizontalDop);
    QVERIFY(!observation.altitudeEllipsoidMeters);
    QVERIFY(!observation.trueHeadingDegrees);
    QVERIFY(!observation.jammingState);
    fix.heading = 0;
    observation = GPSDriverData::position(fix);
    QCOMPARE(observation.trueHeadingDegrees.value(), 0.0);
    QVERIFY(!observation.trueHeadingAccuracyDegrees);

    fix.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
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
    fix.jamming_state = sensor_gps_s::JAMMING_STATE_DETECTED;
    fix.corrections_msg_used = sensor_gps_s::CORRECTIONS_MSG_USED_USED;
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
    QCOMPARE(observation.jammingState.value(), static_cast<int>(sensor_gps_s::JAMMING_STATE_DETECTED));
    QCOMPARE(observation.correctionsUsed.value(), static_cast<int>(sensor_gps_s::CORRECTIONS_MSG_USED_USED));
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
    sensor_gnss_relative_s relative{};
    relative.timestamp = GPSObservation::monotonicNowUs();
    relative.reference_station_id = 42;
    relative.position[0] = 1.5f;
    relative.position_accuracy[2] = 0.2f;
    relative.heading = qDegreesToRadians(-90.0f);
    relative.heading_accuracy = qDegreesToRadians(1.0f);
    relative.relative_position_valid = true;
    relative.carrier_solution_fixed = true;
    relative.heading_valid = true;
    callback(driver, GPSCallbackType::gotRelativePositionMessage, nullptr, sizeof(relative));
    callback(driver, GPSCallbackType::gotRelativePositionMessage, &relative, sizeof(relative) - 1);
    QCOMPARE(received, 0);
    callback(driver, GPSCallbackType::gotRelativePositionMessage, &relative, sizeof(relative));
    QCOMPARE(received, 1);
    QCOMPARE(captured.referenceStationId, 42);
    QCOMPARE(captured.positionNedMeters[0], 1.5);
    QCOMPARE(captured.accuracyNedMeters[2], static_cast<double>(relative.position_accuracy[2]));
    QVERIFY(captured.positionValid);
    QVERIFY(captured.carrierFixed);
    QVERIFY(qAbs(captured.headingDegrees.value() - 270) < 0.001);
    relative.heading_valid = false;
    callback(driver, GPSCallbackType::gotRelativePositionMessage, &relative, sizeof(relative));
    QVERIFY(!captured.headingDegrees);
}

void GPSDriverTest::_satelliteAzimuthEncoding_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<int>("raw");
    QTest::addColumn<GPSSatellite::AzimuthEncoding>("encoding");
    QTest::addColumn<bool>("hasRaw");
    QTest::addColumn<double>("degrees");
    QTest::newRow("ubx-east") << GPSType::u_blox << 64 << GPSSatellite::AzimuthEncoding::ScaledFullCircleByte << true
                              << 64.0 * 360.0 / 255.0;
    QTest::newRow("ubx-north") << GPSType::u_blox << 255 << GPSSatellite::AzimuthEncoding::ScaledFullCircleByte << true
                               << 0.0;
    QTest::newRow("ashtech-truncated") << GPSType::trimble << 14 << GPSSatellite::AzimuthEncoding::DegreesModulo256
                                       << true << qQNaN();
    QTest::newRow("femto-truncated") << GPSType::femto << 14 << GPSSatellite::AzimuthEncoding::DegreesModulo256 << true
                                     << qQNaN();
    QTest::newRow("sbf-count-only") << GPSType::septentrio << 0 << GPSSatellite::AzimuthEncoding::Unknown << false
                                    << qQNaN();
}

void GPSDriverTest::_satelliteAzimuthEncoding()
{
    QFETCH(GPSType, type);
    QFETCH(int, raw);
    QFETCH(GPSSatellite::AzimuthEncoding, encoding);
    QFETCH(bool, hasRaw);
    QFETCH(double, degrees);
    satellite_info_s report{};
    report.timestamp = GPSObservation::monotonicNowUs() - 2000000;
    report.count = 1;
    report.azimuth[0] = static_cast<uint8_t>(raw);
    const auto observation = GPSDriverData::satellites(report, type);
    QCOMPARE(observation.monotonicTimestampUs, report.timestamp);
    QCOMPARE(observation.satellites.size(), 1);
    const auto& satellite = observation.satellites.front();
    QCOMPARE(satellite.azimuthEncoding, encoding);
    QCOMPARE(satellite.rawAzimuth.has_value(), hasRaw);
    if (hasRaw) {
        QCOMPARE(satellite.rawAzimuth.value(), raw);
    }
    QCOMPARE(satellite.azimuthDegrees().has_value(), qIsFinite(degrees));
    if (qIsFinite(degrees)) {
        QCOMPARE(satellite.azimuthDegrees().value(), degrees);
    }
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
    class PositionDriver final : public GPSHelper
    {
    public:
        PositionDriver()
            : GPSHelper(nullptr, nullptr)
        {}

        int configure(unsigned& baudrate, const GPSConfig& config) override
        {
            baudrate = 9600;
            return config.output_mode == OutputMode::GPS ? 0 : -1;
        }

        int receive(unsigned) override { return 1; }
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
