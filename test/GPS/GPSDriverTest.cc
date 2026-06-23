#include "GPSDriverTest.h"

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "GPSType.h"

#include <gps_helper.h> // px4: GPSCallbackType, SurveyInStatus — this is a driver-bridge test

#include <QtCore/QByteArray>

#include <cstring>

namespace {

class FakeGPSTransport : public GPSTransport
{
public:
    int read(uint8_t *buffer, int length, int timeoutMs) override
    {
        lastReadLength = length;
        lastReadTimeoutMs = timeoutMs;
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
        return baudrateOk;
    }

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
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});
    QCOMPARE(driver.receive(10), -1);
}

void GPSDriverTest::_testReadDeviceDataRoutesToTransport()
{
    FakeGPSTransport transport;
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

void GPSDriverTest::_testWriteDeviceDataRoutesToTransport()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const int ret = callback(driver, GPSCallbackType::writeDeviceData,
                             const_cast<char *>(payload.constData()), static_cast<int>(payload.size()));

    QCOMPARE(ret, static_cast<int>(payload.size()));
    QCOMPARE(transport.lastWrite, payload);
}

void GPSDriverTest::_testSetBaudrateRoutesToTransport()
{
    FakeGPSTransport transport;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

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
    callback(driver, GPSCallbackType::gotRTCMMessage,
             const_cast<char *>(rtcm.constData()), static_cast<int>(rtcm.size()));

    QCOMPARE(capture.rtcmCount, 1);
    QCOMPARE(capture.rtcm, rtcm);
}

void GPSDriverTest::_testSurveyInStatusTranslatedAndFlagsDecoded()
{
    FakeGPSTransport transport;
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
    FakeGPSTransport transport;
    transport.writeOk = false;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, GPSDriverSinks{});

    const QByteArray payload = QByteArray::fromHex("deadbeef");
    const int ret = callback(driver, GPSCallbackType::writeDeviceData,
                             const_cast<char *>(payload.constData()), static_cast<int>(payload.size()));

    QCOMPARE(ret, -1);
}

void GPSDriverTest::_testSurveyInStatusPreservesLargeValues()
{
    FakeGPSTransport transport;
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
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::surveyInStatus, nullptr, 0), 0);
    QCOMPARE(capture.surveyInCount, 0);
}

void GPSDriverTest::_testCallbacksWithoutSinksAreSafe()
{
    FakeGPSTransport transport;
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
    FakeGPSTransport transport;
    SinkCapture capture;
    GPSDriver driver(GPSType::u_blox, transport, GPSReceiverConfig{}, capture.sinks());

    QCOMPARE(callback(driver, GPSCallbackType::setClock, nullptr, 0), 0);
    QCOMPARE(capture.rtcmCount, 0);
    QCOMPARE(capture.surveyInCount, 0);
}

UT_REGISTER_TEST(GPSDriverTest, TestLabel::Unit)
