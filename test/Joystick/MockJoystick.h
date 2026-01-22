#pragma once

#include "JoystickSDL.h"

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
                                 QObject *parent = nullptr);

    /// @return true if the virtual joystick is valid and open
    [[nodiscard]] bool isValid() const;

    // -------------------------------------------------------------------------
    // Axis Control (uses parent's setVirtualAxis)
    // -------------------------------------------------------------------------

    /// Set axis value (-32767 to 32767)
    bool setAxis(int axis, int value);

    /// Set left stick position
    bool setLeftStick(int x, int y);

    /// Set right stick position
    bool setRightStick(int x, int y);

    /// Set left trigger value (0 to 32767)
    bool setLeftTrigger(int value);

    /// Set right trigger value (0 to 32767)
    bool setRightTrigger(int value);

    /// Center all axes
    void centerAllAxes();

    // -------------------------------------------------------------------------
    // Button Control (uses parent's setVirtualButton)
    // -------------------------------------------------------------------------

    /// Set button state
    bool setButton(int button, bool pressed);

    /// Press a button
    bool pressButton(int button) { return setButton(button, true); }

    /// Release a button
    bool releaseButton(int button) { return setButton(button, false); }

    /// Release all buttons
    void releaseAllButtons();

    // -------------------------------------------------------------------------
    // Hat Control (uses parent's setVirtualHat)
    // -------------------------------------------------------------------------

    /// Set hat direction (uses Joystick::HatDirection enum)
    bool setHat(int hat, HatDirection direction);

    /// Center the hat
    bool centerHat(int hat = 0) { return setHat(hat, HatCentered); }

    // -------------------------------------------------------------------------
    // Convenience Methods
    // -------------------------------------------------------------------------

    /// Reset all inputs to default state
    void reset();

private:
    explicit MockJoystick(const QString &name, int axisCount, int buttonCount, int hatCount, int virtualInstanceId, QObject *parent);

    int _virtualInstanceId = -1;
};
