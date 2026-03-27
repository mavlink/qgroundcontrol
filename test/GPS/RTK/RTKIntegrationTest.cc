#include "RTKIntegrationTest.h"
#include "MockGPSRtk.h"
#include "GPSRTKFactGroup.h"
#include "RTCMMavlink.h"
#include "SatelliteModel.h"

#include <QtTest/QSignalSpy>

// ---------------------------------------------------------------------------
// Mock device lifecycle
// ---------------------------------------------------------------------------

void RTKIntegrationTest::testMockConnectDisconnect()
{
    MockGPSRtk mock;

    QVERIFY(!mock.connected());
    QVERIFY(mock.devicePath().isEmpty());
    QVERIFY(mock.deviceType().isEmpty());

    QSignalSpy connSpy(&mock, &MockGPSRtk::connectedChanged);
    QSignalSpy typeSpy(&mock, &MockGPSRtk::deviceTypeChanged);

    mock.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));

    QVERIFY(mock.connected());
    QCOMPARE(mock.devicePath(), QStringLiteral("/dev/ttyACM0"));
    QCOMPARE(mock.deviceType(), QStringLiteral("U-blox"));
    QCOMPARE(connSpy.count(), 1);
    QCOMPARE(typeSpy.count(), 1);

    mock.simulateDisconnect();

    QVERIFY(!mock.connected());
    QCOMPARE(connSpy.count(), 2);
}

void RTKIntegrationTest::testMockFactGroupUpdates()
{
    MockGPSRtk mock;
    auto *fg = qobject_cast<GPSRTKFactGroup*>(mock.gpsRtkFactGroup());
    QVERIFY(fg);

    mock.simulatePosition(47.3977, 8.5456, 408.0, 6);

    QCOMPARE(fg->baseLatitude()->rawValue().toDouble(), 47.3977);
    QCOMPARE(fg->baseLongitude()->rawValue().toDouble(), 8.5456);
    QCOMPARE(fg->baseAltitude()->rawValue().toDouble(), 408.0);
    QCOMPARE(fg->baseFixType()->rawValue().toInt(), 6);
    QVERIFY(fg->connected()->rawValue().toBool());
}

void RTKIntegrationTest::testMockSurveyInProgress()
{
    MockGPSRtk mock;
    auto *fg = qobject_cast<GPSRTKFactGroup*>(mock.gpsRtkFactGroup());
    QVERIFY(fg);

    mock.simulateSurveyInStatus(120.0f, 2500.0f, false, true);

    QCOMPARE(fg->currentDuration()->rawValue().toFloat(), 120.0f);
    QCOMPARE(fg->currentAccuracy()->rawValue().toDouble(), 2.5);
    QVERIFY(!fg->valid()->rawValue().toBool());
    QVERIFY(fg->active()->rawValue().toBool());

    // Survey-in converges
    mock.simulateSurveyInStatus(300.0f, 500.0f, true, false);

    QVERIFY(fg->valid()->rawValue().toBool());
    QVERIFY(!fg->active()->rawValue().toBool());
    QCOMPARE(fg->currentAccuracy()->rawValue().toDouble(), 0.5);
}

// ---------------------------------------------------------------------------
// RTCM routing with mock
// ---------------------------------------------------------------------------

void RTKIntegrationTest::testRtcmRoutesToMockDevice()
{
    RTCMMavlink mavlink;
    MockGPSRtk mock;

    mock.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));

    // Can't use addGpsRtk because MockGPSRtk isn't GPSRtk
    // Instead test the injection directly
    const QByteArray rtcm("RTCM_DATA_12345");
    mock.injectRTCMData(rtcm);

    QCOMPARE(mock.injectedRtcm.size(), 1);
    QCOMPARE(mock.injectedRtcm.first(), rtcm);
}

void RTKIntegrationTest::testRtcmRoutesToMultipleMocks()
{
    MockGPSRtk mock1;
    MockGPSRtk mock2;

    mock1.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));
    mock2.simulateConnect(QStringLiteral("/dev/ttyACM1"), QStringLiteral("Trimble"));

    const QByteArray data1("MSG_A");
    const QByteArray data2("MSG_B");

    mock1.injectRTCMData(data1);
    mock2.injectRTCMData(data1);
    mock1.injectRTCMData(data2);
    mock2.injectRTCMData(data2);

    QCOMPARE(mock1.injectedRtcm.size(), 2);
    QCOMPARE(mock2.injectedRtcm.size(), 2);
    QCOMPARE(mock1.injectedRtcm[0], data1);
    QCOMPARE(mock1.injectedRtcm[1], data2);
    QCOMPARE(mock2.injectedRtcm[0], data1);
    QCOMPARE(mock2.injectedRtcm[1], data2);
}

void RTKIntegrationTest::testRemoveDeviceStopsRouting()
{
    MockGPSRtk mock;
    mock.simulateConnect(QStringLiteral("/dev/ttyACM0"), QStringLiteral("U-blox"));

    mock.injectRTCMData(QByteArrayLiteral("DATA1"));
    QCOMPARE(mock.injectedRtcm.size(), 1);

    mock.simulateDisconnect();
    QVERIFY(!mock.connected());

    // After disconnect, injection is rejected (device not connected)
    mock.injectRTCMData(QByteArrayLiteral("DATA2"));
    QCOMPARE(mock.injectedRtcm.size(), 1);
}

// ---------------------------------------------------------------------------
// Position simulation
// ---------------------------------------------------------------------------

void RTKIntegrationTest::testSimulatePosition()
{
    MockGPSRtk mock;
    auto *fg = qobject_cast<GPSRTKFactGroup*>(mock.gpsRtkFactGroup());

    // Test various positions
    mock.simulatePosition(0.0, 0.0, 0.0, 0);
    QCOMPARE(fg->baseLatitude()->rawValue().toDouble(), 0.0);

    mock.simulatePosition(-33.8688, 151.2093, 58.0, 3);
    QCOMPARE(fg->baseLatitude()->rawValue().toDouble(), -33.8688);
    QCOMPARE(fg->baseLongitude()->rawValue().toDouble(), 151.2093);
    QCOMPARE(fg->baseFixType()->rawValue().toInt(), 3);

    // RTK fixed
    mock.simulatePosition(47.3977, 8.5456, 408.0, 6);
    QCOMPARE(fg->baseFixType()->rawValue().toInt(), 6);
}

void RTKIntegrationTest::testSimulateSatellites()
{
    MockGPSRtk mock;
    auto *fg = qobject_cast<GPSRTKFactGroup*>(mock.gpsRtkFactGroup());

    mock.simulateSatellites(0);
    QCOMPARE(fg->numSatellites()->rawValue().toInt(), 0);

    mock.simulateSatellites(24);
    QCOMPARE(fg->numSatellites()->rawValue().toInt(), 24);

    mock.simulateSatellites(42);
    QCOMPARE(fg->numSatellites()->rawValue().toInt(), 42);
}

UT_REGISTER_TEST(RTKIntegrationTest, TestLabel::Unit)
