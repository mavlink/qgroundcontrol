#include "MockJoystick.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(JoystickSDLLog)

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

} // namespace

MockJoystick *MockJoystick::create(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent)
{
    const int instanceId = JoystickSDL::createVirtualJoystick(name, axisCount, buttonCount, hatCount);
    if (instanceId < 0) {
        qCWarning(JoystickSDLLog) << "MockJoystick: Failed to create virtual joystick" << name;
        return nullptr;
    }

    auto *mock = new MockJoystick(name, axisCount, buttonCount, hatCount, instanceId, parent);
    if (!mock->_open()) {
        qCWarning(JoystickSDLLog) << "MockJoystick: Failed to open virtual joystick" << name;
        delete mock;
        return nullptr;
    }
    return mock;
}

MockJoystick::MockJoystick(const QString &name, int axisCount, int buttonCount, int hatCount, int virtualInstanceId, QObject *parent)
    : JoystickSDL(name,
                  QList<int>(),             // No gamepad axes (virtual joystick isn't recognized as gamepad)
                  makeAxisList(axisCount),  // All axes are non-gamepad axes
                  buttonCount,
                  hatCount,
                  virtualInstanceId,
                  parent)
    , _virtualInstanceId(virtualInstanceId)
{
}

MockJoystick::~MockJoystick()
{
    _close();
    if (_virtualInstanceId >= 0) {
        JoystickSDL::destroyVirtualJoystick(_virtualInstanceId);
    }
}

bool MockJoystick::isValid() const
{
    return (_virtualInstanceId >= 0) && (_sdlJoystick != nullptr);
}

// -------------------------------------------------------------------------
// Axis Control
// -------------------------------------------------------------------------

bool MockJoystick::setAxis(int axis, int value)
{
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
    for (int i = 0; i < _axisCount; ++i) {
        setAxis(i, 0);
    }
}

// -------------------------------------------------------------------------
// Button Control
// -------------------------------------------------------------------------

bool MockJoystick::setButton(int button, bool pressed)
{
    return setVirtualButton(button, pressed);
}

void MockJoystick::releaseAllButtons()
{
    for (int i = 0; i < _buttonCount; ++i) {
        setButton(i, false);
    }
}

// -------------------------------------------------------------------------
// Hat Control
// -------------------------------------------------------------------------

bool MockJoystick::setHat(int hat, HatDirection direction)
{
    return setVirtualHat(hat, static_cast<quint8>(direction));
}

// -------------------------------------------------------------------------
// Convenience Methods
// -------------------------------------------------------------------------

void MockJoystick::reset()
{
    centerAllAxes();
    releaseAllButtons();
    for (int i = 0; i < _hatCount; ++i) {
        centerHat(i);
    }
}
