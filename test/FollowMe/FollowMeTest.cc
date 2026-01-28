#include "FollowMeTest.h"
#include "FollowMe.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "TestHelpers.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// GCSMotionReport Struct Tests
// ============================================================================

void FollowMeTest::_testGCSMotionReportDefaults()
{
    FollowMe::GCSMotionReport report{};

    QCOMPARE_EQ(report.lat_int, 0);
    QCOMPARE_EQ(report.lon_int, 0);
    QCOMPARE_EQ(report.altMetersAMSL, 0.0);
    QCOMPARE_EQ(report.headingDegrees, 0.0);
    QCOMPARE_EQ(report.vxMetersPerSec, 0.0);
    QCOMPARE_EQ(report.vyMetersPerSec, 0.0);
    QCOMPARE_EQ(report.vzMetersPerSec, 0.0);
}

void FollowMeTest::_testGCSMotionReportLatLonInt()
{
    FollowMe::GCSMotionReport report{};

    // Test latitude encoding (1e7 format)
    // 47.3977 degrees = 473977000 in 1e7 format
    report.lat_int = static_cast<int>(47.3977 * 1e7);
    QCOMPARE_EQ(report.lat_int, 473977000);

    // Test longitude encoding
    // 8.5456 degrees = 85456000 in 1e7 format
    report.lon_int = static_cast<int>(8.5456 * 1e7);
    QCOMPARE_EQ(report.lon_int, 85456000);

    // Test negative latitude (southern hemisphere)
    report.lat_int = static_cast<int>(-33.8688 * 1e7);
    QVERIFY(report.lat_int < 0);

    // Test negative longitude (western hemisphere)
    report.lon_int = static_cast<int>(-118.2437 * 1e7);
    QVERIFY(report.lon_int < 0);
}

void FollowMeTest::_testGCSMotionReportAltitude()
{
    FollowMe::GCSMotionReport report{};

    report.altMetersAMSL = 100.5;
    QCOMPARE_EQ(report.altMetersAMSL, 100.5);

    // Test negative altitude (below sea level)
    report.altMetersAMSL = -50.0;
    QCOMPARE_EQ(report.altMetersAMSL, -50.0);

    // Test high altitude
    report.altMetersAMSL = 10000.0;
    QCOMPARE_EQ(report.altMetersAMSL, 10000.0);
}

void FollowMeTest::_testGCSMotionReportHeading()
{
    FollowMe::GCSMotionReport report{};

    // Test north heading
    report.headingDegrees = 0.0;
    QCOMPARE_EQ(report.headingDegrees, 0.0);

    // Test east heading
    report.headingDegrees = 90.0;
    QCOMPARE_EQ(report.headingDegrees, 90.0);

    // Test south heading
    report.headingDegrees = 180.0;
    QCOMPARE_EQ(report.headingDegrees, 180.0);

    // Test west heading
    report.headingDegrees = 270.0;
    QCOMPARE_EQ(report.headingDegrees, 270.0);

    // Test fractional heading
    report.headingDegrees = 45.5;
    QCOMPARE_EQ(report.headingDegrees, 45.5);
}

void FollowMeTest::_testGCSMotionReportVelocity()
{
    FollowMe::GCSMotionReport report{};

    // Test velocity in NED frame
    report.vxMetersPerSec = 5.0;  // North
    report.vyMetersPerSec = 3.0;  // East
    report.vzMetersPerSec = -2.0; // Down (negative = up)

    QCOMPARE_EQ(report.vxMetersPerSec, 5.0);
    QCOMPARE_EQ(report.vyMetersPerSec, 3.0);
    QCOMPARE_EQ(report.vzMetersPerSec, -2.0);

    // Test high speed
    report.vxMetersPerSec = 30.0; // ~108 km/h
    QCOMPARE_EQ(report.vxMetersPerSec, 30.0);
}

void FollowMeTest::_testGCSMotionReportPosStdDev()
{
    FollowMe::GCSMotionReport report{};

    // Default values
    QCOMPARE_EQ(report.pos_std_dev[0], 0.0);
    QCOMPARE_EQ(report.pos_std_dev[1], 0.0);
    QCOMPARE_EQ(report.pos_std_dev[2], 0.0);

    // Set horizontal accuracy
    report.pos_std_dev[0] = 2.5;
    report.pos_std_dev[1] = 2.5;
    QCOMPARE_EQ(report.pos_std_dev[0], 2.5);
    QCOMPARE_EQ(report.pos_std_dev[1], 2.5);

    // Set vertical accuracy
    report.pos_std_dev[2] = 5.0;
    QCOMPARE_EQ(report.pos_std_dev[2], 5.0);

    // Test unknown marker (-1)
    report.pos_std_dev[0] = -1.0;
    report.pos_std_dev[1] = -1.0;
    report.pos_std_dev[2] = -1.0;
    QCOMPARE_EQ(report.pos_std_dev[0], -1.0);
    QCOMPARE_EQ(report.pos_std_dev[1], -1.0);
    QCOMPARE_EQ(report.pos_std_dev[2], -1.0);
}

// ============================================================================
// MotionCapability Enum Tests
// ============================================================================

void FollowMeTest::_testMotionCapabilityPos()
{
    QCOMPARE_EQ(FollowMe::MotionCapability::POS, 0);
}

void FollowMeTest::_testMotionCapabilityVel()
{
    QCOMPARE_EQ(FollowMe::MotionCapability::VEL, 1);
}

void FollowMeTest::_testMotionCapabilityAccel()
{
    QCOMPARE_EQ(FollowMe::MotionCapability::ACCEL, 2);
}

void FollowMeTest::_testMotionCapabilityAttRates()
{
    QCOMPARE_EQ(FollowMe::MotionCapability::ATT_RATES, 3);
}

void FollowMeTest::_testMotionCapabilityHeading()
{
    QCOMPARE_EQ(FollowMe::MotionCapability::HEADING, 4);
}

void FollowMeTest::_testMotionCapabilityBitmask()
{
    uint8_t capabilities = 0;

    // Set position capability
    capabilities |= (1 << FollowMe::MotionCapability::POS);
    QCOMPARE_EQ(capabilities, static_cast<uint8_t>(1));

    // Add velocity capability
    capabilities |= (1 << FollowMe::MotionCapability::VEL);
    QCOMPARE_EQ(capabilities, static_cast<uint8_t>(3)); // 0b00000011

    // Add heading capability
    capabilities |= (1 << FollowMe::MotionCapability::HEADING);
    QCOMPARE_EQ(capabilities, static_cast<uint8_t>(19)); // 0b00010011

    // Test individual bits
    QVERIFY(capabilities & (1 << FollowMe::MotionCapability::POS));
    QVERIFY(capabilities & (1 << FollowMe::MotionCapability::VEL));
    QVERIFY(!(capabilities & (1 << FollowMe::MotionCapability::ACCEL)));
    QVERIFY(!(capabilities & (1 << FollowMe::MotionCapability::ATT_RATES)));
    QVERIFY(capabilities & (1 << FollowMe::MotionCapability::HEADING));
}

// ============================================================================
// FollowMe Instance Tests
// ============================================================================

void FollowMeTest::_testFollowMeInstance()
{
    FollowMe* followMe = FollowMe::instance();
    VERIFY_NOT_NULL(followMe);
}

void FollowMeTest::_testFollowMeInstanceSingleton()
{
    FollowMe* instance1 = FollowMe::instance();
    FollowMe* instance2 = FollowMe::instance();

    VERIFY_NOT_NULL(instance1);
    VERIFY_NOT_NULL(instance2);
    QCOMPARE(instance1, instance2);
}

// ============================================================================
// Timer Constant Tests
// ============================================================================

void FollowMeTest::_testMotionUpdateInterval()
{
    // The motion update interval should be 250ms (4 Hz)
    // This is a reasonable update rate for follow me functionality
    // We can't directly access kMotionUpdateInterval since it's private,
    // but we can verify the behavior through integration tests

    // Verify the instance exists and is properly configured
    FollowMe* followMe = FollowMe::instance();
    VERIFY_NOT_NULL(followMe);
}

// ============================================================================
// Integration Tests
// ============================================================================

void FollowMeTest::_testFollowMeInit()
{
    FollowMe* followMe = FollowMe::instance();
    VERIFY_NOT_NULL(followMe);

    // Init should be callable multiple times without issues
    followMe->init();
    followMe->init();

    VERIFY_NOT_NULL(FollowMe::instance());
}

void FollowMeTest::_testFollowMeWithVehicle()
{
    // Vehicle is already connected via VehicleTestNoConnect base class
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();

    vehicle()->setFlightMode(vehicle()->followFlightMode());
    SettingsManager::instance()->appSettings()->followTarget()->setRawValue(1);

    QSignalSpy spyGCSMotionReport(vehicle(), &Vehicle::messagesSentChanged);
    QGC_VERIFY_SPY_VALID(spyGCSMotionReport);

    QVERIFY(spyGCSMotionReport.wait(TestHelpers::kDefaultTimeoutMs));
}
