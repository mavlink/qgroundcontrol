#pragma once


#include "VehicleComponent.h"


class Joystick;

class JoystickComponent : public VehicleComponent
{
    Q_OBJECT
    Q_MOC_INCLUDE("Joystick.h")

    Q_PROPERTY(Joystick *activeJoystick READ activeJoystick NOTIFY activeJoystickChanged)
    Q_PROPERTY(QString joystickStatusText READ joystickStatusText NOTIFY joystickStatusChanged)
    Q_PROPERTY(QString joystickFeaturesText READ joystickFeaturesText NOTIFY activeJoystickChanged)
    Q_PROPERTY(bool hasActiveJoystick READ hasActiveJoystick NOTIFY activeJoystickChanged)

public:
    explicit JoystickComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent = nullptr);
    ~JoystickComponent() override;

    QStringList setupCompleteChangedTriggerList() const final { return QStringList(); }
    QString name() const final { return _name; }
    QString description() const final;
    QString iconResource() const final { return QStringLiteral("/qmlimages/Joystick.png"); }
    bool requiresSetup() const final { return false; }
    bool setupComplete() const final;
    QUrl setupSource() const final;
    QUrl summaryQmlSource() const final;

    Joystick *activeJoystick() const { return _activeJoystick; }
    bool hasActiveJoystick() const { return _activeJoystick != nullptr; }
    QString joystickStatusText() const;
    QString joystickFeaturesText() const;

signals:
    void activeJoystickChanged();
    void joystickStatusChanged();

private slots:
    void _activeJoystickChanged(Joystick *joystick);
    void _joystickCalibrationChanged();
    void _joystickBatteryChanged();

private:
    const QString _name;
    Joystick *_activeJoystick = nullptr;
};
