#include "GimbalControllerDeltaYawTest.h"

#include <QtTest/QTest>

#include <cmath>

#include "Gimbal.h"
#include "GimbalController.h"
#include "GimbalControllerSettings.h"
#include "MockLink.h"
#include "SettingsManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

namespace {

constexpr float kVehicleHeadingDeg = 90.0f;
constexpr float kGimbalBodyYawDeg = 20.0f;

float wrap180(float deg)
{
    return std::remainder(deg, 360.0f);
}

}  // namespace

void GimbalControllerDeltaYawTest::_testHeadingSource_data()
{
    QTest::addColumn<MockLinkGimbal::DeltaYawMode>("mode");
    QTest::addColumn<bool>("earthFrame");
    QTest::addColumn<float>("deltaYawDeg");
    QTest::addColumn<float>("vehicleHeadingDeg");
    QTest::addColumn<float>("gimbalYawDeg");  // body yaw the mock reports (earth frame adds deltaYaw)
    QTest::addColumn<float>("expectedAbsoluteYaw");
    QTest::addColumn<float>("expectedBodyYaw");
    QTest::addColumn<bool>("expectDeltaYawValid");

    // Vehicle-frame yaw without frame flags: delta_yaw must be ignored, heading comes from the vehicle
    QTest::newRow("legacy-no-frame-flags")
        << MockLinkGimbal::DeltaYawMode::Legacy << false << 0.0f << 90.0f << 20.0f << 110.0f << 20.0f << false;
    // Frame flag set but delta_yaw NaN: spec "unknown", fall back to vehicle heading
    QTest::newRow("unknown-nan") << MockLinkGimbal::DeltaYawMode::Unknown << false << 0.0f << 90.0f << 20.0f << 110.0f
                                 << 20.0f << false;
    // Earth-frame q with delta_yaw NaN: body yaw must be derived from the vehicle heading
    QTest::newRow("unknown-nan-earth-frame")
        << MockLinkGimbal::DeltaYawMode::Unknown << true << 90.0f << 90.0f << 20.0f << 110.0f << 20.0f << false;
    // Frame flag set and delta_yaw == 0 is a real heading (north), not "unsent"
    QTest::newRow("explicit-zero-is-valid")
        << MockLinkGimbal::DeltaYawMode::Explicit << false << 0.0f << 90.0f << 20.0f << 20.0f << 20.0f << true;
    QTest::newRow("explicit-differs-from-vehicle")
        << MockLinkGimbal::DeltaYawMode::Explicit << false << 120.0f << 90.0f << 20.0f << 140.0f << 20.0f << true;
    // Earth-frame report: q carries absolute yaw, body yaw derived via delta_yaw
    QTest::newRow("explicit-earth-frame")
        << MockLinkGimbal::DeltaYawMode::Explicit << true << 120.0f << 90.0f << 20.0f << 140.0f << 20.0f << true;
    // Wrap-around in both directions
    QTest::newRow("wrap-positive-vehicle-frame")
        << MockLinkGimbal::DeltaYawMode::Explicit << false << 170.0f << 90.0f << 30.0f << -160.0f << 30.0f << true;
    QTest::newRow("wrap-negative-earth-frame")
        << MockLinkGimbal::DeltaYawMode::Explicit << true << 170.0f << 90.0f << 20.0f << -170.0f << 20.0f << true;
    // Legacy fallback must also wrap correctly on the vehicle-heading path
    QTest::newRow("legacy-wrap") << MockLinkGimbal::DeltaYawMode::Legacy << false << 0.0f << 350.0f << 30.0f << 20.0f
                                 << 30.0f << false;
}

void GimbalControllerDeltaYawTest::_testHeadingSource()
{
    QFETCH(MockLinkGimbal::DeltaYawMode, mode);
    QFETCH(bool, earthFrame);
    QFETCH(float, deltaYawDeg);
    QFETCH(float, vehicleHeadingDeg);
    QFETCH(float, gimbalYawDeg);
    QFETCH(float, expectedAbsoluteYaw);
    QFETCH(float, expectedBodyYaw);
    QFETCH(bool, expectDeltaYawValid);

    PUSH_ATTITUDE(vehicleHeadingDeg, gimbalYawDeg, mode, deltaYawDeg, earthFrame);

    Gimbal* const gimbal = activeGimbal();
    const float absoluteYaw = gimbal->absoluteYaw()->rawValue().toFloat();
    const float bodyYaw = gimbal->bodyYaw()->rawValue().toFloat();
    const float deltaYaw = gimbal->deltaYaw()->rawValue().toFloat();

    QVERIFY2(_near(absoluteYaw, expectedAbsoluteYaw),
             qPrintable(_describe("absoluteYaw", absoluteYaw, expectedAbsoluteYaw)));
    QVERIFY2(_near(bodyYaw, expectedBodyYaw), qPrintable(_describe("bodyYaw", bodyYaw, expectedBodyYaw)));
    if (expectDeltaYawValid) {
        QVERIFY2(_near(deltaYaw, wrap180(deltaYawDeg)),
                 qPrintable(_describe("deltaYaw", deltaYaw, wrap180(deltaYawDeg))));
    } else {
        QVERIFY2(std::isnan(deltaYaw), qPrintable(_describe("deltaYaw should be NaN", deltaYaw, NAN)));
    }
}

void GimbalControllerDeltaYawTest::_testDeltaYawTracksChanges()
{
    Gimbal* const gimbal = activeGimbal();
    const float deltaSteps[] = {0.0f, 45.0f, 120.0f, -170.0f, 359.0f};

    for (const float delta : deltaSteps) {
        PUSH_ATTITUDE(kVehicleHeadingDeg, kGimbalBodyYawDeg, MockLinkGimbal::DeltaYawMode::Explicit, delta);

        const float expectedAbsolute = wrap180(kGimbalBodyYawDeg + delta);
        const float absoluteYaw = gimbal->absoluteYaw()->rawValue().toFloat();
        const float bodyYaw = gimbal->bodyYaw()->rawValue().toFloat();
        const float deltaYaw = gimbal->deltaYaw()->rawValue().toFloat();

        QVERIFY2(_near(deltaYaw, wrap180(delta)), qPrintable(_describe("deltaYaw", deltaYaw, wrap180(delta))));
        QVERIFY2(_near(absoluteYaw, expectedAbsolute),
                 qPrintable(_describe("absoluteYaw", absoluteYaw, expectedAbsolute)));
        QVERIFY2(_near(bodyYaw, kGimbalBodyYawDeg), qPrintable(_describe("bodyYaw", bodyYaw, kGimbalBodyYawDeg)));
        QCOMPARE(_vehicle->heading()->rawValue().toFloat(), kVehicleHeadingDeg);
    }
}

void GimbalControllerDeltaYawTest::_testVehicleHeadingIgnoredWhenDeltaYawValid()
{
    constexpr float kDeltaYawDeg = 120.0f;
    constexpr float kExpectedAbsolute = 140.0f;
    Gimbal* const gimbal = activeGimbal();

    for (const float heading : {90.0f, 0.0f, 270.0f}) {
        PUSH_ATTITUDE(heading, kGimbalBodyYawDeg, MockLinkGimbal::DeltaYawMode::Explicit, kDeltaYawDeg);
        const float absoluteYaw = gimbal->absoluteYaw()->rawValue().toFloat();
        QVERIFY2(_near(absoluteYaw, kExpectedAbsolute),
                 qPrintable(_describe("absoluteYaw (vehicle frame)", absoluteYaw, kExpectedAbsolute)));
    }

    for (const float heading : {90.0f, 0.0f, 270.0f}) {
        PUSH_ATTITUDE(heading, kGimbalBodyYawDeg, MockLinkGimbal::DeltaYawMode::Explicit, kDeltaYawDeg,
                      true /* earthFrame */);
        const float bodyYaw = gimbal->bodyYaw()->rawValue().toFloat();
        QVERIFY2(_near(bodyYaw, kGimbalBodyYawDeg),
                 qPrintable(_describe("bodyYaw (earth frame)", bodyYaw, kGimbalBodyYawDeg)));
    }
}

void GimbalControllerDeltaYawTest::_testDeltaYawAvailabilityIsPerMessage()
{
    Gimbal* const gimbal = activeGimbal();
    constexpr float kDeltaYawDeg = 120.0f;
    constexpr float kFromDelta = 140.0f;    // 20 + 120
    constexpr float kFromVehicle = 110.0f;  // 20 + 90

    struct Step
    {
        MockLinkGimbal::DeltaYawMode mode;
        float expectedAbsolute;
        bool deltaValid;
    };

    const Step steps[] = {
        {MockLinkGimbal::DeltaYawMode::Explicit, kFromDelta, true},
        {MockLinkGimbal::DeltaYawMode::Unknown, kFromVehicle, false},
        {MockLinkGimbal::DeltaYawMode::Explicit, kFromDelta, true},
        {MockLinkGimbal::DeltaYawMode::Legacy, kFromVehicle, false},
        {MockLinkGimbal::DeltaYawMode::Explicit, kFromDelta, true},
    };

    for (const Step& step : steps) {
        PUSH_ATTITUDE(kVehicleHeadingDeg, kGimbalBodyYawDeg, step.mode, kDeltaYawDeg);
        const float absoluteYaw = gimbal->absoluteYaw()->rawValue().toFloat();
        const float deltaYaw = gimbal->deltaYaw()->rawValue().toFloat();
        QVERIFY2(_near(absoluteYaw, step.expectedAbsolute),
                 qPrintable(_describe("absoluteYaw", absoluteYaw, step.expectedAbsolute)));
        QCOMPARE(!std::isnan(deltaYaw), step.deltaValid);
    }
}

void GimbalControllerDeltaYawTest::_testOnScreenControlYawLockUsesAbsoluteYaw_data()
{
    QTest::addColumn<bool>("clickAndPoint");

    QTest::newRow("click-and-point") << true;
    QTest::newRow("click-and-drag") << false;
}

void GimbalControllerDeltaYawTest::_testOnScreenControlYawLockUsesAbsoluteYaw()
{
    QFETCH(bool, clickAndPoint);

    // Delta yaw deliberately differs from vehicle heading so the two heading sources give different targets
    constexpr float kDeltaYawDeg = 170.0f;
    constexpr float kExpectedAbsolute = -170.0f;  // wrap180(20 + 170)
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();
    GimbalControllerSettings* const settings = SettingsManager::instance()->gimbalControllerSettings();

    PUSH_ATTITUDE(kVehicleHeadingDeg, kGimbalBodyYawDeg, MockLinkGimbal::DeltaYawMode::Explicit, kDeltaYawDeg);
    QVERIFY(_near(gimbal->absoluteYaw()->rawValue().toFloat(), kExpectedAbsolute));

    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());

    controller->setGimbalYawLock(true);
    QVERIFY_TRUE_WAIT(mock->yawLock(), TestTimeout::mediumMs());
    // yawLock flips only on a status sent after the command, so that status also carries the post-command yaw
    QVERIFY_TRUE_WAIT(gimbal->yawLock(), TestTimeout::mediumMs());
    // The yaw-lock command carried absolute yaw; the mock must convert it to body yaw so the pointing does not move
    const float absoluteAfterLock = gimbal->absoluteYaw()->rawValue().toFloat();
    QVERIFY2(_near(absoluteAfterLock, kExpectedAbsolute),
             qPrintable(_describe("absoluteYaw after yaw lock", absoluteAfterLock, kExpectedAbsolute)));
    const float bodyAfterLock = gimbal->bodyYaw()->rawValue().toFloat();
    QVERIFY2(_near(bodyAfterLock, kGimbalBodyYawDeg),
             qPrintable(_describe("bodyYaw after yaw lock", bodyAfterLock, kGimbalBodyYawDeg)));

    const float panInc = clickAndPoint ? settings->cameraHFov()->rawValue().toFloat() * 0.5f
                                       : settings->cameraSlideSpeed()->rawValue().toFloat() * 0.1f;
    const float expectedYaw = wrap180(kExpectedAbsolute + panInc);
    const int countBefore = mock->lastPitchYawCommand().count;

    controller->gimbalOnScreenControl(1.0f, 0.0f, clickAndPoint, !clickAndPoint, false);

    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > countBefore, TestTimeout::mediumMs());
    const MockLinkGimbal::PitchYawCommand cmd = mock->lastPitchYawCommand();
    QVERIFY2(_near(cmd.yawDeg, expectedYaw), qPrintable(_describe("commanded yaw", cmd.yawDeg, expectedYaw)));
    QVERIFY(cmd.yawDeg >= -180.0f && cmd.yawDeg <= 180.0f);
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK);
}

UT_REGISTER_TEST(GimbalControllerDeltaYawTest, TestLabel::Integration, TestLabel::Vehicle)
