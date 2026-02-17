#pragma once

#include <memory>

#include "UnitTest.h"

class MockJoystick;

/// Unit tests for MockJoystick functionality
///
/// These tests verify that the MockJoystick test utility itself works correctly,
/// including state tracking, convenience methods, and input sequences.
class MockJoystickTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    // Creation tests
    void _createMockJoystickTest();
    void _createWithCustomCountsTest();

    // Axis state tracking tests
    void _axisStateTrackingTest();
    void _stickConvenienceMethodsTest();
    void _triggerMethodsTest();

    // Button state tracking tests
    void _buttonStateTrackingTest();
    void _gamepadButtonConvenienceTest();
    void _multiButtonPressTest();

    // Hat state tracking tests
    void _hatStateTrackingTest();
    void _dpadConvenienceTest();

    // Input sequence tests
    void _buttonTapSequenceTest();
    void _stickDeflectionSequenceTest();
    void _triggerPullSequenceTest();
    void _dpadTapSequenceTest();

    // Multi-input tests
    void _simultaneousAxesTest();

    // Reset test
    void _resetTest();

private:
    std::unique_ptr<MockJoystick> _mockJoystick;
};
