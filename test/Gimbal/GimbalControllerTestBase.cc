#include "GimbalControllerTestBase.h"

#include <QtTest/QTest>

#include <cmath>

#include "Gimbal.h"
#include "GimbalController.h"
#include "MockLink.h"
#include "UnitTest.h"
#include "Vehicle.h"

void GimbalControllerTestBase::init()
{
    VehicleTestManualConnect::init();

    _connectMockLink(MAV_AUTOPILOT_PX4, MockConfiguration::FailNone, MockConfiguration::OptionEnableGimbal);
    QVERIFY(_vehicle);
    QVERIFY(mockGimbal());

    QVERIFY_TRUE_WAIT(activeGimbal() != nullptr, TestTimeout::longMs());
}

GimbalController* GimbalControllerTestBase::gimbalController() const
{
    return _vehicle ? _vehicle->gimbalController() : nullptr;
}

Gimbal* GimbalControllerTestBase::activeGimbal() const
{
    GimbalController* const controller = gimbalController();
    return controller ? controller->activeGimbal() : nullptr;
}

MockLinkGimbal* GimbalControllerTestBase::mockGimbal() const
{
    return _mockLink ? _mockLink->mockLinkGimbal() : nullptr;
}

void GimbalControllerTestBase::_pushAttitude(float vehicleHeadingDeg, float gimbalYawDeg,
                                             MockLinkGimbal::DeltaYawMode mode, float deltaYawDeg, bool earthFrame)
{
    // Pitch is not part of the yaw math, so a fresh value per push marks when QGC has ingested this message
    _pitchMarker = (_pitchMarker >= 40.0f) ? 1.0f : _pitchMarker + 1.0f;

    // Vehicle heading is truncated to whole degrees; bias by half a degree so float error can't drop it by one
    _mockLink->setVehicleAttitudeOverrideDeg(0.0f, 0.0f, vehicleHeadingDeg + 0.5f);
    const float expectedHeading = std::fmod(vehicleHeadingDeg + 360.0f, 360.0f);
    QVERIFY_TRUE_WAIT(_near(_vehicle->heading()->rawValue().toFloat(), expectedHeading), TestTimeout::mediumMs());

    mockGimbal()->setDeltaYawMode(mode);
    mockGimbal()->setDeltaYawDeg(deltaYawDeg);
    mockGimbal()->setYawInEarthFrame(earthFrame);
    mockGimbal()->setAttitudeDeg(0.0f, _pitchMarker, gimbalYawDeg);
    mockGimbal()->sendGimbalDeviceAttitudeStatusNow();

    QVERIFY_TRUE_WAIT(_near(activeGimbal()->absolutePitch()->rawValue().toFloat(), _pitchMarker),
                      TestTimeout::mediumMs());
}

bool GimbalControllerTestBase::_near(float actual, float expected, float tolerance)
{
    return qAbs(actual - expected) <= tolerance;
}

QString GimbalControllerTestBase::_describe(const char* what, float actual, float expected)
{
    return QStringLiteral("%1: actual %2 expected %3").arg(QLatin1String(what)).arg(actual).arg(expected);
}
