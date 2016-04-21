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
#include "QGCApplication.h"

#include <QQmlEngine>

#ifndef __mobile__
    #define __sdljoystick__
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

#ifdef __sdljoystick__
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
#elif defined(__android__)
    QMutexLocker lock(&m_mutex);

    computePossibleButtons(); //this is just needed to get number of supported buttons 

    QAndroidJniEnvironment env;
    QAndroidJniObject o = QAndroidJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = o.object<jintArray>();
    size_t sz = env->GetArrayLength(jarr);
    jint *buff = env->GetIntArrayElements(jarr, nullptr);

    int SOURCE_GAMEPAD = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_GAMEPAD");
    int SOURCE_JOYSTICK = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_JOYSTICK");

    for (size_t i = 0; i < sz; ++i) {
        int axisCount, buttonCount;
        QAndroidJniObject inputDevice = QAndroidJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", buff[i]);
        int sources = inputDevice.callMethod<jint>("getSources", "()I");
        if (((sources & SOURCE_GAMEPAD) != SOURCE_GAMEPAD) //check if the input device is interesting to us
                && ((sources & SOURCE_JOYSTICK) != SOURCE_JOYSTICK)) continue;

	//get id and name
        QString id = inputDevice.callObjectMethod("getDescriptor", "()Ljava/lang/String;").toString();
        QString name = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();

	//get number of buttons
	jintArray a = env->NewIntArray(31);
	env->SetIntArrayRegion(a,0,31,_possibleButtons);

        //QAndroidJniObject keyMap = inputDevice.callObjectMethod("getKeyCharacterMap", "()Landroid/view/KeyCharacterMap;");
    	//QAndroidJniObject btns = keyMap.callStaticObjectMethod("android/view/KeyCharacterMap","deviceHasKeys", "([I)[Z", a);
    	QAndroidJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", a);
        jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
        size_t btn_sz = env->GetArrayLength(jSupportedButtons);
        jboolean* supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
        buttonCount=0;
        for (size_t j=0;j<btn_sz;j++)
            if (supportedButtons[j]) buttonCount++;

        env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

	//get number of axis
        QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
        axisCount = rangeListNative.callMethod<jint>("size");

        qCDebug(JoystickManagerLog) << "\t" << name << "axes:" << axisCount << "buttons:" << buttonCount;
    	_name2JoystickMap[name] = new Joystick(name, axisCount, buttonCount, buff[i], _multiVehicleManager);
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);
#endif

    if (!_name2JoystickMap.count()) {
        qCDebug(JoystickManagerLog) << "\tnone found";
        return;
    }

    _setActiveJoystickFromSettings();
}

void JoystickManager::computePossibleButtons() {
    static int ret[31];
    int i;

    for (i=1;i<=16;i++) {
        QString name = "KEYCODE_BUTTON_"+QString::number(i);
        ret[i-1] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", name.toStdString().c_str());
    }
    i--;

    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_A");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_B");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_C");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L1");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L2");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R1");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R2");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_MODE");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_SELECT");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_START");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBL");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBR");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_X");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Y");
    ret[i++] = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Z");

    for (int j=0;j<i;j++)
        qCDebug(JoystickManagerLog) << "\tpossible button: "+ret[j];

    _possibleButtons = ret;
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
