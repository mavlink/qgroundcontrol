#include "SerialWriteEngine.h"

#include <QtCore/QDeadlineTimer>
#include <QtCore/QJniEnvironment>
#include <QtCore/qjnitypes.h>

#include <cstring>

#include "QGCLoggingCategory.h"
#include "SerialWireConstants.h"

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialPortLog)
Q_DECLARE_JNI_CLASS(ByteBuffer, "java/nio/ByteBuffer")

namespace {
constexpr int kDefaultWriteTimeoutMs = 5000;
}

bool SerialWriteEngine::allocateScratch(const QString& location)
{
    // Release any prior buffer first: the global ref wraps the old backing alloc, so resetting the unique_ptr
    // before clearing the ref would leave a DirectByteBuffer over freed memory (UAF).
    _scratchBuffer = QJniObject();
    _scratch.reset();
    _scratch = std::make_unique<char[]>(AndroidSerialWire::MAX_CHUNK_BYTES);

    JNIEnv* const env = QJniEnvironment::getJniEnv();
    if (!env) {
        qCWarning(AndroidSerialPortLog) << "No JNI env while allocating write scratch for" << location;
        return false;
    }
    const jobject raw =
        env->NewDirectByteBuffer(_scratch.get(), static_cast<jlong>(AndroidSerialWire::MAX_CHUNK_BYTES));
    if (QJniEnvironment::checkAndClearExceptions(env, QJniEnvironment::OutputMode::Silent) || !raw) {
        qCWarning(AndroidSerialPortLog) << "Failed to allocate write scratch DirectByteBuffer for" << location;
        return false;
    }
    _scratchBuffer = QJniObject::fromLocalRef(raw);
    return true;
}

void SerialWriteEngine::releaseScratch()
{
    // Release the global ref before the heap alloc — DirectByteBuffer wraps the raw pointer.
    _scratchBuffer = QJniObject();
    _scratch.reset();
}

void SerialWriteEngine::reset()
{
    QMutexLocker lk(&_mutex);
    _error.store(false, std::memory_order_relaxed);
    _inFlight.store(0, std::memory_order_relaxed);
}

void SerialWriteEngine::clearInFlight()
{
    QMutexLocker lk(&_mutex);
    _inFlight.store(0, std::memory_order_relaxed);
}

qint64 SerialWriteEngine::enqueueToJava(QJniObject& javaPort, QByteArrayView bytes, const QString& location)
{
    if (!_scratch || !_scratchBuffer.isValid()) {
        qCWarning(AndroidSerialPortLog) << "Write scratch buffer missing on" << location;
        return -1;
    }
    JNIEnv* const env = QJniEnvironment::getJniEnv();
    if (!env) {
        qCWarning(AndroidSerialPortLog) << "No JNI env while enqueuing write on" << location;
        return -1;
    }
    const QtJniTypes::ByteBuffer jBuffer{_scratchBuffer.object()};
    qint64 offset = 0;
    const qint64 total = bytes.size();
    while (offset < total) {
        if (!_javaAlive.load(std::memory_order_acquire)) {
            return offset > 0 ? offset : -1;
        }
        const qint64 remaining = total - offset;
        const int capped = static_cast<int>(qMin<qint64>(remaining, AndroidSerialWire::MAX_CHUNK_BYTES));
        std::memcpy(_scratch.get(), bytes.data() + offset, static_cast<size_t>(capped));

        const jint accepted = javaPort.callMethod<jint>("enqueueWrite", jBuffer, static_cast<jint>(capped));
        if (QJniEnvironment::checkAndClearExceptions(env, QJniEnvironment::OutputMode::Verbose)) {
            qCWarning(AndroidSerialPortLog) << "enqueueWrite threw on" << location;
            return offset > 0 ? offset : -1;
        }
        if (accepted <= 0) {
            qCWarning(AndroidSerialPortLog) << "enqueueWrite rejected on" << location;
            return offset > 0 ? offset : -1;
        }
        offset += accepted;
    }
    return offset;
}

qint64 SerialWriteEngine::submit(QJniObject& javaPort, QByteArrayView bytes, const QString& location)
{
    if (_error.load(std::memory_order_acquire)) {
        return -1;
    }
    if (!_javaAlive.load(std::memory_order_acquire)) {
        return -1;
    }

    const qint64 maxSize = bytes.size();
    const qint64 budget = _writeBufferMaxSize ? _writeBufferMaxSize - _inFlight.load(std::memory_order_acquire) : maxSize;
    // Buffer full -> 0 (backpressure); QSerialPort convention, not a port-level error.
    if (budget <= 0) {
        return 0;
    }

    const qint64 toWrite = qMin(maxSize, budget);
    // Reserve before enqueuing: the Java writer can ack (decrement, clamped to 0) before a post-enqueue
    // fetch_add lands, stranding _inFlight high so waitForDrain never drains.
    {
        QMutexLocker lk(&_mutex);
        _inFlight.fetch_add(toWrite, std::memory_order_relaxed);
    }

    const qint64 enqueued = enqueueToJava(javaPort, QByteArrayView(bytes.data(), toWrite), location);
    if (enqueued != toWrite) {
        QMutexLocker lk(&_mutex);
        const qint64 refund = toWrite - qMax<qint64>(enqueued, 0);
        const qint64 next = _inFlight.load(std::memory_order_relaxed) - refund;
        _inFlight.store(next < 0 ? 0 : next, std::memory_order_relaxed);
    }
    return (enqueued > 0) ? enqueued : -1;
}

void SerialWriteEngine::ack(qint64 n)
{
    {
        QMutexLocker lk(&_mutex);
        const qint64 next = _inFlight.load(std::memory_order_relaxed) - n;
        _inFlight.store(next < 0 ? 0 : next, std::memory_order_relaxed);
    }
    _cv.wakeAll();
}

void SerialWriteEngine::fail()
{
    {
        QMutexLocker lk(&_mutex);
        _error.store(true, std::memory_order_relaxed);
        _inFlight.store(0, std::memory_order_relaxed);
    }
    _cv.wakeAll();
}

void SerialWriteEngine::wakeDrainWaiters()
{
    // Lock so we never wake between a waiter's predicate check and its wait() (no missed wakeup).
    QMutexLocker lk(&_mutex);
    _cv.wakeAll();
}

bool SerialWriteEngine::waitForDrain(int msecs)
{
    QMutexLocker locker(&_mutex);
    const auto settled = [this] {
        return _inFlight.load(std::memory_order_acquire) == 0 || _error.load(std::memory_order_acquire) ||
               !_javaAlive.load(std::memory_order_acquire);
    };

    // Clamp an infinite wait to a bounded one: a silently-wedged Java writer (USB stall with no
    // exception/disconnect callback) would otherwise block the link I/O thread forever.
    const int waitMs = (msecs < 0) ? kDefaultWriteTimeoutMs : msecs;
    QDeadlineTimer deadline(waitMs);
    while (!settled()) {
        if (!_cv.wait(&_mutex, deadline)) {
            break;
        }
    }
    // Drained iff in-flight hit zero with no write error. A still-nonzero backlog means error, gone port,
    // or timeout — the caller separates those via hasError()/inFlight()/the port-alive flag.
    return _inFlight.load(std::memory_order_acquire) == 0 && !_error.load(std::memory_order_acquire);
}
