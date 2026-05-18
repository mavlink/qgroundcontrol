#include "qandroidserialengine_p.h"
#include "QGCAndroidSerialPort.h"

#include <QtCore/QHash>
#include <QtCore/QJniArray>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/qjnitypes.h>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QRandomGenerator>
#include <QtCore/QReadWriteLock>
#include <QtCore/QScopeGuard>

#include "qandroidserialenginereceiver_p.h"
#include "QGCLoggingCategory.h"

#include <jni.h>

Q_DECLARE_JNI_CLASS(QGCUsbSerialManager, "org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager")
Q_DECLARE_JNI_CLASS(QGCUsbSerialManagerSerialParameters,
                    "org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager$SerialParameters")

QGC_LOGGING_CATEGORY(AndroidSerialEngineLog, "Android.Serial.Engine");
QGC_LOGGING_CATEGORY(AndroidSerialEngineVerboseLog, "Android.Serial.Engine:verbose");

namespace {
QtJniTypes::QGCUsbSerialManagerSerialParameters makeSerialParamsJni(const AndroidSerial::SerialParameters& p)
{
    return QJniObject::construct<QtJniTypes::QGCUsbSerialManagerSerialParameters>(
        static_cast<jint>(p.baudRate), static_cast<jint>(p.dataBits),
        static_cast<jint>(p.stopBits), static_cast<jint>(p.parity));
}
}  // namespace

// ----------------------------------------------------------------------------
// JNI call helpers — collapse the per-method state-check / call / exception-check
// boilerplate that otherwise repeats verbatim across ~15 methods.
// ----------------------------------------------------------------------------

namespace {

// Calls QGCUsbSerialManager.<method>(deviceId, args...) and returns the result, or `onError`
// if the JNI call threw. Centralizes the exception-check + warning log path.
//
// Hot-path note: uses the static QJniEnvironment::getJniEnv()+checkAndClearExceptions(JNIEnv*) pair
// rather than constructing a QJniEnvironment{}. On an already-attached thread that's a single
// JavaVM::GetEnv() TLS read instead of a QJniEnvironment object construction; matters because
// this helper runs on every writeSync/writeAsync per MAVLink packet.
template <typename Ret, typename... Args>
Ret callJniDevice(const char* method, int deviceId, Ret onError, Args&&... args)
{
    const Ret r = QJniObject::callStaticMethod<QtJniTypes::QGCUsbSerialManager, Ret>(
        method, static_cast<jint>(deviceId), std::forward<Args>(args)...);
    if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(),
                                                  QJniEnvironment::OutputMode::Silent)) {
        qCWarning(AndroidSerialEngineLog) << "JNI exception in" << method << "for device ID" << deviceId;
        return onError;
    }
    return r;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// Token registry (file-scope statics)
//
// Maps the opaque jlong token Java holds → the live QAndroidSerialEngine that owns the
// JNI bridge. Java never sees a raw C++ pointer; on stale tokens (engine destroyed
// between the Java IO thread fetching the token and calling back) the lookup returns
// null and the callback is a no-op. Mirrors the Qt Bluetooth LowEnergyNotificationHub
// pattern for the same reason: usb-serial-for-android's IO thread join is best-effort
// (2 s polling timeout), so the callback path must tolerate races.
// ----------------------------------------------------------------------------

static QReadWriteLock s_ptrLock;
static QHash<jlong, QAndroidSerialEngine*> s_tokenToEngine;
static QHash<QAndroidSerialEngine*, jlong> s_engineToToken;

// ----------------------------------------------------------------------------
// JNI callbacks (called from Java IO threads — class statics, registered below)
// ----------------------------------------------------------------------------

void QAndroidSerialEngine::jniDeviceHasDisconnected(JNIEnv*, jobject, jlong token)
{
    if (token == 0) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceHasDisconnected called with token=0";
        return;
    }

    // Engine internally marshals to its owner thread; we only need to keep the engine alive
    // across the synchronous dispatch call.
    QReadLocker locker(&s_ptrLock);
    QAndroidSerialEngine* const engine = s_tokenToEngine.value(token, nullptr);
    if (!engine) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceHasDisconnected: stale token, engine already destroyed";
        return;
    }
    engine->dispatchCloseFromJavaThread();
}

void QAndroidSerialEngine::jniDeviceNewData(JNIEnv* env, jobject, jlong token,
                                            QtJniTypes::ByteBuffer buffer, jint length)
{
    // Java passes a direct ByteBuffer; GetDirectBufferAddress returns a raw pointer
    // to it with no extra copy at this boundary. The Java-side memcpy (upstream
    // byte[] → direct buffer) is unavoidable without patching usb-serial-for-android.

    constexpr jsize kMaxNativePayloadBytes = static_cast<jsize>(AndroidSerial::MAX_READ_SIZE);

    if (token == 0) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceNewData called with token=0";
        return;
    }

    if (!buffer.isValid()) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceNewData called with null buffer";
        return;
    }

    if (length <= 0) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceNewData received empty data (length=" << length << ")";
        return;
    }

    void* const ptr = env->GetDirectBufferAddress(buffer.object());
    if (!ptr) {
        qCWarning(AndroidSerialEngineLog)
            << "nativeDeviceNewData: GetDirectBufferAddress returned null — buffer is not direct";
        return;
    }

    // Use the caller-supplied length, not GetDirectBufferCapacity: pooled buffers
    // are allocated at MAX_NATIVE_CALLBACK_DATA_BYTES capacity but may carry fewer
    // valid bytes than their capacity when the payload is smaller.
    const jsize cappedLen = (length > kMaxNativePayloadBytes) ? kMaxNativePayloadBytes : static_cast<jsize>(length);
    if (cappedLen != static_cast<jsize>(length)) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceNewData payload exceeds limit, truncating from"
                                          << length << "to" << cappedLen << "bytes";
    }

    // QByteArray(ptr, n) copies the bytes into Qt-managed heap. This copy is required
    // because the direct buffer is returned to Java's pool immediately after this JNI
    // call returns — we cannot use QByteArray::fromRawData without blocking Java until
    // the Qt dispatch fully consumes the data.
    const QByteArray payload(reinterpret_cast<const char*>(ptr), static_cast<qsizetype>(cappedLen));

    // Hold s_ptrLock(read) across enqueueFromJavaThread to block _unregisterToken (write lock)
    // until this callback returns. Lock order: s_ptrLock(read) → engine._readMutex. The
    // engine's owner-thread close() does not take _readMutex before _unregisterToken, so no
    // inversion.
    QReadLocker locker(&s_ptrLock);
    QAndroidSerialEngine* const engine = s_tokenToEngine.value(token, nullptr);
    if (!engine) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceNewData: stale token, engine already destroyed";
        return;
    }

    if (AndroidSerialEngineVerboseLog().isDebugEnabled()) {
        const auto firstByte = static_cast<unsigned>(static_cast<uint8_t>(payload.at(0)));
        qCDebug(AndroidSerialEngineVerboseLog).nospace().noquote()
            << "nativeDeviceNewData token=" << token
            << " n=" << payload.size()
            << " first=0x" << QString::asprintf("%02x", firstByte);
    }

    engine->enqueueFromJavaThread(payload);
}

void QAndroidSerialEngine::jniDeviceException(JNIEnv* env, jobject, jlong token, jint kind, jstring message)
{
    if (token == 0) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceException called with token=0";
        return;
    }

    if (!message) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceException called with null message";
        return;
    }

    const QString exceptionMessage = QJniObject(message).toString();
    if (QJniEnvironment::checkAndClearExceptions(env, QJniEnvironment::OutputMode::Silent)) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceException: exception converting jstring";
    }

    // Out-of-range kind values fall back to Unknown rather than UB on the cast.
    const auto kindEnum = (kind >= 0 && kind <= static_cast<int>(AndroidSerial::JavaExceptionKind::OpenFailed))
                          ? static_cast<AndroidSerial::JavaExceptionKind>(kind)
                          : AndroidSerial::JavaExceptionKind::Unknown;

    QReadLocker locker(&s_ptrLock);
    QAndroidSerialEngine* const engine = s_tokenToEngine.value(token, nullptr);
    if (!engine) {
        qCWarning(AndroidSerialEngineLog) << "nativeDeviceException: stale token, engine already destroyed";
        return;
    }

    qCWarning(AndroidSerialEngineLog) << "Exception from Java:" << exceptionMessage << "kind=" << kind;
    engine->dispatchExceptionFromJavaThread(kindEnum, exceptionMessage);
}

// ----------------------------------------------------------------------------
// QAndroidSerialEngine implementation
// ----------------------------------------------------------------------------

QAndroidSerialEngine::QAndroidSerialEngine(QAndroidSerialEngineReceiver* sink, QObject* owner)
    : _sink(sink), _owner(owner)
{
}

QAndroidSerialEngine::~QAndroidSerialEngine()
{
    if (_state != State::Closed) {
        close();
    }
    _unregisterToken();
}

void QAndroidSerialEngine::_registerToken()
{
    QWriteLocker locker(&s_ptrLock);

    const auto existingIt = s_engineToToken.constFind(this);
    if (existingIt != s_engineToToken.cend()) {
        s_tokenToEngine.remove(*existingIt);
        s_engineToToken.erase(existingIt);
    }

    // Random non-zero token, retry on collision (mirrors Qt BLE LowEnergyNotificationHub).
    // Opacity over a monotonic counter: a misbehaving .apk can't infer engine lifetime/order
    // from sequential IDs. token=0 stays reserved as the JNI "no token" sentinel.
    jlong token = 0;
    while (token == 0 || s_tokenToEngine.contains(token)) {
        token = static_cast<jlong>(QRandomGenerator::global()->generate64());
    }

    s_tokenToEngine.insert(token, this);
    s_engineToToken.insert(this, token);
    _token = token;
}

void QAndroidSerialEngine::_unregisterToken()
{
    if (_token == 0) {
        return;
    }

    QWriteLocker locker(&s_ptrLock);
    s_engineToToken.remove(this);
    s_tokenToEngine.remove(_token);
    _token = 0;
}

bool QAndroidSerialEngine::open(const QString& portName, const AndroidSerial::SerialParameters& params,
                              int flowControl, bool assertDtr)
{
    if (_state != State::Closed) {
        qCWarning(AndroidSerialEngineLog) << "open called on non-closed port, device ID" << _deviceId;
        return false;
    }

    _state = State::Opening;

    _registerToken();
    if (_token == 0) {
        qCWarning(AndroidSerialEngineLog) << "open: failed to register token for" << portName;
        _state = State::Closed;
        return false;
    }

    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialEngineLog) << "Invalid QJniObject for portName in open";
        _unregisterToken();
        _state = State::Closed;
        return false;
    }

    const auto paramsObj = makeSerialParamsJni(params);
    if (!paramsObj.isValid()) {
        qCWarning(AndroidSerialEngineLog) << "Failed to construct SerialParameters JNI object";
        _unregisterToken();
        _state = State::Closed;
        return false;
    }

    const jint deviceId = QJniObject::callStaticMethod<QtJniTypes::QGCUsbSerialManager, jint>(
        "openWithConfig", name.object<jstring>(), paramsObj,
        static_cast<jint>(flowControl), static_cast<jboolean>(assertDtr), _token);
    if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(),
                                                  QJniEnvironment::OutputMode::Silent)) {
        qCWarning(AndroidSerialEngineLog) << "Exception in openWithConfig for" << portName;
        _unregisterToken();
        _state = State::Closed;
        return false;
    }

    if (static_cast<int>(deviceId) == AndroidSerial::INVALID_DEVICE_ID) {
        qCWarning(AndroidSerialEngineLog) << "openWithConfig returned INVALID_DEVICE_ID for" << portName;
        _unregisterToken();
        _state = State::Closed;
        return false;
    }

    _deviceId = static_cast<int>(deviceId);
    _state = State::Open;
    return true;
}

void QAndroidSerialEngine::close()
{
    if (_state == State::Closed) {
        return;
    }

    // Mute the IO thread before invalidating the token. Java's close also stops the IO
    // manager, but ordering it here means a Java callback racing the JNI close still
    // resolves the token while the engine is alive (s_ptrLock guards the dispatch).
    if (_deviceId != AndroidSerial::INVALID_DEVICE_ID && _state == State::Open) {
        callJniDevice<jboolean>("stopIoManager", _deviceId, JNI_FALSE);
    }

    _state = State::Closing;

    if (_deviceId != AndroidSerial::INVALID_DEVICE_ID) {
        if (!callJniDevice<jboolean>("close", _deviceId, JNI_FALSE)) {
            qCWarning(AndroidSerialEngineLog) << "close returned false for device ID" << _deviceId;
        }
        _deviceId = AndroidSerial::INVALID_DEVICE_ID;
    }

    _unregisterToken();
    _state = State::Closed;

    // Wake any thread blocked in waitForReadyRead so it observes the closed state
    // and exits the wait loop instead of timing out (or blocking forever on -1).
    {
        QMutexLocker locker(&_readMutex);
        _readWaitCondition.wakeAll();
    }
}

int QAndroidSerialEngine::deviceHandle() const
{
    if (_state != State::Open) return -1;
    return callJniDevice<jint>("getDeviceHandle", _deviceId, -1);
}

bool QAndroidSerialEngine::setParameters(const AndroidSerial::SerialParameters& p)
{
    if (_state != State::Open) return false;

    const auto paramsObj = makeSerialParamsJni(p);
    if (!paramsObj.isValid()) {
        qCWarning(AndroidSerialEngineLog) << "Failed to construct SerialParameters JNI object";
        return false;
    }

    return callJniDevice<jboolean>("setSerialParameters", _deviceId, JNI_FALSE, paramsObj);
}

bool QAndroidSerialEngine::setDataTerminalReady(bool set)
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("setDataTerminalReady", _deviceId, JNI_FALSE, static_cast<jboolean>(set));
}

bool QAndroidSerialEngine::setRequestToSend(bool set)
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("setRequestToSend", _deviceId, JNI_FALSE, static_cast<jboolean>(set));
}

quint32 QAndroidSerialEngine::controlLinesMask() const
{
    if (_state != State::Open) return 0u;

    const auto ints = callJniDevice<QJniArray<jint>>("getControlLines", _deviceId, QJniArray<jint>());
    if (!ints.isValid()) return 0u;

    quint32 data = 0;
    for (const jint code : ints) {
        switch (code) {
            case AndroidSerial::RtsControlLine: data |= QGCPinoutSignals::RTS; break;
            case AndroidSerial::CtsControlLine: data |= QGCPinoutSignals::CTS; break;
            case AndroidSerial::DtrControlLine: data |= QGCPinoutSignals::DTR; break;
            case AndroidSerial::DsrControlLine: data |= QGCPinoutSignals::DSR; break;
            case AndroidSerial::CdControlLine:  data |= QGCPinoutSignals::DCD; break;
            case AndroidSerial::RiControlLine:  data |= QGCPinoutSignals::RNG; break;
            default:
                qCWarning(AndroidSerialEngineLog) << "Unknown ControlLine value:" << code;
                break;
        }
    }

    return data;
}

int QAndroidSerialEngine::flowControl() const
{
    if (_state != State::Open) return AndroidSerial::NoFlowControl;
    return callJniDevice<jint>("getFlowControl", _deviceId, AndroidSerial::NoFlowControl);
}

bool QAndroidSerialEngine::setFlowControl(int fc)
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("setFlowControl", _deviceId, JNI_FALSE, static_cast<jint>(fc));
}

bool QAndroidSerialEngine::purgeBuffers(bool input, bool output)
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("purgeBuffers", _deviceId, JNI_FALSE,
                                   static_cast<jboolean>(input), static_cast<jboolean>(output));
}

bool QAndroidSerialEngine::setBreak(bool set)
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("setBreak", _deviceId, JNI_FALSE, static_cast<jboolean>(set));
}

int QAndroidSerialEngine::_write(const char* javaMethod, QSpan<const char> data, int timeout)
{
    if (_state != State::Open) return -1;
    if (data.empty()) {
        qCWarning(AndroidSerialEngineLog) << "Invalid data or length in" << javaMethod << "for device ID" << _deviceId;
        return -1;
    }

    // NewDirectByteBuffer wraps the C++-owned buffer without copying. Java still copies once
    // (direct ByteBuffer → heap byte[]) because usb-serial-for-android's UsbSerialPort.write()
    // takes byte[]. The eliminated copy is the old NewByteArray + SetByteArrayRegion path.
    JNIEnv* const env = QJniEnvironment::getJniEnv();
    const jobject jBufferRaw = env->NewDirectByteBuffer(const_cast<char*>(data.data()),
                                                        static_cast<jlong>(data.size()));
    if (!jBufferRaw) {
        qCWarning(AndroidSerialEngineLog) << "Failed to create DirectByteBuffer in" << javaMethod
                                          << "for device ID" << _deviceId;
        return -1;
    }
    const auto bufferGuard = qScopeGuard([&]{ env->DeleteLocalRef(jBufferRaw); });
    const QtJniTypes::ByteBuffer jBuffer{jBufferRaw};

    const jint rc = callJniDevice<jint>(javaMethod, _deviceId, -1, jBuffer,
                                        static_cast<jint>(data.size()), static_cast<jint>(timeout));
    if (AndroidSerialEngineVerboseLog().isDebugEnabled()) {
        const auto firstByte = data.empty() ? 0u : static_cast<unsigned>(static_cast<uint8_t>(data[0]));
        qCDebug(AndroidSerialEngineVerboseLog).nospace().noquote()
            << javaMethod << " deviceId=" << _deviceId
            << " n=" << data.size() << " rc=" << rc
            << " first=0x" << QString::asprintf("%02x", firstByte);
    }
    return rc;
}

int QAndroidSerialEngine::writeAsync(QSpan<const char> data, int timeout)
{
    return _write("writeAsyncDirect", data, timeout);
}

int QAndroidSerialEngine::writeSync(QSpan<const char> data, int timeout)
{
    return _write("writeDirect", data, timeout);
}

bool QAndroidSerialEngine::startReadThread()
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("startIoManager", _deviceId, JNI_FALSE);
}

bool QAndroidSerialEngine::stopReadThread()
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("stopIoManager", _deviceId, JNI_FALSE);
}

bool QAndroidSerialEngine::readThreadRunning() const
{
    if (_state != State::Open) return false;
    return callJniDevice<jboolean>("ioManagerRunning", _deviceId, JNI_FALSE);
}

// ----------------------------------------------------------------------------
// Read-side staging (engine-internal thread marshaling)
// ----------------------------------------------------------------------------

void QAndroidSerialEngine::enqueueFromJavaThread(QByteArrayView bytes)
{
    {
        QMutexLocker locker(&_readMutex);

        qsizetype toAccept = bytes.size();
        if (_readBufferMaxSize > 0) {
            const qint64 headroom = _readBufferMaxSize - _pendingData.size();
            toAccept = qMin<qsizetype>(toAccept, qMax<qint64>(0, headroom));
        }

        if (toAccept > 0) {
            _pendingData.append(bytes.first(toAccept));
            _readWaitCondition.wakeAll();
        }

        if (toAccept < bytes.size()) {
            qCWarning(AndroidSerialEngineLog) << "Read buffer full, dropped" << (bytes.size() - toAccept) << "bytes";
        }

        if (toAccept <= 0) {
            return;
        }
    }

    _scheduleDrainOnOwnerThread();
}

void QAndroidSerialEngine::_scheduleDrainOnOwnerThread()
{
    if (_readyReadPending.exchange(true)) {
        return;  // Drain already scheduled.
    }

    QPointer<QObject> owner = _owner;
    if (!owner) {
        // Owner gone — nothing to deliver to.
        _readyReadPending.store(false);
        return;
    }

    // Receiver = _drainSentinel (engine member). When the engine is destroyed, the sentinel's
    // destruction cancels any not-yet-delivered queued events, so we never call
    // _drainOnOwnerThread() on a dead `this`.
    QMetaObject::invokeMethod(&_drainSentinel, [this]() {
        _drainOnOwnerThread();
    }, Qt::QueuedConnection);
}

void QAndroidSerialEngine::_drainOnOwnerThread()
{
    QByteArray pending;
    {
        QMutexLocker locker(&_readMutex);
        if (_pendingData.isEmpty()) {
            _readyReadPending.store(false);
            return;
        }

        pending.swap(_pendingData);
        // Reset the pending flag under the lock so a concurrent enqueue either sees
        // the flag false (and posts a fresh drain) or true (and trusts that this drain
        // will see its data). Without this, lost-wakeup is possible.
        _readyReadPending.store(false);
        _readWaitCondition.wakeAll();
    }

    if (_sink && !pending.isEmpty()) {
        _sink->dataReady(std::move(pending));
    }
}

void QAndroidSerialEngine::clearReadStaging()
{
    QMutexLocker locker(&_readMutex);
    _pendingData.clear();
}

QByteArray QAndroidSerialEngine::waitForReadyRead(int msecs, qint64 ownerBufferSize)
{
    QMutexLocker locker(&_readMutex);
    if (_pendingData.isEmpty() && _state == State::Open) {
        QDeadlineTimer deadline(msecs);
        while (_pendingData.isEmpty() && _state == State::Open) {
            if (!_readWaitCondition.wait(&_readMutex, deadline)) {
                break;
            }
        }
    }

    if (_pendingData.isEmpty()) {
        return QByteArray();
    }

    QByteArray data;
    if (_readBufferMaxSize <= 0) {
        data.swap(_pendingData);
    } else {
        const qint64 canAccept = qMax<qint64>(0, _readBufferMaxSize - ownerBufferSize);
        if (canAccept <= 0) {
            // Caller's buffer at capacity. Wake other waiters and let caller retry.
            _readWaitCondition.wakeAll();
            return QByteArray();
        }
        const qsizetype n = qMin<qsizetype>(static_cast<qsizetype>(canAccept), _pendingData.size());
        if (n >= _pendingData.size()) {
            data.swap(_pendingData);
        } else {
            data = _pendingData.first(n);
            _pendingData.remove(0, n);
        }
    }
    return data;
}

void QAndroidSerialEngine::dispatchCloseFromJavaThread()
{
    _postToOwner([](QAndroidSerialEngineReceiver* sink) { sink->closeNotification(); });
}

void QAndroidSerialEngine::dispatchExceptionFromJavaThread(AndroidSerial::JavaExceptionKind kind,
                                                           const QString& message)
{
    _postToOwner([kind, message](QAndroidSerialEngineReceiver* sink) {
        sink->exceptionNotification(kind, message);
    });
}

// ----------------------------------------------------------------------------
// Static initialization
// ----------------------------------------------------------------------------

void QAndroidSerialEngine::initialize()
{
    // Function-local static + lambda: idempotent under repeat calls (mirrors Qt's
    // androidconnectivitymanager.cpp registerNatives pattern). Safe even if invoked
    // outside JNI_OnLoad (e.g. JNI unload+reload).
    static const bool registered = []() {
        QJniEnvironment env;
        const bool ok = env.registerNativeMethods<QtJniTypes::QGCUsbSerialManager>({
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceHasDisconnected, QAndroidSerialEngine),
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceNewData,         QAndroidSerialEngine),
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceException,       QAndroidSerialEngine),
        });
        if (!ok) {
            qCWarning(AndroidSerialEngineLog) << "Failed to register native methods for QGCUsbSerialManager";
        } else {
            qCDebug(AndroidSerialEngineLog) << "Native Functions Registered Successfully";
        }
        return ok;
    }();
    Q_UNUSED(registered)
}
