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
    #include "JoystickSDL.h"
    #include <SDL.h>
#elif defined(Q_OS_ANDROID)
    #include "JoystickAndroid.h"
#endif
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>

QGC_LOGGING_CATEGORY(JoystickManagerLog, "qgc.joystick.joystickmanager")

Q_APPLICATION_STATIC(JoystickManager, _joystickManager);

JoystickManager::JoystickManager(QObject *parent)
    : QObject(parent)
    , _joystickCheckTimer(new QTimer(this))
{
    // qCDebug(JoystickManagerLog) << Q_FUNC_INFO << this;



    _joystickCheckTimer->setInterval(kTimerInterval);
    _joystickCheckTimer->setSingleShot(false);
}

JoystickManager::~JoystickManager()
{
    for (QMap<QString, Joystick*>::key_value_iterator it = _name2JoystickMap.keyValueBegin(); it != _name2JoystickMap.keyValueEnd(); ++it) {
        qCDebug(JoystickManagerLog) << "Releasing joystick:" << it->first;
        it->second->stop();
        delete it->second;
    }

    // qCDebug(JoystickManagerLog) << Q_FUNC_INFO << this;
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
        _joystickCheckTimerCounter = 5;
        _joystickCheckTimer->start();
    });
#endif

    (void) connect(_joystickCheckTimer, &QTimer::timeout, this, &JoystickManager::_updateAvailableJoysticks);
    _joystickCheckTimerCounter = 5;
    _joystickCheckTimer->start();
}

void JoystickManager::_setActiveJoystickFromSettings()
{
    QMap<QString, Joystick*> newMap;

#ifdef QGC_SDL_JOYSTICK
    newMap = JoystickSDL::discover();
#elif defined(Q_OS_ANDROID)
    newMap = JoystickAndroid::discover();
#endif

    if (_activeJoystick && !newMap.contains(_activeJoystick->name())) {
        qCDebug(JoystickManagerLog) << "Active joystick removed";
        setActiveJoystick(nullptr);
    }

    // Check to see if our current mapping contains any joysticks that are not in the new mapping
    // If so, those joysticks have been unplugged, and need to be cleaned up
    for (QMap<QString, Joystick*>::key_value_iterator it = _name2JoystickMap.keyValueBegin(); it != _name2JoystickMap.keyValueEnd(); ++it) {
        if (!newMap.contains(it->first)) {
            qCDebug(JoystickManagerLog) << "Releasing joystick:" << it->first;
            it->second->stopPolling();
            (void) it->second->wait(kTimeout);
            it->second->deleteLater();
        }
    }

    _name2JoystickMap = newMap;
    emit availableJoysticksChanged();

    if (_name2JoystickMap.isEmpty()) {
        setActiveJoystick(nullptr);
        return;
    }

    QSettings settings;
    settings.beginGroup(_settingsGroup);

    QString name = settings.value(_settingsKeyActiveJoystick).toString();
    if (name.isEmpty()) {
        name = _name2JoystickMap.first()->name();
    }

    setActiveJoystick(_name2JoystickMap.value(name, _name2JoystickMap.first()));
    settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());

    settings.endGroup();
}

Joystick *JoystickManager::activeJoystick()
{
    return _activeJoystick;
}

void JoystickManager::setActiveJoystick(Joystick *joystick)
{
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

    if (_activeJoystick) {
        qCDebug(JoystickManagerLog) << "Set active:" << _activeJoystick->name();

        QSettings settings;
        settings.beginGroup(_settingsGroup);
        settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());
        settings.endGroup();
    }

    emit activeJoystickChanged(_activeJoystick);
    emit activeJoystickNameChanged(_activeJoystick ? _activeJoystick->name() : "");
}

QVariantList JoystickManager::joysticks()
{
    QVariantList list;
    for (auto it = _name2JoystickMap.constBegin(); it != _name2JoystickMap.constEnd(); ++it) {
        list += QVariant::fromValue(it.value());
    }

    return list;
}

QString JoystickManager::activeJoystickName() const
{
    return (_activeJoystick ? _activeJoystick->name() : QString());
}

bool JoystickManager::setActiveJoystickName(const QString &name)
{
    if (_name2JoystickMap.contains(name)) {
        setActiveJoystick(_name2JoystickMap[name]);
        return true;
    }

    qCWarning(JoystickManagerLog) << "Set active not in map" << name;
    return false;
}

void JoystickManager::_updateAvailableJoysticks()
{
#ifdef QGC_SDL_JOYSTICK
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT:
            qCDebug(JoystickManagerLog) << "SDL ERROR:" << SDL_GetError();
            break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_JOYDEVICEADDED:
            qCDebug(JoystickManagerLog) << "Joystick added:" << event.jdevice.which;
            _setActiveJoystickFromSettings();
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_JOYDEVICEREMOVED:
            qCDebug(JoystickManagerLog) << "Joystick removed:" << event.jdevice.which;
            _setActiveJoystickFromSettings();
            break;
        default:
            break;
        }
    }
#elif defined(Q_OS_ANDROID)
    _joystickCheckTimerCounter--;
    _setActiveJoystickFromSettings();
    if (_joystickCheckTimerCounter <= 0) {
        _joystickCheckTimer->stop();
    }
#endif
}
