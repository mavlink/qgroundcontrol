#include "JoystickManager.h"
#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "JoystickManagerSettings.h"
#include "JoystickSDL.h"
#include "SDLJoystick.h"
#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include "AndroidEvents.h"
#endif

using JoystickBackend = JoystickSDL;

#include <QtCore/QApplicationStatic>
#include <QtCore/QSettings>
#include <QtGui/QVector3D>

QGC_LOGGING_CATEGORY(JoystickManagerLog, "Joystick.JoystickManager")

Q_APPLICATION_STATIC(JoystickManager, _joystickManager);

JoystickManager::JoystickManager(QObject *parent)
    : QObject(parent)
    , _joystickManagerSettings(SettingsManager::instance()->joystickManagerSettings())
{
    qCDebug(JoystickManagerLog) << this;

    // SDL_PumpEvents() must be called from main thread for device add/remove events
    _pollTimer.setInterval(500);
    (void) connect(&_pollTimer, &QTimer::timeout, this, []() {
        SDLJoystick::pumpEvents();
    });

    (void) connect(_joystickManagerSettings->activeJoystickName(), &Fact::rawValueChanged, this, [this](const QVariant &value) {
        QString joystickName = value.toString();
        _setActiveJoystickByName(joystickName);
    });

    (void) connect(_joystickManagerSettings->joystickEnabledVehiclesIds(), &Fact::rawValueChanged, this, [this](const QVariant &value) {
        Q_UNUSED(value);
        auto multiVehicleManager = MultiVehicleManager::instance();
        auto activeVehicle = multiVehicleManager->activeVehicle();
        if (activeVehicle && _activeJoystick) {
            if (joystickEnabledForVehicle(activeVehicle)) {
                _activeJoystick->_startPollingForVehicle(*activeVehicle);
            }
        }
        emit joystickEnabledChanged();
    });

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &JoystickManager::_activeVehicleChanged);

#ifdef Q_OS_ANDROID
    // Re-scan for joysticks when app resumes - devices may have connected/disconnected while backgrounded
    (void) connect(AndroidEvents::instance(), &AndroidEvents::resumed, this, &JoystickManager::_checkForAddedOrRemovedJoysticks);
#endif
}

JoystickManager::~JoystickManager()
{
    _pollTimer.stop();

    for (QMap<QString, Joystick*>::key_value_iterator it = _name2JoystickMap.keyValueBegin(); it != _name2JoystickMap.keyValueEnd(); ++it) {
        qCDebug(JoystickManagerLog) << "Releasing joystick:" << it->first;
        it->second->stop();
        delete it->second;
    }

    JoystickBackend::shutdown();

    qCDebug(JoystickManagerLog) << this;
}

JoystickManager *JoystickManager::instance()
{
    return _joystickManager();
}

void JoystickManager::init()
{
    if (!JoystickBackend::init()) {
        return;
    }

    _checkForAddedOrRemovedJoysticks();
    _updatePollingTimer();
}

void JoystickManager::_checkForAddedOrRemovedJoysticks()
{
    qCDebug(JoystickManagerLog) << "Checking for added/removed joysticks, current count:" << _name2JoystickMap.size();

    QMap<QString, Joystick*> newJoystickMap = JoystickBackend::discover();

    qCDebug(JoystickManagerLog) << "Discovery returned" << newJoystickMap.size() << "joysticks";

    if (_activeJoystick && !newJoystickMap.contains(_activeJoystick->name())) {
        qCInfo(JoystickManagerLog) << "Active joystick removed:" << _activeJoystick->name();
        _setActiveJoystick(nullptr);
    }

    // Check to see if our current mapping contains any joysticks that are not in the new mapping
    // If so, those joysticks have been unplugged, and need to be cleaned up
    for (QMap<QString, Joystick*>::key_value_iterator it = _name2JoystickMap.keyValueBegin(); it != _name2JoystickMap.keyValueEnd(); ++it) {
        if (!newJoystickMap.contains(it->first)) {
            auto key = it->first;
            auto joystick = it->second;
            qCInfo(JoystickManagerLog) << "Joystick disconnected, releasing:" << key;
            joystick->_stopAllPolling();
            joystick->stop();
            joystick->deleteLater();
        }
    }

    for (const auto &key : newJoystickMap.keys()) {
        if (!_name2JoystickMap.contains(key)) {
            qCInfo(JoystickManagerLog) << "New joystick added:" << key;
        }
    }

    _name2JoystickMap = newJoystickMap;

    _setActiveJoystickFromSettings();
    _updatePollingTimer();

    emit availableJoystickNamesChanged();
}

void JoystickManager::_setActiveJoystickFromSettings()
{
    QString activeJoystickName = _joystickManagerSettings->activeJoystickName()->rawValue().toString();

    // Auto-select first available joystick if:
    // - No joystick name is saved in settings, OR
    // - Saved joystick name doesn't match any currently connected joystick
    if (activeJoystickName.isEmpty() || !_name2JoystickMap.contains(activeJoystickName)) {
        if (_name2JoystickMap.isEmpty()) {
            return;
        }

        activeJoystickName = _name2JoystickMap.first()->name();
        _joystickManagerSettings->activeJoystickName()->setRawValue(activeJoystickName);
        qCDebug(JoystickManagerLog) << "Auto-selecting first available joystick:" << activeJoystickName;
    }

    _setActiveJoystickByName(activeJoystickName);
}

Joystick *JoystickManager::activeJoystick()
{
    return _activeJoystick;
}

void JoystickManager::_setActiveJoystick(Joystick *newActiveJoystick)
{
    if (newActiveJoystick && !_name2JoystickMap.contains(newActiveJoystick->name())) {
        qCWarning(JoystickManagerLog) << "Set active not in map" << newActiveJoystick->name();
        return;
    }

    if (_activeJoystick == newActiveJoystick) {
        return;
    }

    if (_activeJoystick) {
        _activeJoystick->_stopAllPolling();
        _activeJoystick = nullptr;
        emit activeJoystickChanged(nullptr);
    }

    if (newActiveJoystick) {
        qCDebug(JoystickManagerLog) << "Set active:" << newActiveJoystick->name();

        _activeJoystick = newActiveJoystick;

        auto multiVehicleManager = MultiVehicleManager::instance();
        auto activeVehicle = multiVehicleManager->activeVehicle();

        if (activeVehicle && joystickEnabledForVehicle(activeVehicle)) {
            _activeJoystick->_startPollingForVehicle(*activeVehicle);
        }

        emit activeJoystickChanged(_activeJoystick);
    }
}

void JoystickManager::_setActiveJoystickByName(const QString &name)
{
    if (name.isEmpty() || !_name2JoystickMap.contains(name)) {
        _setActiveJoystick(nullptr);
        return;
    }

    _setActiveJoystick(_name2JoystickMap[name]);
}

void JoystickManager::_activeVehicleChanged(Vehicle *activeVehicle)
{
    if (!_activeJoystick) {
        return;
    }

    _activeJoystick->_stopAllPolling();

    if (activeVehicle && joystickEnabledForVehicle(activeVehicle)) {
        _activeJoystick->_startPollingForVehicle(*activeVehicle);
    }
}

bool JoystickManager::joystickEnabledForVehicle(Vehicle *vehicle) const
{
    const QStringList vehicleIds = _joystickManagerSettings->joystickEnabledVehiclesIds()->rawValue().toString().split(",", Qt::SkipEmptyParts);
    return vehicleIds.contains(QString::number(vehicle->id()));
}

void JoystickManager::setJoystickEnabledForVehicle(Vehicle *vehicle, bool enabled)
{
    QStringList vehicleIds = _joystickManagerSettings->joystickEnabledVehiclesIds()->rawValue().toString().split(",", Qt::SkipEmptyParts);
    const QString vehicleIdStr = QString::number(vehicle->id());

    if (enabled) {
        if (!vehicleIds.contains(vehicleIdStr)) {
            vehicleIds.append(vehicleIdStr);
        }
    } else {
        vehicleIds.removeAll(vehicleIdStr);
    }

    _joystickManagerSettings->joystickEnabledVehiclesIds()->setRawValue(vehicleIds.join(","));
}

void JoystickManager::_handleUpdateComplete(int instanceId)
{
    Joystick *joystick = _findJoystickByInstanceId(instanceId);
    if (joystick) {
        emit joystick->updateComplete();
    }
}

void JoystickManager::_handleBatteryUpdated(int instanceId)
{
    Joystick *joystick = _findJoystickByInstanceId(instanceId);
    if (joystick) {
        qCDebug(JoystickManagerLog) << "Battery updated for" << joystick->name();
        emit joystick->batteryStateChanged();
    }
}

void JoystickManager::_handleGamepadRemapped(int instanceId)
{
    Joystick *joystick = _findJoystickByInstanceId(instanceId);
    if (joystick) {
        qCDebug(JoystickManagerLog) << "Gamepad remapped:" << joystick->name();
        emit joystick->mappingRemapped();
    }
}

void JoystickManager::_handleTouchpadEvent(int instanceId, int touchpad, int finger, bool down, float x, float y, float pressure)
{
    Joystick *joystick = _findJoystickByInstanceId(instanceId);
    if (joystick) {
        emit joystick->touchpadEvent(touchpad, finger, down, x, y, pressure);
    }
}

void JoystickManager::_handleSensorUpdate(int instanceId, int sensor, float x, float y, float z)
{
    Joystick *joystick = _findJoystickByInstanceId(instanceId);
    if (joystick) {
        auto *sdlJoystick = qobject_cast<JoystickBackend*>(joystick);
        const QVector3D data(x, y, z);
        // SDL_SENSOR_ACCEL = 1, SDL_SENSOR_GYRO = 2
        if (sensor == 1 || sensor == 4 || sensor == 6) {  // ACCEL, ACCEL_L, ACCEL_R
            if (sdlJoystick) {
                sdlJoystick->updateCachedAccelData(data);
            } else {
                emit joystick->accelerometerDataUpdated(data);
            }
        } else if (sensor == 2 || sensor == 5 || sensor == 7) {  // GYRO, GYRO_L, GYRO_R
            if (sdlJoystick) {
                sdlJoystick->updateCachedGyroData(data);
            } else {
                emit joystick->gyroscopeDataUpdated(data);
            }
        }
    }
}

Joystick *JoystickManager::_findJoystickByInstanceId(int instanceId)
{
    for (Joystick *joystick : _name2JoystickMap) {
        if (auto *sdlJoystick = qobject_cast<JoystickBackend*>(joystick)) {
            if (sdlJoystick->instanceId() == instanceId) {
                return joystick;
            }
        }
    }
    return nullptr;
}

QStringList JoystickManager::linkedGroupMembers(const QString &groupId) const
{
    QStringList members;
    if (groupId.isEmpty()) {
        return members;
    }

    for (auto it = _name2JoystickMap.constBegin(); it != _name2JoystickMap.constEnd(); ++it) {
        if (it.value()->linkedGroupId() == groupId) {
            members.append(it.key());
        }
    }
    return members;
}

Joystick *JoystickManager::joystickByName(const QString &name) const
{
    return _name2JoystickMap.value(name, nullptr);
}

void JoystickManager::_updatePollingTimer()
{
    if (!_pollTimer.isActive()) {
        qCDebug(JoystickManagerLog) << "Starting SDL event pump timer";
        _pollTimer.start();
    }
}

