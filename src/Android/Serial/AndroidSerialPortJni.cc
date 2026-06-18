#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QThread>
#include <cstring>
#include <type_traits>
#include <utility>

#include "AndroidSerialPortPrivate.h"

namespace {

// JNI native preamble: lifetime-fenced registry lookup, stale-token warn; invokes fn(port), value-initialized Ret on any guard rejection.
template <class F, class Ret = std::invoke_result_t<F, AndroidSerialPort*>>
static Ret withPort(jlong token, F&& fn)
{
    if (token != 0) {
        PortRegistry::LookupGuard guard(token);
        if (AndroidSerialPort* const port = guard.port()) {
            return std::forward<F>(fn)(port);
        }
        qCWarning(AndroidSerialPortLog) << "stale token, port already destroyed";
    } else {
        qCWarning(AndroidSerialPortLog) << "called with token=0";
    }
    if constexpr (!std::is_void_v<Ret>) {
        return Ret{};
    }
}

}  // namespace

bool onOwnerThread(const QObject* obj, const char* who)
{
    if (obj->thread() == QThread::currentThread()) {
        return true;
    }
    qCWarning(AndroidSerialPortLog) << who << "called off owner thread";
    return false;
}

int flowControlToWire(QGCFlowControl fc)
{
    switch (fc) {
        case QGCFlowControl::None:
            return AndroidSerialWire::NoFlowControl;
        case QGCFlowControl::HardwareRtsCts:
            return AndroidSerialWire::RtsCtsFlowControl;
        case QGCFlowControl::SoftwareXonXoff:
            return AndroidSerialWire::XonXoffFlowControl;
        case QGCFlowControl::DtrDsr:
            return AndroidSerialWire::DtrDsrFlowControl;
        case QGCFlowControl::XonXoffInline:
            return AndroidSerialWire::XonXoffInlineFlowControl;
    }
    Q_UNREACHABLE_RETURN(AndroidSerialWire::NoFlowControl);
}

QGCSerialPortError exceptionKindToError(AndroidSerialWire::JavaExceptionKind kind)
{
    switch (kind) {
        case AndroidSerialWire::JavaExceptionKind::Resource:
            return QGCSerialPortError::ResourceUnavailable;
        case AndroidSerialWire::JavaExceptionKind::Permission:
            return QGCSerialPortError::PermissionDenied;
        case AndroidSerialWire::JavaExceptionKind::OpenFailed:
            return QGCSerialPortError::OpenFailed;
        case AndroidSerialWire::JavaExceptionKind::Unknown:
            return QGCSerialPortError::Unknown;
    }
    return QGCSerialPortError::Unknown;
}

QtJniTypes::QGCUsbSerialManagerSerialParameters makeSerialParamsJni(qint32 baud, QGCDataBits dataBits,
                                                                    QGCStopBits stopBits, QGCParity parity)
{
    return QJniObject::construct<QtJniTypes::QGCUsbSerialManagerSerialParameters>(
        static_cast<jint>(baud), static_cast<jint>(dataBits), static_cast<jint>(stopBits),
        static_cast<jint>(parity));
}

void AndroidSerialPortPrivate::jniDeviceHasDisconnected(JNIEnv*, jobject, jlong token)
{
    withPort(token, [](AndroidSerialPort* port) { port->d_func()->dispatchCloseFromJavaThread(); });
}

void AndroidSerialPortPrivate::jniDeviceNewData(JNIEnv* env, jobject, jlong token, QtJniTypes::ByteBuffer buffer,
                                                jint length)
{
    constexpr jsize kMaxNativePayloadBytes = static_cast<jsize>(AndroidSerialWire::MAX_CHUNK_BYTES);

    if (!buffer.isValid()) {
        qCWarning(AndroidSerialPortLog) << "called with null buffer";
        return;
    }

    if (length <= 0) {
        qCWarning(AndroidSerialPortLog) << "received empty data (length=" << length << ")";
        return;
    }

    void* const ptr = env->GetDirectBufferAddress(buffer.object());
    if (QJniEnvironment::checkAndClearExceptions(env, QJniEnvironment::OutputMode::Silent) || !ptr) {
        qCWarning(AndroidSerialPortLog) << "GetDirectBufferAddress failed or buffer is not direct";
        return;
    }

    const jlong capacity = env->GetDirectBufferCapacity(buffer.object());
    if (capacity <= 0) {
        qCWarning(AndroidSerialPortLog) << "GetDirectBufferCapacity returned" << capacity;
        return;
    }
    jsize cappedLen = (length > kMaxNativePayloadBytes) ? kMaxNativePayloadBytes : static_cast<jsize>(length);
    cappedLen = qMin<jsize>(cappedLen, static_cast<jsize>(capacity));
    if (cappedLen != static_cast<jsize>(length)) {
        qCWarning(AndroidSerialPortLog) << "payload exceeds limit, truncating from" << length << "to" << cappedLen
                                        << "bytes";
    }

    // Copy required: the direct buffer returns to Java's pool when this JNI call returns.
    const QByteArray payload(reinterpret_cast<const char*>(ptr), static_cast<qsizetype>(cappedLen));
    withPort(token, [&payload](AndroidSerialPort* port) { port->d_func()->enqueueFromJavaThread(payload); });
}

void AndroidSerialPortPrivate::jniDeviceException(JNIEnv* env, jobject, jlong token, jint kind, jstring message)
{
    if (!message) {
        qCWarning(AndroidSerialPortLog) << "called with null message";
        return;
    }

    QJniEnvironment::checkAndClearExceptions(env, QJniEnvironment::OutputMode::Verbose);
    QString exceptionMessage = QJniObject(message).toString();
    if (QJniEnvironment::checkAndClearExceptions(env, QJniEnvironment::OutputMode::Verbose)) {
        qCWarning(AndroidSerialPortLog) << "exception converting jstring";
    }

    const auto kindEnum = (kind >= 0 && kind <= static_cast<int>(AndroidSerialWire::JavaExceptionKind::OpenFailed))
                              ? static_cast<AndroidSerialWire::JavaExceptionKind>(kind)
                              : AndroidSerialWire::JavaExceptionKind::Unknown;

    withPort(token, [&](AndroidSerialPort* port) {
        qCWarning(AndroidSerialPortLog) << "Exception from Java:" << exceptionMessage << "kind=" << kind;
        port->d_func()->dispatchExceptionFromJavaThread(exceptionKindToError(kindEnum), std::move(exceptionMessage));
    });
}

void AndroidSerialPortPrivate::jniDeviceBytesWritten(JNIEnv*, jobject, jlong token, jint n)
{
    if (n <= 0)
        return;
    withPort(token, [n](AndroidSerialPort* port) { port->d_func()->ackBytesWrittenFromJavaThread(n); });
}
