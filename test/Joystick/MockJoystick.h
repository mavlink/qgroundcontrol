#pragma once

#include "JoystickSDL.h"

#include <QtCore/QVector>

/// MockJoystick - A JoystickSDL subclass backed by an SDL virtual joystick for testing
///
/// This class creates an SDL virtual joystick and provides methods to simulate input.
/// Since it inherits from JoystickSDL, it can be used anywhere a Joystick* is expected.
///
/// Usage:
///   auto mock = MockJoystick::create("Test Controller", 6, 16, 1);
///   mock->setAxis(0, 16000);
///   mock->pressButton(0);
///   // Use mock as a Joystick* in tests
///
/// Features:
///   - Full axis, button, hat, ball, and touchpad control
///   - Gamepad-style convenience methods (face buttons, triggers, sticks)
///   - State querying for verification
///   - Input sequence simulation for common patterns
///
class MockJoystick : public JoystickSDL
{
    Q_OBJECT

public:
    ~MockJoystick() override;

    /// Factory method to create a MockJoystick
    /// @return nullptr if creation fails
    static MockJoystick *create(const QString &name = QStringLiteral("Mock Joystick"),
                                 int axisCount = 6,
                                 int buttonCount = 16,
                                 int hatCount = 1,
                                 int ballCount = 0,
                                 int touchpadCount = 0,
                                 QObject *parent = nullptr);

    /// @return true if the virtual joystick is valid and open
    [[nodiscard]] bool isValid() const;

    // =========================================================================
    // Axis Control
    // =========================================================================

    /// Set axis value (-32767 to 32767)
    bool setAxis(int axis, int value);

    /// Set left stick position (axes 0 and 1)
    bool setLeftStick(int x, int y);

    /// Set right stick position (axes 2 and 3)
    bool setRightStick(int x, int y);

    /// Set left trigger value (axis 4, 0 to 32767 for triggers)
    bool setLeftTrigger(int value);

    /// Set right trigger value (axis 5, 0 to 32767 for triggers)
    bool setRightTrigger(int value);

    /// Center all axes (set to 0)
    void centerAllAxes();

    /// Get current axis value (for verification)
    int getAxisValue(int axis) const;

    // =========================================================================
    // Button Control
    // =========================================================================

    /// Set button state
    bool setButton(int button, bool pressed);

    /// Press a button
    bool pressButton(int button) { return setButton(button, true); }

    /// Release a button
    bool releaseButton(int button) { return setButton(button, false); }

    /// Release all buttons
    void releaseAllButtons();

    /// Get current button state (for verification)
    bool getButtonState(int button) const;

    // -------------------------------------------------------------------------
    // Gamepad Face Buttons (A/B/X/Y or Cross/Circle/Square/Triangle)
    // -------------------------------------------------------------------------

    bool pressA() { return pressButton(ButtonA); }
    bool releaseA() { return releaseButton(ButtonA); }
    bool pressB() { return pressButton(ButtonB); }
    bool releaseB() { return releaseButton(ButtonB); }
    bool pressX() { return pressButton(ButtonX); }
    bool releaseX() { return releaseButton(ButtonX); }
    bool pressY() { return pressButton(ButtonY); }
    bool releaseY() { return releaseButton(ButtonY); }

    // -------------------------------------------------------------------------
    // Gamepad Menu Buttons
    // -------------------------------------------------------------------------

    bool pressStart() { return pressButton(ButtonStart); }
    bool releaseStart() { return releaseButton(ButtonStart); }
    bool pressBack() { return pressButton(ButtonBack); }
    bool releaseBack() { return releaseButton(ButtonBack); }
    bool pressGuide() { return pressButton(ButtonGuide); }
    bool releaseGuide() { return releaseButton(ButtonGuide); }

    // -------------------------------------------------------------------------
    // Gamepad Shoulder Buttons
    // -------------------------------------------------------------------------

    bool pressLeftShoulder() { return pressButton(ButtonLeftShoulder); }
    bool releaseLeftShoulder() { return releaseButton(ButtonLeftShoulder); }
    bool pressRightShoulder() { return pressButton(ButtonRightShoulder); }
    bool releaseRightShoulder() { return releaseButton(ButtonRightShoulder); }

    // -------------------------------------------------------------------------
    // Gamepad Stick Buttons (L3/R3)
    // -------------------------------------------------------------------------

    bool pressLeftStickButton() { return pressButton(ButtonLeftStick); }
    bool releaseLeftStickButton() { return releaseButton(ButtonLeftStick); }
    bool pressRightStickButton() { return pressButton(ButtonRightStick); }
    bool releaseRightStickButton() { return releaseButton(ButtonRightStick); }

    // =========================================================================
    // Hat (D-Pad) Control
    // =========================================================================

    /// Set hat direction (uses Joystick::HatDirection enum)
    bool setHat(int hat, HatDirection direction);

    /// Center the hat (return to neutral)
    bool centerHat(int hat = 0) { return setHat(hat, HatCentered); }

    /// Get current hat direction (for verification)
    HatDirection getHatDirection(int hat) const;

    // -------------------------------------------------------------------------
    // D-Pad Convenience Methods (for hat 0)
    // -------------------------------------------------------------------------

    bool pressDPadUp() { return setHat(0, HatUp); }
    bool pressDPadDown() { return setHat(0, HatDown); }
    bool pressDPadLeft() { return setHat(0, HatLeft); }
    bool pressDPadRight() { return setHat(0, HatRight); }
    bool pressDPadUpRight() { return setHat(0, HatRightUp); }
    bool pressDPadUpLeft() { return setHat(0, HatLeftUp); }
    bool pressDPadDownRight() { return setHat(0, HatRightDown); }
    bool pressDPadDownLeft() { return setHat(0, HatLeftDown); }
    bool releaseDPad() { return centerHat(0); }

    // =========================================================================
    // Ball Control (Trackball)
    // =========================================================================

    /// Set ball delta movement
    bool setBall(int ball, int dx, int dy);

    // =========================================================================
    // Touchpad Control
    // =========================================================================

    /// Set touchpad finger state
    /// @param touchpad Touchpad index
    /// @param finger Finger index (0-based)
    /// @param down Whether finger is touching
    /// @param x X position (0.0 to 1.0)
    /// @param y Y position (0.0 to 1.0)
    /// @param pressure Pressure (0.0 to 1.0)
    bool setTouchpad(int touchpad, int finger, bool down, float x, float y, float pressure = 1.0f);

    /// Release all touchpad fingers
    bool releaseTouchpad(int touchpad, int finger) { return setTouchpad(touchpad, finger, false, 0, 0, 0); }

    // =========================================================================
    // Multi-Input Methods (for testing simultaneous inputs)
    // =========================================================================

    /// Press multiple buttons simultaneously
    bool pressButtons(const QVector<int> &buttons);

    /// Release multiple buttons simultaneously
    bool releaseButtons(const QVector<int> &buttons);

    /// Set multiple axes at once
    bool setAxes(const QVector<QPair<int, int>> &axisValues);

    // =========================================================================
    // Input Sequences (for common test patterns)
    // =========================================================================

    /// Simulate a button tap (press then release)
    bool tapButton(int button);

    /// Simulate stick deflection and return to center
    bool deflectLeftStick(int x, int y);
    bool deflectRightStick(int x, int y);

    /// Simulate full trigger pull and release
    bool pullLeftTrigger();
    bool pullRightTrigger();

    /// Simulate a quick D-pad tap in a direction
    bool tapDPad(HatDirection direction);

    // =========================================================================
    // Reset/Convenience Methods
    // =========================================================================

    /// Reset all inputs to default state (axes centered, buttons released, hats centered)
    void reset();

    /// Get the counts for verification
    int mockAxisCount() const { return _mockAxisCount; }
    int mockButtonCount() const { return _mockButtonCount; }
    int mockHatCount() const { return _mockHatCount; }
    int mockBallCount() const { return _mockBallCount; }
    int mockTouchpadCount() const { return _mockTouchpadCount; }

private:
    explicit MockJoystick(const QString &name, int axisCount, int buttonCount, int hatCount,
                          int ballCount, int touchpadCount, int virtualInstanceId, QObject *parent);

    int _virtualInstanceId = -1;
    int _mockAxisCount = 0;
    int _mockButtonCount = 0;
    int _mockHatCount = 0;
    int _mockBallCount = 0;
    int _mockTouchpadCount = 0;

    // Track current state for verification
    QVector<int> _axisStates;
    QVector<bool> _buttonStates;
    QVector<HatDirection> _hatStates;
};
