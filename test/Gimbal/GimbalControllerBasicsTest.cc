#include "GimbalControllerBasicsTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Gimbal.h"
#include "GimbalController.h"
#include "GimbalControllerSettings.h"
#include "MAVLinkProtocol.h"
#include "MockConfiguration.h"
#include "MockLink.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

namespace {

constexpr float kVehicleHeadingDeg = 90.0f;
constexpr float kGimbalBodyYawDeg = 20.0f;
constexpr uint8_t kOtherSysId = 42;
constexpr uint8_t kOtherCompId = 190;

}  // namespace

void GimbalControllerBasicsTest::_testDiscovery()
{
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();

    QCOMPARE(controller->gimbals()->count(), 1);
    QCOMPARE(controller->gimbals()->get(0), gimbal);
    QCOMPARE(gimbal->deviceId()->rawValue().toUInt(), static_cast<uint>(MAV_COMP_ID_GIMBAL));
    QCOMPARE(gimbal->managerCompid()->rawValue().toUInt(), static_cast<uint>(MAV_COMP_ID_AUTOPILOT1));

    // Telemetry fact group is registered as gimbal<managerCompid><deviceId>
    const QString factGroupName = QStringLiteral("gimbal%1%2").arg(MAV_COMP_ID_AUTOPILOT1).arg(MAV_COMP_ID_GIMBAL);
    QCOMPARE(_vehicle->getFactGroup(factGroupName), gimbal);

    QVERIFY(gimbal->supportsYawLock());
    QVERIFY(gimbal->supportsRetract());
}

void GimbalControllerBasicsTest::_testCapabilityFlags()
{
    _disconnectMockLink();
    if (QTest::currentTestFailed()) {
        return;
    }

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    MockConfiguration* const config = new MockConfiguration(QStringLiteral("Gimbal caps MockLink"));
    config->setFirmwareType(MAV_AUTOPILOT_PX4);
    config->setVehicleType(MAV_TYPE_QUADROTOR);
    config->setEnableGimbal(true);
    config->setGimbalHasYawLock(false);
    config->setGimbalHasRetract(false);
    _mockLink = MockLink::startMockLink(config);
    QVERIFY(_mockLink);
    (void) connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });

    QVERIFY_SIGNAL_WAIT(spyVehicle, TestTimeout::longMs());
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
    QVERIFY_TRUE_WAIT(activeGimbal() != nullptr, TestTimeout::longMs());

    QVERIFY(!activeGimbal()->supportsYawLock());
    QVERIFY(!activeGimbal()->supportsRetract());
}

void GimbalControllerBasicsTest::_testControlArbitration()
{
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();

    QVERIFY(!gimbal->gimbalHaveControl());
    QVERIFY(!gimbal->gimbalOthersHaveControl());

    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE) > 0,
                      TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());
    QVERIFY(!gimbal->gimbalOthersHaveControl());

    controller->releaseGimbalControl();
    QVERIFY_TRUE_WAIT(!gimbal->gimbalHaveControl(), TestTimeout::longMs());

    // Someone else takes control: commands must be refused and the confirmation popup requested
    mock->setPrimaryControl(kOtherSysId, kOtherCompId);
    mock->sendGimbalManagerStatusNow();
    QVERIFY_TRUE_WAIT(gimbal->gimbalOthersHaveControl(), TestTimeout::mediumMs());

    MultiSignalSpy spy;
    QVERIFY(spy.init(controller));
    const int pitchYawBefore = mock->lastPitchYawCommand().count;

    controller->gimbalOnScreenControl(1.0f, 0.0f, true /* clickAndPoint */, false, false);

    QVERIFY(spy.emittedOnce("showAcquireGimbalControlPopup"));
    // No command may reach the gimbal; proving absence needs the full window
    QVERIFY(!QTest::qWaitFor([mock, pitchYawBefore]() { return mock->lastPitchYawCommand().count > pitchYawBefore; },
                             TestTimeout::shortMs()));

    // Control released by the other GCS: a command with nobody in control acquires control implicitly
    mock->setPrimaryControl(0, 0);
    mock->sendGimbalManagerStatusNow();
    QVERIFY_TRUE_WAIT(!gimbal->gimbalOthersHaveControl(), TestTimeout::mediumMs());
    QVERIFY(!gimbal->gimbalHaveControl());
    const int configureBefore = _mockLink->receivedMavCommandCount(MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE);

    controller->sendPitchBodyYaw(0.0f, 0.0f, false);

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MAV_CMD_DO_GIMBAL_MANAGER_CONFIGURE) > configureBefore,
                      TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > pitchYawBefore, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());
}

void GimbalControllerBasicsTest::_testYawLockRoundTrip()
{
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();

    PUSH_ATTITUDE(kVehicleHeadingDeg, kGimbalBodyYawDeg, MockLinkGimbal::DeltaYawMode::Unknown, 0.0f);
    QVERIFY(!gimbal->yawLock());

    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());

    controller->setGimbalYawLock(true);
    QVERIFY_TRUE_WAIT(mock->yawLock(), TestTimeout::mediumMs());
    QVERIFY(mock->lastPitchYawCommand().flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK);
    mock->sendGimbalDeviceAttitudeStatusNow();
    QVERIFY_TRUE_WAIT(gimbal->yawLock(), TestTimeout::mediumMs());

    controller->setGimbalYawLock(false);
    QVERIFY_TRUE_WAIT(!mock->yawLock(), TestTimeout::mediumMs());
    mock->sendGimbalDeviceAttitudeStatusNow();
    QVERIFY_TRUE_WAIT(!gimbal->yawLock(), TestTimeout::mediumMs());
}

void GimbalControllerBasicsTest::_testOnScreenControlBodyFrame_data()
{
    QTest::addColumn<bool>("clickAndPoint");
    QTest::addColumn<float>("panPct");
    QTest::addColumn<float>("tiltPct");

    QTest::newRow("click-and-point") << true << 0.5f << -0.5f;
    QTest::newRow("click-and-drag") << false << -1.0f << 1.0f;
}

void GimbalControllerBasicsTest::_testOnScreenControlBodyFrame()
{
    QFETCH(bool, clickAndPoint);
    QFETCH(float, panPct);
    QFETCH(float, tiltPct);

    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();
    GimbalControllerSettings* const settings = SettingsManager::instance()->gimbalControllerSettings();

    PUSH_ATTITUDE(kVehicleHeadingDeg, kGimbalBodyYawDeg, MockLinkGimbal::DeltaYawMode::Unknown, 0.0f);
    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());

    const float pitchBefore = gimbal->absolutePitch()->rawValue().toFloat();
    float panInc = 0.0f;
    float tiltInc = 0.0f;
    if (clickAndPoint) {
        panInc = panPct * settings->cameraHFov()->rawValue().toFloat() * 0.5f;
        tiltInc = tiltPct * settings->cameraVFov()->rawValue().toFloat() * 0.5f;
    } else {
        const float maxSpeed = settings->cameraSlideSpeed()->rawValue().toFloat();
        panInc = panPct * maxSpeed * 0.1f;
        tiltInc = tiltPct * maxSpeed * 0.1f;
    }
    const float expectedYaw = panInc + kGimbalBodyYawDeg;
    const float expectedPitch = tiltInc + pitchBefore;
    const int countBefore = mock->lastPitchYawCommand().count;

    controller->gimbalOnScreenControl(panPct, tiltPct, clickAndPoint, !clickAndPoint, false);

    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > countBefore, TestTimeout::mediumMs());
    const MockLinkGimbal::PitchYawCommand cmd = mock->lastPitchYawCommand();
    QVERIFY2(_near(cmd.yawDeg, expectedYaw), qPrintable(_describe("yaw", cmd.yawDeg, expectedYaw)));
    QVERIFY2(_near(cmd.pitchDeg, expectedPitch), qPrintable(_describe("pitch", cmd.pitchDeg, expectedPitch)));
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_IN_VEHICLE_FRAME);
    QVERIFY(!(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK));
}

void GimbalControllerBasicsTest::_testCommandPaths()
{
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();

    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());

    int count = mock->lastPitchYawCommand().count;

    controller->sendPitchBodyYaw(-30.0f, 45.0f, false);
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    MockLinkGimbal::PitchYawCommand cmd = mock->lastPitchYawCommand();
    QCOMPARE(cmd.pitchDeg, -30.0f);
    QCOMPARE(cmd.yawDeg, 45.0f);
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_IN_VEHICLE_FRAME);
    QVERIFY(!(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK));
    count = cmd.count;

    // Absolute yaw is normalised into [-180, 180] before sending, in both directions
    controller->sendPitchAbsoluteYaw(10.0f, 270.0f, false);
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    cmd = mock->lastPitchYawCommand();
    QCOMPARE(cmd.pitchDeg, 10.0f);
    QCOMPARE(cmd.yawDeg, -90.0f);
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK);
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_IN_EARTH_FRAME);
    count = cmd.count;

    controller->sendPitchAbsoluteYaw(10.0f, -270.0f, false);
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    cmd = mock->lastPitchYawCommand();
    QCOMPARE(cmd.yawDeg, 90.0f);
    count = cmd.count;

    controller->centerGimbal();
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    cmd = mock->lastPitchYawCommand();
    QCOMPARE(cmd.pitchDeg, 0.0f);
    QCOMPARE(cmd.yawDeg, 0.0f);
    count = cmd.count;

    controller->setGimbalRetract(true);
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    cmd = mock->lastPitchYawCommand();
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_RETRACT);
    QCOMPARE(cmd.deviceId, static_cast<uint8_t>(MAV_COMP_ID_GIMBAL));
    count = cmd.count;

    controller->setGimbalRetract(false);
    QVERIFY_TRUE_WAIT(mock->lastPitchYawCommand().count > count, TestTimeout::mediumMs());
    cmd = mock->lastPitchYawCommand();
    QVERIFY(!(cmd.flags & GIMBAL_MANAGER_FLAGS_RETRACT));
}

void GimbalControllerBasicsTest::_testJoystickRateSlots()
{
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();
    const float speed =
        SettingsManager::instance()->gimbalControllerSettings()->joystickButtonsSpeed()->rawValue().toFloat();

    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());

    int count = mock->lastSetAttitudeCommand().count;
    controller->gimbalYawStart(1);
    QVERIFY_TRUE_WAIT(mock->lastSetAttitudeCommand().count > count, TestTimeout::mediumMs());
    MockLinkGimbal::SetAttitudeCommand cmd = mock->lastSetAttitudeCommand();
    QVERIFY2(_near(cmd.yawRateDegS, speed), qPrintable(_describe("yawRate", cmd.yawRateDegS, speed)));
    QVERIFY2(_near(cmd.pitchRateDegS, 0.0f), qPrintable(_describe("pitchRate", cmd.pitchRateDegS, 0.0f)));
    QCOMPARE(gimbal->yawRate(), speed);

    // While one axis is moving the resend timer keeps the autopilot fed
    count = cmd.count;
    QVERIFY_TRUE_WAIT(mock->lastSetAttitudeCommand().count > count, TestTimeout::mediumMs());

    controller->gimbalPitchStart(-1);
    QVERIFY_TRUE_WAIT(_near(mock->lastSetAttitudeCommand().pitchRateDegS, -speed), TestTimeout::mediumMs());
    QVERIFY(_near(mock->lastSetAttitudeCommand().yawRateDegS, speed));

    controller->gimbalYawStop();
    QVERIFY_TRUE_WAIT(_near(mock->lastSetAttitudeCommand().yawRateDegS, 0.0f), TestTimeout::mediumMs());
    QVERIFY(_near(mock->lastSetAttitudeCommand().pitchRateDegS, -speed));

    controller->gimbalPitchStop();
    QVERIFY_TRUE_WAIT(_near(mock->lastSetAttitudeCommand().pitchRateDegS, 0.0f), TestTimeout::mediumMs());
    QCOMPARE(gimbal->pitchRate(), 0.0f);
    QCOMPARE(gimbal->yawRate(), 0.0f);

    // Both axes stopped: no further resends
    const int afterStop = mock->lastSetAttitudeCommand().count;
    QVERIFY(!QTest::qWaitFor([mock, afterStop]() { return mock->lastSetAttitudeCommand().count > afterStop; },
                             TestTimeout::shortMs()));
}

void GimbalControllerBasicsTest::_testRateCommand()
{
    GimbalController* const controller = gimbalController();
    Gimbal* const gimbal = activeGimbal();
    MockLinkGimbal* const mock = mockGimbal();

    controller->acquireGimbalControl();
    QVERIFY_TRUE_WAIT(gimbal->gimbalHaveControl(), TestTimeout::longMs());

    const int countBefore = mock->lastSetAttitudeCommand().count;
    controller->sendGimbalRate(15.0f, -25.0f);
    QVERIFY_TRUE_WAIT(mock->lastSetAttitudeCommand().count > countBefore, TestTimeout::mediumMs());

    const MockLinkGimbal::SetAttitudeCommand cmd = mock->lastSetAttitudeCommand();
    QVERIFY2(_near(cmd.pitchRateDegS, 15.0f), qPrintable(_describe("pitchRate", cmd.pitchRateDegS, 15.0f)));
    QVERIFY2(_near(cmd.yawRateDegS, -25.0f), qPrintable(_describe("yawRate", cmd.yawRateDegS, -25.0f)));
    QVERIFY(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_IN_VEHICLE_FRAME);
    QVERIFY(!(cmd.flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK));

    // Rate commands preserve the gimbal's current yaw-lock state
    controller->setGimbalYawLock(true);
    QVERIFY_TRUE_WAIT(mock->yawLock(), TestTimeout::mediumMs());
    mock->sendGimbalDeviceAttitudeStatusNow();
    QVERIFY_TRUE_WAIT(gimbal->yawLock(), TestTimeout::mediumMs());
    const int lockedCount = mock->lastSetAttitudeCommand().count;
    controller->sendGimbalRate(5.0f, 5.0f);
    QVERIFY_TRUE_WAIT(mock->lastSetAttitudeCommand().count > lockedCount, TestTimeout::mediumMs());
    QVERIFY(mock->lastSetAttitudeCommand().flags & GIMBAL_MANAGER_FLAGS_YAW_LOCK);

    // Zero rates stop the resend timer: once the stop message lands, nothing further may arrive
    controller->sendGimbalRate(0.0f, 0.0f);
    QVERIFY_TRUE_WAIT(_near(mock->lastSetAttitudeCommand().pitchRateDegS, 0.0f) &&
                          _near(mock->lastSetAttitudeCommand().yawRateDegS, 0.0f),
                      TestTimeout::mediumMs());
    const int afterStop = mock->lastSetAttitudeCommand().count;
    QVERIFY(!QTest::qWaitFor([mock, afterStop]() { return mock->lastSetAttitudeCommand().count > afterStop; },
                             TestTimeout::shortMs()));
}

UT_REGISTER_TEST(GimbalControllerBasicsTest, TestLabel::Integration, TestLabel::Vehicle)
