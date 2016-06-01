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
    #include "JoystickSDL.h"
    #define __sdljoystick__
#endif

#ifdef __android__
    /*
     * Android Joystick not yet supported
     * #include "JoystickAndroid.h"
     */
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

JoystickManager::~JoystickManager() {
    QMap<QString, Joystick*>::iterator i;
    for (i = _name2JoystickMap.begin(); i != _name2JoystickMap.end(); ++i) {
        qDebug() << "Releasing joystick:" << i.key();
        delete i.value();
    }
    qDebug() << "Done";
}

void JoystickManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

    _multiVehicleManager = _toolbox->multiVehicleManager();

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

#ifdef __sdljoystick__
    _name2JoystickMap = JoystickSDL::discover(_multiVehicleManager);
#elif defined(__android__)
    /*
     * Android Joystick not yet supported
     * _name2JoystickMap = JoystickAndroid::discover(_multiVehicleManager);
     */
#endif

    if (!_name2JoystickMap.count()) {
        qCDebug(JoystickManagerLog) << "\tnone found";
        return;
    }

    _setActiveJoystickFromSettings();
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
