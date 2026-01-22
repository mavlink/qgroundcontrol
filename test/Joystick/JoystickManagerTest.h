#pragma once

#include "UnitTest.h"

#include <memory>

class MockJoystick;

/// Unit tests for JoystickManager
///
/// Tests the manager's ability to discover joysticks, track active joystick,
/// handle hot-plug events, and emit appropriate signals.
class JoystickManagerTest : public UnitTest
{
    Q_OBJECT

public:
    JoystickManagerTest();

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

private:
    void _pumpEvents();

    std::unique_ptr<MockJoystick> _mockJoystick1;
    std::unique_ptr<MockJoystick> _mockJoystick2;
};
