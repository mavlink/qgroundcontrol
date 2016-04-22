#include "JoystickAndroid.h"

#include "QGCApplication.h"

#include <QQmlEngine>

int JoystickAndroid::_androidBtnListCount;
int *JoystickAndroid::_androidBtnList;
QMutex JoystickAndroid::m_mutex;

JoystickAndroid::JoystickAndroid(const QString& name, int id, MultiVehicleManager* multiVehicleManager)
    : Joystick(name,0,0,multiVehicleManager) //buttonCount and axisCount is computed below
    , deviceId(id)
{
    int i;
    
    QAndroidJniEnvironment env;

    QAndroidJniObject inputDevice = QAndroidJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", id);

    //get number of buttons
    jintArray a = env->NewIntArray(_androidBtnListCount);
    env->SetIntArrayRegion(a,0,_androidBtnListCount,_androidBtnList);

    QAndroidJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", a);
    jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
    int btn_sz = env->GetArrayLength(jSupportedButtons);
    jboolean* supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
    _buttonCount=0;
    for (i=0;i<btn_sz;i++) 
        if (supportedButtons[i]) _buttonCount++;

    //create a mapping table (btnCode) that maps button number with button code
    btnValue = new bool[_buttonCount];
    btnCode = new int[_buttonCount];
    int c = 0;
    for (i=0;i<btn_sz;i++) 
        if (supportedButtons[i]) {
		btnValue[c] = false;
        btnCode[c] = _androidBtnList[i];
		c++;
	}

    env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

    //get number of axis
    QAndroidJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    _axisCount = rangeListNative.callMethod<jint>("size");
    axisValue = new int[_axisCount];
    axisCode = new int[_axisCount];
    for (i=0;i<_axisCount;i++) { 
        QAndroidJniObject range = rangeListNative.callObjectMethod("get", "()Landroid/view/InputDevice/MotionRange;");
        if (range.isValid())
            axisCode[i] = range.callMethod<jint>("getAxis");
        axisValue[i] = 0;
    }

    _axisCount = 4;

    qDebug() << "axis:" <<_axisCount << "buttons:" <<_buttonCount;
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

bool JoystickAndroid::handleKeyEvent(jobject event) {
    QJNIObjectPrivate ev(event);
    QMutexLocker lock(&m_mutex);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId!=deviceId) return false;
 
    qDebug() << "handleKeyEvent!" << deviceId;
    return true;
}

bool JoystickAndroid::handleGenericMotionEvent(jobject event) {
    QJNIObjectPrivate ev(event);
    QMutexLocker lock(&m_mutex);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId!=deviceId) return false;
 
    qDebug() << "handleMotionEvent!" << deviceId;
    return true;
}

QMap<QString, Joystick*> JoystickAndroid::discover(MultiVehicleManager* _multiVehicleManager) {
    bool joystickFound = false;
    static QMap<QString, Joystick*> ret;

    _buttonList(); //it's enough to run it once, should be in a static constructor

    QMutexLocker lock(&m_mutex);

    QAndroidJniEnvironment env;
    QAndroidJniObject o = QAndroidJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = o.object<jintArray>();
    size_t sz = env->GetArrayLength(jarr);
    jint *buff = env->GetIntArrayElements(jarr, nullptr);

    int SOURCE_GAMEPAD = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_GAMEPAD");
    int SOURCE_JOYSTICK = QAndroidJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_JOYSTICK");

    for (size_t i = 0; i < sz; ++i) {
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

        qDebug() << "\t" << name << "id:" << buff[i];
        ret[name] = new JoystickAndroid(name, buff[i], _multiVehicleManager);
        joystickFound = true;
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);


    return ret;
}

bool JoystickAndroid::open(void) {
    return true;
}

void JoystickAndroid::close(void) {
}

bool JoystickAndroid::update(void)
{
    return true;
}

bool JoystickAndroid::getButton(int i) {
    return btnValue[ i ];
}

int JoystickAndroid::getAxis(int i) {
    return axisValue[ i ];
}


//helper method
void JoystickAndroid::_buttonList() {
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

    for (int j=0;j<_androidBtnListCount;j++)
        qDebug() << "\tpossible button: "+QString::number(_androidBtnList[j]);

}

