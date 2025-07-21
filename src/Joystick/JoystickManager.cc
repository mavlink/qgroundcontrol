/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickManager.h"
#include "Joystick.h"
#if defined(QGC_SDL_JOYSTICK)
    #include <SDL3/SDL.h>
    #include "JoystickSDL.h"
#elif defined(Q_OS_ANDROID)
    #include "JoystickAndroid.h"
#endif
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QMutexLocker>
#include <QtCore/QSettings>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>

QGC_LOGGING_CATEGORY(JoystickManagerLog, "qgc.joystick.joystickmanager")

Q_APPLICATION_STATIC(JoystickManager, _joystickManager);

JoystickManager::JoystickManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(JoystickManagerLog) << this;

    _joystickCheckTimer.setInterval(kTimerInterval);
    _joystickCheckTimer.setSingleShot(false);
    (void) connect(&_joystickCheckTimer, &QTimer::timeout, this, &JoystickManager::_updateAvailableJoysticks);
}

JoystickManager::~JoystickManager()
{
    QMutexLocker locker(&_mutex);
    for (auto [name, js] : _name2JoystickMap.asKeyValueRange()) {
        qCDebug(JoystickManagerLog) << "Releasing joystick:" << name;
        if (js) {
            js->stopPolling();
            (void) js->wait(kTimeout);
            delete js;
        }
    }

    qCDebug(JoystickManagerLog) << this;
}

JoystickManager *JoystickManager::instance()
{
    return _joystickManager();
}

void JoystickManager::registerQmlTypes()
{
    (void) qmlRegisterUncreatableType<JoystickManager>("QGroundControl.JoystickManager", 1, 0, "JoystickManager", "Reference only");
    (void) qmlRegisterUncreatableType<Joystick>("QGroundControl.JoystickManager", 1, 0, "Joystick", "Reference only");
}

void JoystickManager::init()
{
#ifdef QGC_SDL_JOYSTICK
    if (!JoystickSDL::init()) {
        return;
    }
    _setActiveJoystickFromSettings();
#elif defined(Q_OS_ANDROID)
    if (!JoystickAndroid::init()) {
        return;
    }
    (void) connect(this, &JoystickManager::updateAvailableJoysticksSignal, this, [this]() {
        QMutexLocker locker(&_mutex);
        _joystickCheckTimerCounter = 5;
        _joystickCheckTimer.start();
    });
#endif

    _joystickCheckTimerCounter = 5;
    _joystickCheckTimer.start();
}

void JoystickManager::_setActiveJoystickFromSettings()
{
    QMap<QString, Joystick*> newMap;

#ifdef QGC_SDL_JOYSTICK
    newMap = JoystickSDL::discover();
#elif defined(Q_OS_ANDROID)
    newMap = JoystickAndroid::discover();
#endif

    bool activeRemoved = false;
    {
        QMutexLocker locker(&_mutex);

        if (_activeJoystick && !newMap.contains(_activeJoystick->name())) {
            qCDebug(JoystickManagerLog) << "Active joystick removed";
            activeRemoved = true;
        }

        // Clean up unplugged sticks
        for (auto [name, js] : _name2JoystickMap.asKeyValueRange()) {
            if (!newMap.contains(name)) {
                qCDebug(JoystickManagerLog) << "Releasing joystick:" << name;
                js->stopPolling();
                (void) js->wait(kTimeout);
                js->deleteLater();
            }
        }

        _name2JoystickMap = newMap;
    }

    emit availableJoysticksChanged();

    Joystick *desiredActive = nullptr;

    if (!_name2JoystickMap.isEmpty()) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);
        QString name = settings.value(_settingsKeyActiveJoystick).toString();
        if (name.isEmpty() || !_name2JoystickMap.contains(name)) {
            name = _name2JoystickMap.first()->name();
        }
        desiredActive = _name2JoystickMap.value(name, nullptr);
        settings.endGroup();
    }

    if (activeRemoved) {
        setActiveJoystick(nullptr);
    }
    setActiveJoystick(desiredActive);
}

Joystick *JoystickManager::activeJoystick()
{
    QMutexLocker locker(&_mutex);
    return _activeJoystick;
}

void JoystickManager::setActiveJoystick(Joystick *joystick)
{
    Joystick *activeJoystick = nullptr;
    QString activeName;

    {
        QMutexLocker locker(&_mutex);

        if (joystick && !_name2JoystickMap.contains(joystick->name())) {
            qCWarning(JoystickManagerLog) << "Set active not in map" << joystick->name();
            return;
        }

        if (_activeJoystick == joystick) {
            return;
        }

        if (_activeJoystick) {
            _activeJoystick->stopPolling();
        }

        _activeJoystick = joystick;
        activeJoystick = _activeJoystick;
        activeName = _activeJoystick ? _activeJoystick->name() : QString();

        if (_activeJoystick) {
            qCDebug(JoystickManagerLog) << "Set active:" << _activeJoystick->name();

            QSettings settings;
            settings.beginGroup(_settingsGroup);
            settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());
            settings.endGroup();
        }
    }

    emit activeJoystickChanged(activeJoystick);
    emit activeJoystickNameChanged(activeName);
}

QVariantList JoystickManager::joysticks()
{
    QVariantList list;
    QMutexLocker locker(&_mutex);
    for (auto js : _name2JoystickMap) {
        list += QVariant::fromValue(js);
    }
    return list;
}

QStringList JoystickManager::joystickNames()
{
    QMutexLocker locker(&_mutex);
    return _name2JoystickMap.keys();
}

QString JoystickManager::activeJoystickName()
{
    QMutexLocker locker(&_mutex);
    return (_activeJoystick ? _activeJoystick->name() : QString());
}

bool JoystickManager::setActiveJoystickName(const QString &name)
{
    Joystick *target = nullptr;
    {
        QMutexLocker locker(&_mutex);
        if (_name2JoystickMap.contains(name)) {
            target = _name2JoystickMap[name];
        } else {
            qCWarning(JoystickManagerLog) << "Set active not in map" << name;
            return false;
        }
    }
    setActiveJoystick(target);
    return true;
}

void JoystickManager::_updateAvailableJoysticks()
{
#ifdef QGC_SDL_JOYSTICK
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            qCDebug(JoystickManagerLog) << "Gamepad added:" << event.gdevice.which;
            _setActiveJoystickFromSettings();
            break;
        case SDL_EVENT_JOYSTICK_ADDED:
            qCDebug(JoystickManagerLog) << "Joystick added:" << event.jdevice.which;
            _setActiveJoystickFromSettings();
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            qCDebug(JoystickManagerLog) << "Gamepad removed:" << event.gdevice.which;
            _setActiveJoystickFromSettings();
            break;
        case SDL_EVENT_JOYSTICK_REMOVED:
            qCDebug(JoystickManagerLog) << "Joystick removed:" << event.jdevice.which;
            _setActiveJoystickFromSettings();
            break;
        case SDL_EVENT_QUIT:
            qCWarning(JoystickManagerLog) << "SDL quit event received";
            break;
        default:
            break;
        }
    }
#elif defined(Q_OS_ANDROID)
    _joystickCheckTimerCounter--;
    _setActiveJoystickFromSettings();
    if (_joystickCheckTimerCounter <= 0) {
        _joystickCheckTimer.stop();
    }
#endif
}
