#include "GPSDriverTest.h"

#include <QtCore/QByteArray>

#include <cstring>
#include <gps_helper.h>  // px4: GPSCallbackType, SurveyInStatus — this is a driver-bridge test
#include <limits>
#include <optional>

#include "GPSDriver.h"
#include "GPSReceiverTypes.h"
#include "GPSTransport.h"

Q_DECLARE_METATYPE(GPSReceiverConfig)

namespace {

const std::atomic_bool neverStop{false};

class FakeGPSTransport : public GPSTransport
{
public:
    FakeGPSTransport() : GPSTransport(neverStop) {}

    OpenResult open() override { return {OpenStatus::Opened}; }

    bool fatalError() const override { return false; }

    ReadResult read(uint8_t* buffer, int length, int timeoutMs) override
    {
        lastReadLength = length;
        lastReadTimeoutMs = timeoutMs;
        if (readOverride) {
            return *readOverride;
        }
        const int n = qMin(static_cast<int>(scriptedRead.size()), length);
        (void) memcpy(buffer, scriptedRead.constData(), static_cast<size_t>(n));
        return {ReadStatus::Data, n};
    }

    WriteResult write(const uint8_t* buffer, int length) override
    {
        lastWrite = QByteArray(reinterpret_cast<const char*>(buffer), length);
        if (writeOverride) {
            return *writeOverride;
        }
        return writeOk ? WriteResult{WriteStatus::Completed, length, length, 0} : WriteResult{WriteStatus::Error};
    }

    bool setBaudrate(unsigned baudrate) override
    {
        lastBaudrate = baudrate;
        return baudrateOk;
    }

    std::optional<ReadResult> readOverride;
    std::optional<WriteResult> writeOverride;
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

int callback(GPSDriver& driver, GPSCallbackType type, void* data1, int data2)
{
    return driver.handleCallback(static_cast<int>(type), data1, data2);
}

}  // namespace

void GPSDriverTest::_testReceiveUnconfiguredReturnsError()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    QCOMPARE(driver.receive(10), -1);
}

void GPSDriverTest::_testReadDeviceDataRoutesToTransport()
{
    FakeGPSTransport transport;
    transport.scriptedRead = QByteArray::fromHex("b5620102");
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

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
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const int ret = callback(driver, GPSCallbackType::writeDeviceData, const_cast<char*>(payload.constData()),
                             static_cast<int>(payload.size()));

    QCOMPARE(ret, static_cast<int>(payload.size()));
    QCOMPARE(transport.lastWrite, payload);
}

void GPSDriverTest::_testSetBaudrateRoutesToTransport()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    QCOMPARE(callback(driver, GPSCallbackType::setBaudrate, nullptr, 115200), 0);
    QCOMPARE(transport.lastBaudrate, 115200u);

    transport.baudrateOk = false;
    QCOMPARE(callback(driver, GPSCallbackType::setBaudrate, nullptr, 9600), -1);
}

void GPSDriverTest::_testRtcmMessageForwardedToSink()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSReceiverType::septentrio, transport, GPSReceiverConfig{}, capture.sinks());

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
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

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

    QCOMPARE(capture.surveyIn.coordinate.latitude(), 47.1);
    QCOMPARE(capture.surveyIn.coordinate.longitude(), 8.2);
    QCOMPARE(capture.surveyIn.altitudeEllipsoidMeters, 500.0f);
    QCOMPARE(capture.surveyIn.meanAccuracyMeters.value(), 1.234);
    QCOMPARE(capture.surveyIn.duration.count(), 56u);
}

void GPSDriverTest::_testWriteDeviceDataErrorPropagates()
{
    FakeGPSTransport transport;
    transport.writeOk = false;
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const int ret = callback(driver, GPSCallbackType::writeDeviceData, const_cast<char*>(payload.constData()),
                             static_cast<int>(payload.size()));

    QCOMPARE(ret, -1);
}

void GPSDriverTest::_testSurveyInStatusPreservesLargeValues()
{
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

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
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::surveyInStatus, nullptr, 0), 0);
    QCOMPARE(capture.surveyInCount, 0);
}

void GPSDriverTest::_testSurveyInCoordinates_data()
{
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<bool>("valid");
    QTest::newRow("unknown") << qQNaN() << qQNaN() << false;
    QTest::newRow("missing-longitude") << 47.0 << qQNaN() << false;
    QTest::newRow("out-of-range") << 91.0 << 8.0 << false;
    QTest::newRow("origin") << 0.0 << 0.0 << true;
    QTest::newRow("surveying") << 47.0 << 8.0 << true;
}

void GPSDriverTest::_testSurveyInCoordinates()
{
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(bool, valid);
    FakeGPSTransport transport;
    SinkCapture capture;
    QVERIFY(!capture.surveyIn.coordinate.isValid());
    QVERIFY(qIsNaN(capture.surveyIn.altitudeEllipsoidMeters));
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, capture.sinks());
    SurveyInStatus status{};
    status.latitude = latitude;
    status.longitude = longitude;
    status.altitude = -25.0f;
    status.flags = 0x02;
    callback(driver, GPSCallbackType::surveyInStatus, &status, 0);
    QCOMPARE(capture.surveyIn.coordinate.isValid(), valid);
    QVERIFY(qIsNaN(capture.surveyIn.coordinate.altitude()));
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
    GPSReceiverConfig config;
    config.base = GPSFixedBaseConfig{.coordinate = coordinate, .altitudeEllipsoidMeters = static_cast<float>(altitude)};
    GPSDriver driver(GPSReceiverType::ublox, transport, config, GPSDriverSinks{});
    expectLogMessage("GPS.GPSDriver", QtWarningMsg,
                     QRegularExpression(QStringLiteral(
                         "Fixed base position requires valid coordinates and finite ellipsoid altitude")));
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
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

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
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, capture.sinks());

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
    for (const auto type :
         {GPSReceiverType::ublox, GPSReceiverType::septentrio, GPSReceiverType::trimble, GPSReceiverType::femto}) {
        const auto name = QByteArray::number(int(type));
        QTest::newRow((name + "-zero").constData())
            << int(type) << quint32(0) << (type == GPSReceiverType::ublox || type == GPSReceiverType::septentrio);
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
    GPSDriver driver(GPSReceiverType(type), transport, GPSReceiverConfig{}, capture.sinks());
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
    QTest::addColumn<GPSReceiverConfig>("config");
    QTest::addColumn<QString>("message");
    const QString surveyMessage = QStringLiteral("Survey-in requires representable positive accuracy and duration");
    const auto survey = [&](const char* name, double accuracy, std::chrono::seconds duration) {
        QTest::newRow(name) << GPSReceiverConfig{.base = GPSSurveyInConfig{accuracy, duration}} << surveyMessage;
    };
    survey("zero-survey-accuracy", 0, std::chrono::seconds(180));
    survey("negative-survey-accuracy", -1, std::chrono::seconds(180));
    survey("nan-survey-accuracy", qQNaN(), std::chrono::seconds(180));
    survey("infinite-survey-accuracy", qInf(), std::chrono::seconds(180));
    survey("unrepresentable-survey-accuracy", 0.00001, std::chrono::seconds(180));
    survey("overflowing-survey-accuracy", 429496.7296, std::chrono::seconds(180));
    survey("zero-survey-duration", 2, std::chrono::seconds(0));
    survey("negative-survey-duration", 2, std::chrono::seconds(-1));
    survey("overflowing-survey-duration", 2, std::chrono::seconds(4294967296LL));
    const QString fixedMessage = QStringLiteral("Fixed base altitude or accuracy exceeds driver limits");
    const auto fixed = [&](const char* name, float altitude, float accuracy) {
        QTest::newRow(name) << GPSReceiverConfig{.base = GPSFixedBaseConfig{QGeoCoordinate(47, 8), altitude, accuracy}}
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
    QFETCH(GPSReceiverConfig, config);
    QFETCH(QString, message);
    FakeGPSTransport transport;
    GPSDriver driver(GPSReceiverType::ublox, transport, config, GPSDriverSinks{});
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
    GPSDriver driver(GPSReceiverType::ublox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
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
         {GPSWriteResult{GPSWriteStatus::TimedOut, 16, 16, 0}, GPSWriteResult{GPSWriteStatus::Completed, 16, 12, 4},
          GPSWriteResult{GPSWriteStatus::Completed, 12, 12, 0}, GPSWriteResult{GPSWriteStatus::Cancelled, 0, 0, 0}}) {
        transport.writeOverride = result;
        QCOMPARE(callback(driver, GPSCallbackType::writeDeviceData, buffer, sizeof(buffer)), -1);
    }
    transport.writeOverride = GPSWriteResult{GPSWriteStatus::Completed, 16, 16, 0};
    QCOMPARE(callback(driver, GPSCallbackType::writeDeviceData, buffer, sizeof(buffer)), 16);
}
