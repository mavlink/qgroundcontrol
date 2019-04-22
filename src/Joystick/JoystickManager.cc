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
    #include "JoystickAndroid.h"
#endif

QGC_LOGGING_CATEGORY(JoystickManagerLog, "JoystickManagerLog")

const char * JoystickManager::_settingsGroup =              "JoystickManager";
const char * JoystickManager::_settingsKeyActiveJoystick =  "ActiveJoystick";
const char * JoystickManager::_joystickModeSettingsKey =    "JoystickMode";
const char * JoystickManager::_joystickEnabledSettingsKey = "JoystickEnabled";

JoystickManager::JoystickManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _activeJoystick(NULL)
    , _multiVehicleManager(NULL)
    , _joystickMessageSender(NULL)
    , _joystickMode(Vehicle::JoystickModeRC)
    , _joystickEnabled(false)
    //default config for generic vehicle
    , _supportsThrottleModeCenterZero(true)
    , _supportsNegativeThrust(false)
    , _supportsJSButton(false)
    , _manualControlReservedButtonCount(0)
    , _joystickAction(NULL)
{
    connect(this, &JoystickManager::activeJoystickChanged, this, &JoystickManager::_loadSettings);
}

JoystickManager::~JoystickManager() {
    QMap<QString, Joystick*>::iterator i;
    for (i = _name2JoystickMap.begin(); i != _name2JoystickMap.end(); ++i) {
        qDebug() << "Releasing joystick:" << i.key();
        delete i.value();
    }
    if(_joystickMessageSender) {
        delete _joystickMessageSender;
    }
    for(int i = 0; i < _keyConfigurationList.size(); i++) {
        if(_keyConfigurationList[i]) {
            delete _keyConfigurationList[i];
        }
    }
    _keyConfigurationList.clear();
    qDebug() << "Done";
}

bool JoystickManager::supportsThrottleModeCenterZero(void)
{
    return _supportsThrottleModeCenterZero;
}

bool JoystickManager::supportsNegativeThrust(void)
{
    return _supportsNegativeThrust;
}
bool JoystickManager::supportsJSButton(void)
{
    return _supportsJSButton;
}
int  JoystickManager::manualControlReservedButtonCount(void)
{
    return _manualControlReservedButtonCount;
}

QStringList JoystickManager::joystickAction(void)
{
    return _joystickAction;
}

void JoystickManager::setJoystickAction(QStringList list)
{
    _joystickAction = list;
}

void JoystickManager::setSupportsThrottleModeCenterZero(bool enabled)
{
    _supportsThrottleModeCenterZero = enabled;
}

void JoystickManager::setSupportsNegativeThrust(bool enabled)
{
    _supportsNegativeThrust = enabled;
}
void JoystickManager::setSupportsJSButton(bool enabled)
{
    _supportsJSButton = enabled;
}
void JoystickManager::setManualControlReservedButtonCount(int count)
{
    _manualControlReservedButtonCount = count;
}

void JoystickManager::_loadSettings(void)
{
    QSettings settings;

    if(!_activeJoystick)
        return;

    settings.beginGroup(_settingsGroup);

    bool convertOk;

    _joystickMode = (Vehicle::JoystickMode_t)settings.value(_joystickModeSettingsKey, Vehicle::JoystickModeRC).toInt(&convertOk);
    if (!convertOk) {
        _joystickMode = Vehicle::JoystickModeRC;
    }

    // Joystick enabled is a global setting so first make sure there are any joysticks connected
    if (_toolbox->joystickManager()->joysticks().count()) {
        setJoystickEnabled(settings.value(_joystickEnabledSettingsKey, false).toBool());
    }
}

void JoystickManager::_saveSettings(void)
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);

    settings.setValue(_joystickModeSettingsKey, _joystickMode);

    // The joystick enabled setting should only be changed if a joystick is present
    // since the checkbox can only be clicked if one is present
    if (_toolbox->joystickManager()->joysticks().count()) {
        settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
    }
}

int JoystickManager::joystickMode(void)
{
    return _joystickMode;
}

void JoystickManager::setJoystickMode(int mode)
{
    if (mode < 0 || mode >= Vehicle::JoystickModeMax) {
        qCWarning(VehicleLog) << "Invalid joystick mode" << mode;
        return;
    }

    _joystickMode = (Vehicle::JoystickMode_t)mode;
    _saveSettings();
    emit joystickModeChanged(mode);
}

QStringList JoystickManager::joystickModes(void)
{
    QStringList list;

    list << "Normal" << "Attitude" << "Position" << "Force" << "Velocity";

    return list;
}

bool JoystickManager::joystickEnabled(void)
{
    return _joystickEnabled;
}

void JoystickManager::setJoystickEnabled(bool enabled)
{
    _joystickEnabled = enabled;
    _startJoystick(_joystickEnabled);
    _saveSettings();
    emit joystickEnabledChanged(_joystickEnabled);
}

KeyConfiguration* JoystickManager::getKeyConfiguration(int index)
{
    if(index < 0 || index > 1) {
        qWarning() << "keyConfigurationList index error.";
        return NULL;
    }
    return _keyConfigurationList[index];
}

QQmlListProperty<KeyConfiguration> JoystickManager::keyConfigurationList()
{
    return QQmlListProperty<KeyConfiguration>(this, _keyConfigurationList);
}

void JoystickManager::_startJoystick(bool start)
{
    if (_activeJoystick) {
        if (start) {
            if (_joystickEnabled) {
                _activeJoystick->startPolling(NULL);
            }
        } else {
            _activeJoystick->stopPolling();
        }
    }
}

void JoystickManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    _multiVehicleManager = _toolbox->multiVehicleManager();
    _joystickMessageSender = new JoystickMessageSender(this);
    _keyConfigurationList << new KeyConfiguration(this, 5, 12, 1/* sbus */);
    _keyConfigurationList << new KeyConfiguration(this, 1, 16, 2/* sbus */);

    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterType<KeyConfiguration>();
}

void JoystickManager::init() {
#ifdef __sdljoystick__
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
    QMap<QString,Joystick*> newMap;

#ifdef __sdljoystick__
    // Get the latest joystick mapping
    newMap = JoystickSDL::discover(_multiVehicleManager, this);
#elif defined(__android__)
    newMap = JoystickAndroid::discover(_multiVehicleManager, this);
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

    if (joystick != NULL && !_name2JoystickMap.contains(joystick->name())) {
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
    
    if (_activeJoystick != NULL) {
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
