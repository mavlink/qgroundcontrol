#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QJniObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>
#include <QtCore/private/qiodevice_p.h>
#include <QtCore/qjnitypes.h>
#include <atomic>
#include <jni.h>
#include <memory>
#include <utility>

#include "AndroidSerialPort.h"  // Q_DECLARE_PUBLIC requires the full type for its static_cast.
#include "AndroidSerialPortRegistry.h"
#include "AndroidSerialRxQueue.h"
#include "QGCLoggingCategory.h"
#include "QGCSerialPortTypes.h"
#include "SerialWireConstants.h"  // hand-maintained; keep in sync with SerialWireConstants.java
#include "SerialWriteEngine.h"

Q_DECLARE_JNI_CLASS(ByteBuffer, "java/nio/ByteBuffer")
Q_DECLARE_JNI_CLASS(QGCSerialPort, "org/mavlink/qgroundcontrol/serial/QGCSerialPort")
Q_DECLARE_JNI_CLASS(QGCUsbSerialManager, "org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager")
Q_DECLARE_JNI_CLASS(QGCUsbSerialManagerSerialParameters,
                    "org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager$SerialParameters")

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)

int flowControlToWire(QGCFlowControl fc);

// Warns (naming `who`) and returns false when called off obj's owner thread.
bool onOwnerThread(const QObject* obj, const char* who);
QGCSerialPortError exceptionKindToError(AndroidSerialWire::JavaExceptionKind kind);
QtJniTypes::QGCUsbSerialManagerSerialParameters makeSerialParamsJni(qint32 baud, QGCDataBits dataBits,
                                                                    QGCStopBits stopBits, QGCParity parity);

// Private impl for AndroidSerialPort (Qt d-ptr; public class is a thin QIODevice façade).
//
// Cross-language port bridge: the Java PortRegistry owns the UsbSerialPort; the C++
// AndroidSerialPortRegistry owns this QIODevice and hands Java an opaque jlong token at open.
// Java callbacks (data/disconnect/exception/bytesWritten) carry that token; the C++ side looks the
// owner up via PortRegistry::LookupGuard (withPort) so a stale token after destruction is a no-op.
// open() is single-owner: one QIODevice maps to exactly one Java port for its lifetime.
class AndroidSerialPortPrivate : public QIODevicePrivate
{
public:
    Q_DECLARE_PUBLIC(AndroidSerialPort)

    AndroidSerialPortPrivate() = default;

    // Registered against the QGCSerialPort Java class by AndroidSerialPort::initializeNative.
    static void jniDeviceHasDisconnected(JNIEnv* env, jobject obj, jlong token);
    static void jniDeviceNewData(JNIEnv* env, jobject obj, jlong token, QtJniTypes::ByteBuffer buffer, jint length);
    static void jniDeviceException(JNIEnv* env, jobject obj, jlong token, jint kind, jstring message);
    static void jniDeviceBytesWritten(JNIEnv* env, jobject obj, jlong token, jint n);

    // Registered against the QGCUsbSerialManager Java class; no token (manager-wide attach/permission event).
    static void jniUsbDevicesChanged(JNIEnv* env, jobject obj);

    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceHasDisconnected, nativeDeviceHasDisconnected)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceNewData, nativeDeviceNewData)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceException, nativeDeviceException)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniDeviceBytesWritten, nativeDeviceBytesWritten)
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jniUsbDevicesChanged, nativeUsbDevicesChanged)

    bool javaPortAlive() const noexcept { return javaAlive.load(std::memory_order_acquire); }

    // Marshal `fn(AndroidSerialPort*)` onto the owner thread via a queued call; no-op if the owner is gone
    // by the time it runs (QPointer-fenced). The single home for the JNI-thread -> owner-thread hop that
    // postBytesWritten / dispatchClose / dispatchException / appendToBuffer all need.
    template <class F>
    void postToOwner(F&& fn)
    {
        Q_Q(AndroidSerialPort);
        QPointer<AndroidSerialPort> self = q;
        QMetaObject::invokeMethod(
            q,
            [self, fn = std::forward<F>(fn)]() mutable {
                if (self) {
                    fn(self.data());
                }
            },
            Qt::QueuedConnection);
    }

    void setError(QGCSerialPortError code, const QString& message = {});

    // Java-writer-thread ack: forward to the write engine, then post bytesWritten(n) to the owner thread.
    void ackBytesWrittenFromJavaThread(qint64 n);
    void postBytesWritten(qint64 n);

    bool openJavaPort(qint64 handle, QIODeviceBase::OpenMode mode);
    void teardownJavaPort();  // Safe after partial open(); single-shot via javaAlive exchange.

    // JNI-thread entry; copies payload and posts to owner thread.
    void enqueueFromJavaThread(QByteArrayView bytes);
    // Owner-thread continuation of enqueueFromJavaThread; drops payloads stamped with a stale RX generation.
    void appendToBuffer(QByteArray payload, quint32 generation);
    void dispatchCloseFromJavaThread();
    void dispatchExceptionFromJavaThread(QGCSerialPortError errorCode, QString message);

    // JNI port object and associated lifecycle state.
    QJniObject javaPort;
    jlong handle = 0;
    std::atomic<bool> javaAlive{false};  // owner exchange(false) in close(), JNI store(false) on unplug.

    // Write-side accounting + JNI submission. Holds references to javaAlive/javaPort above, so it must be
    // declared after them.
    SerialWriteEngine writeEngine{javaAlive};

    QString systemLocation;
    // Single source of truth for wire params; stashed while closed, re-synced to Java on every (re)open.
    SerialPortConfig pending;

    QGCSerialPortError error = QGCSerialPortError::NoError;

    // Re-entrancy guards (mirrors qserialport_unix.cpp) — block readyRead↔bytesWritten recursion.
    bool emittedReadyRead = false;
    bool emittedBytesWritten = false;

    // RX flow-control accounting (backlog reservation, generation epoch, drop-warn latches). Pure logic,
    // host-unit-tested in AndroidSerialRxQueueTest; the byte storage stays in QIODevicePrivate::buffer.
    AndroidSerialRxQueue rxQueue;

    // Suppresses the racing Java IOException as ResourceUnavailable during a forced close.
    bool expectClosure = false;
};
