/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "JoystickManager.h"

#include <QQmlEngine>

#ifdef Q_OS_MAC
    #include <SDL.h>
#else
    #include <SDL/SDL.h>
#endif

QGC_LOGGING_CATEGORY(JoystickManagerLog, "JoystickManagerLog")

IMPLEMENT_QGC_SINGLETON(JoystickManager, JoystickManager)

const char * JoystickManager::_settingsGroup =              "JoystickManager";
const char * JoystickManager::_settingsKeyActiveJoystick =  "ActiveJoystick";

JoystickManager::JoystickManager(QObject* parent)
    : QGCSingleton(parent)
    , _activeJoystick(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    
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
            _name2JoystickMap[name] = new Joystick(name, axisCount, buttonCount, i);
        } else {
            qCDebug(JoystickManagerLog) << "\tSkipping duplicate" << name;
        }
    }
    
    if (!_name2JoystickMap.count()) {
        qCDebug(JoystickManagerLog) << "\tnone found";
        return;
    }
    
    _setActiveJoystickFromSettings();
}

JoystickManager::~JoystickManager()
{

}

void JoystickManager::_setActiveJoystickFromSettings(void)
{
    QSettings settings;
    
    settings.beginGroup(_settingsGroup);
    QString name = settings.value(_settingsKeyActiveJoystick).toString();
    
    if (name.isEmpty()) {
        name = _name2JoystickMap.first()->name();
    }
    
    setActiveJoystick(_name2JoystickMap.value(name, _name2JoystickMap.first()));
    settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());
}

Joystick* JoystickManager::activeJoystick(void)
{
    return _activeJoystick;
}

void JoystickManager::setActiveJoystick(Joystick* joystick)
{
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
}

QVariantList JoystickManager::joysticks(void)
{
    QVariantList list;
    
    foreach (QString name, _name2JoystickMap.keys()) {
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
    return _activeJoystick ? _activeJoystick->name() : QString();
}

void JoystickManager::setActiveJoystickName(const QString& name)
{
    if (!_name2JoystickMap.contains(name)) {
        qCWarning(JoystickManagerLog) << "Set active not in map" << name;
        return;
    }
    
    setActiveJoystick(_name2JoystickMap[name]);
}
