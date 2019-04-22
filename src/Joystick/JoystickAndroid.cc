#include "JoystickAndroid.h"

#include "QGCApplication.h"

#include <QQmlEngine>
#include <QTimer>
#include <fstream>

#define KEY_DOWN KeyConfiguration::keyAction_down
#define KEY_UP KeyConfiguration::keyAction_up
#define SHORT_PRESS KeyConfiguration::keyAction_shortPress
#define LONG_PRESS KeyConfiguration::keyAction_longPress
#define LONG_PRESS_TIME 1000L//1000ms
#define JOYSTICK_CONFIG_FILENAME "JoystickConfig.ini"
#define RC_JOYSTICK_CONFIG_FILENAME "/data/rc-service/joystickconfig.ini"
#define DEFAULT_MIN_CHANNEL_VALUE 364
#define DEFAULT_MAX_CHANNEL_VALUE 1684

int JoystickAndroid::_androidBtnListCount;
int *JoystickAndroid::_androidBtnList;
int JoystickAndroid::ACTION_DOWN;
int JoystickAndroid::ACTION_UP;
int JoystickAndroid::KEYCODE_A;
int JoystickAndroid::KEYCODE_B;
int JoystickAndroid::KEYCODE_C;
int JoystickAndroid::KEYCODE_D;
int JoystickAndroid::KEYCODE_CAM;
QMutex JoystickAndroid::m_mutex;

JoystickAndroid::JoystickAndroid(const QString& name, int axisCount, int buttonCount, int id, MultiVehicleManager* multiVehicleManager, JoystickManager* joystickManager)
    : Joystick(name,axisCount,buttonCount,0,multiVehicleManager,joystickManager)
    , deviceId(id)
{
    int i;
    
    QAndroidJniEnvironment env;
    QAndroidJniObject inputDevice = QAndroidJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", id);

    //set button mapping (number->code)
    jintArray b = env->NewIntArray(_androidBtnListCount);
    env->SetIntArrayRegion(b,0,_androidBtnListCount,_androidBtnList);

    QAndroidJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", b);
    jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
    jboolean* supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
    //create a mapping table (btnCode) that maps button number with button code
    btnValue = new bool[_buttonCount];
    btnCode = new int[_buttonCount];
    int c = 0;
    for (i=0;i<_androidBtnListCount;i++)
        if (supportedButtons[i]) {
            btnValue[c] = false;
            btnCode[c] = _androidBtnList[i];
            c++;
        }

    env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

    // set axis mapping (number->code)
    axisValue = new int[_axisCount];
    axisCode = new int[_axisCount];
    QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    for (i = 0; i < _axisCount; i++) {
        QAndroidJniObject range = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;",i);
        axisCode[i] = range.callMethod<jint>("getAxis");
        // Don't allow two axis with the same code
        for (int j = 0; j < i; j++) {
            if (axisCode[i] == axisCode[j]) {
                axisCode[i] = -1;
                break;
            }
        }
        axisValue[i] = 0;
    }
    memset(_keyEvents, 0, sizeof(_keyEvents));
    _keyEvents[KEY_A].keyCode = KEYCODE_A;
    _keyEvents[KEY_B].keyCode = KEYCODE_B;
    _keyEvents[KEY_C].keyCode = KEYCODE_C;
    _keyEvents[KEY_D].keyCode = KEYCODE_D;
    _keyEvents[KEY_CAM].keyCode = KEYCODE_CAM;
    for(i = 0; i < KEY_MAX; i++) {
        _keyEvents[i].timer = new QTimer(this);
        connect(_keyEvents[i].timer, SIGNAL(timeout()), this, SLOT(handleLongPress()));
        _keyEvents[i].timer->setSingleShot(true);
        _keyEvents[i].timer->setProperty("keyindex", i);
    }

    qCDebug(JoystickLog) << "axis:" <<_axisCount << "buttons:" <<_buttonCount;

    eventReader = new InputEventReader();
    connect(eventReader, &InputEventReader::keyEventRecieved, this, &JoystickAndroid::_handleKeyEvent);
    connect(eventReader, &InputEventReader::axisEventRecieved, this, &JoystickAndroid::_handleGenericMotionEvent);
    eventReader->start();
    _configSaver = new QSettings(JOYSTICK_CONFIG_FILENAME, QSettings::IniFormat);
}

JoystickAndroid::~JoystickAndroid() {
    delete btnCode;
    delete axisCode;
    delete btnValue;
    delete axisValue;

    disconnect(eventReader, &InputEventReader::keyEventRecieved, this, &JoystickAndroid::_handleKeyEvent);
    disconnect(eventReader, &InputEventReader::axisEventRecieved, this, &JoystickAndroid::_handleGenericMotionEvent);
    eventReader->quit();
    delete eventReader;
    delete _configSaver;
}

QMap<QString, Joystick*> JoystickAndroid::discover(MultiVehicleManager* _multiVehicleManager, JoystickManager* _joystickManager) {
    bool joystickFound = false;
    static QMap<QString, Joystick*> ret;

    _initStatic(); //it's enough to run it once, should be in a static constructor

    QMutexLocker lock(&m_mutex);

    QAndroidJniEnvironment env;
    QAndroidJniObject o = QAndroidJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = o.object<jintArray>();
    int sz = env->GetArrayLength(jarr);
    jint *buff = env->GetIntArrayElements(jarr, nullptr);

    int SOURCE_GAMEPAD = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_GAMEPAD");
    int SOURCE_JOYSTICK = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_JOYSTICK");

    for (int i = 0; i < sz; ++i) {
        QAndroidJniObject inputDevice = QAndroidJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", buff[i]);
        int sources = inputDevice.callMethod<jint>("getSources", "()I");
        if (((sources & SOURCE_GAMEPAD) != SOURCE_GAMEPAD) //check if the input device is interesting to us
                && ((sources & SOURCE_JOYSTICK) != SOURCE_JOYSTICK)) continue;

        //get id and name
        QString id = inputDevice.callObjectMethod("getDescriptor", "()Ljava/lang/String;").toString();
        QString name = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();


        if (joystickFound) { //skipping {
            qWarning() << "Skipping joystick:" << name;
            continue;
        }

        //get number of axis
        QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
        int axisCount = rangeListNative.callMethod<jint>("size");

        //get number of buttons
        jintArray a = env->NewIntArray(_androidBtnListCount);
        env->SetIntArrayRegion(a,0,_androidBtnListCount,_androidBtnList);
        QAndroidJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", a);
        jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
        jboolean* supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
        int buttonCount = 0;
        for (int j=0;j<_androidBtnListCount;j++)
            if (supportedButtons[j]) buttonCount++;
        env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

        qCDebug(JoystickLog) << "\t" << name << "id:" << buff[i] << "axes:" << axisCount << "buttons:" << buttonCount;

        ret[name] = new JoystickAndroid(name, axisCount, buttonCount, buff[i], _multiVehicleManager, _joystickManager);
        joystickFound = true;
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);

    return ret;
}

void JoystickAndroid::_handleKeyEvent(int keycode, int action)
{
    _handleKeyEventInner(keycode, action);
}

void JoystickAndroid::_handleGenericMotionEvent(int axiscode, float value)
{
    for(int i = 0; i < _axisCount; i++) {
        if(axisCode[i] == axiscode) {
            axisValue[i] = (int)(value*32767.f);
        }
    }
}

bool JoystickAndroid::_open(void) {
    return true;
}

void JoystickAndroid::_close(void) {
}

bool JoystickAndroid::_update(void)
{
    return true;
}

bool JoystickAndroid::_getButton(int i) {
    return btnValue[ i ];
}

int JoystickAndroid::_getAxis(int i) {
    return axisValue[ i ];
}

uint8_t JoystickAndroid::_getHat(int hat,int i) {
    Q_UNUSED(hat);
    Q_UNUSED(i);

    return 0;
}

bool JoystickAndroid::_handleKeyEventInner(int keycode, int action) {
    int keyIndex;
    int sbus, ch, value;

    keyIndex = getKeyIndexByCode(keycode);
    if(keyIndex < 0) {
        qDebug() << "unsupport key " << keycode << ", don't proccess here";
        return false;
    }

    if(action == ACTION_DOWN) {
        _keyEvents[keyIndex].isPressed = true;
        if(getChannelValue(keycode, KEY_DOWN, &sbus, &ch, &value)) {
            sendChannelValue(sbus, ch, value);
        }
        _keyEvents[keyIndex].timer->start(LONG_PRESS_TIME);
    } else {
        if(getChannelValue(keycode, KEY_UP, &sbus, &ch, &value)) {
            sendChannelValue(sbus, ch, value);
        }
        if(!_keyEvents[keyIndex].isLongPress) {
            _keyEvents[keyIndex].timer->stop();
            if(getChannelValue(keycode, SHORT_PRESS, &sbus, &ch, &value)) {
                sendChannelValue(sbus, ch, value);
            }
        }
        _keyEvents[keyIndex].isPressed = false;
        _keyEvents[keyIndex].isLongPress = false;
    }

    return true;
}

void JoystickAndroid::handleLongPress()
{
    int keyIndex = sender()->property("keyindex").toInt();
    int sbus, ch, value;
    _keyEvents[keyIndex].isLongPress = true;
    if(getChannelValue(_keyEvents[keyIndex].keyCode, LONG_PRESS, &sbus, &ch, &value)) {
        sendChannelValue(sbus, ch, value);
    }
}

int JoystickAndroid::getKeyIndexByCode(int code) {
    if(code == KEYCODE_A)
        return KEY_A;
    if(code == KEYCODE_B)
        return KEY_B;
    if(code == KEYCODE_C)
        return KEY_C;
    if(code == KEYCODE_D)
        return KEY_D;
    if(code == KEYCODE_CAM)
        return KEY_CAM;

    return -1;
}

void JoystickAndroid::sendChannelValue(int sbus, int ch, int value) {
    qgcApp()->toolbox()->joystickManager()->joystickMessageSender()->setChannelValue(sbus, ch, value);
}

bool JoystickAndroid::getChannelValue(int keyCode, KeyConfiguration::KeyAction_t action, int* sbus, int *ch, int* value) {
    bool ret = KeyConfiguration::getChannelValue(keyCode, action, sbus, ch, value);
    if(ret) {
        qDebug() << "keyCode = " << keyCode << " sbus = " << *sbus << " ch = " << *ch << " value = " << *value;
    }
    return ret;
}

void JoystickAndroid::saveJoystickSettings()
{
    QSettings settings;
    static const char* functionSettingsKey[Joystick::maxFunction] = {
        "RollAxis",
        "PitchAxis",
        "YawAxis",
        "ThrottleAxis",
        "WheelAxis"
    };
    static const char* functionChannelsKey[4] = {
        "RollChannel",
        "PitchChannel",
        "YawChannel",
        "ThrottleChannel",
    };
    static const int functionChennels[4] = { 1, 2, 4, 3 };

    _configSaver->beginGroup("Basic");
    _configSaver->setValue("calibrated", _calibrated);
    _configSaver->setValue("transmitterMode", _transmitterMode);
    _configSaver->endGroup();

    QString groupTpl  ("Axis%1Calibration");
    for (int axis=0; axis<_axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];

        _configSaver->beginGroup(groupTpl.arg(axis));
        _configSaver->setValue("AxisTrim", calibration->center);
        _configSaver->setValue("AxisMin", calibration->min);
        _configSaver->setValue("AxisMax", calibration->max);
        _configSaver->setValue("AxisRev", calibration->reversed);
        _configSaver->setValue("AxisDeadband", calibration->deadband);
        _configSaver->endGroup();
    }

    _configSaver->beginGroup("Function");
    settings.beginGroup("Joysticks");
    settings.beginGroup(_name);
    for (int function=0; function<maxFunction; function++) {
        _configSaver->setValue(functionSettingsKey[function], settings.value(functionSettingsKey[function]));
    }
    _configSaver->endGroup();

    _configSaver->beginGroup("FunctionChannel");
    for (int i = 0; i < 4; i++) {
        _configSaver->setValue(functionChannelsKey[i], functionChennels[i]);
    }
    _configSaver->setValue("MinChannelValue", DEFAULT_MIN_CHANNEL_VALUE);
    _configSaver->setValue("MaxChannelValue", DEFAULT_MAX_CHANNEL_VALUE);
    _configSaver->endGroup();

    _configSaver->beginGroup("Additional");
    _configSaver->setValue("Exponential", QString::number(_exponential));
    _configSaver->setValue("Accumulator", _accumulator);
    _configSaver->setValue("Deadband", _deadband);
    _configSaver->setValue("ThrottleMode", _throttleMode);
    _configSaver->setValue("CenterZeroSupport", _joystickManager->supportsThrottleModeCenterZero());
    _configSaver->setValue("NegativeThrust", _negativeThrust);
    _configSaver->endGroup();
    _configSaver->sync();

    std::ifstream infile(JOYSTICK_CONFIG_FILENAME);
    std::ofstream outfile(RC_JOYSTICK_CONFIG_FILENAME);
    char buf[2048];
    while(infile) {
        infile.read(buf, 2048);
        outfile.write(buf, infile.gcount());
    }

    infile.close();
    outfile.close();
}

//helper method
void JoystickAndroid::_initStatic() {
    //this gets list of all possible buttons - this is needed to check how many buttons our gamepad supports
    //instead of the whole logic below we could have just a simple array of hardcoded int values as these 'should' not change

    //int JoystickAndroid::_androidBtnListCount;
    _androidBtnListCount = 31;
    static int ret[31]; //there are 31 buttons in total accordingy to the API
    int i;
    //int *JoystickAndroid::
    _androidBtnList = ret;

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

    ACTION_DOWN = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_DOWN");
    ACTION_UP = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_UP");
    KEYCODE_A = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BREAK");//button A
    KEYCODE_B = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BACK");//button B
    KEYCODE_C = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_VOLUME_UP");//button C
    KEYCODE_D = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_VOLUME_DOWN");//buton D
    KEYCODE_CAM = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_CAMERA");//buton CAMERA
}

