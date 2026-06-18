#include "AndroidSerialPort.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QPointer>
#include <QtCore/QScopedValueRollback>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/qjnitypes.h>
#include <QtCore/qscopeguard.h>
#include <atomic>
#include <utility>

#include "AndroidSerialPortPrivate.h"
#include "AndroidSerialPortRegistry.h"
#include "QGCLoggingCategory.h"
#include "QGCSerialPortInfo.h"
#include "SerialPlatform.h"
#include "SerialPortInfoCodec.h"

QGC_LOGGING_CATEGORY(AndroidSerialPortLog, "Android.Serial.Port")

// JNI wire glue, parity/flow marshalling and the jni* callbacks live in
// AndroidSerialPortPrivate.h + AndroidSerialPortJni.cc.

namespace {

constexpr int kDefaultWriteTimeoutMs = 5000;

}  // namespace

void AndroidSerialPortPrivate::setError(QGCSerialPortError code, const QString& message)
{
    Q_Q(AndroidSerialPort);
    if (!onOwnerThread(q, "setError")) {
        return;
    }
    error = code;
    // errorString is QIODevicePrivate's storage; touch directly since Private isn't a QIODevice subclass.
    if (!message.isEmpty())
        errorString = message;
    emit q->errorOccurred(code);
}

void AndroidSerialPortPrivate::postBytesWritten(qint64 n)
{
    postToOwner([n](AndroidSerialPort* self) {
        AndroidSerialPortPrivate* const d = self->d_func();
        if (!d->emittedBytesWritten) {
            QScopedValueRollback<bool> r(d->emittedBytesWritten, true);
            emit self->bytesWritten(n);
        }
    });
}

void AndroidSerialPortPrivate::ackBytesWrittenFromJavaThread(qint64 n)
{
    writeEngine.ack(n);   // decrement in-flight, wake drain waiters
    postBytesWritten(n);  // marshal bytesWritten(n) to the owner thread
}

bool AndroidSerialPortPrivate::openJavaPort(qint64 newHandle, QIODeviceBase::OpenMode mode)
{
    const QJniObject name = QJniObject::fromString(systemLocation);
    if (!name.isValid()) {
        qCWarning(AndroidSerialPortLog) << "Invalid QJniObject for portName";
        return false;
    }

    const auto paramsObj = makeSerialParamsJni(pending.baud, pending.dataBits, pending.stopBits, pending.parity);
    if (!paramsObj.isValid()) {
        qCWarning(AndroidSerialPortLog) << "Failed to construct SerialParameters JNI object";
        return false;
    }

    const bool startReader = (mode & QIODevice::ReadOnly) != 0;
    QJniObject newJavaPort = QJniObject::callStaticMethod<QtJniTypes::QGCUsbSerialManager, QtJniTypes::QGCSerialPort>(
        "openConfiguredPort", name.object<jstring>(), static_cast<jlong>(newHandle), paramsObj,
        static_cast<jint>(flowControlToWire(pending.flowControl)),
        static_cast<jboolean>(true),  // assertDtr
        static_cast<jboolean>(startReader));
    if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(), QJniEnvironment::OutputMode::Verbose)) {
        qCWarning(AndroidSerialPortLog) << "Exception in openConfiguredPort for" << systemLocation;
        return false;
    }
    if (!newJavaPort.isValid()) {
        qCWarning(AndroidSerialPortLog) << "openConfiguredPort returned null for" << systemLocation;
        // Classify the failure: the device is present but the port couldn't be claimed (busy/duplicate/transient), so SerialWorker reports a reason instead of an empty errorString.
        setError(QGCSerialPortError::OpenFailed, QObject::tr("Could not open port (device busy or unavailable)"));
        return false;
    }

    javaPort = std::move(newJavaPort);
    // Warm m_className so callMethod() uses Qt's jmethodID cache (empty after jobject-ctor = uncached GetMethodID per call).
    (void) javaPort.className();
    javaAlive.store(true, std::memory_order_release);
    return true;
}

void AndroidSerialPortPrivate::teardownJavaPort()
{
    // Invariant: the C++ PortRegistry token must be unregistered LAST — after this Java close — or LookupGuard cannot fence in-flight JNI callbacks (use-after-free).
    // The Java writer thread is owned and joined Java-side by close(); nothing to join here.
    if (javaPortAlive()) {
        // Best-effort: stopIoManager's bool return is moot in the teardown path.
        (void) javaPort.callMethod<jboolean>("stopIoManager");
    }

    // exchange(false) wins the close race; a prior flip (double-close / Java auto-close on unplug) skips the redundant Java close().
    const bool wasAlive = javaAlive.exchange(false, std::memory_order_acq_rel);
    writeEngine.wakeDrainWaiters();  // unblock any in-progress waitForDrain now that the port is gone
    if (wasAlive && javaPort.isValid()) {
        if (!javaPort.callMethod<jboolean>("close")) {
            qCWarning(AndroidSerialPortLog) << "close returned false for serial port";
        }
    }
    javaPort = QJniObject();
    writeEngine.releaseScratch();
}

void AndroidSerialPortPrivate::dispatchCloseFromJavaThread()
{
    // Called from a JNI callback with PortRegistry::LookupGuard held.
    // Mark dead so the write guard stops submitting; the queued close() then skips the redundant Java close() (Java auto-closed on unplug).
    javaAlive.store(false, std::memory_order_release);
    writeEngine.wakeDrainWaiters();  // unblock waitForBytesWritten / flush waiting on javaAlive

    postToOwner([](AndroidSerialPort* self) {
        AndroidSerialPortPrivate* const d = self->d_func();
        if (self->isOpen()) {
            qCDebug(AndroidSerialPortLog) << "Closing serial port" << d->systemLocation << "after disconnect";
            self->close();
        } else {
            qCDebug(AndroidSerialPortLog) << "Serial port" << d->systemLocation << "already closed at disconnect";
        }
    });
}

void AndroidSerialPortPrivate::dispatchExceptionFromJavaThread(QGCSerialPortError errorCode, QString message)
{
    // Called from a JNI callback with PortRegistry::LookupGuard held.
    // Latch the write error on the JNI thread so a blocked waitForBytesWritten/flush wakes immediately.
    writeEngine.fail();
    postToOwner([errorCode, message = std::move(message)](AndroidSerialPort* self) {
        AndroidSerialPortPrivate* const d = self->d_func();
        if (d->expectClosure && errorCode == QGCSerialPortError::ResourceUnavailable) {
            qCDebug(AndroidSerialPortLog)
                << "Suppressed expected Resource exception on" << d->systemLocation << "during close:" << message;
            return;
        }
        qCWarning(AndroidSerialPortLog) << "Exception arrived on" << d->systemLocation
                                        << "error=" << static_cast<int>(errorCode) << ":" << message;
        // Close FIRST so reentrant DirectConnection slots on errorOccurred see fully-closed state.
        if (errorCode == QGCSerialPortError::ResourceUnavailable && self->isOpen()) {
            self->close();
        }
        d->setError(errorCode, message);
    });
}

AndroidSerialPort::AndroidSerialPort(QObject* parent) : QGCSerialPort(*new AndroidSerialPortPrivate, parent)
{
    initializeNative();
}

AndroidSerialPort::AndroidSerialPort(const QString& systemLocation, QObject* parent)
    : QGCSerialPort(*new AndroidSerialPortPrivate, parent)
{
    Q_D(AndroidSerialPort);
    d->systemLocation = systemLocation;
    initializeNative();
}

void AndroidSerialPortPrivate::jniUsbDevicesChanged(JNIEnv*, jobject)
{
    // Fired from a Java/binder thread; LinkManager's queued connection governs delivery and receiver lifetime, so emitting the relay signal is all that's needed.
    SerialPlatform::SerialDevicesNotifier::instance()->notifyChanged();
}

bool AndroidSerialPort::initializeNative()
{
    // Function-local static keeps native registration idempotent across JNI unload+reload.
    static const bool registered = []() {
        QJniEnvironment env;
        const bool ok = env.registerNativeMethods<QtJniTypes::QGCSerialPort>({
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceHasDisconnected, AndroidSerialPortPrivate),
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceNewData, AndroidSerialPortPrivate),
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceException, AndroidSerialPortPrivate),
            Q_JNI_NATIVE_SCOPED_METHOD(jniDeviceBytesWritten, AndroidSerialPortPrivate),
        });
        if (!ok) {
            qCWarning(AndroidSerialPortLog) << "Failed to register native methods for QGCSerialPort";
            return false;
        }

        const bool managerOk = env.registerNativeMethods<QtJniTypes::QGCUsbSerialManager>({
            Q_JNI_NATIVE_SCOPED_METHOD(jniUsbDevicesChanged, AndroidSerialPortPrivate),
        });
        if (!managerOk) {
            qCWarning(AndroidSerialPortLog) << "Failed to register native methods for QGCUsbSerialManager";
            return false;
        }

        // The C++/Java wire constants are kept in sync by hand across (SerialWireConstants.{h,java}),
        // so the former runtime handshake is gone. The only remaining invariant — our FC_* ordinals vs mik3y's
        // external FlowControl enum — is asserted by SerialWireConstantsTest at build time.

        qCDebug(AndroidSerialPortLog) << "Native Functions Registered Successfully";
        return true;
    }();

    return registered;
}

AndroidSerialPort::~AndroidSerialPort()
{
    Q_D(AndroidSerialPort);
    // Silent teardown: do NOT call the virtual close() from a destructor — it emits readChannelFinished() (DirectConnection slots run
    // against a half-destroyed object) and posts a queued lambda to a dying object. Backstop for "destroyed while open".
    if (isOpen()) {
        d->expectClosure = true;
        d->teardownJavaPort();
        QIODevice::close();
    }
    // Invariant: the C++ PortRegistry token must be unregistered LAST — after the Java close above — or LookupGuard cannot fence in-flight JNI callbacks (use-after-free).
    // Unconditional unregister: the token must leave the registry before the object dies (independent of open state), or a stale JNI callback dereferences freed memory.
    if (d->handle != 0) {
        PortRegistry::unregisterPort(d->handle);
        d->handle = 0;
    }
}

QList<QGCSerialPortInfo> AndroidSerialPort::availableDevices()
{
    QJniEnvironment env;

    // Single JNI hop: Java packs every port into one UTF-8 JSON byte[] (QGCUsbSerialManager.availablePortsPacked);
    // SerialPortInfoCodec (JNI-free, host-tested) owns the wire format.
    const QByteArray packed =
        QJniObject::callStaticMethod<QtJniTypes::QGCUsbSerialManager, QByteArray>("availablePortsPacked");
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialPortLog) << "Exception occurred while calling availablePortsPacked";
        return {};
    }
    if (packed.isEmpty()) {
        qCDebug(AndroidSerialPortLog) << "availablePortsPacked returned empty";
        return {};
    }

    const QList<QGCSerialPortInfo::Data> decoded = SerialPortInfoCodec::unpack(packed);
    QList<QGCSerialPortInfo> serialPortInfoList;
    serialPortInfoList.reserve(decoded.size());
    for (const QGCSerialPortInfo::Data& data : decoded) {
        serialPortInfoList.emplace_back(data);
    }
    return serialPortInfoList;
}

bool AndroidSerialPort::_validateOpenMode(QIODeviceBase::OpenMode mode)
{
    Q_D(AndroidSerialPort);
    static const OpenMode kUnsupported = Append | Truncate | Text | Unbuffered;
    if ((mode & kUnsupported) || mode == NotOpen) {
        d->setError(QGCSerialPortError::UnsupportedOperation, tr("Unsupported open mode"));
        return false;
    }
    return true;
}

bool AndroidSerialPort::open(QIODeviceBase::OpenMode mode)
{
    Q_D(AndroidSerialPort);

    // Teardown does a blocking JNI join, so a GUI-thread open would ANR (Qt's NMEA position source tries exactly that); refuse gracefully, callers run us off-thread.
    if (QCoreApplication::instance() && (thread() == QCoreApplication::instance()->thread())) {
        d->setError(QGCSerialPortError::OpenFailed, tr("Serial port cannot be opened on the GUI thread"));
        return false;
    }

    if (isOpen()) {
        d->setError(QGCSerialPortError::OpenFailed, tr("Device is already open"));
        return false;
    }
    if (!_validateOpenMode(mode))
        return false;

    // If JNI native methods never registered (Java/C++ wire-constant drift), callbacks never fire: the port "opens" but reads hang and disconnects go undetected. Fail loudly.
    if (!initializeNative()) {
        d->setError(QGCSerialPortError::OpenFailed, tr("Serial JNI bridge not initialized"));
        return false;
    }

    qCDebug(AndroidSerialPortLog) << "Opening" << d->systemLocation;
    clearError();

    const jlong handle = PortRegistry::allocateToken();
    PortRegistry::registerPort(handle, this);
    d->handle = handle;

    auto tokenCleanup = qScopeGuard([d, handle] {
        PortRegistry::unregisterPort(handle);
        d->handle = 0;
        d->javaAlive.store(false, std::memory_order_release);
    });

    if (!d->openJavaPort(handle, mode))
        return false;
    // After this point we own javaPort; any subsequent failure must tear it down.
    auto javaCleanup = qScopeGuard([d] { d->teardownJavaPort(); });

    if (!d->writeEngine.allocateScratch(d->systemLocation)) {
        d->setError(QGCSerialPortError::OpenFailed, tr("Failed to allocate write buffer"));
        return false;
    }

    // Reset write accounting; the Java writer thread is started Java-side inside openConfiguredPort.
    d->writeEngine.reset();
    // Clear any RX backlog left over from a prior open so stale queued lambdas can't under-count the cap.
    d->rxQueue.flush();

    javaCleanup.dismiss();
    tokenCleanup.dismiss();

    QIODevice::open(mode);
    // Purge AFTER QIODevice::open so clear()'s isOpen() guard doesn't leave a stale NotOpen error.
    if (!clear()) {
        qCDebug(AndroidSerialPortLog) << "Best-effort purge on open failed for" << d->systemLocation;
    }
    // clear() may have set a non-NoError code (e.g. purgeBuffers failure); a successful open must end clean.
    clearError();
    return true;
}

void AndroidSerialPort::close()
{
    Q_D(AndroidSerialPort);

    if (!onOwnerThread(this, "close")) {
        return;
    }

    if (!isOpen())
        return;

    qCDebug(AndroidSerialPortLog) << "Closing" << d->systemLocation;

    d->expectClosure = true;
    emit aboutToClose();

    d->teardownJavaPort();

    if (d->handle != 0) {
        PortRegistry::unregisterPort(d->handle);
        d->handle = 0;
    }
    d->emittedReadyRead = false;
    d->emittedBytesWritten = false;
    d->buffer.clear();
    d->rxQueue.flush();
    d->writeEngine.reset();

    QIODevice::close();

    // Defer expectClosure=false so racing JNI IOException lambdas (QueuedConnection on this owner thread) drain first — event-loop FIFO.
    QPointer<AndroidSerialPort> self = this;
    QMetaObject::invokeMethod(
        this,
        [self]() {
            if (!self)
                return;
            self->d_func()->expectClosure = false;
        },
        Qt::QueuedConnection);

    emit readChannelFinished();
}

qint64 AndroidSerialPort::readData(char* data, qint64 maxSize)
{
    // Sequential device: RX lands in QIODevicePrivate::buffer (appendToBuffer) and is drained by the base read()
    // after readyRead; this override is never the data source, so it returns 0 ("no data now", not EOF).
    if (!onOwnerThread(this, "readData")) {
        return -1;
    }
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return 0;
}

qint64 AndroidSerialPort::writeData(const char* data, qint64 maxSize)
{
    Q_D(AndroidSerialPort);
    if (!onOwnerThread(this, "writeData")) {
        return -1;
    }

    if (!data || maxSize <= 0) {
        qCWarning(AndroidSerialPortLog) << "Invalid data or size for" << d->systemLocation;
        return -1;
    }

    return d->writeEngine.submit(d->javaPort, QByteArrayView(data, maxSize), d->systemLocation);
}

bool AndroidSerialPort::waitForReadyRead(int msecs)
{
    Q_D(AndroidSerialPort);

    if (!d->buffer.isEmpty())
        return true;
    if (!isOpen()) {
        d->setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }

    if (d->javaPortAlive() && msecs != 0) {
        QEventLoop loop;
        const QMetaObject::Connection ready = connect(this, &QIODevice::readyRead, &loop, &QEventLoop::quit);
        const QMetaObject::Connection closed = connect(this, &QIODevice::aboutToClose, &loop, &QEventLoop::quit);
        if (msecs > 0) {
            QTimer::singleShot(msecs, Qt::PreciseTimer, &loop, &QEventLoop::quit);
        }
        loop.exec(QEventLoop::ExcludeUserInputEvents);
        disconnect(ready);
        disconnect(closed);
        if (!isOpen()) {
            return false;
        }
    }

    if (d->buffer.isEmpty()) {
        qCWarning(AndroidSerialPortLog) << "Timeout while waiting for ready read on" << d->systemLocation;
        d->setError(QGCSerialPortError::Timeout, tr("Timeout while waiting for ready read"));
        return false;
    }

    if (!d->emittedReadyRead) {
        QScopedValueRollback<bool> rollback(d->emittedReadyRead, true);
        emit readyRead();
    }
    return true;
}

bool AndroidSerialPort::waitForBytesWritten(int msecs)
{
    Q_D(AndroidSerialPort);

    if (d->writeEngine.waitForDrain(msecs)) {
        return true;
    }
    // A write error or a gone port already failed the wait (error reported via dispatchException / teardown);
    // a clean deadline expiry with the port still alive is the only Timeout case.
    if (!d->writeEngine.hasError() && d->javaPortAlive()) {
        d->setError(QGCSerialPortError::Timeout, tr("Timeout while waiting for bytes written"));
    }
    return false;
}

qint64 AndroidSerialPort::bytesToWrite() const
{
    Q_D(const AndroidSerialPort);
    // writeData enqueues straight to Java (never QIODevice's writeBuffer), so in-flight is the whole story; QIODevice::bytesToWrite() is always 0 here.
    return d->writeEngine.inFlight();
}

QString AndroidSerialPort::systemLocation() const
{
    Q_D(const AndroidSerialPort);
    return d->systemLocation;
}

void AndroidSerialPort::setSystemLocation(const QString& location)
{
    Q_D(AndroidSerialPort);
    d->systemLocation = location;
}

QString AndroidSerialPort::portName() const
{
    Q_D(const AndroidSerialPort);
    const int idx = d->systemLocation.lastIndexOf(QLatin1Char('/'));
    return idx >= 0 ? d->systemLocation.mid(idx + 1) : d->systemLocation;
}

SerialPortConfig AndroidSerialPort::portConfig() const
{
    Q_D(const AndroidSerialPort);
    return d->pending;
}

QGCSerialPortError AndroidSerialPort::error() const
{
    Q_D(const AndroidSerialPort);
    return d->error;
}

bool AndroidSerialPort::reconfigure(const SerialPortConfig& cfg)
{
    Q_D(AndroidSerialPort);

    if (!cfg.isValid()) {
        d->setError(QGCSerialPortError::UnsupportedOperation, tr("Invalid serial configuration"));
        return false;
    }
    if (cfg == d->pending)
        return true;

    // Pre-open: stash. Java owns truth once open() runs; closed callers tee up next open.
    if (!isOpen()) {
        d->pending = cfg;
        return true;
    }

    if (!d->javaPortAlive()) {
        d->setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }

    // Two JNI hops (params batched, flowControl separate); stash only what Java accepted so d->pending stays in sync on partial failure.
    const bool paramsDiff = d->pending.baud != cfg.baud || d->pending.dataBits != cfg.dataBits ||
                            d->pending.stopBits != cfg.stopBits || d->pending.parity != cfg.parity;
    if (paramsDiff) {
        const auto paramsObj = makeSerialParamsJni(cfg.baud, cfg.dataBits, cfg.stopBits, cfg.parity);
        if (!paramsObj.isValid()) {
            qCWarning(AndroidSerialPortLog) << "Failed to construct SerialParameters JNI object";
            d->setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set serial parameters"));
            return false;
        }
        const jboolean setParamsOk = d->javaPort.callMethod<jboolean>("setSerialParameters", paramsObj);
        if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(),
                                                     QJniEnvironment::OutputMode::Verbose) ||
            !setParamsOk) {
            d->setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set serial parameters"));
            return false;
        }
        d->pending.baud = cfg.baud;
        d->pending.dataBits = cfg.dataBits;
        d->pending.stopBits = cfg.stopBits;
        d->pending.parity = cfg.parity;
    }

    if (d->pending.flowControl != cfg.flowControl) {
        const jboolean setFlowOk =
            d->javaPort.callMethod<jboolean>("setFlowControl", static_cast<jint>(flowControlToWire(cfg.flowControl)));
        if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(),
                                                     QJniEnvironment::OutputMode::Verbose) ||
            !setFlowOk) {
            d->setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set flow control"));
            return false;
        }
        d->pending.flowControl = cfg.flowControl;
    }

    return true;
}

bool AndroidSerialPort::setDataTerminalReady(bool set)
{
    Q_D(AndroidSerialPort);
    if (!isOpen() || !d->javaPortAlive()) {
        d->setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    const jboolean setDtrOk = d->javaPort.callMethod<jboolean>("setDataTerminalReady", static_cast<jboolean>(set));
    if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(), QJniEnvironment::OutputMode::Verbose) ||
        !setDtrOk) {
        d->setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set DTR"));
        return false;
    }
    return true;
}

bool AndroidSerialPort::flush()
{
    Q_D(AndroidSerialPort);
    if (!isOpen()) {
        d->setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }

    if (d->writeEngine.waitForDrain(kDefaultWriteTimeoutMs)) {
        return true;
    }
    if (!d->writeEngine.hasError() && d->javaPortAlive()) {
        d->setError(QGCSerialPortError::Timeout, tr("Timeout while flushing"));
    }
    return false;
}

bool AndroidSerialPort::clear(bool input, bool output)
{
    Q_D(AndroidSerialPort);
    if (!onOwnerThread(this, "clear")) {
        return false;
    }
    if (!isOpen() || !d->javaPortAlive()) {
        d->setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    if (input) {
        d->buffer.clear();
        d->rxQueue.flush();
    }
    if (output) {
        d->writeEngine.clearInFlight();
    }
    const jboolean purgeOk =
        d->javaPort.callMethod<jboolean>("purgeBuffers", static_cast<jboolean>(input), static_cast<jboolean>(output));
    if (QJniEnvironment::checkAndClearExceptions(QJniEnvironment::getJniEnv(), QJniEnvironment::OutputMode::Verbose) ||
        !purgeOk) {
        d->setError(QGCSerialPortError::Unknown, tr("Failed to purge buffers"));
        return false;
    }
    return true;
}

void AndroidSerialPort::clearError()
{
    Q_D(AndroidSerialPort);
    d->error = QGCSerialPortError::NoError;
    setErrorString(tr("No error"));
}

void AndroidSerialPort::setWriteBufferSize(qint64 size)
{
    Q_D(AndroidSerialPort);
    d->writeEngine.setWriteBufferMaxSize(size);
}

qint64 AndroidSerialPort::writeBufferSize() const
{
    Q_D(const AndroidSerialPort);
    return d->writeEngine.writeBufferMaxSize();
}

void AndroidSerialPortPrivate::enqueueFromJavaThread(QByteArrayView bytes)
{
    // JNI thread -> owner thread via one QueuedConnection hop; payload copied once into the capture.
    if (bytes.isEmpty())
        return;

    if (!javaPortAlive())
        return;

    // Bound the posted-lambda backlog against the read-buffer cap: a stalled owner thread would otherwise let queued payloads grow unbounded (appendToBuffer's cap only fires once drained). Drop here instead.
    const AndroidSerialRxQueue::Reservation reservation = rxQueue.reserve(bytes.size());
    if (!reservation.accepted) {
        if (reservation.shouldWarn) {
            qCWarning(AndroidSerialPortLog) << "RX queue backlog cap" << rxQueue.cap() << "exceeded on"
                                            << systemLocation << "- dropping data (consumer stalled)";
        }
        return;
    }

    postToOwner([generation = reservation.generation,
                 payload = QByteArray(bytes.constData(), bytes.size())](AndroidSerialPort* self) mutable {
        self->d_func()->appendToBuffer(std::move(payload), generation);
    });
}

void AndroidSerialPortPrivate::appendToBuffer(QByteArray payload, quint32 generation)
{
    // Owner thread — sole writer of buffer alongside clear()/close().
    Q_Q(AndroidSerialPort);
    if (!onOwnerThread(q, "appendToBuffer")) {
        return;
    }
    // Drop payloads stamped before a clear(Input)/close() bumped the epoch (caller asked them flushed); still release the reservation below so cap accounting stays balanced.
    const bool stale = rxQueue.isStale(generation);
    // Release the backlog reservation taken in enqueueFromJavaThread regardless of whether the payload is kept.
    auto releaseReservation = qScopeGuard([this, reserved = payload.size()] { rxQueue.releaseReservation(reserved); });
    if (stale || !q->isOpen() || !javaPortAlive() || payload.isEmpty())
        return;

    // Bound RX growth if the consumer stalls (mirrors HostSerialPort's lossless 512 KB cap; lossy here because Android's reader has no pause primitive).
    const AndroidSerialRxQueue::BufferCapDecision capDecision = rxQueue.checkBufferCap(buffer.size(), payload.size());
    if (capDecision.overCap) {
        if (capDecision.shouldWarn) {
            qCWarning(AndroidSerialPortLog) << "RX buffer cap" << rxQueue.cap() << "exceeded on" << systemLocation
                                            << "- dropping data (consumer stalled)";
        }
        return;
    }

    buffer.append(payload);

    if (!emittedReadyRead) {
        emittedReadyRead = true;
        // Not QScopedValueRollback: a readyRead slot may delete the port, so the flag restore must be
        // gated on a QPointer liveness check — an unconditional rollback would write to freed memory.
        QPointer<AndroidSerialPort> self = q;
        emit q->readyRead();
        if (self)
            emittedReadyRead = false;
    }
}
