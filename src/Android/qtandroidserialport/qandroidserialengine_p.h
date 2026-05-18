#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSpan>
#include <QtCore/QtClassHelperMacros>
#include <QtCore/QtTypes>
#include <QtCore/QWaitCondition>
#include <QtCore/qjnitypes.h>

#include <atomic>
#include <jni.h>
#include <utility>

#include "AndroidSerial.h"
#include "iqserialportengine_p.h"

QT_BEGIN_NAMESPACE
class QObject;
class QString;
QT_END_NAMESPACE

Q_DECLARE_JNI_CLASS(ByteBuffer, "java/nio/ByteBuffer")

class QAndroidSerialEngineReceiver;

// RAII per-port handle to the Android JNI USB serial bridge.
//
// Owns the staging buffer (_pendingData) and read-side thread marshaling. Bytes arrive on
// the Java IO thread, are appended to _pendingData under _readMutex, and are delivered to
// the receiver on the owner thread via QAndroidSerialEngineReceiver::dataReady.
class QAndroidSerialEngine : public IQSerialPortEngine
{
public:
    using State = IQSerialPortEngine::State;

    // owner: QObject whose thread should receive dataReady/closeNotification/
    // exceptionNotification (typically the QSerialPort).
    QAndroidSerialEngine(QAndroidSerialEngineReceiver* sink, QObject* owner);
    ~QAndroidSerialEngine() override;

    Q_DISABLE_COPY_MOVE(QAndroidSerialEngine)

    // Atomic open: applies parameters + flow control + DTR in a single JNI roundtrip.
    // On failure the Java side has already cleaned up — no separate close() is needed.
    [[nodiscard]] bool open(const QString& portName, const AndroidSerial::SerialParameters& params,
                            int flowControl, bool assertDtr) override;
    void close() override;
    bool isOpen() const override { return _state == State::Open; }
    State state() const override { return _state; }

    int deviceHandle() const override;

    [[nodiscard]] bool setParameters(const AndroidSerial::SerialParameters& p) override;
    [[nodiscard]] bool setDataTerminalReady(bool set) override;
    [[nodiscard]] bool setRequestToSend(bool set) override;
    [[nodiscard]] bool setFlowControl(int flowControl) override;
    int  flowControl() const override;
    [[nodiscard]] bool setBreak(bool set) override;
    [[nodiscard]] bool purgeBuffers(bool input, bool output) override;

    quint32 controlLinesMask() const override;

    int writeSync(QSpan<const char> data, int timeout) override;
    int writeAsync(QSpan<const char> data, int timeout) override;

    [[nodiscard]] bool startReadThread() override;
    [[nodiscard]] bool stopReadThread() override;
    bool readThreadRunning() const override;

    // -- Read-side machinery (was on QSerialPortPrivate) ----------------------

    // Mirror of QSerialPort::readBufferSize policy. 0 = unlimited.
    void setReadBufferMaxSize(qint64 bytes) override { _readBufferMaxSize = bytes; }

    // Drops any staged bytes that haven't been delivered yet. Called from QSerialPort::clear.
    void clearReadStaging() override;

    // Owner-thread blocking wait. Returns the staged bytes or empty on timeout.
    // ownerBufferSize is the current QIODevice buffer size, used to enforce the read cap.
    QByteArray waitForReadyRead(int msecs, qint64 ownerBufferSize) override;

    static void initialize();

private:
    // -- JNI native callbacks (registered on QGCUsbSerialManager) ---------------
    // Static so JNI can register them; class-scoped so they can't collide with
    // free-function natives in other TUs. Resolve token→engine under s_ptrLock(read).
    static void jniDeviceHasDisconnected(JNIEnv* env, jobject obj, jlong token);
    static void jniDeviceNewData(JNIEnv* env, jobject obj, jlong token,
                                 QtJniTypes::ByteBuffer buffer, jint length);
    static void jniDeviceException(JNIEnv* env, jobject obj, jlong token, jint kind, jstring message);

    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceHasDisconnected, nativeDeviceHasDisconnected)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceNewData,         nativeDeviceNewData)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceException,       nativeDeviceException)

    // -- JNI-thread dispatch entry points (called only by jni* statics above) ---
    // Marshal to the owner thread before invoking the receiver.
    void enqueueFromJavaThread(QByteArrayView bytes);
    void dispatchCloseFromJavaThread();
    void dispatchExceptionFromJavaThread(AndroidSerial::JavaExceptionKind kind, const QString& message);

    QAndroidSerialEngineReceiver* _sink;
    QPointer<QObject> _owner;
    int _deviceId = AndroidSerial::INVALID_DEVICE_ID;
    jlong _token = 0;
    // Atomic so cross-thread readers (notably waitForReadyRead) observe close()
    // transitions without a data race.
    std::atomic<State> _state{State::Closed};

    // Read staging — Java IO thread appends, owner thread drains.
    qint64 _readBufferMaxSize = 0;
    std::atomic<bool> _readyReadPending{false};
    QMutex _readMutex;
    QWaitCondition _readWaitCondition;
    QByteArray _pendingData;

    // Sentinel for queued drain events. Declared LAST so it is destroyed FIRST: that removes
    // any pending _drainOnOwnerThread invocations from the owner's event queue before
    // _readMutex/_readWaitCondition go away. Without this, a late-delivered drain would
    // wakeAll() on a destroyed mutex (FORTIFY pthread abort).
    QObject _drainSentinel;

    void _registerToken();
    void _unregisterToken();
    int  _write(const char* javaMethod, QSpan<const char> data, int timeout);
    void _scheduleDrainOnOwnerThread();
    void _drainOnOwnerThread();

    // Posts `f(sink)` to the owner thread via QueuedConnection. No-op if owner or sink is null.
    template <typename F>
    void _postToOwner(F&& f)
    {
        QPointer<QObject> owner = _owner;
        if (!owner || !_sink) return;
        QAndroidSerialEngineReceiver* sink = _sink;
        QMetaObject::invokeMethod(owner.data(), [owner, sink, f = std::forward<F>(f)]() {
            if (!owner) return;
            f(sink);
        }, Qt::QueuedConnection);
    }
};
