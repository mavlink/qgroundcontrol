/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickAndroid.h"
#include "JoystickManager.h"
#include "AndroidInterface.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>

QGC_LOGGING_CATEGORY(JoystickAndroidLog, "qgc.joystick.joystickandroid")

QList<int> JoystickAndroid::_androidBtnList(_androidBtnListCount);
int JoystickAndroid::ACTION_DOWN = 0;
int JoystickAndroid::ACTION_UP = 0;
int JoystickAndroid::AXIS_HAT_X = 0;
int JoystickAndroid::AXIS_HAT_Y = 0;
QMutex JoystickAndroid::_mutex;

JoystickAndroid::JoystickAndroid(const QString &name, int axisCount, int buttonCount, int id, QObject *parent)
    : Joystick(name, axisCount, buttonCount, 0, parent)
    , deviceId(id)
{
    btnCode.resize(_buttonCount);
    axisCode.resize(_axisCount);
    btnValue.resize(_buttonCount);
    axisValue.resize(_axisCount);

    QJniEnvironment env;
    const jintArray btnArr = env->NewIntArray(_androidBtnListCount);
    env->SetIntArrayRegion(btnArr, 0, _androidBtnListCount, _androidBtnList.constData());
    const QJniObject inputDevice = QJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", id);
    const QJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", btnArr);
    const jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
    jboolean *const supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
    int c = 0;
    for (int i = 0; i < _androidBtnListCount; i++) {
        if (supportedButtons[i]) {
            btnValue[c] = false;
            btnCode[c] = _androidBtnList[i];
            c++;
        }
    }

    env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

    const QJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    for (int i = 0; i < _axisCount; i++) {
        const QJniObject range = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
        axisCode[i] = range.callMethod<jint>("getAxis");
        for (int j = 0; j < i; j++) {
            if (axisCode[i] == axisCode[j]) {
                axisCode[i] = -1;
                break;
            }
        }
        axisValue[i] = 0;
    }
    qCDebug(JoystickAndroidLog) << "axis:" << _axisCount << "buttons:" << _buttonCount;

    QtAndroidPrivate::registerGenericMotionEventListener(this);
    QtAndroidPrivate::registerKeyEventListener(this);
}

JoystickAndroid::~JoystickAndroid()
{
    QtAndroidPrivate::unregisterGenericMotionEventListener(this);
    QtAndroidPrivate::unregisterKeyEventListener(this);
}

QMap<QString, Joystick*> JoystickAndroid::discover()
{
    static QMap<QString, Joystick*> ret;

    QMutexLocker lock(&_mutex);

    const QJniObject object = QJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = object.object<jintArray>();

    QJniEnvironment env;
    const int len = env->GetArrayLength(jarr);
    jint *const buff = env->GetIntArrayElements(jarr, nullptr);

    const int SOURCE_GAMEPAD = QJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_GAMEPAD");
    const int SOURCE_JOYSTICK = QJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_JOYSTICK");

    QList<QString> names;
    for (int i = 0; i < len; ++i) {
        const QJniObject inputDevice = QJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", buff[i]);
        const int sources = inputDevice.callMethod<jint>("getSources", "()I");
        if (((sources & SOURCE_GAMEPAD) != SOURCE_GAMEPAD) && ((sources & SOURCE_JOYSTICK) != SOURCE_JOYSTICK)) {
            continue;
        }

        const QString id = inputDevice.callObjectMethod("getDescriptor", "()Ljava/lang/String;").toString();
        const QString name = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();

        names.push_back(name);

        if (ret.contains(name)) {
            continue;
        }

        const QJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
        const int axisCount = rangeListNative.callMethod<jint>("size");

        jintArray arr = env->NewIntArray(_androidBtnListCount);
        env->SetIntArrayRegion(arr, 0, _androidBtnListCount, _androidBtnList.constData());
        const QJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", arr);
        const jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
        jboolean *const supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
        int buttonCount = 0;
        for (int j = 0; j < _androidBtnListCount; j++) {
            if (supportedButtons[j]) {
                buttonCount++;
            }
        }
        env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

        qCDebug(JoystickAndroidLog) << name << "id:" << buff[i] << "axes:" << axisCount << "buttons:" << buttonCount;

        ret[name] = new JoystickAndroid(name, axisCount, buttonCount, buff[i]);
    }

    for (auto i = ret.begin(); i != ret.end();) {
        if (!names.contains(i.key())) {
            i = ret.erase(i);
        } else {
            i++;
        }
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);

    return ret;
}

bool JoystickAndroid::handleKeyEvent(jobject event)
{
    QMutexLocker lock(&_mutex);

    QJniObject ev(event);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId != deviceId) {
        return false;
    }

    const int action = ev.callMethod<jint>("getAction", "()I");
    const int keyCode = ev.callMethod<jint>("getKeyCode", "()I");

    for (int i = 0; i < _buttonCount; i++) {
        if (btnCode[i] != keyCode) {
            continue;
        }

        if (action == ACTION_DOWN) {
            btnValue[i] = true;
        } else if (action == ACTION_UP) {
            btnValue[i] = false;
        }

        return true;
    }

    return false;
}

bool JoystickAndroid::handleGenericMotionEvent(jobject event)
{
    QMutexLocker lock(&_mutex);

    QJniObject ev(event);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId != deviceId) {
        return false;
    }

    for (int i = 0; i < _axisCount; i++) {
        const float v = ev.callMethod<jfloat>("getAxisValue", "(I)F", axisCode[i]);
        axisValue[i] = static_cast<int>(v * 32767.f);
    }

    return true;
}

int  JoystickAndroid::_getAndroidHatAxis(int axisHatCode) const
{
    for (int i = 0; i < _axisCount; i++) {
        if (axisCode[i] == axisHatCode) {
            return _getAxis(i);
        }
    }

    return 0;
}

bool JoystickAndroid::_getHat(int hat, int i) const
{
    // Android supports only one hat button
    if (hat != 0) {
        return false;
    }

    switch (i) {
    case 0:
        return (_getAndroidHatAxis(AXIS_HAT_Y) < 0);
    case 1:
        return (_getAndroidHatAxis(AXIS_HAT_Y) > 0);
    case 2:
        return (_getAndroidHatAxis(AXIS_HAT_X) < 0);
    case 3:
        return (_getAndroidHatAxis(AXIS_HAT_X) > 0);
    default:
        return false;
    }
}

bool JoystickAndroid::init()
{
    QList<int> ret(_androidBtnListCount);

    (void) AndroidInterface::cleanJavaException();

    int i;
    for (i = 1; i <= 16; i++) {
        const QString name = QStringLiteral("KEYCODE_BUTTON_") + QString::number(i);
        ret[i - 1] = QJniObject::getStaticField<jint>("android/view/KeyEvent", name.toStdString().c_str());
    }
    i--;

    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_A");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_B");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_C");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L1");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L2");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R1");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R2");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_MODE");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_SELECT");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_START");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBL");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBR");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_X");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Y");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Z");

    ACTION_DOWN = QJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_DOWN");
    ACTION_UP = QJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_UP");
    AXIS_HAT_X = QJniObject::getStaticField<jint>("android/view/MotionEvent", "AXIS_HAT_X");
    AXIS_HAT_Y = QJniObject::getStaticField<jint>("android/view/MotionEvent", "AXIS_HAT_Y");

    _androidBtnList = ret;

    return true;
}

static void jniUpdateAvailableJoysticks(JNIEnv *envA, jobject thizA)
{
    Q_UNUSED(envA); Q_UNUSED(thizA);

    qCDebug(JoystickAndroidLog) << "jniUpdateAvailableJoysticks triggered";

    emit JoystickManager::instance()->updateAvailableJoysticksSignal();
}

void JoystickAndroid::setNativeMethods()
{
    qCDebug(JoystickAndroidLog) << "Registering Native Functions";

    static const JNINativeMethod javaMethods[] {
        {"nativeUpdateAvailableJoysticks", "()V", reinterpret_cast<void*>(jniUpdateAvailableJoysticks)}
    };

    static constexpr const char *kJniClassName = "org/mavlink/qgroundcontrol/QGCUsbSerialManager";

    (void) AndroidInterface::cleanJavaException();

    QJniEnvironment jniEnv;
    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if (!objectClass) {
        (void) AndroidInterface::cleanJavaException();
        qCWarning(JoystickAndroidLog) << "Couldn't find class:" << kJniClassName;
        return;
    }

    const jint val = jniEnv->RegisterNatives(objectClass, javaMethods, std::size(javaMethods));
    if (val < 0) {
        qCWarning(JoystickAndroidLog) << "Error registering methods:" << val;
    } else {
        qCDebug(JoystickAndroidLog) << "Native Functions Registered";
    }

    (void) AndroidInterface::cleanJavaException();
}
