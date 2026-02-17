#pragma once

#include <QtCore/QMap>

#include <memory>

#include "Joystick.h"
#include "UnitTest.h"

class MockJoystick;
class JoystickSDL;

/// Integration tests for JoystickSDL using SDL virtual joystick
///
/// These tests create real virtual joysticks via SDL and verify that
/// JoystickSDL correctly discovers and reads input from them.
class JoystickTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    // Discovery tests
    void _discoverVirtualJoystickTest();
    void _multipleJoysticksTest();

    // Identity tests
    void _joystickIdentityTest();
    void _joystickCapabilitiesTest();

    // Axis reading tests
    void _readAxisValuesTest();
    void _axisRangeTest();

    // Button reading tests
    void _readButtonStatesTest();
    void _buttonPressReleaseSequenceTest();

    // Hat reading tests
    void _readHatDirectionsTest();

    // Polling/update tests
    void _pollingUpdatesValuesTest();

    // Calibration tests
    void _calibrationDataTest();
    void _adjustRangeTest();

    // Error handling tests
    void _invalidAxisIndexTest();
    void _invalidButtonIndexTest();
    void _zeroCapabilityJoystickTest();

    // Hot-plug tests
    void _joystickDisconnectTest();

    // Hat-to-button mapping tests
    void _hatButtonMappingTest();
    void _hatMutualExclusionTest();

    // Button action tests
    void _buttonActionAssignmentTest();

    // Connection state tests
    void _connectionStateTest();

    // Player index tests
    void _playerIndexTest();

    // Gamepad binding query tests
    void _gamepadBindingQueryTest();

private:
    JoystickSDL* _findJoystickByInstanceId(int instanceId);
    void _pumpEvents();

    std::unique_ptr<MockJoystick> _mockJoystick;
    QMap<QString, Joystick*> _discoveredJoysticks;
};
