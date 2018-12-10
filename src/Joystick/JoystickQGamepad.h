#ifndef JOYSTICKQGAMEPAD_H
#define JOYSTICKQGAMEPAD_H

#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"

#include <QtGamepad/QGamepad>

class JoystickQGamepad : public Joystick
{
public:
    JoystickQGamepad(int id, QString name, MultiVehicleManager *multiVehicleManager, QObject *_parent = nullptr);

    ~JoystickQGamepad();

private:
    QGamepad *gamepad;
    bool *btnValue;
    int *axisValue;

    virtual bool _open();
    virtual void _close();
    virtual bool _update();

    virtual bool _getButton(int i);
    virtual int _getAxis(int i);
    virtual uint8_t _getHat(int hat, int i);
};

#endif // JOYSTICKQGAMEPAD_H
