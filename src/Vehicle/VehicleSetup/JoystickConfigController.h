#pragma once


#include "RemoteControlCalibrationController.h"
#include "Joystick.h"


class JoystickConfigController : public RemoteControlCalibrationController
{
    Q_OBJECT
    QML_ELEMENT

public:
    JoystickConfigController(QObject *parent = nullptr);
    ~JoystickConfigController();

    Q_PROPERTY(Joystick* joystick READ joystick WRITE _setJoystick NOTIFY joystickChanged REQUIRED)

    Joystick* joystick() const { return _joystick; }

    // Overrides from RemoteControlCalibrationController
    void start() final override;

signals:
   void joystickChanged(Joystick* joystick);

private slots:
    void _setJoystick(Joystick* joystick);

private:
    // Overrides from RemoteControlCalibrationController
    void _saveStoredCalibrationValues() override;
    void _readStoredCalibrationValues() override;

    Joystick* _joystick = nullptr;
};
