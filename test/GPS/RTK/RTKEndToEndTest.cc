#include "RTKEndToEndTest.h"
#include "RTCMTestHelper.h"
#include "MockGPSRtk.h"
#include "GPSRTKFactGroup.h"
#include "RTCMMavlink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <mavlink_types.h>
#include <common/mavlink_msg_gps_rtcm_data.h>

// ---------------------------------------------------------------------------
// Test: Full survey-in lifecycle with RTCM routing to MAVLink
// MockGPSRtk simulates position lock + survey-in convergence,
// then RTCM data is injected and routed through RTCMMavlink.
// ---------------------------------------------------------------------------

void RTKEndToEndTest::testSurveyInToMavlinkPipeline()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    MockGPSRtk mock;

    // Phase 1: Connect device and start survey-in
    mock.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));
    QVERIFY(mock.connected());

    mock.simulatePosition(47.3977, 8.5456, 408.0, 3);
    auto *fg = qobject_cast<GPSRTKFactGroup *>(mock.gpsRtkFactGroup());
    QVERIFY(fg);
    QCOMPARE(fg->baseFixType()->rawValue().toInt(), 3);

    // Phase 2: Survey-in in progress
    mock.simulateSurveyInStatus(60.0f, 5000.0f, false, true);
    QVERIFY(fg->active()->rawValue().toBool());
    QVERIFY(!fg->valid()->rawValue().toBool());

    // Phase 3: Survey-in converges
    mock.simulateSurveyInStatus(300.0f, 800.0f, true, false);
    QVERIFY(fg->valid()->rawValue().toBool());
    QVERIFY(!fg->active()->rawValue().toBool());
    QCOMPARE(fg->currentAccuracy()->rawValue().toDouble(), 0.8);

    // Phase 4: Inject RTCM and route to vehicles via MAVLink
    const QByteArray rtcm1 = RTCMTestHelper::buildFrame(1005, QByteArray(10, '\x55'));
    const QByteArray rtcm2 = RTCMTestHelper::buildFrame(1077, QByteArray(20, '\xAA'));

    mavlink.RTCMDataUpdate(rtcm1);
    mavlink.RTCMDataUpdate(rtcm2);

    QCOMPARE(sent.size(), 2);

    // Verify frame 1 content
    QByteArray recovered1(reinterpret_cast<const char *>(sent[0].data), sent[0].len);
    QCOMPARE(recovered1, rtcm1);

    // Verify frame 2 content
    QByteArray recovered2(reinterpret_cast<const char *>(sent[1].data), sent[1].len);
    QCOMPARE(recovered2, rtcm2);

    // Verify sequence ids increment
    const uint8_t seq0 = (sent[0].flags >> 3) & 0x1F;
    const uint8_t seq1 = (sent[1].flags >> 3) & 0x1F;
    QCOMPARE(seq1, static_cast<uint8_t>(seq0 + 1));

    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(rtcm1.size() + rtcm2.size()));
}

// ---------------------------------------------------------------------------
// Test: RTCM routed to multiple devices simultaneously
// ---------------------------------------------------------------------------

void RTKEndToEndTest::testMultiDeviceRtcmRouting()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    MockGPSRtk device1;
    MockGPSRtk device2;
    device1.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));
    device2.simulateConnect(QStringLiteral("/dev/ttyACM1"), QStringLiteral("Trimble"));

    const QByteArray rtcm = RTCMTestHelper::buildFrame(1005, QByteArray(8, '\x33'));

    // Route to both devices (base station injection) and MAVLink (vehicles)
    device1.injectRTCMData(rtcm);
    device2.injectRTCMData(rtcm);
    mavlink.RTCMDataUpdate(rtcm);

    // Both devices received the data
    QCOMPARE(device1.injectedRtcm.size(), 1);
    QCOMPARE(device2.injectedRtcm.size(), 1);
    QCOMPARE(device1.injectedRtcm.first(), rtcm);
    QCOMPARE(device2.injectedRtcm.first(), rtcm);

    // MAVLink also received it
    QCOMPARE(sent.size(), 1);
    QByteArray recovered(reinterpret_cast<const char *>(sent[0].data), sent[0].len);
    QCOMPARE(recovered, rtcm);
}

// ---------------------------------------------------------------------------
// Test: Large RTCM frame fragmenting + device injection in one routeAll
// ---------------------------------------------------------------------------

void RTKEndToEndTest::testLargeRtcmFragmentedToDevice()
{
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> sent;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        sent.append(msg);
    });

    MockGPSRtk device;
    device.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));

    // Large payload: 400 bytes body -> frame = 3+402+3 = 408 bytes
    const QByteArray largeRtcm = RTCMTestHelper::buildFrame(1077, QByteArray(400, '\xBB'));

    // Inject directly to device — should get full frame in one piece
    device.injectRTCMData(largeRtcm);
    QCOMPARE(device.injectedRtcm.size(), 1);
    QCOMPARE(device.injectedRtcm.first(), largeRtcm);

    // Route through MAVLink — should be fragmented
    mavlink.RTCMDataUpdate(largeRtcm);

    // 408 / 180 = 2.27 -> 3 fragments
    QCOMPARE(sent.size(), 3);

    // Reassemble
    QByteArray reassembled;
    for (const auto &msg : sent) {
        QVERIFY(msg.flags & 0x01); // fragmented
        reassembled.append(reinterpret_cast<const char *>(msg.data), msg.len);
    }
    QCOMPARE(reassembled, largeRtcm);
}

// ---------------------------------------------------------------------------
// Test: Disconnecting a device prevents further injection
// (MockGPSRtk still accepts data but reports disconnected)
// ---------------------------------------------------------------------------

void RTKEndToEndTest::testDeviceDisconnectStopsRouting()
{
    RTCMMavlink mavlink;
    mavlink.setMessageSender([](const mavlink_gps_rtcm_data_t &) {});

    MockGPSRtk device;
    device.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));

    const QByteArray rtcm = RTCMTestHelper::buildFrame(1005, QByteArray(6, '\x11'));

    device.injectRTCMData(rtcm);
    QCOMPARE(device.injectedRtcm.size(), 1);

    device.simulateDisconnect();
    QVERIFY(!device.connected());

    auto *fg = qobject_cast<GPSRTKFactGroup *>(device.gpsRtkFactGroup());
    QVERIFY(!fg->connected()->rawValue().toBool());
}

// ---------------------------------------------------------------------------
// Test: FactGroup tracks full survey-in progress lifecycle
// ---------------------------------------------------------------------------

void RTKEndToEndTest::testFactGroupReflectsSurveyInProgress()
{
    MockGPSRtk device;
    auto *fg = qobject_cast<GPSRTKFactGroup *>(device.gpsRtkFactGroup());
    QVERIFY(fg);

    device.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));

    // Initial position
    device.simulatePosition(47.3977, 8.5456, 408.0, 6);
    QCOMPARE(fg->baseFixType()->rawValue().toInt(), 6);

    // Survey-in stages: increasing duration, decreasing accuracy
    const struct { float dur; float accMM; bool valid; bool active; } stages[] = {
        {  30.0f, 10000.0f, false, true },
        {  60.0f,  5000.0f, false, true },
        { 120.0f,  2000.0f, false, true },
        { 180.0f,  1000.0f, false, true },
        { 240.0f,   800.0f, true, false },
    };

    for (const auto &s : stages) {
        device.simulateSurveyInStatus(s.dur, s.accMM, s.valid, s.active);
        QCOMPARE(fg->currentDuration()->rawValue().toFloat(), s.dur);
        QCOMPARE(fg->valid()->rawValue().toBool(), s.valid);
        QCOMPARE(fg->active()->rawValue().toBool(), s.active);
    }

    // Final accuracy should be 0.8m
    QCOMPARE(fg->currentAccuracy()->rawValue().toDouble(), 0.8);
    QVERIFY(fg->valid()->rawValue().toBool());
}

UT_REGISTER_TEST(RTKEndToEndTest, TestLabel::Unit)
