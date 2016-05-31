/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "JoystickManager.h"
#include "QGCApplication.h"

#include <QQmlEngine>

#ifndef __mobile__
    #ifdef Q_OS_MAC
        #include <SDL.h>
    #else
        #include <SDL/SDL.h>
    #endif
#endif

QGC_LOGGING_CATEGORY(JoystickManagerLog, "JoystickManagerLog")

const char * JoystickManager::_settingsGroup =              "JoystickManager";
const char * JoystickManager::_settingsKeyActiveJoystick =  "ActiveJoystick";

JoystickManager::JoystickManager(QGCApplication* app)
    : QGCTool(app)
    , _activeJoystick(NULL)
    , _multiVehicleManager(NULL)
{
    
}

void JoystickManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

    _multiVehicleManager = _toolbox->multiVehicleManager();

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

#ifndef __mobile__
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
        qWarning() << "Couldn't initialize SimpleDirectMediaLayer:" << SDL_GetError();
        return;
    }

    // Load available joysticks

    qCDebug(JoystickManagerLog) << "Available joysticks";

    for (int i=0; i<SDL_NumJoysticks(); i++) {
        QString name = SDL_JoystickName(i);

        if (!_name2JoystickMap.contains(name)) {
            int axisCount, buttonCount;

            SDL_Joystick* sdlJoystick = SDL_JoystickOpen(i);
            axisCount = SDL_JoystickNumAxes(sdlJoystick);
            buttonCount = SDL_JoystickNumButtons(sdlJoystick);
            SDL_JoystickClose(sdlJoystick);

            qCDebug(JoystickManagerLog) << "\t" << name << "axes:" << axisCount << "buttons:" << buttonCount;
            _name2JoystickMap[name] = new Joystick(name, axisCount, buttonCount, i, _multiVehicleManager);
        } else {
            qCDebug(JoystickManagerLog) << "\tSkipping duplicate" << name;
        }
    }
#endif

    if (!_name2JoystickMap.count()) {
        qCDebug(JoystickManagerLog) << "\tnone found";
        return;
    }

    _setActiveJoystickFromSettings();
}


void JoystickManager::_setActiveJoystickFromSettings(void)
{
#ifndef __mobile__
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    QString name = settings.value(_settingsKeyActiveJoystick).toString();
    
    if (name.isEmpty()) {
        name = _name2JoystickMap.first()->name();
    }
    
    setActiveJoystick(_name2JoystickMap.value(name, _name2JoystickMap.first()));
    settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());
#endif
}

Joystick* JoystickManager::activeJoystick(void)
{
    return _activeJoystick;
}

void JoystickManager::setActiveJoystick(Joystick* joystick)
{
#ifdef __mobile__
    Q_UNUSED(joystick)
#else
    QSettings settings;
    
    if (!_name2JoystickMap.contains(joystick->name())) {
        qCWarning(JoystickManagerLog) << "Set active not in map" << joystick->name();
        return;
    }
    
    if (_activeJoystick) {
        _activeJoystick->stopPolling();
    }
    
    _activeJoystick = joystick;
    
    settings.beginGroup(_settingsGroup);
    settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());
    
    emit activeJoystickChanged(_activeJoystick);
    emit activeJoystickNameChanged(_activeJoystick->name());
#endif
}

QVariantList JoystickManager::joysticks(void)
{
    QVariantList list;
    
    foreach (const QString &name, _name2JoystickMap.keys()) {
        list += QVariant::fromValue(_name2JoystickMap[name]);
    }
    
    return list;
}

QStringList JoystickManager::joystickNames(void)
{
    return _name2JoystickMap.keys();
}

QString JoystickManager::activeJoystickName(void)
{
#ifdef __mobile__
    return QString();
#else
    return _activeJoystick ? _activeJoystick->name() : QString();
#endif
}

void JoystickManager::setActiveJoystickName(const QString& name)
{
    if (!_name2JoystickMap.contains(name)) {
        qCWarning(JoystickManagerLog) << "Set active not in map" << name;
        return;
    }
    
    setActiveJoystick(_name2JoystickMap[name]);
}
