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

    jlong token = 0;
    constexpr int kMaxTokenRetries = 100;
    for (int i = 0; i < kMaxTokenRetries; ++i) {
        token = static_cast<jlong>(QRandomGenerator::global()->generate64());
        if (token != 0 && !s_tokenToPtr.contains(token)) {
            break;
        }
        token = 0;
    }
    Q_ASSERT_X(token != 0, "registerPointer", "Failed to generate a unique token");

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
        qCDebug(AndroidSerialLog) << "unregisterPointer: pointer was not registered (already removed or never registered)";
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
static bool dispatchToPortObject(QSerialPort* serialPort, Functor&& func, const char* context,
                                 Qt::ConnectionType connType = Qt::QueuedConnection)
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
        // QueuedConnection by default — avoids deadlock when the Qt thread is
        // blocked in waitForReadyRead() (condition-waiting on _readMutex).
        // The lambda re-resolves the token on the target thread, so the pointer
        // only needs to remain valid through dispatch (guaranteed by
        // _stopAsyncRead joining the IO thread before unregisterPointer).
        const bool ok = QMetaObject::invokeMethod(serialPort, std::forward<Functor>(func), connType);
        if (!ok) {
            qCWarning(AndroidSerialLog) << context << ": failed to invoke method on target thread";
        }
        return ok;
    }

    qCWarning(AndroidSerialLog) << context << ": target thread has no event loop, cannot dispatch safely";
    return false;
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

    // Look up the QSerialPort under lock for dispatch. The lambda re-resolves
    // the token on the target thread, so the outer lookup only needs to be valid
    // long enough to determine the target thread for dispatch.
    QSerialPort* serialPort = nullptr;
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
            serialPort,
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

    // Look up the pointer under the read lock, then release the lock before
    // calling newDataArrived(). Holding s_ptrLock across newDataArrived() causes
    // a deadlock: newDataArrived takes _readMutex and queues to the Qt thread,
    // while close() on the Qt thread takes s_ptrLock (write) after _readMutex.
    // The pointer remains valid after releasing s_ptrLock because close() calls
    // _stopAsyncRead() (which joins the Java read thread we are on) before
    // unregisterPointer(), so the pointer cannot be destroyed while we are here.
    QSerialPortPrivate* serialPortPrivate = nullptr;
    {
        QReadLocker locker(&s_ptrLock);
        serialPortPrivate = s_tokenToPtr.value(token, nullptr);
    }
    if (!serialPortPrivate) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData: stale token, object already destroyed";
        return;
    }

    serialPortPrivate->newDataArrived(payload.constData(), payload.size());
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
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "nativeDeviceException: exception converting jstring";
    }

    QSerialPort* serialPort = nullptr;
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
            serialPort,
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
// Device enumeration
// ----------------------------------------------------------------------------

QList<QSerialPortInfo> availableDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    QJniObject objArray = QJniObject::callStaticObjectMethod(
        kJniUsbSerialManagerClassName, "availableDevicesInfo", "()[Ljava/lang/String;");
    if (QJniEnvironment::checkAndClearExceptions() || !objArray.isValid()) {
        if (!objArray.isValid()) {
            qCDebug(AndroidSerialLog) << "availableDevicesInfo returned null";
        } else {
            qCWarning(AndroidSerialLog) << "Exception occurred while calling availableDevicesInfo";
        }
        return serialPortInfoList;
    }

    QJniEnvironment env;
    const jobjectArray rawArray = objArray.object<jobjectArray>();
    const jsize count = env->GetArrayLength(rawArray);
    for (jsize i = 0; i < count; ++i) {
        jobject element = env->GetObjectArrayElement(rawArray, i);
        if (!element) {
            qCWarning(AndroidSerialLog) << "Null string at index" << i;
            continue;
        }

        const QStringList strList = QJniObject(element).toString().split(QLatin1Char('\t'));
        env->DeleteLocalRef(element);

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

    (void)QJniEnvironment::checkAndClearExceptions();

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

    const jint result = QJniObject::callStaticMethod<jint>(
        kJniUsbSerialManagerClassName, "getDeviceId", "(Ljava/lang/String;)I",
        name.object<jstring>());
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in getDeviceId";
        return -1;
    }

    return static_cast<int>(result);
}

int getDeviceHandle(int deviceId)
{
    const jint result = QJniObject::callStaticMethod<jint>(
        kJniUsbSerialManagerClassName, "getDeviceHandle", "(I)I",
        static_cast<jint>(deviceId));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in getDeviceHandle";
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

    const jint deviceId = QJniObject::callStaticMethod<jint>(
        kJniUsbSerialManagerClassName, "open", "(Ljava/lang/String;J)I",
        name.object<jstring>(), token);
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in open";
        return INVALID_DEVICE_ID;
    }

    return static_cast<int>(deviceId);
}

bool close(int deviceId)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "close", "(I)Z",
        static_cast<jint>(deviceId));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in close";
        return false;
    }

    return result;
}

bool isOpen(const QString& portName)
{
    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in isOpen";
        return false;
    }

    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "isDeviceNameOpen", "(Ljava/lang/String;)Z",
        name.object<jstring>());
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in isOpen";
        return false;
    }

    return result;
}

// ----------------------------------------------------------------------------
// Read / write
// ----------------------------------------------------------------------------

int write(int deviceId, const char* data, int length, int timeout, bool async)
{
    if (!data || length <= 0) {
        qCWarning(AndroidSerialLog) << "Invalid data or length in write";
        return -1;
    }

    QJniEnvironment env;
    const jbyteArray jarray = env->NewByteArray(static_cast<jsize>(length));
    if (!jarray) {
        qCWarning(AndroidSerialLog) << "Failed to create jbyteArray in write";
        return -1;
    }

    env->SetByteArrayRegion(jarray, 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(data));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while setting byte array region in write";
        env->DeleteLocalRef(jarray);
        return -1;
    }

    jint result;
    if (async) {
        result = QJniObject::callStaticMethod<jint>(
            kJniUsbSerialManagerClassName, "writeAsync", "(I[BI)I",
            static_cast<jint>(deviceId), jarray, static_cast<jint>(timeout));
    } else {
        result = QJniObject::callStaticMethod<jint>(
            kJniUsbSerialManagerClassName, "write", "(I[BII)I",
            static_cast<jint>(deviceId), jarray, static_cast<jint>(length),
            static_cast<jint>(timeout));
    }

    env->DeleteLocalRef(jarray);

    if (QJniEnvironment::checkAndClearExceptions()) {
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
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "setParameters", "(IIIII)Z",
        static_cast<jint>(deviceId), static_cast<jint>(baudRate), static_cast<jint>(dataBits),
        static_cast<jint>(stopBits), static_cast<jint>(parity));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in setParameters";
        return false;
    }

    return result;
}

// ----------------------------------------------------------------------------
// Control lines
// ----------------------------------------------------------------------------

bool setDataTerminalReady(int deviceId, bool set)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "setDataTerminalReady", "(IZ)Z",
        static_cast<jint>(deviceId), static_cast<jboolean>(set));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in setDataTerminalReady";
        return false;
    }

    return result;
}

bool setRequestToSend(int deviceId, bool set)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "setRequestToSend", "(IZ)Z",
        static_cast<jint>(deviceId), static_cast<jboolean>(set));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in setRequestToSend";
        return false;
    }

    return result;
}

QSerialPort::PinoutSignals getControlLines(int deviceId)
{
    QJniObject jarray = QJniObject::callStaticObjectMethod(
        kJniUsbSerialManagerClassName, "getControlLines", "(I)[I",
        static_cast<jint>(deviceId));

    if (!jarray.isValid()) {
        qCWarning(AndroidSerialLog) << "getControlLines returned null";
        (void)QJniEnvironment::checkAndClearExceptions();
        return QSerialPort::PinoutSignals();
    }

    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getControlLines";
        return QSerialPort::PinoutSignals();
    }

    QJniEnvironment env;
    const jintArray rawArray = jarray.object<jintArray>();
    const jsize len = env->GetArrayLength(rawArray);
    if (len <= 0) {
        return QSerialPort::PinoutSignals();
    }

    QVarLengthArray<jint, 8> ints(len);
    env->GetIntArrayRegion(rawArray, 0, len, ints.data());
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Failed to copy int array in getControlLines";
        return QSerialPort::PinoutSignals();
    }

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

    return data;
}

// ----------------------------------------------------------------------------
// Flow control
// ----------------------------------------------------------------------------

int getFlowControl(int deviceId)
{
    const jint result = QJniObject::callStaticMethod<jint>(
        kJniUsbSerialManagerClassName, "getFlowControl", "(I)I",
        static_cast<jint>(deviceId));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in getFlowControl";
        return QSerialPort::NoFlowControl;
    }

    return static_cast<int>(result);
}

bool setFlowControl(int deviceId, int flowControl)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "setFlowControl", "(II)Z",
        static_cast<jint>(deviceId), static_cast<jint>(flowControl));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in setFlowControl";
        return false;
    }

    return result;
}

// ----------------------------------------------------------------------------
// Buffer / break
// ----------------------------------------------------------------------------

bool purgeBuffers(int deviceId, bool input, bool output)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "purgeBuffers", "(IZZ)Z",
        static_cast<jint>(deviceId), static_cast<jboolean>(input), static_cast<jboolean>(output));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in purgeBuffers";
        return false;
    }

    return result;
}

bool setBreak(int deviceId, bool set)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "setBreak", "(IZ)Z",
        static_cast<jint>(deviceId), static_cast<jboolean>(set));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in setBreak";
        return false;
    }

    return result;
}

// ----------------------------------------------------------------------------
// IO manager (read thread)
// ----------------------------------------------------------------------------

bool startReadThread(int deviceId)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "startIoManager", "(I)Z",
        static_cast<jint>(deviceId));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in startIoManager";
        return false;
    }

    return result;
}

bool stopReadThread(int deviceId)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "stopIoManager", "(I)Z",
        static_cast<jint>(deviceId));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in stopIoManager";
        return false;
    }

    return result;
}

bool readThreadRunning(int deviceId)
{
    const jboolean result = QJniObject::callStaticMethod<jboolean>(
        kJniUsbSerialManagerClassName, "ioManagerRunning", "(I)Z",
        static_cast<jint>(deviceId));
    if (QJniEnvironment::checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception in ioManagerRunning";
        return false;
    }

    return result;
}

} // namespace AndroidSerial
