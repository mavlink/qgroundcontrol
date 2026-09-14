#pragma once

#include <QtTest/QTest>

#include "BaseClasses/VehicleTestManualConnect.h"
#include "MockLinkGimbal.h"

class Gimbal;
class GimbalController;

/// Shared fixture for GimbalController tests: connects a PX4 MockLink with a gimbal and waits for discovery.
class GimbalControllerTestBase : public VehicleTestManualConnect
{
    Q_OBJECT

protected slots:
    void init() override;

protected:
    GimbalController* gimbalController() const;
    Gimbal* activeGimbal() const;
    MockLinkGimbal* mockGimbal() const;

    /// Pins vehicle heading and gimbal body yaw, pushes one attitude status and waits for QGC to ingest it.
    void _pushAttitude(float vehicleHeadingDeg, float gimbalYawDeg, MockLinkGimbal::DeltaYawMode mode,
                       float deltaYawDeg, bool earthFrame = false);

    static bool _near(float actual, float expected, float tolerance = 0.1f);
    static QString _describe(const char* what, float actual, float expected);

private:
    float _pitchMarker = 0.0f;
};

/// _pushAttitude() verifies internally; QtTest's QVERIFY only returns from the helper, so callers must bail out too.
#define PUSH_ATTITUDE(...)                \
    do {                                  \
        _pushAttitude(__VA_ARGS__);       \
        if (QTest::currentTestFailed()) { \
            return;                       \
        }                                 \
    } while (false)

Q_DECLARE_METATYPE(MockLinkGimbal::DeltaYawMode)
