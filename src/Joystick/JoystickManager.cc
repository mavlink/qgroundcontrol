#include "JoystickManager.h"
#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "JoystickManagerSettings.h"
#if defined(QGC_SDL_JOYSTICK)
    #include "JoystickSDL.h"
    using JoystickBackend = JoystickSDL;
#elif defined(Q_OS_ANDROID)
    #include "JoystickAndroid.h"
    #include "AndroidEvents.h"
    using JoystickBackend = JoystickAndroid;
#endif
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(JoystickManagerLog, "Joystick.JoystickManager")

Q_APPLICATION_STATIC(JoystickManager, _joystickManager);

JoystickManager::JoystickManager(QObject *parent)
    : QObject(parent)
    , _joystickManagerSettings(SettingsManager::instance()->joystickManagerSettings())
{
    qCDebug(JoystickManagerLog) << this;

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
    });

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &JoystickManager::_activeVehicleChanged);
}

JoystickManager::~JoystickManager()
{
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

#ifdef Q_OS_ANDROID
    (void) connect(this, &JoystickManager::updateAvailableJoysticks, this, &JoystickManager::_checkForAddedOrRemovedJoysticks);
    (void) connect(AndroidEvents::instance(), &AndroidEvents::resumed, this, &JoystickManager::_checkForAddedOrRemovedJoysticks);
#endif

    _checkForAddedOrRemovedJoysticks();
}

void JoystickManager::_checkForAddedOrRemovedJoysticks()
{
    QMap<QString, Joystick*> newJoystickMap = JoystickBackend::discover();

    if (_activeJoystick && !newJoystickMap.contains(_activeJoystick->name())) {
        qCDebug(JoystickManagerLog) << "Active joystick removed";
        _setActiveJoystick(nullptr);
    }

    // Check to see if our current mapping contains any joysticks that are not in the new mapping
    // If so, those joysticks have been unplugged, and need to be cleaned up
    for (QMap<QString, Joystick*>::key_value_iterator it = _name2JoystickMap.keyValueBegin(); it != _name2JoystickMap.keyValueEnd(); ++it) {
        if (!newJoystickMap.contains(it->first)) {
            auto key = it->first;
            auto joystick = it->second;
            qCDebug(JoystickManagerLog) << "Releasing joystick:" << key;
            joystick->_stopAllPolling();
            joystick->stop();
            joystick->deleteLater();
        }
    }

    for (const auto &key : newJoystickMap.keys()) {
        if (!_name2JoystickMap.contains(key)) {
            qCDebug(JoystickManagerLog) << "New joystick added:" << key;
        }
    }

    _name2JoystickMap = newJoystickMap;

    _setActiveJoystickFromSettings();

    emit availableJoystickNamesChanged();
}

void JoystickManager::_setActiveJoystickFromSettings()
{
    QString activeJoystickName = _joystickManagerSettings->activeJoystickName()->rawValue().toString();

    if (activeJoystickName.isEmpty()) {
        if (_name2JoystickMap.isEmpty()) {
            return;
        }

        activeJoystickName = _name2JoystickMap.first()->name();
        _joystickManagerSettings->activeJoystickName()->setRawValue(activeJoystickName);
        qCDebug(JoystickManagerLog) << "No active joystick specified, using first available:" << activeJoystickName;
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
    Q_UNUSED(instanceId);
    // SDL event watcher notifies when joystick update cycle completes
    // Currently unused - placeholder for future features like input latency monitoring
}
