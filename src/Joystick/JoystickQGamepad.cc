#include "JoystickQGamepad.h"

#include "QGCApplication.h"

#include <QQmlEngine>

#define NUM_AXIS 6
#define NUM_BUTTONS 19

JoystickQGamepad::JoystickQGamepad(int id, QString name, MultiVehicleManager *multiVehicleManager, QObject *_parent)
    : Joystick(name, NUM_AXIS, NUM_BUTTONS, 0, multiVehicleManager)
{
    setParent(_parent);
    axisValue = new int[NUM_AXIS];
    btnValue = new bool[NUM_BUTTONS];

    gamepad = new QGamepad(id, this);
}

JoystickQGamepad::~JoystickQGamepad()
{
    if (gamepad != nullptr) {
        delete gamepad;
        gamepad = nullptr;
    }
}

bool JoystickQGamepad::_open(void)
{
    return true;
}

void JoystickQGamepad::_close(void)
{
}

bool JoystickQGamepad::_update(void)
{
    return true;
}

bool JoystickQGamepad::_getButton(int i)
{
    switch (i) {
        case 0:
            return gamepad->buttonA();
        case 1:
            return gamepad->buttonB();
        case 2:
            return gamepad->buttonCenter();
        case 3:
            return gamepad->buttonDown();
        case 4:
            return gamepad->buttonGuide();
        case 5:
            return gamepad->buttonL1();
        case 6:
            return gamepad->buttonL2() >= 0.1;
        case 7:
            return gamepad->buttonL3();
        case 8:
            return gamepad->buttonLeft();
        case 9:
            return gamepad->buttonR1();
        case 10:
            return gamepad->buttonR2() >= 0.1;
        case 11:
            return gamepad->buttonR3();
        case 12:
            return gamepad->buttonRight();
        case 13:
            return gamepad->buttonSelect();
        case 14:
            return gamepad->buttonStart();
        case 15:
            return gamepad->buttonUp();
        case 16:
            return gamepad->buttonDown();
        case 17:
            return gamepad->buttonX();
        case 18:
            return gamepad->buttonY();
    }

    return false;
}

inline int getAxisValue(double v)
{
    return static_cast<int>(round(v * 32767));
}

int JoystickQGamepad::_getAxis(int i)
{
    switch (i) {
        case 0:
            return getAxisValue(gamepad->axisLeftX());
        case 1:
            return getAxisValue(gamepad->axisLeftY());
        case 2:
            return getAxisValue(gamepad->axisRightX());
        case 3:
            return getAxisValue(gamepad->axisRightY());
        case 4:
            return getAxisValue(gamepad->buttonL2());
        case 5:
            return getAxisValue(gamepad->buttonR2());
    }

    return 0;
}

uint8_t JoystickQGamepad::_getHat(int hat, int i)
{
    Q_UNUSED(hat);
    Q_UNUSED(i);

    return 0;
}
