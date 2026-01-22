#include "JoystickAndroid.h"
#include "JoystickManager.h"
#include "AndroidInterface.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>

QGC_LOGGING_CATEGORY(JoystickAndroidLog, "Joystick.JoystickAndroid")

QList<int> JoystickAndroid::_androidBtnList(_androidBtnListCount);
int JoystickAndroid::_actionDown = 0;
int JoystickAndroid::_actionUp = 0;
int JoystickAndroid::_axisHatX = 0;
int JoystickAndroid::_axisHatY = 0;
QMutex JoystickAndroid::_mutex;
QMap<int, JoystickAndroid*> JoystickAndroid::_instances;

// Source constants (API 12+)
static constexpr int SOURCE_GAMEPAD = 0x00000401;
static constexpr int SOURCE_JOYSTICK = 0x01000010;
static constexpr int SOURCE_DPAD = 0x00000201;
static constexpr int SOURCE_KEYBOARD = 0x00000101;
static constexpr int SOURCE_MOUSE = 0x00002002;
static constexpr int SOURCE_TOUCHSCREEN = 0x00001002;
static constexpr int SOURCE_BLUETOOTH_STYLUS = 0x00008002;

// Android Sensor types (from android.hardware.Sensor)
static constexpr int SENSOR_TYPE_ACCELEROMETER = 1;
static constexpr int SENSOR_TYPE_GYROSCOPE = 4;

// Android Light types (from android.hardware.lights.Light, API 31+)
static constexpr int LIGHT_TYPE_PLAYER_ID = 2;

// Android BatteryManager status constants (from android.os.BatteryManager)
static constexpr int BATTERY_STATUS_CHARGING = 2;
static constexpr int BATTERY_STATUS_DISCHARGING = 3;
static constexpr int BATTERY_STATUS_NOT_CHARGING = 4;
static constexpr int BATTERY_STATUS_FULL = 5;

// Android MotionEvent axis constants (from android.view.MotionEvent)
static constexpr int AXIS_X = 0;
static constexpr int AXIS_Y = 1;
static constexpr int AXIS_Z = 11;
static constexpr int AXIS_RX = 12;
static constexpr int AXIS_RY = 13;
static constexpr int AXIS_RZ = 14;
static constexpr int AXIS_HAT_X = 15;
static constexpr int AXIS_HAT_Y = 16;
static constexpr int AXIS_LTRIGGER = 17;
static constexpr int AXIS_RTRIGGER = 18;
static constexpr int AXIS_THROTTLE = 19;
static constexpr int AXIS_RUDDER = 20;
static constexpr int AXIS_WHEEL = 21;
static constexpr int AXIS_GAS = 22;
static constexpr int AXIS_BRAKE = 23;

// Android KeyEvent button constants (from android.view.KeyEvent)
static constexpr int KEYCODE_BUTTON_A = 96;
static constexpr int KEYCODE_BUTTON_B = 97;
static constexpr int KEYCODE_BUTTON_C = 98;
static constexpr int KEYCODE_BUTTON_X = 99;
static constexpr int KEYCODE_BUTTON_Y = 100;
static constexpr int KEYCODE_BUTTON_Z = 101;
static constexpr int KEYCODE_BUTTON_L1 = 102;
static constexpr int KEYCODE_BUTTON_R1 = 103;
static constexpr int KEYCODE_BUTTON_L2 = 104;
static constexpr int KEYCODE_BUTTON_R2 = 105;
static constexpr int KEYCODE_BUTTON_THUMBL = 106;
static constexpr int KEYCODE_BUTTON_THUMBR = 107;
static constexpr int KEYCODE_BUTTON_START = 108;
static constexpr int KEYCODE_BUTTON_SELECT = 109;
static constexpr int KEYCODE_BUTTON_MODE = 110;
static constexpr int KEYCODE_BUTTON_1 = 188;
static constexpr int KEYCODE_BUTTON_16 = 203;

// Hat direction indices (for _getHat switch)
static constexpr int HAT_DIR_UP = 0;
static constexpr int HAT_DIR_DOWN = 1;
static constexpr int HAT_DIR_LEFT = 2;
static constexpr int HAT_DIR_RIGHT = 3;

// Color constants for LED
static constexpr jint ALPHA_OPAQUE = 0xFF000000;

// Common USB vendor IDs for gamepad type detection
static constexpr quint16 VENDOR_MICROSOFT = 0x045e;
static constexpr quint16 VENDOR_SONY = 0x054c;
static constexpr quint16 VENDOR_NINTENDO = 0x057e;
static constexpr quint16 VENDOR_LOGITECH = 0x046d;

// Number of numbered buttons (KEYCODE_BUTTON_1 through KEYCODE_BUTTON_16)
static constexpr int NUMBERED_BUTTON_COUNT = 16;

JoystickAndroid::JoystickAndroid(const QString &name, int axisCount, int buttonCount, int id, QObject *parent)
    : Joystick(name, axisCount, buttonCount, 0, parent)
    , _deviceId(id)
{
    _btnCode.resize(_buttonCount);
    _axisCode.resize(_axisCount);
    _btnValue.resize(_buttonCount);
    _axisValue.resize(_axisCount);

    QJniEnvironment env;
    const jintArray btnArr = env->NewIntArray(_androidBtnListCount);
    env->SetIntArrayRegion(btnArr, 0, _androidBtnListCount, _androidBtnList.constData());
    const QJniObject inputDevice = QJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", id);
    if (!inputDevice.isValid()) {
        qCWarning(JoystickAndroidLog) << "Failed to get InputDevice for id:" << id;
        env->DeleteLocalRef(btnArr);
        return;
    }

    const QJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", btnArr);
    const jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
    jboolean *const supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
    int supportedBtnIdx = 0;
    for (int i = 0; i < _androidBtnListCount; i++) {
        if (supportedButtons[i]) {
            _btnValue[supportedBtnIdx] = false;
            _btnCode[supportedBtnIdx] = _androidBtnList[i];
            supportedBtnIdx++;
        }
    }

    env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);
    env->DeleteLocalRef(btnArr);

    const QJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    for (int i = 0; i < _axisCount; i++) {
        const QJniObject range = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
        _axisCode[i] = range.callMethod<jint>("getAxis");
        for (int j = 0; j < i; j++) {
            if (_axisCode[i] == _axisCode[j]) {
                _axisCode[i] = -1;
                break;
            }
        }
        _axisValue[i] = 0;
    }

    _cacheDeviceInfo();

    qCDebug(JoystickAndroidLog) << "Created joystick:" << name
                                 << "axes:" << _axisCount
                                 << "buttons:" << _buttonCount
                                 << "gamepad:" << _isGamepad
                                 << "vendor:" << QString::number(_vendorId, 16)
                                 << "product:" << QString::number(_productId, 16);

    QtAndroidPrivate::registerGenericMotionEventListener(this);
    QtAndroidPrivate::registerKeyEventListener(this);

    QMutexLocker lock(&_mutex);
    _instances.insert(_deviceId, this);
}

JoystickAndroid::~JoystickAndroid()
{
    {
        QMutexLocker lock(&_mutex);
        _instances.remove(_deviceId);
    }

    // Close lights session if open
    if (_lightsSession.isValid()) {
        _lightsSession.callMethod<void>("close");
    }

    QtAndroidPrivate::unregisterGenericMotionEventListener(this);
    QtAndroidPrivate::unregisterKeyEventListener(this);
}

void JoystickAndroid::_cacheDeviceInfo()
{
    const QJniObject inputDevice = QJniObject::callStaticObjectMethod(
        "android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", _deviceId);

    if (!inputDevice.isValid()) {
        return;
    }

    // Get descriptor (API 16+) - used as GUID
    if (AndroidInterface::getApiLevel() >= 16) {
        _descriptor = inputDevice.callObjectMethod("getDescriptor", "()Ljava/lang/String;").toString();
    }

    // Get vendor/product IDs (API 19+)
    if (AndroidInterface::getApiLevel() >= 19) {
        _vendorId = static_cast<quint16>(inputDevice.callMethod<jint>("getVendorId"));
        _productId = static_cast<quint16>(inputDevice.callMethod<jint>("getProductId"));
    }

    // Check if virtual (API 16+)
    _isVirtual = inputDevice.callMethod<jboolean>("isVirtual");

    // Get sources and determine if gamepad
    _sources = inputDevice.callMethod<jint>("getSources");
    _isGamepad = ((_sources & SOURCE_GAMEPAD) == SOURCE_GAMEPAD) ||
                 ((_sources & SOURCE_JOYSTICK) == SOURCE_JOYSTICK);

    // Check for vibrator (API 16+)
    const QJniObject vibrator = inputDevice.callObjectMethod("getVibrator", "()Landroid/os/Vibrator;");
    if (vibrator.isValid()) {
        _hasVibrator = vibrator.callMethod<jboolean>("hasVibrator");
    }

    _deviceType = _sources;
}

QJniObject JoystickAndroid::_getInputDevice() const
{
    return QJniObject::callStaticObjectMethod(
        "android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", _deviceId);
}

QJniObject JoystickAndroid::_getSensorManager() const
{
    if (AndroidInterface::getApiLevel() < 31) {
        return QJniObject();
    }

    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return QJniObject();
    }

    return inputDevice.callObjectMethod("getSensorManager", "()Landroid/hardware/SensorManager;");
}

QJniObject JoystickAndroid::_getLightsManager() const
{
    if (AndroidInterface::getApiLevel() < 31) {
        return QJniObject();
    }

    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return QJniObject();
    }

    return inputDevice.callObjectMethod("getLightsManager", "()Landroid/hardware/lights/LightsManager;");
}

QJniObject JoystickAndroid::_getSensor(int type) const
{
    const QJniObject sensorManager = _getSensorManager();
    if (!sensorManager.isValid()) {
        return QJniObject();
    }

    return sensorManager.callObjectMethod("getDefaultSensor", "(I)Landroid/hardware/Sensor;", type);
}

QJniObject JoystickAndroid::_getLightsList() const
{
    const QJniObject lightsManager = _getLightsManager();
    if (!lightsManager.isValid()) {
        return QJniObject();
    }

    return lightsManager.callObjectMethod("getLights", "()Ljava/util/List;");
}

QJniObject JoystickAndroid::_getBatteryState() const
{
    if (AndroidInterface::getApiLevel() < 31) {
        return QJniObject();
    }

    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return QJniObject();
    }

    const QJniObject battery = inputDevice.callObjectMethod("getBattery", "()Landroid/hardware/BatteryState;");
    if (!battery.isValid() || !battery.callMethod<jboolean>("isPresent")) {
        return QJniObject();
    }

    return battery;
}

QMap<QString, Joystick*> JoystickAndroid::discover()
{
    static QMap<QString, Joystick*> previous;
    QMap<QString, Joystick*> current;

    QMutexLocker lock(&_mutex);

    const QJniObject object = QJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = object.object<jintArray>();

    QJniEnvironment env;
    const int len = env->GetArrayLength(jarr);
    jint *const buff = env->GetIntArrayElements(jarr, nullptr);

    for (int i = 0; i < len; ++i) {
        const QJniObject inputDevice = QJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", buff[i]);
        const int sources = inputDevice.callMethod<jint>("getSources", "()I");
        if (((sources & SOURCE_GAMEPAD) != SOURCE_GAMEPAD) && ((sources & SOURCE_JOYSTICK) != SOURCE_JOYSTICK)) {
            continue;
        }

        const QString name = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();

        // Reuse existing joystick if present
        if (previous.contains(name)) {
            current[name] = previous[name];
            (void) previous.remove(name);
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
        env->DeleteLocalRef(arr);

        qCDebug(JoystickAndroidLog) << name << "id:" << buff[i] << "axes:" << axisCount << "buttons:" << buttonCount;

        current[name] = new JoystickAndroid(name, axisCount, buttonCount, buff[i]);
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);

    // Delete joysticks that are no longer present
    for (auto *joystick : std::as_const(previous)) {
        joystick->deleteLater();
    }

    previous = current;
    return current;
}

bool JoystickAndroid::_handleKeyEvent(jobject event)
{
    QMutexLocker lock(&_mutex);

    QJniObject ev(event);
    const int eventDeviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (eventDeviceId != _deviceId) {
        return false;
    }

    const int action = ev.callMethod<jint>("getAction", "()I");
    const int keyCode = ev.callMethod<jint>("getKeyCode", "()I");

    for (int i = 0; i < _buttonCount; i++) {
        if (_btnCode[i] != keyCode) {
            continue;
        }

        if (action == _actionDown) {
            _btnValue[i] = true;
        } else if (action == _actionUp) {
            _btnValue[i] = false;
        }

        return true;
    }

    return false;
}

bool JoystickAndroid::_handleGenericMotionEvent(jobject event)
{
    QMutexLocker lock(&_mutex);

    QJniObject ev(event);
    const int eventDeviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (eventDeviceId != _deviceId) {
        return false;
    }

    for (int i = 0; i < _axisCount; i++) {
        const float v = ev.callMethod<jfloat>("getAxisValue", "(I)F", _axisCode[i]);
        _axisValue[i] = static_cast<int>(v * AxisMax);
    }

    return true;
}

int JoystickAndroid::_getAndroidHatAxis(int axisHatCode) const
{
    for (int i = 0; i < _axisCount; i++) {
        if (_axisCode[i] == axisHatCode) {
            return _getAxisValue(i);
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
    case HAT_DIR_UP:
        return (_getAndroidHatAxis(_axisHatY) < 0);
    case HAT_DIR_DOWN:
        return (_getAndroidHatAxis(_axisHatY) > 0);
    case HAT_DIR_LEFT:
        return (_getAndroidHatAxis(_axisHatX) < 0);
    case HAT_DIR_RIGHT:
        return (_getAndroidHatAxis(_axisHatX) > 0);
    default:
        return false;
    }
}

int JoystickAndroid::playerIndex() const
{
    if (AndroidInterface::getApiLevel() < 19) {
        return -1;
    }

    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return -1;
    }

    return inputDevice.callMethod<jint>("getControllerNumber");
}

QString JoystickAndroid::gamepadType() const
{
    if (!_isGamepad) {
        return QString();
    }

    switch (_vendorId) {
    case VENDOR_MICROSOFT:
        return QStringLiteral("Xbox");
    case VENDOR_SONY:
        return QStringLiteral("PlayStation");
    case VENDOR_NINTENDO:
        return QStringLiteral("Nintendo");
    case VENDOR_LOGITECH:
        return QStringLiteral("Logitech");
    default:
        return QStringLiteral("Generic");
    }
}

bool JoystickAndroid::hasButton(int button) const
{
    return button >= 0 && button < _buttonCount;
}

bool JoystickAndroid::hasAxis(int axis) const
{
    if (axis < 0 || axis >= _axisCount) {
        return false;
    }
    return _axisCode[axis] >= 0;
}

void JoystickAndroid::rumble(quint16 lowFreq, quint16 highFreq, quint32 durationMs)
{
    Q_UNUSED(lowFreq);
    Q_UNUSED(highFreq);

    if (!_hasVibrator || durationMs == 0) {
        return;
    }

    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return;
    }

    const QJniObject vibrator = inputDevice.callObjectMethod("getVibrator", "()Landroid/os/Vibrator;");
    if (!vibrator.isValid()) {
        return;
    }

    // API 26+ supports amplitude, but basic vibrate() works on API 16+
    if (AndroidInterface::getApiLevel() >= 26) {
        // Use VibrationEffect for better control
        const int amplitude = qMax(lowFreq, highFreq) * 255 / 65535;
        const QJniObject effect = QJniObject::callStaticObjectMethod(
            "android/os/VibrationEffect",
            "createOneShot",
            "(JI)Landroid/os/VibrationEffect;",
            static_cast<jlong>(durationMs),
            qBound(1, amplitude, 255));

        if (effect.isValid()) {
            vibrator.callMethod<void>("vibrate", "(Landroid/os/VibrationEffect;)V", effect.object());
        }
    } else {
        // Basic vibration for older APIs
        vibrator.callMethod<void>("vibrate", "(J)V", static_cast<jlong>(durationMs));
    }
}

int JoystickAndroid::batteryPercent() const
{
    const QJniObject battery = _getBatteryState();
    if (!battery.isValid()) {
        return -1;
    }

    const float capacity = battery.callMethod<jfloat>("getCapacity");
    return (capacity >= 0) ? static_cast<int>(capacity * 100.0F) : -1;
}

QString JoystickAndroid::powerState() const
{
    const QJniObject battery = _getBatteryState();
    if (!battery.isValid()) {
        return QString();
    }

    switch (battery.callMethod<jint>("getStatus")) {
    case BATTERY_STATUS_CHARGING: return QStringLiteral("Charging");
    case BATTERY_STATUS_DISCHARGING: return QStringLiteral("Discharging");
    case BATTERY_STATUS_NOT_CHARGING: return QStringLiteral("NotCharging");
    case BATTERY_STATUS_FULL: return QStringLiteral("Full");
    default: return QStringLiteral("Unknown");
    }
}

bool JoystickAndroid::hasLED() const
{
    const QJniObject lights = _getLightsList();
    return lights.isValid() && lights.callMethod<jint>("size") > 0;
}

bool JoystickAndroid::hasRGBLED() const
{
    // Android doesn't distinguish between mono and RGB LEDs in the API
    // Most controller LEDs that are accessible are RGB
    return hasLED();
}

void JoystickAndroid::setLED(quint8 red, quint8 green, quint8 blue)
{
    const QJniObject lights = _getLightsList();
    if (!lights.isValid() || lights.callMethod<jint>("size") == 0) {
        return;
    }

    const QJniObject light = lights.callObjectMethod("get", "(I)Ljava/lang/Object;", 0);
    if (!light.isValid()) {
        return;
    }

    const jint color = ALPHA_OPAQUE | (red << 16) | (green << 8) | blue;

    const QJniObject stateBuilder = QJniObject("android/hardware/lights/LightState$Builder");
    stateBuilder.callObjectMethod("setColor", "(I)Landroid/hardware/lights/LightState$Builder;", color);
    const QJniObject lightState = stateBuilder.callObjectMethod("build", "()Landroid/hardware/lights/LightState;");

    const QJniObject requestBuilder = QJniObject("android/hardware/lights/LightsRequest$Builder");
    requestBuilder.callObjectMethod(
        "addLight",
        "(Landroid/hardware/lights/Light;Landroid/hardware/lights/LightState;)Landroid/hardware/lights/LightsRequest$Builder;",
        light.object(), lightState.object());
    const QJniObject request = requestBuilder.callObjectMethod("build", "()Landroid/hardware/lights/LightsRequest;");

    // Open session if not already open (reuse for subsequent LED changes)
    if (!_lightsSession.isValid()) {
        const QJniObject lightsManager = _getLightsManager();
        _lightsSession = lightsManager.callObjectMethod(
            "openSession", "()Landroid/hardware/lights/LightsManager$LightsSession;");
    }

    if (_lightsSession.isValid()) {
        _lightsSession.callMethod<void>("requestLights", "(Landroid/hardware/lights/LightsRequest;)V", request.object());
    }
}

bool JoystickAndroid::hasGyroscope() const
{
    return _getSensor(SENSOR_TYPE_GYROSCOPE).isValid();
}

bool JoystickAndroid::hasAccelerometer() const
{
    return _getSensor(SENSOR_TYPE_ACCELEROMETER).isValid();
}

bool JoystickAndroid::_setSensorEnabled(int sensorType, const char *jniMethodName, bool enabled, bool &enabledFlag)
{
    if (!_getSensor(sensorType).isValid()) {
        enabledFlag = false;
        return !enabled;
    }

    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        "org/mavlink/qgroundcontrol/QGCJoystickManager",
        jniMethodName,
        "(IZ)Z",
        _deviceId,
        static_cast<jboolean>(enabled));

    enabledFlag = result && enabled;
    return result;
}

bool JoystickAndroid::setGyroscopeEnabled(bool enabled)
{
    return _setSensorEnabled(SENSOR_TYPE_GYROSCOPE, "enableGyroscope", enabled, _gyroEnabled);
}

bool JoystickAndroid::setAccelerometerEnabled(bool enabled)
{
    return _setSensorEnabled(SENSOR_TYPE_ACCELEROMETER, "enableAccelerometer", enabled, _accelEnabled);
}

QVector3D JoystickAndroid::gyroscopeData() const
{
    return _gyroData;
}

QVector3D JoystickAndroid::accelerometerData() const
{
    return _accelData;
}

QString JoystickAndroid::axisLabel(int axis) const
{
    if (axis < 0 || axis >= _axisCount) {
        return QString();
    }

    const int code = _axisCode[axis];

    switch (code) {
    case AXIS_X: return QStringLiteral("X");
    case AXIS_Y: return QStringLiteral("Y");
    case AXIS_Z: return QStringLiteral("Z");
    case AXIS_RX: return QStringLiteral("RX");
    case AXIS_RY: return QStringLiteral("RY");
    case AXIS_RZ: return QStringLiteral("RZ");
    case AXIS_HAT_X: return QStringLiteral("Hat X");
    case AXIS_HAT_Y: return QStringLiteral("Hat Y");
    case AXIS_LTRIGGER: return QStringLiteral("LT");
    case AXIS_RTRIGGER: return QStringLiteral("RT");
    case AXIS_THROTTLE: return QStringLiteral("Throttle");
    case AXIS_RUDDER: return QStringLiteral("Rudder");
    case AXIS_WHEEL: return QStringLiteral("Wheel");
    case AXIS_GAS: return QStringLiteral("Gas");
    case AXIS_BRAKE: return QStringLiteral("Brake");
    default: return QStringLiteral("Axis %1").arg(axis);
    }
}

QString JoystickAndroid::buttonLabel(int button) const
{
    if (button < 0 || button >= _buttonCount) {
        return QString();
    }

    const int code = _btnCode[button];

    switch (code) {
    case KEYCODE_BUTTON_A: return QStringLiteral("A");
    case KEYCODE_BUTTON_B: return QStringLiteral("B");
    case KEYCODE_BUTTON_C: return QStringLiteral("C");
    case KEYCODE_BUTTON_X: return QStringLiteral("X");
    case KEYCODE_BUTTON_Y: return QStringLiteral("Y");
    case KEYCODE_BUTTON_Z: return QStringLiteral("Z");
    case KEYCODE_BUTTON_L1: return QStringLiteral("L1");
    case KEYCODE_BUTTON_R1: return QStringLiteral("R1");
    case KEYCODE_BUTTON_L2: return QStringLiteral("L2");
    case KEYCODE_BUTTON_R2: return QStringLiteral("R2");
    case KEYCODE_BUTTON_THUMBL: return QStringLiteral("L3");
    case KEYCODE_BUTTON_THUMBR: return QStringLiteral("R3");
    case KEYCODE_BUTTON_START: return QStringLiteral("Start");
    case KEYCODE_BUTTON_SELECT: return QStringLiteral("Select");
    case KEYCODE_BUTTON_MODE: return QStringLiteral("Mode");
    default:
        // KEYCODE_BUTTON_1 through BUTTON_16 are 188-203
        if (code >= KEYCODE_BUTTON_1 && code <= KEYCODE_BUTTON_16) {
            return QStringLiteral("Button %1").arg(code - KEYCODE_BUTTON_1 + 1);
        }
        return QStringLiteral("Button %1").arg(button);
    }
}

QString JoystickAndroid::serial() const
{
    // Android doesn't directly expose serial number for input devices
    // but we can try to get it from the descriptor which may contain unique identifiers
    return _descriptor;
}

QString JoystickAndroid::deviceType() const
{
    QStringList types;

    if ((_deviceType & SOURCE_GAMEPAD) == SOURCE_GAMEPAD) {
        types << QStringLiteral("Gamepad");
    }
    if ((_deviceType & SOURCE_JOYSTICK) == SOURCE_JOYSTICK) {
        types << QStringLiteral("Joystick");
    }
    if ((_deviceType & SOURCE_DPAD) == SOURCE_DPAD) {
        types << QStringLiteral("DPad");
    }
    if ((_deviceType & SOURCE_KEYBOARD) == SOURCE_KEYBOARD) {
        types << QStringLiteral("Keyboard");
    }

    if (types.isEmpty()) {
        return QStringLiteral("Unknown");
    }

    return types.join(QStringLiteral(", "));
}

QString JoystickAndroid::connectionType() const
{
    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return QString();
    }

    // Check if device is external (API 29+)
    if (AndroidInterface::getApiLevel() >= 29) {
        const bool isExternal = inputDevice.callMethod<jboolean>("isExternal");
        if (!isExternal) {
            return QStringLiteral("Internal");
        }
    }

    // Try to determine connection type from sources and characteristics.
    // Note: Android's InputDevice API doesn't directly expose connection type,
    // so we use a heuristic based on the device descriptor string. This may
    // not be accurate for all devices - some Bluetooth devices may be reported
    // as USB and vice versa.
    const QString descriptor = _descriptor.toLower();
    if (descriptor.contains(QStringLiteral("bluetooth")) ||
        descriptor.contains(QStringLiteral("bt"))) {
        return QStringLiteral("Bluetooth");
    }

    // Default to USB for external devices when connection type cannot be determined
    return QStringLiteral("USB");
}

QVariantMap JoystickAndroid::getAxisInitialState(int axis) const
{
    QVariantMap result;
    result[QStringLiteral("valid")] = false;

    if (axis < 0 || axis >= _axisCount) {
        return result;
    }

    const QJniObject inputDevice = _getInputDevice();
    if (!inputDevice.isValid()) {
        return result;
    }

    const int code = _axisCode.at(axis);
    if (code < 0) {
        return result;
    }

    const QJniObject motionRange = inputDevice.callObjectMethod(
        "getMotionRange", "(I)Landroid/view/InputDevice$MotionRange;", code);

    if (!motionRange.isValid()) {
        return result;
    }

    result[QStringLiteral("valid")] = true;
    result[QStringLiteral("min")] = motionRange.callMethod<jfloat>("getMin");
    result[QStringLiteral("max")] = motionRange.callMethod<jfloat>("getMax");
    result[QStringLiteral("flat")] = motionRange.callMethod<jfloat>("getFlat");
    result[QStringLiteral("fuzz")] = motionRange.callMethod<jfloat>("getFuzz");
    result[QStringLiteral("resolution")] = motionRange.callMethod<jfloat>("getResolution");

    return result;
}

float JoystickAndroid::_getSensorDataRate(int sensorType) const
{
    const QJniObject sensor = _getSensor(sensorType);
    if (!sensor.isValid()) {
        return 0.0f;
    }

    // getMinDelay() returns minimum delay in microseconds (fastest rate)
    const int minDelayUs = sensor.callMethod<jint>("getMinDelay");
    return (minDelayUs > 0) ? 1000000.0f / static_cast<float>(minDelayUs) : 0.0f;
}

float JoystickAndroid::gyroscopeDataRate() const
{
    return _getSensorDataRate(SENSOR_TYPE_GYROSCOPE);
}

float JoystickAndroid::accelerometerDataRate() const
{
    return _getSensorDataRate(SENSOR_TYPE_ACCELEROMETER);
}

bool JoystickAndroid::hasPlayerLED() const
{
    const QJniObject lights = _getLightsList();
    if (!lights.isValid()) {
        return false;
    }

    const int lightCount = lights.callMethod<jint>("size");

    for (int i = 0; i < lightCount; ++i) {
        const QJniObject light = lights.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
        if (light.isValid() && light.callMethod<jint>("getType") == LIGHT_TYPE_PLAYER_ID) {
            return true;
        }
    }

    return false;
}

bool JoystickAndroid::init()
{
    QList<int> buttonCodes(_androidBtnListCount);

    (void) AndroidInterface::cleanJavaException();

    // Populate numbered buttons (KEYCODE_BUTTON_1 through KEYCODE_BUTTON_16)
    int idx = 0;
    for (int btnNum = 1; btnNum <= NUMBERED_BUTTON_COUNT; btnNum++) {
        const QString name = QStringLiteral("KEYCODE_BUTTON_") + QString::number(btnNum);
        buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", name.toStdString().c_str());
    }

    // Populate named buttons (A, B, C, L1, L2, R1, R2, etc.)
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_A");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_B");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_C");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L1");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L2");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R1");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R2");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_MODE");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_SELECT");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_START");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBL");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBR");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_X");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Y");
    buttonCodes[idx++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Z");

    _actionDown = QJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_DOWN");
    _actionUp = QJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_UP");
    _axisHatX = QJniObject::getStaticField<jint>("android/view/MotionEvent", "AXIS_HAT_X");
    _axisHatY = QJniObject::getStaticField<jint>("android/view/MotionEvent", "AXIS_HAT_Y");

    _androidBtnList = buttonCodes;

    return true;
}

static void jniUpdateAvailableJoysticks(JNIEnv *envA, jobject thizA)
{
    Q_UNUSED(envA); Q_UNUSED(thizA);

    qCDebug(JoystickAndroidLog) << "jniUpdateAvailableJoysticks triggered";

    JoystickManager *manager = JoystickManager::instance();
    if (manager) {
        emit manager->updateAvailableJoysticks();
    }
}

static void jniSensorData(JNIEnv *envA, jobject thizA, jint deviceId, jint sensorType, jfloat x, jfloat y, jfloat z)
{
    Q_UNUSED(envA); Q_UNUSED(thizA);

    JoystickAndroid::_updateSensorData(deviceId, sensorType, x, y, z);
}

void JoystickAndroid::_updateSensorData(int deviceId, int sensorType, float x, float y, float z)
{
    QMutexLocker lock(&_mutex);

    JoystickAndroid *joystick = _instances.value(deviceId, nullptr);
    if (!joystick) {
        return;
    }

    const QVector3D data(x, y, z);

    if (sensorType == SENSOR_TYPE_GYROSCOPE) {
        joystick->_gyroData = data;
        emit joystick->gyroscopeDataUpdated(data);
    } else if (sensorType == SENSOR_TYPE_ACCELEROMETER) {
        joystick->_accelData = data;
        emit joystick->accelerometerDataUpdated(data);
    }
}

void JoystickAndroid::setNativeMethods()
{
    qCDebug(JoystickAndroidLog) << "Registering Native Functions";

    static const JNINativeMethod javaMethods[] {
        {"nativeUpdateAvailableJoysticks", "()V", reinterpret_cast<void*>(jniUpdateAvailableJoysticks)},
        {"nativeSensorData", "(IIFFF)V", reinterpret_cast<void*>(jniSensorData)}
    };

    static constexpr const char *kJniClassName = "org/mavlink/qgroundcontrol/QGCJoystickManager";

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
