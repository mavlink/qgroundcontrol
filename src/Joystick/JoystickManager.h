#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(JoystickManagerLog)

class Joystick;
class JoystickManagerSettings;
class Vehicle;

class JoystickManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Joystick.h")

    /// List of available joystick names
    ///     Note: The active joystick name may be not in this list if the joystick is not currently connected
    Q_PROPERTY(QStringList  availableJoystickNames READ availableJoystickNames NOTIFY availableJoystickNamesChanged)

    /// Currently active joystick
    Q_PROPERTY(Joystick *activeJoystick READ activeJoystick NOTIFY activeJoystickChanged)

public:
    explicit JoystickManager(QObject *parent = nullptr);
    ~JoystickManager();

    static JoystickManager *instance();

    QStringList availableJoystickNames() const { return _name2JoystickMap.keys(); }

    Joystick *activeJoystick();

    Q_INVOKABLE bool joystickEnabledForVehicle(Vehicle *vehicle) const;
    Q_INVOKABLE void setJoystickEnabledForVehicle(Vehicle *vehicle, bool enabled);

signals:
    void activeJoystickChanged(Joystick *joystick);
    void availableJoystickNamesChanged();
#if defined(Q_OS_ANDROID)
    void _updateAvailableJoysticksSignal();
#endif

public slots:
    void init();

private slots:
    void _updateAvailableJoysticks();
    void _activeVehicleChanged(Vehicle *activeVehicle);
    void _setActiveJoystickByName(const QString &name);

private:
    void _checkForAddedOrRemovedJoysticks();
    void _setActiveJoystickFromSettings();
    void _setActiveJoystick(Joystick *joystick);

    JoystickManagerSettings *_joystickManagerSettings = nullptr;
    Joystick *_activeJoystick = nullptr;
    QMap<QString, Joystick*> _name2JoystickMap;

    int _checkForAddedOrRemovedJoysticksTimerCounter = 0;
    QTimer _checkForAddedOrRemovedJoysticksTimer;

    static constexpr int kTimeout = 1000;
    static constexpr const char *_settingsGroup = "JoystickManager";
    static constexpr const char *_settingsKeyActiveNameJoystick = "ActiveJoystickName";
};
