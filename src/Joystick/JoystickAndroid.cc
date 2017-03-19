#include "JoystickAndroid.h"

#include "QGCApplication.h"

#include <QQmlEngine>

int JoystickAndroid::_androidBtnListCount;
int *JoystickAndroid::_androidBtnList;
int JoystickAndroid::ACTION_DOWN;
int JoystickAndroid::ACTION_UP;
QMutex JoystickAndroid::m_mutex;

JoystickAndroid::JoystickAndroid(const QString& name, int axisCount, int buttonCount, int id, MultiVehicleManager* multiVehicleManager)
    : Joystick(name,axisCount,buttonCount,0,multiVehicleManager)
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

    //set axis mapping (number->code)
    axisValue = new int[_axisCount];
    axisCode = new int[_axisCount];
    QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    for (i=0;i<_axisCount;i++) {
        QAndroidJniObject range = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;",i);
        axisCode[i] = range.callMethod<jint>("getAxis");
        axisValue[i] = 0;
    }


    qCDebug(JoystickLog) << "axis:" <<_axisCount << "buttons:" <<_buttonCount;
    QtAndroidPrivate::registerGenericMotionEventListener(this);
    QtAndroidPrivate::registerKeyEventListener(this);
}

JoystickAndroid::~JoystickAndroid() {
    delete btnCode;
    delete axisCode;
    delete btnValue;
    delete axisValue;

    QtAndroidPrivate::unregisterGenericMotionEventListener(this);
    QtAndroidPrivate::unregisterKeyEventListener(this);
}


QMap<QString, Joystick*> JoystickAndroid::discover(MultiVehicleManager* _multiVehicleManager) {
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

        ret[name] = new JoystickAndroid(name, axisCount, buttonCount, buff[i], _multiVehicleManager);
        joystickFound = true;
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);


    return ret;
}


bool JoystickAndroid::handleKeyEvent(jobject event) {
    QJNIObjectPrivate ev(event);
    QMutexLocker lock(&m_mutex);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId!=deviceId) return false;
 
    const int action = ev.callMethod<jint>("getAction", "()I");
    const int keyCode = ev.callMethod<jint>("getKeyCode", "()I");

    for (int i=0;i<_buttonCount;i++) {
        if (btnCode[i]==keyCode) {
            if (action==ACTION_DOWN) btnValue[i] = true;
            if (action==ACTION_UP) btnValue[i] = false;
            return true;
        }
    }
    return false;
}

bool JoystickAndroid::handleGenericMotionEvent(jobject event) {
    QJNIObjectPrivate ev(event);
    QMutexLocker lock(&m_mutex);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId!=deviceId) return false;
 
    for (int i=0;i<_axisCount;i++) {
        const float v = ev.callMethod<jfloat>("getAxisValue", "(I)F",axisCode[i]);
        axisValue[i] = (int)(v*32767.f);
    }
    return true;
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
}

