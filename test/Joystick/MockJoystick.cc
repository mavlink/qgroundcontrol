#include "MockJoystick.h"

#include <QtCore/QLoggingCategory>

#include "SDLJoystick.h"

namespace {

QList<int> makeAxisList(int count)
{
    QList<int> axes;
    axes.reserve(count);
    for (int i = 0; i < count; ++i) {
        axes.append(i);
    }
    return axes;
}

}  // namespace

MockJoystick* MockJoystick::create(const QString& name, int axisCount, int buttonCount, int hatCount, int ballCount,
                                   int touchpadCount, QObject* parent)
{
    const int instanceId = SDLJoystick::createVirtualJoystick(name, axisCount, buttonCount, hatCount);
    if (instanceId < 0) {
        qWarning() << "MockJoystick: Failed to create virtual joystick" << name;
        return nullptr;
    }

    auto* mock = new MockJoystick(name, axisCount, buttonCount, hatCount, ballCount, touchpadCount, instanceId, parent);
    if (!mock->_open()) {
        qWarning() << "MockJoystick: Failed to open virtual joystick" << name;
        delete mock;
        return nullptr;
    }
    return mock;
}

MockJoystick::MockJoystick(const QString& name, int axisCount, int buttonCount, int hatCount, int ballCount,
                           int touchpadCount, int virtualInstanceId, QObject* parent)
    : JoystickSDL(name,
                  QList<int>(),             // No gamepad axes (virtual joystick isn't recognized as gamepad)
                  makeAxisList(axisCount),  // All axes are non-gamepad axes
                  buttonCount, hatCount, virtualInstanceId, parent),
      _virtualInstanceId(virtualInstanceId),
      _mockAxisCount(axisCount),
      _mockButtonCount(buttonCount),
      _mockHatCount(hatCount),
      _mockBallCount(ballCount),
      _mockTouchpadCount(touchpadCount),
      _axisStates(axisCount, 0),
      _buttonStates(buttonCount, false),
      _hatStates(hatCount, HatCentered)
{
}

MockJoystick::~MockJoystick()
{
    _close();
    if (_virtualInstanceId >= 0) {
        SDLJoystick::destroyVirtualJoystick(_virtualInstanceId);
    }
}

bool MockJoystick::isValid() const
{
    return (_virtualInstanceId >= 0) && (_sdlJoystick != nullptr);
}

// =============================================================================
// Axis Control
// =============================================================================

bool MockJoystick::setAxis(int axis, int value)
{
    if (axis >= 0 && axis < _mockAxisCount) {
        _axisStates[axis] = value;
    }
    return setVirtualAxis(axis, value);
}

bool MockJoystick::setLeftStick(int x, int y)
{
    return setAxis(AxisLeftX, x) && setAxis(AxisLeftY, y);
}

bool MockJoystick::setRightStick(int x, int y)
{
    return setAxis(AxisRightX, x) && setAxis(AxisRightY, y);
}

bool MockJoystick::setLeftTrigger(int value)
{
    return setAxis(AxisTriggerLeft, value);
}

bool MockJoystick::setRightTrigger(int value)
{
    return setAxis(AxisTriggerRight, value);
}

void MockJoystick::centerAllAxes()
{
    for (int i = 0; i < _mockAxisCount; ++i) {
        setAxis(i, 0);
    }
}

int MockJoystick::getAxisValue(int axis) const
{
    if (axis >= 0 && axis < _axisStates.size()) {
        return _axisStates[axis];
    }
    return 0;
}

// =============================================================================
// Button Control
// =============================================================================

bool MockJoystick::setButton(int button, bool pressed)
{
    if (button >= 0 && button < _mockButtonCount) {
        _buttonStates[button] = pressed;
    }
    return setVirtualButton(button, pressed);
}

void MockJoystick::releaseAllButtons()
{
    for (int i = 0; i < _mockButtonCount; ++i) {
        setButton(i, false);
    }
}

bool MockJoystick::getButtonState(int button) const
{
    if (button >= 0 && button < _buttonStates.size()) {
        return _buttonStates[button];
    }
    return false;
}

// =============================================================================
// Hat Control
// =============================================================================

bool MockJoystick::setHat(int hat, HatDirection direction)
{
    if (hat >= 0 && hat < _mockHatCount) {
        _hatStates[hat] = direction;
    }
    return setVirtualHat(hat, static_cast<quint8>(direction));
}

Joystick::HatDirection MockJoystick::getHatDirection(int hat) const
{
    if (hat >= 0 && hat < _hatStates.size()) {
        return _hatStates[hat];
    }
    return HatCentered;
}

// =============================================================================
// Ball Control
// =============================================================================

bool MockJoystick::setBall(int ball, int dx, int dy)
{
    return setVirtualBall(ball, dx, dy);
}

// =============================================================================
// Touchpad Control
// =============================================================================

bool MockJoystick::setTouchpad(int touchpad, int finger, bool down, float x, float y, float pressure)
{
    return setVirtualTouchpad(touchpad, finger, down, x, y, pressure);
}

// =============================================================================
// Multi-Input Methods
// =============================================================================

bool MockJoystick::pressButtons(const QVector<int>& buttons)
{
    bool success = true;
    for (int button : buttons) {
        if (!pressButton(button)) {
            success = false;
        }
    }
    return success;
}

bool MockJoystick::releaseButtons(const QVector<int>& buttons)
{
    bool success = true;
    for (int button : buttons) {
        if (!releaseButton(button)) {
            success = false;
        }
    }
    return success;
}

bool MockJoystick::setAxes(const QVector<QPair<int, int>>& axisValues)
{
    bool success = true;
    for (const auto& pair : axisValues) {
        if (!setAxis(pair.first, pair.second)) {
            success = false;
        }
    }
    return success;
}

// =============================================================================
// Input Sequences
// =============================================================================

bool MockJoystick::tapButton(int button)
{
    bool pressed = pressButton(button);
    bool released = releaseButton(button);
    return pressed && released;
}

bool MockJoystick::deflectLeftStick(int x, int y)
{
    bool set = setLeftStick(x, y);
    bool center = setLeftStick(0, 0);
    return set && center;
}

bool MockJoystick::deflectRightStick(int x, int y)
{
    bool set = setRightStick(x, y);
    bool center = setRightStick(0, 0);
    return set && center;
}

bool MockJoystick::pullLeftTrigger()
{
    bool pull = setLeftTrigger(AxisMax);
    bool release = setLeftTrigger(0);
    return pull && release;
}

bool MockJoystick::pullRightTrigger()
{
    bool pull = setRightTrigger(AxisMax);
    bool release = setRightTrigger(0);
    return pull && release;
}

bool MockJoystick::tapDPad(HatDirection direction)
{
    bool set = setHat(0, direction);
    bool center = centerHat(0);
    return set && center;
}

// =============================================================================
// Reset
// =============================================================================

void MockJoystick::reset()
{
    centerAllAxes();
    releaseAllButtons();
    for (int i = 0; i < _mockHatCount; ++i) {
        centerHat(i);
    }
}
