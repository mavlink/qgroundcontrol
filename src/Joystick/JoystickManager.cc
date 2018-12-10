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

#ifdef QGC_ENABLE_GAMEPAD
    #include "JoystickQGamepad.h"
#else
    #ifndef __mobile__
        #include "JoystickSDL.h"
        #define __sdljoystick__
    #endif

    #ifdef __android__
        #include "JoystickAndroid.h"
    #endif
#endif

QGC_LOGGING_CATEGORY(JoystickManagerLog, "JoystickManagerLog")

const char * JoystickManager::_settingsGroup =              "JoystickManager";
const char * JoystickManager::_settingsKeyActiveJoystick =  "ActiveJoystick";

JoystickManager::JoystickManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _activeJoystick(nullptr)
    , _multiVehicleManager(nullptr)
{
}

JoystickManager::~JoystickManager() {
    for (auto i = _name2JoystickMap.begin(); i != _name2JoystickMap.end(); ++i) {
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
}

void JoystickManager::init() {
#ifdef QGC_ENABLE_GAMEPAD
    connect(&_joystickCheckTimer, &QTimer::timeout, this, &JoystickManager::_setActiveJoystickFromSettings);
    _joystickCheckTimer.start(1000);
#elif defined(__sdljoystick__)
    if (JoystickSDL::init()) {
        _setActiveJoystickFromSettings();
        connect(&_joystickCheckTimer, &QTimer::timeout, this, &JoystickManager::_updateAvailableJoysticks);
        _joystickCheckTimer.start(250);
    }
#elif defined(__android__)
    _setActiveJoystickFromSettings();
    //TODO: Investigate Android events for Joystick hot plugging & run _joystickCheckTimer if possible
#endif
}

void JoystickManager::_setActiveJoystickFromSettings(void)
{
#ifdef QGC_ENABLE_GAMEPAD
    QList<QString> names;
    bool changed = false;

    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    for (const int& id : gamepads) {
#ifdef __ios__
        // ios does not have gamepadName because it uses QT < 5.11.0
        QString name = "gamepad" + QString::number(id);
#else
        QString name = QGamepadManager::instance()->gamepadName(id);
#endif
        names.append(name);
        if (!_name2JoystickMap.contains(name)) {
            qCDebug(JoystickManagerLog) << "New joystick added: " << name;
            _name2JoystickMap[name] = new JoystickQGamepad(id, name, _multiVehicleManager, this);
            changed = true;
        }
    }

    for (auto j : _name2JoystickMap.toStdMap()) {
        bool found = false;
        for (const QString& name : names) {
            if (name == j.first) {
                found = true;
                break;
            }
        }

        if (!found) {
            qCDebug(JoystickManagerLog) << "Releasing joystick: " << j.first;
            if (_activeJoystick == j.second) {
                qCDebug(JoystickManagerLog) << "Active joystick removed";
                setActiveJoystick(nullptr);
                changed = true;
            }
            delete(j.second);
            _name2JoystickMap.remove(j.first);
        }
    }

    if (!changed) {
        return;
    }

    emit availableJoysticksChanged();

    if (_activeJoystick != nullptr) {
        return;
    }

#else
    QMap<QString,Joystick*> newMap;

#ifdef __sdljoystick__
    // Get the latest joystick mapping
    newMap = JoystickSDL::discover(_multiVehicleManager);
#elif defined(__android__)
    newMap = JoystickAndroid::discover(_multiVehicleManager);
#endif

    if (_activeJoystick && !newMap.contains(_activeJoystick->name())) {
        qCDebug(JoystickManagerLog) << "Active joystick removed";
        setActiveJoystick(NULL);
    }

    // Check to see if our current mapping contains any joysticks that are not in the new mapping
    // If so, those joysticks have been unplugged, and need to be cleaned up
    QMap<QString, Joystick*>::iterator i;
    for (i = _name2JoystickMap.begin(); i != _name2JoystickMap.end(); ++i) {
        if (!newMap.contains(i.key())) {
            qCDebug(JoystickManagerLog) << "Releasing joystick:" << i.key();
            i.value()->stopPolling();
            i.value()->wait(1000);
            i.value()->deleteLater();
        }
    }

    _name2JoystickMap = newMap;
    emit availableJoysticksChanged();

    if (!_name2JoystickMap.count()) {
        setActiveJoystick(NULL);
        return;
    }
#endif

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

    if (joystick != nullptr && !_name2JoystickMap.contains(joystick->name())) {
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
    
    if (_activeJoystick != nullptr) {
        qCDebug(JoystickManagerLog) << "Set active:" << _activeJoystick->name();

        settings.beginGroup(_settingsGroup);
        settings.setValue(_settingsKeyActiveJoystick, _activeJoystick->name());
    }

    emit activeJoystickChanged(_activeJoystick);
    emit activeJoystickNameChanged(_activeJoystick?_activeJoystick->name():"");
}

QVariantList JoystickManager::joysticks(void)
{
    QVariantList list;
    
    for (const QString &name: _name2JoystickMap.keys()) {
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

/*
 * TODO: move this to the right place: JoystickSDL.cc and JoystickAndroid.cc respectively and call through Joystick.cc
 */
void JoystickManager::_updateAvailableJoysticks(void)
{
#ifdef __sdljoystick__
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT:
            qCDebug(JoystickManagerLog) << "SDL ERROR:" << SDL_GetError();
            break;
        case SDL_JOYDEVICEADDED:
            qCDebug(JoystickManagerLog) << "Joystick added:" << event.jdevice.which;
            _setActiveJoystickFromSettings();
            break;
        case SDL_JOYDEVICEREMOVED:
            qCDebug(JoystickManagerLog) << "Joystick removed:" << event.jdevice.which;
            _setActiveJoystickFromSettings();
            break;
        default:
            break;
        }
    }
#elif defined(__android__)
    /*
     * TODO: Investigate Android events for Joystick hot plugging
     */
#endif
}
