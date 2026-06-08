#pragma once

// Write-side accounting + JNI submission for AndroidSerialPort, factored out of AndroidSerialPortPrivate.
// Owns the in-flight byte count, the write-error latch, the write-scratch DirectByteBuffer and the drain
// condition variable — i.e. everything between "owner thread enqueues" and "Java writer thread acks".
// It does NOT own the Java port or its liveness flag: those belong to the port's open/close lifecycle, so
// the engine holds references to them (stable for the owning AndroidSerialPortPrivate's lifetime).
//
// Threading: submit()/allocateScratch()/releaseScratch()/reset() run on the owner thread; ack()/fail() run
// on the JNI writer thread; waitForDrain() runs on whatever thread calls flush()/waitForBytesWritten().
// _mutex serialises the drain predicate against waiters.

#include <QtCore/QByteArrayView>
#include <QtCore/QJniObject>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>
#include <QtCore/QtTypes>

#include <atomic>
#include <memory>

class SerialWriteEngine
{
public:
    explicit SerialWriteEngine(const std::atomic<bool>& javaAlive)
        : _javaAlive(javaAlive)
    {
    }

    // Owner thread: (re)allocate the pinned write-scratch DirectByteBuffer. Logs and returns false on
    // failure (the caller maps that to QGCSerialPortError::OpenFailed).
    bool allocateScratch(const QString& location);
    void releaseScratch();

    // Owner thread: clear in-flight + error accounting on open()/close().
    void reset();
    // Owner thread: clear only the in-flight count on clear(output) (a purge, not a failure).
    void clearInFlight();

    void setWriteBufferMaxSize(qint64 size) { _writeBufferMaxSize = size; }
    qint64 writeBufferMaxSize() const { return _writeBufferMaxSize; }
    qint64 inFlight() const { return _inFlight.load(std::memory_order_acquire); }
    bool hasError() const { return _error.load(std::memory_order_acquire); }

    // Owner thread: enqueue up to maxSize bytes to the Java writer, honouring the write-buffer budget.
    // Returns bytes accepted, 0 for backpressure (buffer full), or -1 on error. javaPort is passed per call
    // (owner thread owns its lifetime) rather than aliased, so the engine holds no reference into the port.
    qint64 submit(QJniObject& javaPort, QByteArrayView bytes, const QString& location);

    // JNI writer thread: ack n written bytes (decrement in-flight, wake drain waiters).
    void ack(qint64 n);
    // JNI thread: latch a write-side failure and wake waiters (e.g. on a Java IOException).
    void fail();

    // Owner/JNI thread: wake drain waiters after the port's javaAlive flag was flipped false elsewhere
    // (disconnect/teardown), so a blocked waitForDrain re-checks its predicate and returns (not drained).
    void wakeDrainWaiters();

    // flush()/waitForBytesWritten() thread: block until drained / error / port-gone, bounded by msecs.
    // Returns true when in-flight reached zero; false on a write error, a gone port, or the deadline.
    // Callers disambiguate the false case via hasError()/inFlight().
    bool waitForDrain(int msecs);

private:
    qint64 enqueueToJava(QJniObject& javaPort, QByteArrayView bytes, const QString& location);

    const std::atomic<bool>& _javaAlive;

    QMutex _mutex;
    QWaitCondition _cv;
    std::atomic<qint64> _inFlight{0};  // bytes enqueued to Java, not yet acked
    std::atomic<bool> _error{false};   // write-side failure latch

    std::unique_ptr<char[]> _scratch;
    QJniObject _scratchBuffer;
    qint64 _writeBufferMaxSize = 0;
};
