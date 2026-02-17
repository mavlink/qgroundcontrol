#pragma once

#include <memory>

#include "MockJoystick.h"
#include "UnitTest.h"

/// Unit tests for JoystickManager
///
/// Tests the manager's ability to discover joysticks, track active joystick,
/// handle hot-plug events, and emit appropriate signals.
class JoystickManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    // Discovery tests
    void _availableJoystickNamesTest();
    void _activeJoystickTest();

    // Hot-plug tests
    void _joystickAddedSignalTest();
    void _joystickRemovedSignalTest();

    // Active joystick selection
    void _setActiveJoystickTest();
    void _autoSelectFirstJoystickTest();

    // Polling control tests
    void _pollingControlTest();

    // Multiple controller handling tests
    void _multipleControllerManagementTest();

private:
    std::unique_ptr<MockJoystick> _mockJoystick1;
    std::unique_ptr<MockJoystick> _mockJoystick2;
};
