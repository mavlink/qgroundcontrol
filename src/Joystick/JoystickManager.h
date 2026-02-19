#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
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

    /// Number of connected joysticks
    Q_PROPERTY(int joystickCount READ joystickCount NOTIFY availableJoystickNamesChanged)

#ifdef QGC_UNITTEST_BUILD
    friend class JoystickManagerTest;
#endif

public:
    explicit JoystickManager(QObject *parent = nullptr);
    ~JoystickManager() override;

    static JoystickManager *instance();

    QStringList availableJoystickNames() const { return _name2JoystickMap.keys(); }
    int joystickCount() const { return _name2JoystickMap.count(); }

    Joystick *activeJoystick();

    Q_INVOKABLE bool joystickEnabledForVehicle(Vehicle *vehicle) const;
    Q_INVOKABLE void setJoystickEnabledForVehicle(Vehicle *vehicle, bool enabled);

    /// Get all joysticks in a linked group
    /// Returns empty list if groupId is empty or no matches found
    Q_INVOKABLE QStringList linkedGroupMembers(const QString &groupId) const;

    /// Get joystick by name (returns nullptr if not found)
    Q_INVOKABLE Joystick *joystickByName(const QString &name) const;

signals:
    void activeJoystickChanged(Joystick *joystick);
    void availableJoystickNamesChanged();
    void joystickEnabledChanged();

public slots:
    void init();

private slots:
    /// Checks for added or removed joysticks and updates the internal map accordingly
    void _checkForAddedOrRemovedJoysticks();
    void _activeVehicleChanged(Vehicle *activeVehicle);
    void _setActiveJoystickByName(const QString &name);

    // SDL event handlers (called via QMetaObject::invokeMethod from sdlEventWatcher)
    void _handleUpdateComplete(int instanceId);
    void _handleBatteryUpdated(int instanceId);
    void _handleGamepadRemapped(int instanceId);
    void _handleTouchpadEvent(int instanceId, int touchpad, int finger, bool down, float x, float y, float pressure);
    void _handleSensorUpdate(int instanceId, int sensor, float x, float y, float z);

private:
    void _setActiveJoystickFromSettings();
    void _setActiveJoystick(Joystick *joystick);
    Joystick *_findJoystickByInstanceId(int instanceId);
    void _updatePollingTimer();

    JoystickManagerSettings *_joystickManagerSettings = nullptr;
    Joystick *_activeJoystick = nullptr;
    QMap<QString, Joystick*> _name2JoystickMap;
    QTimer _pollTimer;
};
