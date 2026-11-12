#include "AndroidSerial.h"

#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QMutex>
#include <QtCore/QStringList>
#include <QtCore/QtJniTypes>
#include <qserialportinfo_p.h>

#include "AndroidInterface.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidSerialSelfTestLog, "Android.AndroidSerial.SelfTest")

// The Java class of a port that availablePorts() lists, for the signature below.
Q_DECLARE_JNI_CLASS(QGCUsbSerialPortInfo, "org/mavlink/qgroundcontrol/serial/QGCUsbSerialPortInfo")

namespace AndroidSerial {

// ----------------------------------------------------------------------------
// JNI method ID cache
// ----------------------------------------------------------------------------

template <typename Id>
struct JavaBinding;

/// The Java methods or fields that C++ binds, in the order in which they are declared.
template <typename Id>
struct JavaBindingList
{
    JavaBinding<Id>* first = nullptr;
    JavaBinding<Id>* last = nullptr;
};

/// A method or field that JNI finds in a Java class by name and signature, with the ID it is found under. Each
/// one adds itself to the list of its kind, over which all of them are looked up, so that none is listed twice.
template <typename Id>
struct JavaBinding
{
    JavaBinding(const char* javaName, const char* jniSignature, JavaBindingList<Id>& list)
        : name(javaName)
        , signature(jniSignature)
    {
        (list.last ? list.last->next : list.first) = this;
        list.last = this;
    }

    Q_DISABLE_COPY_MOVE(JavaBinding)

    const char* const name;
    const char* const signature;
    // Null until the IDs are cached, and where Java has no such method or field.
    Id id = nullptr;
    JavaBinding* next = nullptr;
};

static JavaBindingList<jmethodID> s_javaMethods;
static JavaBindingList<jfieldID> s_javaFields;

/// A static method of QGCUsbSerialManager.java, declared by the JNI types of what it returns and of what it
/// takes. JNI takes the arguments of a call as C varargs, so arguments that disagree with the signature the
/// method was found under would compile, and corrupt the call. Here the signature is derived from the declared
/// types, and a call compiles only with arguments of exactly those types.
template <typename Ret, typename... Args>
struct JavaMethod : JavaBinding<jmethodID>
{
    static constexpr auto kSignature = QtJniTypes::methodSignature<Ret, Args...>();
    /// What JNI returns from a call: an array of a Java class is a jobjectArray to it.
    using Returned = std::conditional_t<std::is_array_v<Ret>, jobjectArray, Ret>;

    explicit JavaMethod(const char* javaName)
        : JavaBinding<jmethodID>(javaName, kSignature.data(), s_javaMethods)
    {}

    /// Calls the method, which the caller has found available. The caller then checks whether Java threw.
    Returned call(JNIEnv* env, jclass javaClass, Args... args) const
    {
        if constexpr (std::is_same_v<Ret, jboolean>) {
            return env->CallStaticBooleanMethod(javaClass, id, args...);
        } else if constexpr (std::is_same_v<Ret, jlong>) {
            return env->CallStaticLongMethod(javaClass, id, args...);
        } else if constexpr (std::is_same_v<Ret, jint>) {
            return env->CallStaticIntMethod(javaClass, id, args...);
        } else {
            return static_cast<Returned>(env->CallStaticObjectMethod(javaClass, id, args...));
        }
    }

    // Arguments that the call above would have to convert select this instead: a conversion could hide a mix-up.
    template <typename... Other>
    Returned call(JNIEnv*, jclass, Other...) const = delete;
};

/// A field of QGCUsbSerialPortInfo.java, declared by its JNI type, from which its signature is derived.
template <typename T>
struct JavaField : JavaBinding<jfieldID>
{
    static constexpr auto kSignature = QtJniTypes::fieldSignature<T>();

    explicit JavaField(const char* javaName)
        : JavaBinding<jfieldID>(javaName, kSignature.data(), s_javaFields)
    {}
};

// Each method and field is declared under its own name.
struct JniMethodCache
{
    JavaMethod<QtJniTypes::QGCUsbSerialPortInfo[]> availablePorts{"availablePorts"};
    JavaMethod<jboolean, jstring> hasPermission{"hasPermission"};
    JavaMethod<jboolean, jstring> requestPermission{"requestPermission"};
    JavaMethod<jboolean, jstring> isPortNameValid{"isPortNameValid"};
    JavaMethod<jlong, jstring> getPortHandle{"getPortHandle"};
    JavaMethod<jstring> getLastErrorDetail{"getLastErrorDetail"};
    JavaMethod<jint, jlong, jbyteArray, jint, jint, jint, jstring> echoToCallbacks{"echoToCallbacks"};
    JavaMethod<jint, jstring, jlong> open{"open"};
    JavaMethod<jint, jlong> close{"close"};
    JavaMethod<jint, jlong> getFileDescriptor{"getFileDescriptor"};
    JavaMethod<jint, jlong> startReading{"startReading"};
    JavaMethod<jint, jlong> stopReading{"stopReading"};
    JavaMethod<jboolean, jlong> isReading{"isReading"};
    JavaMethod<jbyteArray, jlong, jint, jint> read{"read"};
    JavaMethod<jint, jlong, jbyteArray, jint, jint> writeWithProgress{"writeWithProgress"};
    JavaMethod<jint, jlong, jbyteArray, jint> writeAsync{"writeAsync"};
    JavaMethod<jint, jlong, jint, jint, jint, jint> setParameters{"setParameters"};
    JavaMethod<jint, jlong> getFlowControl{"getFlowControl"};
    JavaMethod<jint, jlong, jint> setFlowControl{"setFlowControl"};
    JavaMethod<jintArray, jlong> getSupportedFlowControl{"getSupportedFlowControl"};
    JavaMethod<jboolean, jlong> getXon{"getXon"};
    JavaMethod<jint, jlong, jint> setLatencyTimer{"setLatencyTimer"};
    JavaMethod<jint, jlong, jboolean> setBreak{"setBreak"};
    JavaMethod<jint, jlong, jboolean, jboolean> purgeBuffers{"purgeBuffers"};
    JavaMethod<jint, jlong, jboolean> setDataTerminalReady{"setDataTerminalReady"};
    JavaMethod<jint, jlong, jboolean> setRequestToSend{"setRequestToSend"};
    JavaMethod<jintArray, jlong> getControlLines{"getControlLines"};
    JavaMethod<jintArray, jlong> getSupportedControlLines{"getSupportedControlLines"};
};

static JniMethodCache s_methods;
static QMutex s_cacheLock;
// Published only after s_methods is filled, so a non-null load also means the method IDs are ready.
static std::atomic<jclass> s_serialManagerClass{nullptr};

static void cacheMethodIds(JNIEnv* env, jclass javaClass)
{
    int missing = 0;
    for (JavaBinding<jmethodID>* method = s_javaMethods.first; method; method = method->next) {
        method->id = env->GetStaticMethodID(javaClass, method->name, method->signature);
        if (!method->id) {
            // Leave it null so only the calls that need this method fail, not the whole serial backend.
            qCCritical(AndroidSerialLog) << "Java method not found:" << method->name << method->signature;
            (void) QJniEnvironment::checkAndClearExceptions(env);
            ++missing;
        }
    }

    qCDebug(AndroidSerialLog) << "Cached JNI method IDs," << missing << "missing";
}

struct JniPortInfoFields
{
    JavaField<jstring> name{"name"};
    JavaField<jstring> stableName{"stableName"};
    JavaField<jstring> productName{"productName"};
    JavaField<jstring> manufacturerName{"manufacturerName"};
    JavaField<jstring> serialNumber{"serialNumber"};
    JavaField<jint> vendorId{"vendorId"};
    JavaField<jint> productId{"productId"};
};

static JniPortInfoFields s_portInfoFields;

static void cachePortInfoFieldIds(QJniEnvironment& env)
{
    // Qt keeps the class it finds referenced, so the field IDs stay valid.
    const jclass portInfoClass = env.findClass<QtJniTypes::QGCUsbSerialPortInfo>();
    if (!portInfoClass) {
        qCCritical(AndroidSerialLog) << "Class Not Found:"
                                     << QtJniTypes::Traits<QtJniTypes::QGCUsbSerialPortInfo>::className().data();
        return;
    }

    for (JavaBinding<jfieldID>* field = s_javaFields.first; field; field = field->next) {
        field->id = env->GetFieldID(portInfoClass, field->name, field->signature);
        if (!field->id) {
            // Leave it null so the port list only lacks this field.
            qCCritical(AndroidSerialLog) << "Java field not found:" << field->name << field->signature;
            (void) env.checkAndClearExceptions();
        }
    }
}

// ----------------------------------------------------------------------------
// Class resolution
// ----------------------------------------------------------------------------

static jclass getSerialManagerClass()
{
    if (const jclass cached = s_serialManagerClass.load(std::memory_order_acquire)) {
        return cached;
    }

    QMutexLocker locker(&s_cacheLock);
    if (const jclass cached = s_serialManagerClass.load(std::memory_order_relaxed)) {
        return cached;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment";
        return nullptr;
    }

    // Qt keeps the class it finds referenced, so the class and its method IDs stay valid.
    const jclass serialManagerClass = env.findClass(kJniUsbSerialManagerClassName);
    if (!serialManagerClass) {
        qCWarning(AndroidSerialLog) << "Class Not Found:" << kJniUsbSerialManagerClassName;
        return nullptr;
    }

    cacheMethodIds(env.jniEnv(), serialManagerClass);
    cachePortInfoFieldIds(env);
    (void) env.checkAndClearExceptions();
    s_serialManagerClass.store(serialManagerClass, std::memory_order_release);
    return serialManagerClass;
}

// ----------------------------------------------------------------------------
// Native method registration
// ----------------------------------------------------------------------------

// The JNI callbacks, defined below, each with the native method of QGCUsbSerialManager.java that it is registered
// as. Qt derives the JNI signature to register it under from the function's own types; one written by hand
// that disagreed with them would compile, and corrupt the arguments of every call. Qt's macros for a function of
// a scope are used because its plain ones take only a function of the global namespace.
static jboolean jniDeviceNewData(JNIEnv* env, jclass javaClass, jlong token, jbyteArray data, jint length);
static void jniDeviceBytesWritten(JNIEnv* env, jclass javaClass, jlong token, jint count);
static void jniDeviceResourceError(JNIEnv* env, jclass javaClass, jlong token, jint reason, jstring detail);
static void jniDevicesChanged(JNIEnv* env, jclass javaClass);
Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceNewData, nativeDeviceNewData)
Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceBytesWritten, nativeDeviceBytesWritten)
Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceResourceError, nativeDeviceResourceError)
Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDevicesChanged, nativeDevicesChanged)

bool setNativeMethods()
{
    qCDebug(AndroidSerialLog) << "Registering Native Functions";

    const JNINativeMethod javaMethods[]{
        Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceNewData, AndroidSerial),
        Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceBytesWritten, AndroidSerial),
        Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceResourceError, AndroidSerial),
        Q_JNI_NATIVE_SCOPED_METHOD(jniDevicesChanged, AndroidSerial),
    };

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment";
        return false;
    }

    if (!env.registerNativeMethods(kJniUsbSerialManagerClassName, javaMethods, std::size(javaMethods))) {
        qCWarning(AndroidSerialLog) << "Failed to register native methods for" << kJniUsbSerialManagerClassName;
        return false;
    }

    if (!getSerialManagerClass()) {
        qCWarning(AndroidSerialLog) << "Failed to resolve" << kJniUsbSerialManagerClassName;
        return false;
    }

    qCDebug(AndroidSerialLog) << "Native Functions Registered Successfully";
    return true;
}

// ----------------------------------------------------------------------------
// JNI callbacks (called from Java threads)
// ----------------------------------------------------------------------------

// The first length bytes of data are the input. The array is the buffer of Java's read thread, which the next read
// overwrites, so the bytes are copied before this returns. Returns whether the port has room for more input: if
// not, Java's read thread waits until the port starts reading again. Input that reaches no port holds nothing back.
static jboolean jniDeviceNewData(JNIEnv* env, jclass, jlong token, jbyteArray data, jint length)
{
    constexpr jsize kMaxNativePayloadBytes = 16 * 1024;

    if (token == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with token=0";
        return JNI_TRUE;
    }

    if (!data) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with null data";
        return JNI_TRUE;
    }

    const jsize arrayLen = env->GetArrayLength(data);
    if (length <= 0 || length > arrayLen) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with length" << length << "for an array of"
                                    << arrayLen << "bytes";
        return JNI_TRUE;
    }

    const jsize cappedLen = (length > kMaxNativePayloadBytes) ? kMaxNativePayloadBytes : length;
    if (cappedLen != length) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData payload exceeds limit, truncating from" << length << "to"
                                    << cappedLen << "bytes";
    }

    // On the stack, since this runs for every read. The stack of a Java thread has room for it.
    char payload[kMaxNativePayloadBytes];
    env->GetByteArrayRegion(data, 0, cappedLen, reinterpret_cast<jbyte*>(payload));
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData failed to copy JNI byte array";
        return JNI_TRUE;
    }

    return deliverNewData(token, payload, static_cast<int>(cappedLen)) ? JNI_TRUE : JNI_FALSE;
}

static void jniDeviceBytesWritten(JNIEnv*, jclass, jlong token, jint count)
{
    if (count <= 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceBytesWritten called with count" << count;
        return;
    }

    deliverBytesWritten(token, count);
}

// The port can no longer transfer data. The reason is a QGCUsbSerialResourceError of Java.
static void jniDeviceResourceError(JNIEnv*, jclass, jlong token, jint reason, jstring detail)
{
    if (token == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceResourceError called with token=0";
        return;
    }

    // Java passes an empty detail, not null, where it has none. The port is told of its loss either way.
    const QString errorDetail = detail ? QJniObject(detail).toString() : QString();
    // The port that the error belongs to warns of it, and so did Java.
    qCDebug(AndroidSerialLog) << "nativeDeviceResourceError:" << reason << errorDetail;

    // A reason that ResourceErrorReason does not list still reaches the port, which has a text for it.
    deliverResourceError(token, static_cast<ResourceErrorReason>(reason), errorDetail);
}

DeviceWatcher::DeviceWatcher()
{
    // The first to ask for the watcher may be the Android thread that reports a change, which would otherwise
    // own it and may end long before the application does.
    if (const QCoreApplication* const application = QCoreApplication::instance()) {
        moveToThread(application->thread());
    }
}

DeviceWatcher* DeviceWatcher::instance()
{
    static DeviceWatcher watcher;
    return &watcher;
}

static void jniDevicesChanged(JNIEnv*, jclass)
{
    qCDebug(AndroidSerialLog) << "USB serial devices changed";

    DeviceWatcher* const watcher = DeviceWatcher::instance();
    emit watcher->devicesChanged();
}

// ----------------------------------------------------------------------------
// Calling Java
// ----------------------------------------------------------------------------

// What the exception that ended this thread's last call into Java says of itself, and nothing once a call has
// returned. Java keeps the detail of a failure only where it catches the failure.
static thread_local QString t_exceptionDetail;

/// Returns whether the call into Java that this thread has just made threw, after a warning. What the exception
/// says of itself is kept for getLastErrorDetail().
static bool javaThrew(QJniEnvironment& env, const char* methodName)
{
    t_exceptionDetail.clear();
    if (!env->ExceptionCheck()) {
        return false;
    }

    const AndroidInterface::JniLocalRef<jthrowable> exception(env.jniEnv(), env->ExceptionOccurred());
    // Logs the exception with its stack trace. Java can be asked for its text only once it is cleared.
    (void) env.checkAndClearExceptions();
    if (exception.get()) {
        t_exceptionDetail = QJniObject(exception.get()).toString();
    }
    qCWarning(AndroidSerialLog) << "Exception occurred while calling" << methodName;
    return true;
}

/// Returns the serial manager class, or null after a warning if it or the method is unavailable.
static jclass findJavaMethod(QJniEnvironment& env, const JavaBinding<jmethodID>& method)
{
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in" << method.name;
        return nullptr;
    }

    const jclass cls = getSerialManagerClass();
    if (!cls) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in" << method.name;
        return nullptr;
    }

    if (!method.id) {
        qCWarning(AndroidSerialLog) << "Java method unavailable in" << method.name;
        return nullptr;
    }

    return cls;
}

/// Calls a static Java method that returns an int, a long or a boolean. Returns fallback, after a warning, if
/// the method is unavailable or throws. The arguments have the JNI types that the method declares.
template <typename Ret, typename... Args>
static Ret callJava(QJniEnvironment& env, const JavaMethod<Ret, Args...>& method, std::type_identity_t<Ret> fallback,
                    Args... args)
{
    static_assert(std::is_same_v<Ret, jint> || std::is_same_v<Ret, jlong> || std::is_same_v<Ret, jboolean>);

    const jclass cls = findJavaMethod(env, method);
    if (!cls) {
        return fallback;
    }

    const Ret result = method.call(env.jniEnv(), cls, args...);
    if (javaThrew(env, method.name)) {
        return fallback;
    }

    return result;
}

/// Calls a static Java method that returns an object, such as an array. The reference is null, after a
/// warning, if the method is unavailable, throws or returns null.
template <typename Ret, typename... Args>
static auto callJavaObject(QJniEnvironment& env, const JavaMethod<Ret, Args...>& method, Args... args)
{
    AndroidInterface::JniLocalRef<typename JavaMethod<Ret, Args...>::Returned> result(env.jniEnv());

    const jclass cls = findJavaMethod(env, method);
    if (!cls) {
        return result;
    }

    result.reset(method.call(env.jniEnv(), cls, args...));
    if (javaThrew(env, method.name)) {
        result.reset();
    } else if (!result.get()) {
        qCWarning(AndroidSerialLog) << method.name << "returned null";
    }

    return result;
}

/// Calls a static Java method that returns a QGCUsbSerialResult.
template <typename... Args>
static Result callJavaCommand(const JavaMethod<jint, Args...>& method, Args... args)
{
    QJniEnvironment env;
    const jint result = callJava(env, method, static_cast<jint>(Result::Failed), args...);
    if (result > static_cast<jint>(Result::Ok) || result < static_cast<jint>(Result::AlreadyOpen)) {
        qCWarning(AndroidSerialLog) << method.name << "returned" << result << "which is not a result";
        return Result::Failed;
    }

    return static_cast<Result>(result);
}

static jboolean toJava(bool value)
{
    return value ? JNI_TRUE : JNI_FALSE;
}

/// Java takes a timeout as a count of milliseconds in an int.
static jint toJava(std::chrono::milliseconds timeout)
{
    return static_cast<jint>(
        std::clamp<std::chrono::milliseconds::rep>(timeout.count(), 0, (std::numeric_limits<jint>::max)()));
}

static bool callJavaBool(const JavaMethod<jboolean, jlong>& method, PortHandle handle)
{
    QJniEnvironment env;
    return callJava(env, method, JNI_FALSE, static_cast<jlong>(handle)) == JNI_TRUE;
}

static bool callJavaBool(const JavaMethod<jboolean, jstring>& method, const QString& portName)
{
    const QJniObject jname = QJniObject::fromString(portName);
    if (!jname.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in" << method.name;
        return false;
    }

    QJniEnvironment env;
    return callJava(env, method, JNI_FALSE, jname.object<jstring>()) == JNI_TRUE;
}

static QList<int> callJavaIntArray(const JavaMethod<jintArray, jlong>& method, PortHandle handle)
{
    // A Java int is a C++ int on Android, so the array is copied straight into the list.
    static_assert(std::is_same_v<jint, int>);
    QList<int> ints;

    QJniEnvironment env;
    const auto jarray = callJavaObject(env, method, static_cast<jlong>(handle));
    if (!jarray.get()) {
        return ints;
    }

    ints.resize(env->GetArrayLength(jarray.get()));
    env->GetIntArrayRegion(jarray.get(), 0, static_cast<jsize>(ints.size()), ints.data());
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Failed to read the int array from" << method.name;
        ints.clear();
    }

    return ints;
}

static AndroidInterface::JniLocalRef<jbyteArray> toJavaBytes(QJniEnvironment& env, const char* data, int length,
                                                             const char* caller)
{
    AndroidInterface::JniLocalRef<jbyteArray> jarray(env.jniEnv());
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in" << caller;
        return jarray;
    }

    jarray.reset(env->NewByteArray(static_cast<jsize>(length)));
    if (!jarray.get()) {
        qCWarning(AndroidSerialLog) << "Failed to create jbyteArray in" << caller;
        (void) env.checkAndClearExceptions();
        return jarray;
    }

    env->SetByteArrayRegion(jarray.get(), 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(data));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while setting byte array region in" << caller;
        jarray.reset();
    }

    return jarray;
}

// ----------------------------------------------------------------------------
// Devices
// ----------------------------------------------------------------------------

static QString stringField(JNIEnv* env, jobject object, const JavaField<jstring>& field)
{
    QString string;
    if (!field.id) {
        return string;
    }

    const AndroidInterface::JniLocalRef<jstring> value(env,
                                                       static_cast<jstring>(env->GetObjectField(object, field.id)));
    if (value.get()) {
        string.resize(env->GetStringLength(value.get()));
        env->GetStringRegion(value.get(), 0, static_cast<jsize>(string.size()),
                             reinterpret_cast<jchar*>(string.data()));
    }

    return string;
}

static quint16 usbIdField(JNIEnv* env, jobject object, const JavaField<jint>& field)
{
    return field.id ? static_cast<quint16>(env->GetIntField(object, field.id)) : 0;
}

QList<QSerialPortInfo> availablePorts()
{
    const auto& method = s_methods.availablePorts;
    QList<QSerialPortInfo> serialPortInfoList;

    QJniEnvironment env;
    const auto portInfos = callJavaObject(env, method);
    if (!portInfos.get()) {
        return serialPortInfoList;
    }

    const jsize count = env->GetArrayLength(portInfos.get());
    for (jsize i = 0; i < count; ++i) {
        const AndroidInterface::JniLocalRef<jobject> portInfo(env.jniEnv(),
                                                              env->GetObjectArrayElement(portInfos.get(), i));
        if (!portInfo.get()) {
            qCWarning(AndroidSerialLog) << method.name << "returned null at index" << i;
            continue;
        }

        QSerialPortInfoPrivate info;
        info.device = stringField(env.jniEnv(), portInfo.get(), s_portInfoFields.name);
        if (info.device.isEmpty()) {
            qCWarning(AndroidSerialLog) << method.name << "returned a port without a name at index" << i;
            continue;
        }
        info.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(info.device);
        info.stableName = stringField(env.jniEnv(), portInfo.get(), s_portInfoFields.stableName);
        info.description = stringField(env.jniEnv(), portInfo.get(), s_portInfoFields.productName);
        info.manufacturer = stringField(env.jniEnv(), portInfo.get(), s_portInfoFields.manufacturerName);
        info.serialNumber = stringField(env.jniEnv(), portInfo.get(), s_portInfoFields.serialNumber);
        info.vendorIdentifier = usbIdField(env.jniEnv(), portInfo.get(), s_portInfoFields.vendorId);
        info.hasVendorIdentifier = (info.vendorIdentifier != 0);
        info.productIdentifier = usbIdField(env.jniEnv(), portInfo.get(), s_portInfoFields.productId);
        info.hasProductIdentifier = (info.productIdentifier != 0);

        serialPortInfoList.append(info);
    }

    (void) env.checkAndClearExceptions();

    return serialPortInfoList;
}

QList<QSerialPortInfo> portsAwaitingPermission()
{
    QList<QSerialPortInfo> ports = availablePorts();
    // Java gives a port its stable name once QGC may open its device.
    ports.removeIf([](const QSerialPortInfo& port) { return !port.stableName().isEmpty(); });
    return ports;
}

bool hasPermission(const QString& portName)
{
    return callJavaBool(s_methods.hasPermission, portName);
}

bool requestPermission(const QString& portName)
{
    return callJavaBool(s_methods.requestPermission, portName);
}

bool isPortNameValid(const QString& portName)
{
    return callJavaBool(s_methods.isPortNameValid, portName);
}

bool isPortOpen(const QString& portName)
{
    return getPortHandle(portName) != kNoPort;
}

PortHandle getPortHandle(const QString& portName)
{
    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in getPortHandle";
        return kNoPort;
    }

    QJniEnvironment env;
    return callJava(env, s_methods.getPortHandle, static_cast<jlong>(kNoPort), name.object<jstring>());
}

// ----------------------------------------------------------------------------
// Failure detail
// ----------------------------------------------------------------------------

QString getLastErrorDetail()
{
    // An exception ended the call, so Java has kept no detail of its failure.
    if (!t_exceptionDetail.isEmpty()) {
        return t_exceptionDetail;
    }

    QJniEnvironment env;
    const auto detail = callJavaObject(env, s_methods.getLastErrorDetail);
    return detail.get() ? QJniObject(detail.get()).toString() : QString();
}

// ----------------------------------------------------------------------------
// Open / close
// ----------------------------------------------------------------------------

Result open(const QString& portName, PortHandle handle)
{
    if (handle == kNoPort) {
        qCWarning(AndroidSerialLog) << "open called without a handle from registerPort";
        return Result::Failed;
    }

    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in open";
        return Result::Failed;
    }

    return callJavaCommand(s_methods.open, name.object<jstring>(), static_cast<jlong>(handle));
}

Result close(PortHandle handle)
{
    return callJavaCommand(s_methods.close, static_cast<jlong>(handle));
}

int getFileDescriptor(PortHandle handle)
{
    QJniEnvironment env;
    return callJava(env, s_methods.getFileDescriptor, -1, static_cast<jlong>(handle));
}

// ----------------------------------------------------------------------------
// Reading
// ----------------------------------------------------------------------------

Result startReading(PortHandle handle)
{
    return callJavaCommand(s_methods.startReading, static_cast<jlong>(handle));
}

Result stopReading(PortHandle handle)
{
    return callJavaCommand(s_methods.stopReading, static_cast<jlong>(handle));
}

bool isReading(PortHandle handle)
{
    return callJavaBool(s_methods.isReading, handle);
}

QByteArray read(PortHandle handle, int length, std::chrono::milliseconds timeout)
{
    QByteArray data;

    QJniEnvironment env;
    const auto jarray =
        callJavaObject(env, s_methods.read, static_cast<jlong>(handle), static_cast<jint>(length), toJava(timeout));
    if (!jarray.get()) {
        return data;
    }

    data.resize(env->GetArrayLength(jarray.get()));
    env->GetByteArrayRegion(jarray.get(), 0, static_cast<jsize>(data.size()), reinterpret_cast<jbyte*>(data.data()));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Failed to copy the bytes from read";
        data.clear();
    }

    return data;
}

// ----------------------------------------------------------------------------
// Writing
// ----------------------------------------------------------------------------

/// Calls one of the Java write methods with the data as a Java array. Returns what the method returns, or
/// Result::Failed if the call cannot be made.
template <typename... Args>
static int callJavaWrite(const JavaMethod<jint, jlong, jbyteArray, jint, Args...>& method, PortHandle handle,
                         const char* data, int length, Args... args)
{
    if (!data || length <= 0) {
        qCWarning(AndroidSerialLog) << "Invalid data or length in" << method.name;
        return static_cast<int>(Result::Failed);
    }

    QJniEnvironment env;
    const auto jarray = toJavaBytes(env, data, length, method.name);
    if (!jarray.get()) {
        return static_cast<int>(Result::Failed);
    }

    return callJava(env, method, static_cast<jint>(Result::Failed), static_cast<jlong>(handle), jarray.get(),
                    static_cast<jint>(length), args...);
}

int write(PortHandle handle, const char* data, int length, std::chrono::milliseconds timeout)
{
    const int written = writeWithProgress(handle, data, length, timeout);
    return (written == length || written < 0) ? written : static_cast<int>(Result::Failed);
}

int writeWithProgress(PortHandle handle, const char* data, int length, std::chrono::milliseconds timeout)
{
    return callJavaWrite(s_methods.writeWithProgress, handle, data, length, toJava(timeout));
}

int writeAsync(PortHandle handle, const char* data, int length)
{
    return callJavaWrite(s_methods.writeAsync, handle, data, length);
}

// ----------------------------------------------------------------------------
// Port settings
// ----------------------------------------------------------------------------

// Java takes the constants of usb-serial-for-android's UsbSerialPort, and names a value of one of its enums by
// the code that QGCUsbSerialManager.java gives it. The functions below return nothing for a value that
// QSerialPort's enum does not list.

static std::optional<jint> toJavaDataBits(QSerialPort::DataBits dataBits)
{
    switch (dataBits) {
        case QSerialPort::Data5:
        case QSerialPort::Data6:
        case QSerialPort::Data7:
        case QSerialPort::Data8:
            return static_cast<jint>(dataBits);
    }

    return std::nullopt;
}

static std::optional<jint> toJavaStopBits(QSerialPort::StopBits stopBits)
{
    switch (stopBits) {
        case QSerialPort::OneStop:
            return 1;
        case QSerialPort::OneAndHalfStop:
            return 3;
        case QSerialPort::TwoStop:
            return 2;
    }

    return std::nullopt;
}

static std::optional<jint> toJavaParity(QSerialPort::Parity parity)
{
    switch (parity) {
        case QSerialPort::NoParity:
            return 0;
        case QSerialPort::OddParity:
            return 1;
        case QSerialPort::EvenParity:
            return 2;
        case QSerialPort::MarkParity:
            return 3;
        case QSerialPort::SpaceParity:
            return 4;
    }

    return std::nullopt;
}

static std::optional<FlowControl> toJavaFlowControl(QSerialPort::FlowControl flowControl)
{
    switch (flowControl) {
        case QSerialPort::NoFlowControl:
            return FlowControl::None;
        case QSerialPort::HardwareControl:
            return FlowControl::RtsCts;
        case QSerialPort::SoftwareControl:
            return FlowControl::XonXoff;
    }

    return std::nullopt;
}

static std::optional<FlowControl> fromJavaFlowControl(jint code)
{
    if (code < static_cast<jint>(FlowControl::None) || code > static_cast<jint>(FlowControl::XonXoffInline)) {
        qCWarning(AndroidSerialLog) << "Unknown flow control code:" << code;
        return std::nullopt;
    }

    return static_cast<FlowControl>(code);
}

Result setParameters(PortHandle handle, qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                     QSerialPort::Parity parity)
{
    const std::optional<jint> javaDataBits = toJavaDataBits(dataBits);
    const std::optional<jint> javaStopBits = toJavaStopBits(stopBits);
    const std::optional<jint> javaParity = toJavaParity(parity);
    if (!javaDataBits || !javaStopBits || !javaParity) {
        qCWarning(AndroidSerialLog) << "Invalid serial parameters:" << dataBits << stopBits << parity;
        return Result::Unsupported;
    }

    return callJavaCommand(s_methods.setParameters, static_cast<jlong>(handle), static_cast<jint>(baudRate),
                           *javaDataBits, *javaStopBits, *javaParity);
}

FlowControl getFlowControl(PortHandle handle)
{
    QJniEnvironment env;
    const jint code =
        callJava(env, s_methods.getFlowControl, static_cast<jint>(FlowControl::None), static_cast<jlong>(handle));
    return fromJavaFlowControl(code).value_or(FlowControl::None);
}

Result setFlowControl(PortHandle handle, QSerialPort::FlowControl flowControl)
{
    const std::optional<FlowControl> javaFlowControl = toJavaFlowControl(flowControl);
    if (!javaFlowControl) {
        qCWarning(AndroidSerialLog) << "Invalid flow control:" << flowControl;
        return Result::Unsupported;
    }

    return callJavaCommand(s_methods.setFlowControl, static_cast<jlong>(handle), static_cast<jint>(*javaFlowControl));
}

QList<FlowControl> getSupportedFlowControl(PortHandle handle)
{
    QList<FlowControl> supported;

    const QList<int> codes = callJavaIntArray(s_methods.getSupportedFlowControl, handle);
    for (const int code : codes) {
        if (const std::optional<FlowControl> flowControl = fromJavaFlowControl(code)) {
            supported.append(*flowControl);
        }
    }

    return supported;
}

bool getXon(PortHandle handle)
{
    return callJavaBool(s_methods.getXon, handle);
}

Result setLatencyTimer(PortHandle handle, int milliseconds)
{
    return callJavaCommand(s_methods.setLatencyTimer, static_cast<jlong>(handle), static_cast<jint>(milliseconds));
}

Result setBreak(PortHandle handle, bool set)
{
    return callJavaCommand(s_methods.setBreak, static_cast<jlong>(handle), toJava(set));
}

Result purgeBuffers(PortHandle handle, QSerialPort::Directions directions)
{
    return callJavaCommand(s_methods.purgeBuffers, static_cast<jlong>(handle),
                           toJava(directions.testFlag(QSerialPort::Input)),
                           toJava(directions.testFlag(QSerialPort::Output)));
}

// ----------------------------------------------------------------------------
// Control lines
// ----------------------------------------------------------------------------

Result setDataTerminalReady(PortHandle handle, bool set)
{
    return callJavaCommand(s_methods.setDataTerminalReady, static_cast<jlong>(handle), toJava(set));
}

Result setRequestToSend(PortHandle handle, bool set)
{
    return callJavaCommand(s_methods.setRequestToSend, static_cast<jlong>(handle), toJava(set));
}

static QSerialPort::PinoutSignals callJavaControlLines(const JavaMethod<jintArray, jlong>& method, PortHandle handle)
{
    // By the code that controlLineCode() in QGCUsbSerialManager.java gives each line.
    static constexpr QSerialPort::PinoutSignal kSignals[] = {
        QSerialPort::RequestToSendSignal, QSerialPort::ClearToSendSignal,       QSerialPort::DataTerminalReadySignal,
        QSerialPort::DataSetReadySignal,  QSerialPort::DataCarrierDetectSignal, QSerialPort::RingIndicatorSignal,
    };

    QSerialPort::PinoutSignals data = QSerialPort::PinoutSignals();

    const QList<int> lines = callJavaIntArray(method, handle);
    for (const int line : lines) {
        if (line < 0 || line >= static_cast<int>(std::size(kSignals))) {
            qCWarning(AndroidSerialLog) << "Unknown control line code:" << line;
            continue;
        }
        data |= kSignals[line];
    }

    return data;
}

QSerialPort::PinoutSignals getControlLines(PortHandle handle)
{
    return callJavaControlLines(s_methods.getControlLines, handle);
}

bool isControlLineActive(PortHandle handle, QSerialPort::PinoutSignal line)
{
    return getControlLines(handle).testFlag(line);
}

QSerialPort::PinoutSignals getSupportedControlLines(PortHandle handle)
{
    return callJavaControlLines(s_methods.getSupportedControlLines, handle);
}

bool isControlLineSupported(PortHandle handle, QSerialPort::PinoutSignal line)
{
    return getSupportedControlLines(handle).testFlag(line);
}

// ----------------------------------------------------------------------------
// Self-test
// ----------------------------------------------------------------------------

/// Adds each method or field of the list that JNI did not find in Java to the failures.
template <typename Id>
static void appendUnbound(QStringList& failures, const char* kind, const JavaBindingList<Id>& bindings)
{
    for (const JavaBinding<Id>* binding = bindings.first; binding; binding = binding->next) {
        if (!binding->id) {
            failures.append(QStringLiteral("Java %1 %2 %3 is not bound")
                                .arg(QLatin1StringView(kind), QLatin1StringView(binding->name),
                                     QLatin1StringView(binding->signature)));
        }
    }
}

bool selfTest()
{
    // Notes what Java's callbacks deliver.
    struct Echo final : PortListener
    {
        bool newDataArrived(const char* bytes, int length) override
        {
            heard.append(QStringLiteral("data ") + QString::fromLatin1(QByteArray(bytes, length).toHex()));
            return true;
        }

        void bytesWritten(int count) override { heard.append(QStringLiteral("written %1").arg(count)); }

        void resourceErrorArrived(ResourceErrorReason reason, const QString& detail) override
        {
            heard.append(QStringLiteral("resource error %1 %2").arg(static_cast<int>(reason)).arg(detail));
        }

        QStringList heard;
    } echo;

    // One byte more than Java is told to deliver, bytes that are negative in Java, a count that needs more than
    // 16 bits and a text that is not ASCII: what a wrong signature or conversion would change.
    static constexpr char kData[] = {'\x00', '\x7f', '\x80', '\xff', '\x51'};
    constexpr jint kDataLength = 4;
    constexpr jint kWritten = 70000;
    const QString detail = QStringLiteral("d\u00e9tail");
    const QStringList expected = {
        QStringLiteral("data 007f80ff"),
        QStringLiteral("written 70000"),
        QStringLiteral("resource error %1 %2").arg(static_cast<int>(ResourceErrorReason::WriteFailed)).arg(detail),
        QStringLiteral("devices changed"),
    };

    DeviceWatcher* const watcher = DeviceWatcher::instance();
    // Direct, because Java calls back on this thread before echoToCallbacks returns.
    const QMetaObject::Connection watching = QObject::connect(
        watcher, &DeviceWatcher::devicesChanged, watcher,
        [&echo]() { echo.heard.append(QStringLiteral("devices changed")); }, Qt::DirectConnection);
    const PortHandle handle = registerPort(&echo);

    Result echoed = Result::Failed;
    {
        QJniEnvironment env;
        const auto javaData = toJavaBytes(env, kData, static_cast<int>(sizeof(kData)), "echoToCallbacks");
        const QJniObject javaDetail = QJniObject::fromString(detail);
        if (javaData.get() && javaDetail.isValid()) {
            echoed = callJavaCommand(s_methods.echoToCallbacks, static_cast<jlong>(handle), javaData.get(), kDataLength,
                                     kWritten, static_cast<jint>(ResourceErrorReason::WriteFailed),
                                     javaDetail.object<jstring>());
        }
    }

    unregisterPort(handle);
    (void) QObject::disconnect(watching);

    QStringList failures;
    // A method or field that Java lacks is only logged when the IDs are cached, and QGC runs on without it. The
    // calls here reach three of the methods and none of the fields.
    if (!getSerialManagerClass()) {
        failures.append(
            QStringLiteral("Java class %1 is not bound").arg(QLatin1StringView(kJniUsbSerialManagerClassName)));
    } else {
        appendUnbound(failures, "method", s_javaMethods);
        appendUnbound(failures, "field", s_javaFields);
    }
    if (echoed != Result::Ok) {
        failures.append(QStringLiteral("echoToCallbacks returned %1").arg(static_cast<int>(echoed)));
    }
    if (echo.heard != expected) {
        failures.append(QStringLiteral("Java's callbacks delivered (%1), not (%2)")
                            .arg(echo.heard.join(QStringLiteral(", ")), expected.join(QStringLiteral(", "))));
    }

    // Calls that Java itself answers, with a result that a call which fails on its way cannot return.
    const Result opened = open(QStringLiteral("usb:self-test"), handle);
    if (opened != Result::DeviceNotFound) {
        failures.append(QStringLiteral("Opening a device that does not exist returned %1, not %2")
                            .arg(static_cast<int>(opened))
                            .arg(static_cast<int>(Result::DeviceNotFound)));
    }
    const int written = writeAsync(handle, kData, static_cast<int>(sizeof(kData)));
    if (written != static_cast<int>(Result::NotOpen)) {
        failures.append(QStringLiteral("Writing to a port that is not open returned %1, not %2")
                            .arg(written)
                            .arg(static_cast<int>(Result::NotOpen)));
    }

    for (const QString& failure : std::as_const(failures)) {
        qCCritical(AndroidSerialSelfTestLog).noquote() << failure;
    }
    if (failures.isEmpty()) {
        qCInfo(AndroidSerialSelfTestLog) << "USB serial JNI self-test passed";
    }

    return failures.isEmpty();
}

}  // namespace AndroidSerial
