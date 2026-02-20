#include "AndroidSerial.h"

#include <QtCore/QHash>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QReadWriteLock>
#include <QtCore/QThread>
#include <qserialport_p.h>
#include <qserialportinfo_p.h>

#include <utility>

#include "AndroidInterface.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidSerialLog, "Android.AndroidSerial");

namespace AndroidSerial {

// ----------------------------------------------------------------------------
// Token-based pointer tracking (UAF protection)
//
// Java receives an opaque random jlong token instead of a raw C++ pointer.
// A bidirectional hash map under QReadWriteLock maps tokens ↔ pointers.
// JNI callbacks (readers) take a shared read lock; register/unregister
// (writers) take an exclusive write lock.  Pattern follows Qt Bluetooth's
// LowEnergyNotificationHub.
// ----------------------------------------------------------------------------

static QReadWriteLock s_ptrLock;
static QHash<jlong, QSerialPortPrivate*> s_tokenToPtr;
static QHash<QSerialPortPrivate*, jlong> s_ptrToToken;

void registerPointer(QSerialPortPrivate* ptr)
{
    if (!ptr) {
        qCWarning(AndroidSerialLog) << "registerPointer called with null pointer";
        return;
    }

    QWriteLocker locker(&s_ptrLock);

    const auto existingIt = s_ptrToToken.constFind(ptr);
    if (existingIt != s_ptrToToken.cend()) {
        s_tokenToPtr.remove(*existingIt);
        s_ptrToToken.erase(existingIt);
    }

    jlong token;
    do {
        token = static_cast<jlong>(QRandomGenerator::global()->generate64());
    } while (token == 0 || s_tokenToPtr.contains(token));

    s_tokenToPtr.insert(token, ptr);
    s_ptrToToken.insert(ptr, token);
}

void unregisterPointer(QSerialPortPrivate* ptr)
{
    if (!ptr) {
        return;
    }

    QWriteLocker locker(&s_ptrLock);
    const jlong token = s_ptrToToken.take(ptr);
    if (token == 0) {
        return;
    }
    s_tokenToPtr.remove(token);
}

static QSerialPortPrivate* lookupByToken(jlong token)
{
    QReadLocker locker(&s_ptrLock);
    return s_tokenToPtr.value(token, nullptr);
}

static jlong lookupToken(QSerialPortPrivate* ptr)
{
    QReadLocker locker(&s_ptrLock);
    return s_ptrToToken.value(ptr, 0);
}

static QSerialPort* lookupPortByTokenLocked(jlong token)
{
    QSerialPortPrivate* const serialPortPrivate = s_tokenToPtr.value(token, nullptr);
    if (!serialPortPrivate) {
        return nullptr;
    }

    return qobject_cast<QSerialPort*>(serialPortPrivate->q_ptr);
}

template <typename Functor>
static bool dispatchToPortObject(QSerialPort* serialPort, Functor&& func, const char* context)
{
    if (!serialPort) {
        qCWarning(AndroidSerialLog) << context << ": null serial port";
        return false;
    }

    QThread* const targetThread = serialPort->thread();
    const bool sameThread = (targetThread == QThread::currentThread());
    const bool hasEventLoop = targetThread && targetThread->eventDispatcher();

    if (sameThread) {
        std::forward<Functor>(func)();
        return true;
    }

    if (hasEventLoop) {
        const bool queued = QMetaObject::invokeMethod(serialPort, std::forward<Functor>(func), Qt::QueuedConnection);
        if (!queued) {
            qCWarning(AndroidSerialLog) << context << ": failed to queue method on target thread";
        }
        return queued;
    }

    qCWarning(AndroidSerialLog) << context << ": target thread has no event loop, running inline fallback";
    std::forward<Functor>(func)();
    return true;
}

// ----------------------------------------------------------------------------
// JNI method ID cache
// ----------------------------------------------------------------------------

struct JniMethodCache
{
    jmethodID availableDevicesInfo = nullptr;
    jmethodID getDeviceId = nullptr;
    jmethodID getDeviceHandle = nullptr;
    jmethodID open = nullptr;
    jmethodID close = nullptr;
    jmethodID isDeviceNameOpen = nullptr;
    jmethodID read = nullptr;
    jmethodID write = nullptr;
    jmethodID writeAsync = nullptr;
    jmethodID setParameters = nullptr;
    jmethodID getCarrierDetect = nullptr;
    jmethodID getClearToSend = nullptr;
    jmethodID getDataSetReady = nullptr;
    jmethodID getDataTerminalReady = nullptr;
    jmethodID setDataTerminalReady = nullptr;
    jmethodID getRingIndicator = nullptr;
    jmethodID getRequestToSend = nullptr;
    jmethodID setRequestToSend = nullptr;
    jmethodID getControlLines = nullptr;
    jmethodID getFlowControl = nullptr;
    jmethodID setFlowControl = nullptr;
    jmethodID purgeBuffers = nullptr;
    jmethodID setBreak = nullptr;
    jmethodID startIoManager = nullptr;
    jmethodID stopIoManager = nullptr;
    jmethodID ioManagerRunning = nullptr;
};

static JniMethodCache s_methods;
static bool s_methodsCached = false;
static QMutex s_cacheLock;
static jclass s_serialManagerClass = nullptr;

static bool cacheMethodIds(JNIEnv* env, jclass javaClass)
{
    struct MethodDef
    {
        jmethodID* target;
        const char* name;
        const char* sig;
    };

    const MethodDef defs[] = {
        {&s_methods.availableDevicesInfo, "availableDevicesInfo", "()[Ljava/lang/String;"},
        {&s_methods.getDeviceId, "getDeviceId", "(Ljava/lang/String;)I"},
        {&s_methods.getDeviceHandle, "getDeviceHandle", "(I)I"},
        {&s_methods.open, "open", "(Ljava/lang/String;J)I"},
        {&s_methods.close, "close", "(I)Z"},
        {&s_methods.isDeviceNameOpen, "isDeviceNameOpen", "(Ljava/lang/String;)Z"},
        {&s_methods.read, "read", "(III)[B"},
        {&s_methods.write, "write", "(I[BII)I"},
        {&s_methods.writeAsync, "writeAsync", "(I[BI)I"},
        {&s_methods.setParameters, "setParameters", "(IIIII)Z"},
        {&s_methods.getCarrierDetect, "getCarrierDetect", "(I)Z"},
        {&s_methods.getClearToSend, "getClearToSend", "(I)Z"},
        {&s_methods.getDataSetReady, "getDataSetReady", "(I)Z"},
        {&s_methods.getDataTerminalReady, "getDataTerminalReady", "(I)Z"},
        {&s_methods.setDataTerminalReady, "setDataTerminalReady", "(IZ)Z"},
        {&s_methods.getRingIndicator, "getRingIndicator", "(I)Z"},
        {&s_methods.getRequestToSend, "getRequestToSend", "(I)Z"},
        {&s_methods.setRequestToSend, "setRequestToSend", "(IZ)Z"},
        {&s_methods.getControlLines, "getControlLines", "(I)[I"},
        {&s_methods.getFlowControl, "getFlowControl", "(I)I"},
        {&s_methods.setFlowControl, "setFlowControl", "(II)Z"},
        {&s_methods.purgeBuffers, "purgeBuffers", "(IZZ)Z"},
        {&s_methods.setBreak, "setBreak", "(IZ)Z"},
        {&s_methods.startIoManager, "startIoManager", "(I)Z"},
        {&s_methods.stopIoManager, "stopIoManager", "(I)Z"},
        {&s_methods.ioManagerRunning, "ioManagerRunning", "(I)Z"},
    };

    for (const auto& def : defs) {
        *def.target = env->GetStaticMethodID(javaClass, def.name, def.sig);
        if (!*def.target) {
            qCWarning(AndroidSerialLog) << "Failed to cache method:" << def.name << def.sig;
            (void)QJniEnvironment::checkAndClearExceptions(env);
            return false;
        }
    }

    s_methodsCached = true;
    qCDebug(AndroidSerialLog) << "All JNI method IDs cached successfully";
    return true;
}

// ----------------------------------------------------------------------------
// Class resolution
// ----------------------------------------------------------------------------

static jclass getSerialManagerClass()
{
    QMutexLocker locker(&s_cacheLock);

    if (s_serialManagerClass && s_methodsCached) {
        return s_serialManagerClass;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment";
        return nullptr;
    }

    if (!s_serialManagerClass) {
        const jclass resolvedClass = env.findClass(kJniUsbSerialManagerClassName);
        if (!resolvedClass) {
            qCWarning(AndroidSerialLog) << "Class Not Found:" << kJniUsbSerialManagerClassName;
            return nullptr;
        }

        s_serialManagerClass = static_cast<jclass>(env->NewGlobalRef(resolvedClass));
        if (env->GetObjectRefType(resolvedClass) == JNILocalRefType) {
            env->DeleteLocalRef(resolvedClass);
        }

        if (!s_serialManagerClass) {
            qCWarning(AndroidSerialLog) << "Failed to create global ref for class:" << kJniUsbSerialManagerClassName;
            (void)env.checkAndClearExceptions();
            return nullptr;
        }
    }

    if (!s_methodsCached && !cacheMethodIds(env.jniEnv(), s_serialManagerClass)) {
        qCWarning(AndroidSerialLog) << "Failed to cache JNI method IDs";
        env->DeleteGlobalRef(s_serialManagerClass);
        s_serialManagerClass = nullptr;
        s_methods = {};
        (void)env.checkAndClearExceptions();
        return nullptr;
    }

    s_methodsCached = true;
    (void)env.checkAndClearExceptions();
    return s_serialManagerClass;
}

void cleanupJniCache()
{
    QMutexLocker locker(&s_cacheLock);
    QJniEnvironment env;
    if (s_serialManagerClass && env.isValid()) {
        env->DeleteGlobalRef(s_serialManagerClass);
    }
    s_serialManagerClass = nullptr;
    s_methods = {};
    s_methodsCached = false;
}

// ----------------------------------------------------------------------------
// Native method registration
// ----------------------------------------------------------------------------

// Forward declarations for JNI callbacks (defined below)
static void jniDeviceHasDisconnected(JNIEnv* env, jobject obj, jlong token);
static void jniDeviceNewData(JNIEnv* env, jobject obj, jlong token, jbyteArray data);
static void jniDeviceException(JNIEnv* env, jobject obj, jlong token, jstring message);

void setNativeMethods()
{
    qCDebug(AndroidSerialLog) << "Registering Native Functions";

    const JNINativeMethod javaMethods[]{
        {"nativeDeviceHasDisconnected", "(J)V", reinterpret_cast<void*>(jniDeviceHasDisconnected)},
        {"nativeDeviceNewData", "(J[B)V", reinterpret_cast<void*>(jniDeviceNewData)},
        {"nativeDeviceException", "(JLjava/lang/String;)V", reinterpret_cast<void*>(jniDeviceException)},
    };

    QJniEnvironment env;
    if (!env.registerNativeMethods(kJniUsbSerialManagerClassName, javaMethods, std::size(javaMethods))) {
        qCWarning(AndroidSerialLog) << "Failed to register native methods for" << kJniUsbSerialManagerClassName;
        return;
    }

    if (!getSerialManagerClass()) {
        qCWarning(AndroidSerialLog) << "Failed to cache JNI method IDs";
        return;
    }

    qCDebug(AndroidSerialLog) << "Native Functions Registered Successfully";
}

// ----------------------------------------------------------------------------
// JNI callbacks (called from Java threads)
// ----------------------------------------------------------------------------

static void jniDeviceHasDisconnected(JNIEnv*, jobject, jlong token)
{
    if (token == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceHasDisconnected called with token=0";
        return;
    }

    QPointer<QSerialPort> serialPort;
    {
        QReadLocker locker(&s_ptrLock);
        serialPort = lookupPortByTokenLocked(token);
        if (!serialPort) {
            qCWarning(AndroidSerialLog) << "nativeDeviceHasDisconnected: stale token, object already destroyed";
            return;
        }
        qCDebug(AndroidSerialLog) << "Device disconnected:" << serialPort->portName();
    }

    if (!dispatchToPortObject(
            serialPort.data(),
            [token]() {
                QSerialPortPrivate* const p = lookupByToken(token);
                if (!p) {
                    qCDebug(AndroidSerialLog) << "Token already invalidated in nativeDeviceHasDisconnected";
                    return;
                }

                QSerialPort* const port = qobject_cast<QSerialPort*>(p->q_ptr);
                if (port && port->isOpen()) {
                    port->close();
                    qCDebug(AndroidSerialLog) << "Serial port closed in nativeDeviceHasDisconnected";
                } else {
                    qCDebug(AndroidSerialLog) << "Serial port was already closed in nativeDeviceHasDisconnected";
                }
            },
            "nativeDeviceHasDisconnected")) {
        qCWarning(AndroidSerialLog) << "nativeDeviceHasDisconnected: failed to dispatch cleanup";
    }
}

static void jniDeviceNewData(JNIEnv* env, jobject, jlong token, jbyteArray data)
{
    constexpr jsize kMaxNativePayloadBytes = static_cast<jsize>(MAX_READ_SIZE);

    if (token == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with token=0";
        return;
    }

    if (!data) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with null data";
        return;
    }

    const jsize len = env->GetArrayLength(data);
    if (len <= 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData received empty data array";
        return;
    }

    const jsize cappedLen = (len > kMaxNativePayloadBytes) ? kMaxNativePayloadBytes : len;
    if (cappedLen != len) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData payload exceeds limit, truncating from" << len << "to"
                                    << cappedLen << "bytes";
    }

    QByteArray payload(cappedLen, Qt::Uninitialized);
    env->GetByteArrayRegion(data, 0, cappedLen, reinterpret_cast<jbyte*>(payload.data()));
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData failed to copy JNI byte array";
        return;
    }

    {
        // Deliver inline while holding read lock so unregister/destroy cannot
        // invalidate the pointer until this handoff is complete.
        QReadLocker locker(&s_ptrLock);
        QSerialPortPrivate* const serialPortPrivate = s_tokenToPtr.value(token, nullptr);
        if (!serialPortPrivate) {
            qCWarning(AndroidSerialLog) << "nativeDeviceNewData: stale token, object already destroyed";
            return;
        }

        serialPortPrivate->newDataArrived(payload.constData(), payload.size());
    }
}

static void jniDeviceException(JNIEnv*, jobject, jlong token, jstring message)
{
    if (token == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceException called with token=0";
        return;
    }

    if (!message) {
        qCWarning(AndroidSerialLog) << "nativeDeviceException called with null message";
        return;
    }

    const QString exceptionMessage = QJniObject(message).toString();

    QPointer<QSerialPort> serialPort;
    {
        QReadLocker locker(&s_ptrLock);
        serialPort = lookupPortByTokenLocked(token);
        if (!serialPort) {
            qCWarning(AndroidSerialLog) << "nativeDeviceException: stale token, object already destroyed";
            return;
        }
    }

    qCWarning(AndroidSerialLog) << "Exception from Java:" << exceptionMessage;

    if (!dispatchToPortObject(
            serialPort.data(),
            [token, exceptionMessage]() {
                QSerialPortPrivate* const p = lookupByToken(token);
                if (!p) {
                    qCDebug(AndroidSerialLog) << "Token already invalidated in nativeDeviceException";
                    return;
                }

                p->exceptionArrived(exceptionMessage);
            },
            "nativeDeviceException")) {
        qCWarning(AndroidSerialLog) << "nativeDeviceException: failed to dispatch exception callback";
    }
}

// ----------------------------------------------------------------------------
// Helper: get env + class + check cached method in one shot
// ----------------------------------------------------------------------------

struct JniContext
{
    QJniEnvironment env;
    jclass cls = nullptr;
    bool valid = false;
};

static bool getContext(JniContext& ctx, const char* caller)
{
    if (!ctx.env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in" << caller;
        return false;
    }

    ctx.cls = getSerialManagerClass();
    if (!ctx.cls) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in" << caller;
        return false;
    }

    ctx.valid = true;
    return true;
}

// ----------------------------------------------------------------------------
// Device enumeration
// ----------------------------------------------------------------------------

QList<QSerialPortInfo> availableDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    JniContext ctx;
    if (!getContext(ctx, "availableDevices"))
        return serialPortInfoList;

    AndroidInterface::JniLocalRef<jobjectArray> objArray(
        ctx.env.jniEnv(),
        static_cast<jobjectArray>(ctx.env->CallStaticObjectMethod(ctx.cls, s_methods.availableDevicesInfo)));
    if (!objArray.get()) {
        qCDebug(AndroidSerialLog) << "availableDevicesInfo returned null";
        (void)ctx.env.checkAndClearExceptions();
        return serialPortInfoList;
    }

    if (ctx.env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling availableDevicesInfo";
        return serialPortInfoList;
    }

    const jsize count = ctx.env->GetArrayLength(objArray.get());
    for (jsize i = 0; i < count; ++i) {
        AndroidInterface::JniLocalRef<jstring> jstr(
            ctx.env.jniEnv(), static_cast<jstring>(ctx.env->GetObjectArrayElement(objArray.get(), i)));
        if (!jstr.get()) {
            qCWarning(AndroidSerialLog) << "Null string at index" << i;
            continue;
        }

        const QStringList strList = QJniObject(jstr.get()).toString().split(QLatin1Char('\t'));

        if (strList.size() < 6) {
            qCWarning(AndroidSerialLog) << "Invalid device info at index" << i << ":" << strList;
            continue;
        }

        bool pidOK, vidOK;
        QSerialPortInfoPrivate info;
        info.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(strList[0]);
        info.device = strList[0];
        info.description = strList[1];
        info.manufacturer = strList[2];
        info.serialNumber = strList[3];
        info.productIdentifier = strList[4].toInt(&pidOK);
        info.hasProductIdentifier = (pidOK && (info.productIdentifier != INVALID_DEVICE_ID));
        info.vendorIdentifier = strList[5].toInt(&vidOK);
        info.hasVendorIdentifier = (vidOK && (info.vendorIdentifier != INVALID_DEVICE_ID));

        serialPortInfoList.append(info);
    }

    (void)ctx.env.checkAndClearExceptions();

    return serialPortInfoList;
}

// ----------------------------------------------------------------------------
// Device ID / handle lookup
// ----------------------------------------------------------------------------

int getDeviceId(const QString& portName)
{
    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in getDeviceId";
        return -1;
    }

    JniContext ctx;
    if (!getContext(ctx, "getDeviceId"))
        return -1;

    jint result = -1;
    if (!AndroidInterface::callStaticIntMethod(ctx.env, ctx.cls, s_methods.getDeviceId, "getDeviceId",
                                               AndroidSerialLog(), result, name.object<jstring>())) {
        return -1;
    }

    return static_cast<int>(result);
}

int getDeviceHandle(int deviceId)
{
    JniContext ctx;
    if (!getContext(ctx, "getDeviceHandle"))
        return -1;

    jint result = -1;
    if (!AndroidInterface::callStaticIntMethod(ctx.env, ctx.cls, s_methods.getDeviceHandle, "getDeviceHandle",
                                               AndroidSerialLog(), result, static_cast<jint>(deviceId))) {
        return -1;
    }

    return static_cast<int>(result);
}

// ----------------------------------------------------------------------------
// Open / close / isOpen
// ----------------------------------------------------------------------------

int open(const QString& portName, QSerialPortPrivate* classPtr)
{
    if (!classPtr) {
        qCWarning(AndroidSerialLog) << "open called with null serialPort";
        return INVALID_DEVICE_ID;
    }

    const jlong token = lookupToken(classPtr);
    if (token == 0) {
        qCWarning(AndroidSerialLog) << "open called with unregistered pointer — call registerPointer first";
        return INVALID_DEVICE_ID;
    }

    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in open";
        return INVALID_DEVICE_ID;
    }

    JniContext ctx;
    if (!getContext(ctx, "open"))
        return INVALID_DEVICE_ID;

    jint deviceId = INVALID_DEVICE_ID;
    if (!AndroidInterface::callStaticIntMethod(ctx.env, ctx.cls, s_methods.open, "open", AndroidSerialLog(), deviceId,
                                               name.object<jstring>(), token)) {
        return INVALID_DEVICE_ID;
    }

    return static_cast<int>(deviceId);
}

bool close(int deviceId)
{
    JniContext ctx;
    if (!getContext(ctx, "close"))
        return false;

    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, s_methods.close, "close", AndroidSerialLog(),
                                                   result, static_cast<jint>(deviceId))) {
        return false;
    }

    return (result == JNI_TRUE);
}

bool isOpen(const QString& portName)
{
    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in isOpen";
        return false;
    }

    JniContext ctx;
    if (!getContext(ctx, "isOpen"))
        return false;

    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, s_methods.isDeviceNameOpen, "isDeviceNameOpen",
                                                   AndroidSerialLog(), result, name.object<jstring>())) {
        return false;
    }

    return (result == JNI_TRUE);
}

// ----------------------------------------------------------------------------
// Read / write
// ----------------------------------------------------------------------------

QByteArray read(int deviceId, int length, int timeout)
{
    JniContext ctx;
    if (!getContext(ctx, "read"))
        return QByteArray();

    AndroidInterface::JniLocalRef<jbyteArray> jarray(
        ctx.env.jniEnv(), static_cast<jbyteArray>(
                              ctx.env->CallStaticObjectMethod(ctx.cls, s_methods.read, static_cast<jint>(deviceId),
                                                              static_cast<jint>(length), static_cast<jint>(timeout))));

    if (!jarray.get()) {
        qCWarning(AndroidSerialLog) << "read method returned null";
        (void)ctx.env.checkAndClearExceptions();
        return QByteArray();
    }

    if (ctx.env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling read";
        return QByteArray();
    }

    const jsize len = ctx.env->GetArrayLength(jarray.get());
    jbyte* const bytes = ctx.env->GetByteArrayElements(jarray.get(), nullptr);
    if (!bytes) {
        qCWarning(AndroidSerialLog) << "Failed to get byte array elements in read";
        return QByteArray();
    }

    const QByteArray data(reinterpret_cast<char*>(bytes), len);
    ctx.env->ReleaseByteArrayElements(jarray.get(), bytes, JNI_ABORT);

    return data;
}

int write(int deviceId, const char* data, int length, int timeout, bool async)
{
    if (!data || length <= 0) {
        qCWarning(AndroidSerialLog) << "Invalid data or length in write";
        return -1;
    }

    JniContext ctx;
    if (!getContext(ctx, "write"))
        return -1;

    AndroidInterface::JniLocalRef<jbyteArray> jarray(ctx.env.jniEnv(),
                                                     ctx.env->NewByteArray(static_cast<jsize>(length)));
    if (!jarray.get()) {
        qCWarning(AndroidSerialLog) << "Failed to create jbyteArray in write";
        return -1;
    }

    ctx.env->SetByteArrayRegion(jarray.get(), 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(data));
    if (ctx.env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while setting byte array region in write";
        return -1;
    }

    jint result;
    if (async) {
        result = ctx.env->CallStaticIntMethod(ctx.cls, s_methods.writeAsync, static_cast<jint>(deviceId), jarray.get(),
                                              static_cast<jint>(timeout));
    } else {
        result = ctx.env->CallStaticIntMethod(ctx.cls, s_methods.write, static_cast<jint>(deviceId), jarray.get(),
                                              static_cast<jint>(length), static_cast<jint>(timeout));
    }

    if (ctx.env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling write/writeAsync";
        return -1;
    }

    return static_cast<int>(result);
}

// ----------------------------------------------------------------------------
// Port configuration
// ----------------------------------------------------------------------------

bool setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
{
    JniContext ctx;
    if (!getContext(ctx, "setParameters"))
        return false;

    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, s_methods.setParameters, "setParameters",
                                                   AndroidSerialLog(), result, static_cast<jint>(deviceId),
                                                   static_cast<jint>(baudRate), static_cast<jint>(dataBits),
                                                   static_cast<jint>(stopBits), static_cast<jint>(parity))) {
        return false;
    }

    return (result == JNI_TRUE);
}

// ----------------------------------------------------------------------------
// Control line helpers (DRY macro for bool getters)
// ----------------------------------------------------------------------------

static bool callBoolMethod(jmethodID method, int deviceId, const char* name)
{
    JniContext ctx;
    if (!getContext(ctx, name))
        return false;

    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, method, name, AndroidSerialLog(), result,
                                                   static_cast<jint>(deviceId))) {
        return false;
    }

    return (result == JNI_TRUE);
}

static bool callBoolSetMethod(jmethodID method, int deviceId, bool set, const char* name)
{
    JniContext ctx;
    if (!getContext(ctx, name))
        return false;

    const jboolean jSet = set ? JNI_TRUE : JNI_FALSE;
    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, method, name, AndroidSerialLog(), result,
                                                   static_cast<jint>(deviceId), jSet)) {
        return false;
    }

    return (result == JNI_TRUE);
}

// ----------------------------------------------------------------------------
// Control lines
// ----------------------------------------------------------------------------

bool getCarrierDetect(int deviceId)
{
    return callBoolMethod(s_methods.getCarrierDetect, deviceId, "getCarrierDetect");
}

bool getClearToSend(int deviceId)
{
    return callBoolMethod(s_methods.getClearToSend, deviceId, "getClearToSend");
}

bool getDataSetReady(int deviceId)
{
    return callBoolMethod(s_methods.getDataSetReady, deviceId, "getDataSetReady");
}

bool getDataTerminalReady(int deviceId)
{
    return callBoolMethod(s_methods.getDataTerminalReady, deviceId, "getDataTerminalReady");
}

bool getRingIndicator(int deviceId)
{
    return callBoolMethod(s_methods.getRingIndicator, deviceId, "getRingIndicator");
}

bool getRequestToSend(int deviceId)
{
    return callBoolMethod(s_methods.getRequestToSend, deviceId, "getRequestToSend");
}

bool setDataTerminalReady(int deviceId, bool set)
{
    return callBoolSetMethod(s_methods.setDataTerminalReady, deviceId, set, "setDataTerminalReady");
}

bool setRequestToSend(int deviceId, bool set)
{
    return callBoolSetMethod(s_methods.setRequestToSend, deviceId, set, "setRequestToSend");
}

QSerialPort::PinoutSignals getControlLines(int deviceId)
{
    JniContext ctx;
    if (!getContext(ctx, "getControlLines"))
        return QSerialPort::PinoutSignals();

    AndroidInterface::JniLocalRef<jintArray> jarray(
        ctx.env.jniEnv(), static_cast<jintArray>(ctx.env->CallStaticObjectMethod(ctx.cls, s_methods.getControlLines,
                                                                                 static_cast<jint>(deviceId))));
    if (!jarray.get()) {
        qCWarning(AndroidSerialLog) << "getControlLines returned null";
        (void)ctx.env.checkAndClearExceptions();
        return QSerialPort::PinoutSignals();
    }

    if (ctx.env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getControlLines";
        return QSerialPort::PinoutSignals();
    }

    jint* const ints = ctx.env->GetIntArrayElements(jarray.get(), nullptr);
    if (!ints) {
        qCWarning(AndroidSerialLog) << "Failed to get int array elements in getControlLines";
        return QSerialPort::PinoutSignals();
    }

    const jsize len = ctx.env->GetArrayLength(jarray.get());
    QSerialPort::PinoutSignals data = QSerialPort::PinoutSignals();

    for (jsize i = 0; i < len; ++i) {
        switch (ints[i]) {
            case RtsControlLine:
                data |= QSerialPort::RequestToSendSignal;
                break;
            case CtsControlLine:
                data |= QSerialPort::ClearToSendSignal;
                break;
            case DtrControlLine:
                data |= QSerialPort::DataTerminalReadySignal;
                break;
            case DsrControlLine:
                data |= QSerialPort::DataSetReadySignal;
                break;
            case CdControlLine:
                data |= QSerialPort::DataCarrierDetectSignal;
                break;
            case RiControlLine:
                data |= QSerialPort::RingIndicatorSignal;
                break;
            default:
                qCWarning(AndroidSerialLog) << "Unknown ControlLine value:" << ints[i];
                break;
        }
    }

    ctx.env->ReleaseIntArrayElements(jarray.get(), ints, JNI_ABORT);
    (void)ctx.env.checkAndClearExceptions();

    return data;
}

// ----------------------------------------------------------------------------
// Flow control
// ----------------------------------------------------------------------------

int getFlowControl(int deviceId)
{
    JniContext ctx;
    if (!getContext(ctx, "getFlowControl"))
        return QSerialPort::NoFlowControl;

    jint flowControl = QSerialPort::NoFlowControl;
    if (!AndroidInterface::callStaticIntMethod(ctx.env, ctx.cls, s_methods.getFlowControl, "getFlowControl",
                                               AndroidSerialLog(), flowControl, static_cast<jint>(deviceId))) {
        return QSerialPort::NoFlowControl;
    }

    return static_cast<int>(flowControl);
}

bool setFlowControl(int deviceId, int flowControl)
{
    JniContext ctx;
    if (!getContext(ctx, "setFlowControl"))
        return false;

    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, s_methods.setFlowControl, "setFlowControl",
                                                   AndroidSerialLog(), result, static_cast<jint>(deviceId),
                                                   static_cast<jint>(flowControl))) {
        return false;
    }

    return result == JNI_TRUE;
}

// ----------------------------------------------------------------------------
// Buffer / break
// ----------------------------------------------------------------------------

bool purgeBuffers(int deviceId, bool input, bool output)
{
    JniContext ctx;
    if (!getContext(ctx, "purgeBuffers"))
        return false;

    const jboolean jInput = input ? JNI_TRUE : JNI_FALSE;
    const jboolean jOutput = output ? JNI_TRUE : JNI_FALSE;

    jboolean result = JNI_FALSE;
    if (!AndroidInterface::callStaticBooleanMethod(ctx.env, ctx.cls, s_methods.purgeBuffers, "purgeBuffers",
                                                   AndroidSerialLog(), result, static_cast<jint>(deviceId), jInput,
                                                   jOutput)) {
        return false;
    }

    return (result == JNI_TRUE);
}

bool setBreak(int deviceId, bool set)
{
    return callBoolSetMethod(s_methods.setBreak, deviceId, set, "setBreak");
}

// ----------------------------------------------------------------------------
// IO manager (read thread)
// ----------------------------------------------------------------------------

bool startReadThread(int deviceId)
{
    return callBoolMethod(s_methods.startIoManager, deviceId, "startIoManager");
}

bool stopReadThread(int deviceId)
{
    return callBoolMethod(s_methods.stopIoManager, deviceId, "stopIoManager");
}

bool readThreadRunning(int deviceId)
{
    return callBoolMethod(s_methods.ioManagerRunning, deviceId, "ioManagerRunning");
}

}  // namespace AndroidSerial
