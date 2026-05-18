// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "QGCAndroidSerialPort.h"

#include "AndroidSerial.h"
#include "QGCLoggingCategory.h"
#ifdef Q_OS_ANDROID
#include "qandroidserialengine_p.h"
#endif

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QSpan>

#include <chrono>
#include <climits>
#include <utility>

using namespace std::chrono_literals;

QGC_LOGGING_CATEGORY(QGCAndroidSerialPortLog, "Android.Serial.Port")

// Per-call sync drain cap. Matches MAX_SYNC_WRITE_CHUNK in legacy qserialport_p.h:
// 16 KB amortizes JNI + bulkTransfer setup overhead vs 4 KB; on saturated CDC links the
// per-call time scales sub-linearly with chunk size (most cost is the round-trip ACK).
namespace {
constexpr qint64 kMaxSyncWriteChunk = 16 * 1024;
constexpr int    kDefaultWriteTimeoutMs = 5000;

int parityToWire(QGCParity parity)
{
    switch (parity) {
        case QGCParity::None:  return AndroidSerial::NoParity;
        case QGCParity::Odd:   return AndroidSerial::OddParity;
        case QGCParity::Even:  return AndroidSerial::EvenParity;
        case QGCParity::Mark:  return AndroidSerial::MarkParity;
        case QGCParity::Space: return AndroidSerial::SpaceParity;
    }
    return AndroidSerial::NoParity;
}

int flowControlToWire(QGCFlowControl fc)
{
    switch (fc) {
        case QGCFlowControl::None:            return AndroidSerial::NoFlowControl;
        case QGCFlowControl::HardwareRtsCts:  return AndroidSerial::RtsCtsFlowControl;
        case QGCFlowControl::SoftwareXonXoff: return AndroidSerial::XonXoffFlowControl;
    }
    return AndroidSerial::NoFlowControl;
}

// Single mapping point for refining JavaExceptionKind → QGCSerialPortError as new
// exception kinds emerge. Mirrors the legacy exceptionKindToErrorCode().
QGCSerialPortError exceptionKindToError(AndroidSerial::JavaExceptionKind kind)
{
    switch (kind) {
        case AndroidSerial::JavaExceptionKind::Resource:   return QGCSerialPortError::ResourceUnavailable;
        case AndroidSerial::JavaExceptionKind::Permission: return QGCSerialPortError::PermissionDenied;
        case AndroidSerial::JavaExceptionKind::OpenFailed: return QGCSerialPortError::OpenFailed;
        case AndroidSerial::JavaExceptionKind::Unknown:    return QGCSerialPortError::Unknown;
    }
    return QGCSerialPortError::Unknown;
}
}  // namespace

// ============================================================================
// Factory
// ============================================================================

namespace {
// Default engine factory: instantiates the JNI-backed engine on Android, returns nullptr on
// host (where tests must inject MockQSerialPortEngine via setEngineFactory before constructing
// a QGCAndroidSerialPort). Keeps QGCAndroidSerialPort.cc host-buildable so production logic
// can be exercised without JNI.
QGCAndroidSerialPortFactory::EngineFactory defaultFactory()
{
#ifdef Q_OS_ANDROID
    return [](QAndroidSerialEngineReceiver* sink, QObject* owner) {
        return std::unique_ptr<IQSerialPortEngine>(new QAndroidSerialEngine(sink, owner));
    };
#else
    return [](QAndroidSerialEngineReceiver*, QObject*) {
        return std::unique_ptr<IQSerialPortEngine>{};
    };
#endif
}
}  // namespace

QGCAndroidSerialPortFactory::EngineFactory& QGCAndroidSerialPortFactory::_engineFactory()
{
    static EngineFactory factory = defaultFactory();
    return factory;
}

void QGCAndroidSerialPortFactory::setEngineFactory(EngineFactory factory)
{
    _engineFactory() = factory ? std::move(factory) : defaultFactory();
}

void QGCAndroidSerialPortFactory::resetEngineFactory()
{
    setEngineFactory(EngineFactory{});
}

// ============================================================================
// Construction
// ============================================================================

QGCAndroidSerialPort::QGCAndroidSerialPort(QObject* parent)
    : QIODevice(parent)
{
}

QGCAndroidSerialPort::QGCAndroidSerialPort(const QString& systemLocation, QObject* parent)
    : QIODevice(parent)
    , _systemLocation(systemLocation)
{
}

QGCAndroidSerialPort::~QGCAndroidSerialPort()
{
    if (isOpen()) {
        close();
    }
}

// ============================================================================
// Lifecycle
// ============================================================================

bool QGCAndroidSerialPort::open(QIODeviceBase::OpenMode mode)
{
    if (isOpen()) {
        _setError(QGCSerialPortError::OpenFailed, tr("Device is already open"));
        return false;
    }

    static const OpenMode kUnsupported = Append | Truncate | Text | Unbuffered;
    if ((mode & kUnsupported) || mode == NotOpen) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Unsupported open mode"));
        return false;
    }

    qCDebug(QGCAndroidSerialPortLog) << "Opening" << _systemLocation;

    clearError();

    const AndroidSerial::SerialParameters params{
        .baudRate = _baudRate,
        .dataBits = static_cast<int>(_dataBits),
        .stopBits = static_cast<int>(_stopBits),
        .parity   = parityToWire(_parity),
    };

    _engine = QGCAndroidSerialPortFactory::_engineFactory()(this, this);
    if (!_engine) {
        qCWarning(QGCAndroidSerialPortLog) << "Engine factory returned null for" << _systemLocation;
        _setError(QGCSerialPortError::OpenFailed, tr("Failed to construct serial engine"));
        return false;
    }
    _engine->setReadBufferMaxSize(_readBufferMaxSize);

    // assertDtr=true: CP210x / CH340 chips gate UART output on DTR. SerialLink later
    // resets DTR per dtrForceLow configuration.
    if (!_engine->open(_systemLocation, params, flowControlToWire(_flowControl), /*assertDtr=*/true)) {
        qCWarning(QGCAndroidSerialPortLog) << "Error opening" << _systemLocation;
        _setError(QGCSerialPortError::OpenFailed, tr("Device not found: %1").arg(_systemLocation));
        _engine.reset();
        return false;
    }

    if (_engine->deviceHandle() == -1) {
        qCWarning(QGCAndroidSerialPortLog) << "Failed to get device handle for" << _systemLocation;
        _setError(QGCSerialPortError::OpenFailed, tr("Failed to obtain device handle"));
        close();
        return false;
    }

    if (mode & QIODevice::ReadOnly) {
        if (!_startAsyncRead()) {
            qCWarning(QGCAndroidSerialPortLog) << "Failed to start async read for" << _systemLocation;
            close();
            return false;
        }
    }

    // Owner-thread write timer. QChronoTimer is thread-affine — start() from a foreign
    // thread is not safe, so create here in open() rather than lazily in writeData().
    if (!_writeTimer) {
        _writeTimer = std::make_unique<QChronoTimer>(this);
        _writeTimer->setSingleShot(true);
        connect(_writeTimer.get(), &QChronoTimer::timeout, this,
                &QGCAndroidSerialPort::_drainWriteBuffer);
    }

    QIODevice::open(mode);

    // Purge stale buffers AFTER QIODevice::open so clear()'s isOpen() guard doesn't
    // leave a stale NotOpen error on a successful open.
    (void)clear();
    return true;
}

void QGCAndroidSerialPort::close()
{
    if (!isOpen()) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return;
    }

    qCDebug(QGCAndroidSerialPortLog) << "Closing" << _systemLocation;

    // Set BEFORE aboutToClose so any synchronously-dispatched exception inside the slot
    // (e.g. a queued Resource exception that races the close) is also suppressed.
    _expectClosure = true;

    // QIODevice contract: aboutToClose fires BEFORE the device is torn down so slots
    // can still drain pending data. (Fixes the inversion in the legacy QSerialPortPrivate
    // that called d->close() before QIODevice::close(), flagged by qt-qml-expert.)
    emit aboutToClose();

    (void)_stopAsyncRead();

    _writeTimer.reset();

    if (_engine) {
        if (!_engine->isOpen()) {
            qCWarning(QGCAndroidSerialPortLog) << "Failed to close device, port not open";
        }
        _engine.reset();
    }

    // Re-entrancy guards reset so a reopen of this instance starts clean.
    _emittedReadyRead = false;
    _emittedBytesWritten = false;
    _expectClosure = false;
    _breakEnabled = false;
    _readBuffer.clear();
    _readOffset = 0;
    _writeBuffer.clear();

    QIODevice::close();

    // Notify consumers (e.g. bootloader upload) that no more reads will arrive.
    emit readChannelFinished();
}

// ============================================================================
// QAndroidSerialEngineReceiver — engine has marshaled to the owner thread.
// ============================================================================

void QGCAndroidSerialPort::dataReady(QByteArray&& bytes)
{
    if (bytes.isEmpty()) return;
    // Compact the dead prefix only when it accounts for half the backing buffer or more —
    // bounds wasted memory without paying for a memmove on every append.
    if (_readOffset > 0 && _readOffset >= _readBuffer.size() / 2) {
        _readBuffer.remove(0, _readOffset);
        _readOffset = 0;
    }
    _readBuffer.append(std::move(bytes));
    if (!_emittedReadyRead) {
        _emittedReadyRead = true;
        emit readyRead();
        _emittedReadyRead = false;
    }
}

void QGCAndroidSerialPort::closeNotification()
{
    if (isOpen()) {
        qCDebug(QGCAndroidSerialPortLog) << "Closing serial port" << _systemLocation
                                         << "after disconnect";
        // Peer is gone — draining queued writes spams "null port" warnings.
        _writeBuffer.clear();
        close();
    } else {
        qCDebug(QGCAndroidSerialPortLog) << "Serial port" << _systemLocation
                                         << "already closed at disconnect";
    }
}

void QGCAndroidSerialPort::exceptionNotification(AndroidSerial::JavaExceptionKind kind,
                                                  const QString& message)
{
    if (_expectClosure && kind == AndroidSerial::JavaExceptionKind::Resource) {
        qCDebug(QGCAndroidSerialPortLog) << "Suppressed expected Resource exception on"
                                         << _systemLocation << "during close:" << message;
        return;
    }

    qCWarning(QGCAndroidSerialPortLog) << "Exception arrived on" << _systemLocation
                                       << "kind=" << static_cast<int>(kind) << ":" << message;

    _setError(exceptionKindToError(kind), message);

    // Resource exceptions = device unreachable. Close eagerly instead of waiting for the
    // DETACH-driven closeNotification — otherwise queued writes keep firing _drainWriteBuffer
    // → engine->writeSync → "null port" warns.
    if (kind == AndroidSerial::JavaExceptionKind::Resource && isOpen()) {
        _writeBuffer.clear();
        close();
    }
}

// ============================================================================
// QIODevice I/O
// ============================================================================

qint64 QGCAndroidSerialPort::readData(char* data, qint64 maxSize)
{
    if (!data || maxSize <= 0) return 0;
    const qsizetype available = _readBuffer.size() - _readOffset;
    if (available <= 0) return 0;

    const qint64 toCopy = qMin<qint64>(maxSize, available);
    std::memcpy(data, _readBuffer.constData() + _readOffset, toCopy);
    _readOffset += toCopy;
    // Fully drained — drop the buffer outright so we don't carry a stale allocation.
    if (_readOffset >= _readBuffer.size()) {
        _readBuffer.clear();
        _readOffset = 0;
    }
    return toCopy;
}

qint64 QGCAndroidSerialPort::readLineData(char* data, qint64 maxSize)
{
    return QIODevice::readLineData(data, maxSize);
}

qint64 QGCAndroidSerialPort::writeData(const char* data, qint64 maxSize)
{
    if (!data || maxSize <= 0) {
        qCWarning(QGCAndroidSerialPortLog) << "Invalid data or size in writeData for" << _systemLocation;
        _setError(QGCSerialPortError::Write, tr("Invalid data or size"));
        return -1;
    }

    // Cap check first — if writeBufferMaxSize is set and the buffer is at capacity,
    // returning 0 would be read as "no error, no bytes written" and silently drop the
    // payload. Signal Write error and return -1 to match upstream qserialport_win.cpp.
    const qint64 toAppend = _writeBufferMaxSize
        ? qMin(maxSize, _writeBufferMaxSize - _writeBuffer.size())
        : maxSize;
    if (toAppend <= 0) {
        qCWarning(QGCAndroidSerialPortLog) << "writeBuffer full ("
                                           << _writeBuffer.size() << "/"
                                           << _writeBufferMaxSize << "bytes) on"
                                           << _systemLocation << ", dropping write of"
                                           << maxSize << "bytes";
        _setError(QGCSerialPortError::Write, tr("Write buffer full"));
        return -1;
    }

    _writeBuffer.append(data, toAppend);

    // Fresh writes drain on next event-loop tick (interval 0). Partial-write reschedules
    // in _drainWriteBuffer() use a 5 ms back-off so backpressure doesn't spin the loop.
    if (_writeTimer && !_writeTimer->isActive()) {
        _writeTimer->setInterval(0ns);
        _writeTimer->start();
    }

    return toAppend;
}

void QGCAndroidSerialPort::_drainWriteBuffer()
{
    qint64 totalWritten = 0;
    bool writeFailed = false;

    while (!_writeBuffer.isEmpty()) {
        const qint64 chunk = qMin<qint64>(_writeBuffer.size(), kMaxSyncWriteChunk);
        const qint64 written = _writeToEngine(_writeBuffer.constData(), chunk,
                                              kDefaultWriteTimeoutMs, /*async=*/false);
        if (written <= 0) {
            // Peer gone — drop queued bytes. Retrying spins the event loop and starves
            // the queued exception/close metaCalls that would otherwise tear the port down.
            _writeBuffer.clear();
            writeFailed = true;
            break;
        }
        _writeBuffer.remove(0, written);
        totalWritten += written;
    }

    if (totalWritten > 0 && !_emittedBytesWritten) {
        _emittedBytesWritten = true;
        emit bytesWritten(totalWritten);
        _emittedBytesWritten = false;
    }

    if (!writeFailed && !_writeBuffer.isEmpty() && _writeTimer) {
        _writeTimer->setInterval(5ms);
        _writeTimer->start();
    }
}

qint64 QGCAndroidSerialPort::_writeToEngine(const char* data, qint64 maxSize, int timeout, bool async)
{
    if (!_engine) return -1;

    // JNI layer uses jint (32-bit); cap to INT_MAX to avoid silent truncation.
    const int capped = static_cast<int>(qMin(maxSize, static_cast<qint64>(INT_MAX)));
    const QSpan<const char> chunk{data, capped};
    const qint64 result = async ? _engine->writeAsync(chunk, timeout)
                                : _engine->writeSync(chunk, timeout);
    if (result < 0) {
        qCWarning(QGCAndroidSerialPortLog) << "Failed to write to" << _systemLocation;
        _setError(QGCSerialPortError::Write, tr("Failed to write to port"));
    }
    return result;
}

// ============================================================================
// Read thread
// ============================================================================

bool QGCAndroidSerialPort::_startAsyncRead()
{
    if (!_engine) return false;
    if (_engine->readThreadRunning()) return true;
    if (!_engine->startReadThread()) {
        qCWarning(QGCAndroidSerialPortLog) << "Failed to start async read thread for"
                                           << _systemLocation;
        _setError(QGCSerialPortError::Read, tr("Failed to start async read"));
        return false;
    }
    return true;
}

bool QGCAndroidSerialPort::_stopAsyncRead()
{
    if (!_engine) return true;
    if (!_engine->readThreadRunning()) return true;
    if (!_engine->stopReadThread()) {
        qCWarning(QGCAndroidSerialPortLog) << "Failed to stop async read thread for"
                                           << _systemLocation;
        _setError(QGCSerialPortError::Read, tr("Failed to stop async read"));
        return false;
    }
    return true;
}

// ============================================================================
// Blocking waits
// ============================================================================

bool QGCAndroidSerialPort::waitForReadyRead(int msecs)
{
    if (_readBuffer.size() - _readOffset > 0) return true;
    if (!_engine) return false;

    // Drain pending writes — the write timer needs the event loop, and a blocked caller
    // (e.g. Bootloader::_read) doesn't pump it.
    if (!_writeBuffer.isEmpty()) {
        _drainWriteBuffer();
    }

    QByteArray data = _engine->waitForReadyRead(msecs, _readBuffer.size() - _readOffset);
    if (data.isEmpty()) {
        qCWarning(QGCAndroidSerialPortLog) << "Timeout while waiting for ready read on"
                                           << _systemLocation;
        _setError(QGCSerialPortError::Timeout, tr("Timeout while waiting for ready read"));
        return false;
    }

    if (_readOffset > 0 && _readOffset >= _readBuffer.size() / 2) {
        _readBuffer.remove(0, _readOffset);
        _readOffset = 0;
    }
    _readBuffer.append(std::move(data));
    return true;
}

bool QGCAndroidSerialPort::waitForBytesWritten(int msecs)
{
    QDeadlineTimer deadline(msecs);
    while (!_writeBuffer.isEmpty()) {
        const int remaining = deadline.isForever() ? kDefaultWriteTimeoutMs
                                                   : static_cast<int>(deadline.remainingTime());
        if (!deadline.isForever() && remaining <= 0) {
            _setError(QGCSerialPortError::Timeout, tr("Timeout while waiting for bytes written"));
            return false;
        }

        const qint64 chunk = qMin<qint64>(_writeBuffer.size(), kMaxSyncWriteChunk);
        const qint64 written = _writeToEngine(_writeBuffer.constData(), chunk,
                                              qMax(1, remaining), /*async=*/false);
        if (written <= 0) return false;

        _writeBuffer.remove(0, written);
        if (!_emittedBytesWritten) {
            _emittedBytesWritten = true;
            emit bytesWritten(written);
            _emittedBytesWritten = false;
        }
    }
    return true;
}

// ============================================================================
// Buffers / state queries
// ============================================================================

qint64 QGCAndroidSerialPort::bytesAvailable() const
{
    return (_readBuffer.size() - _readOffset) + QIODevice::bytesAvailable();
}

qint64 QGCAndroidSerialPort::bytesToWrite() const
{
    return _writeBuffer.size() + QIODevice::bytesToWrite();
}

bool QGCAndroidSerialPort::canReadLine() const
{
    // Search only the live tail — the dead prefix [0, _readOffset) was already consumed.
    const QByteArrayView tail(_readBuffer.constData() + _readOffset,
                              _readBuffer.size() - _readOffset);
    return tail.contains('\n') || QIODevice::canReadLine();
}

void QGCAndroidSerialPort::setReadBufferSize(qint64 size)
{
    _readBufferMaxSize = size;
    if (_engine) _engine->setReadBufferMaxSize(size);
}

void QGCAndroidSerialPort::setWriteBufferSize(qint64 size)
{
    _writeBufferMaxSize = size;
}

// ============================================================================
// Port identification
// ============================================================================

void QGCAndroidSerialPort::setSystemLocation(const QString& location)
{
    _systemLocation = location;
}

QString QGCAndroidSerialPort::portName() const
{
    // System location is typically "/dev/bus/usb/<bus>/<dev>" — keep last component.
    const int idx = _systemLocation.lastIndexOf(QLatin1Char('/'));
    return idx >= 0 ? _systemLocation.mid(idx + 1) : _systemLocation;
}

// ============================================================================
// Configuration setters — cached when closed, applied to engine when open.
// ============================================================================

bool QGCAndroidSerialPort::setBaudRate(qint32 baudRate)
{
    if (baudRate <= 0) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Invalid baud rate value"));
        return false;
    }
    if (_baudRate == baudRate) return true;
    if (isOpen()) {
        if (!_engine) return false;
        const AndroidSerial::SerialParameters params{
            .baudRate = baudRate,
            .dataBits = static_cast<int>(_dataBits),
            .stopBits = static_cast<int>(_stopBits),
            .parity   = parityToWire(_parity),
        };
        if (!_engine->setParameters(params)) {
            _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set baud rate"));
            return false;
        }
    }
    _baudRate = baudRate;
    emit baudRateChanged(baudRate);
    return true;
}

bool QGCAndroidSerialPort::setDataBits(QGCDataBits dataBits)
{
    if (_dataBits == dataBits) return true;
    if (isOpen()) {
        if (!_engine) return false;
        const AndroidSerial::SerialParameters params{
            .baudRate = _baudRate,
            .dataBits = static_cast<int>(dataBits),
            .stopBits = static_cast<int>(_stopBits),
            .parity   = parityToWire(_parity),
        };
        if (!_engine->setParameters(params)) {
            _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set data bits"));
            return false;
        }
    }
    _dataBits = dataBits;
    emit dataBitsChanged(dataBits);
    return true;
}

bool QGCAndroidSerialPort::setParity(QGCParity parity)
{
    if (_parity == parity) return true;
    if (isOpen()) {
        if (!_engine) return false;
        const AndroidSerial::SerialParameters params{
            .baudRate = _baudRate,
            .dataBits = static_cast<int>(_dataBits),
            .stopBits = static_cast<int>(_stopBits),
            .parity   = parityToWire(parity),
        };
        if (!_engine->setParameters(params)) {
            _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set parity"));
            return false;
        }
    }
    _parity = parity;
    emit parityChanged(parity);
    return true;
}

bool QGCAndroidSerialPort::setStopBits(QGCStopBits stopBits)
{
    if (_stopBits == stopBits) return true;
    if (isOpen()) {
        if (!_engine) return false;
        const AndroidSerial::SerialParameters params{
            .baudRate = _baudRate,
            .dataBits = static_cast<int>(_dataBits),
            .stopBits = static_cast<int>(stopBits),
            .parity   = parityToWire(_parity),
        };
        if (!_engine->setParameters(params)) {
            _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set stop bits"));
            return false;
        }
    }
    _stopBits = stopBits;
    emit stopBitsChanged(stopBits);
    return true;
}

bool QGCAndroidSerialPort::setSerialParameters(qint32 baudRate, QGCDataBits dataBits,
                                               QGCStopBits stopBits, QGCParity parity)
{
    if (baudRate <= 0) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Invalid baud rate value"));
        return false;
    }
    const bool baudDiff   = _baudRate != baudRate;
    const bool dataDiff   = _dataBits != dataBits;
    const bool stopDiff   = _stopBits != stopBits;
    const bool parityDiff = _parity   != parity;
    if (!baudDiff && !dataDiff && !stopDiff && !parityDiff) return true;
    if (isOpen()) {
        if (!_engine) return false;
        const AndroidSerial::SerialParameters params{
            .baudRate = baudRate,
            .dataBits = static_cast<int>(dataBits),
            .stopBits = static_cast<int>(stopBits),
            .parity   = parityToWire(parity),
        };
        if (!_engine->setParameters(params)) {
            _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set serial parameters"));
            return false;
        }
    }
    _baudRate = baudRate;
    _dataBits = dataBits;
    _stopBits = stopBits;
    _parity   = parity;
    if (baudDiff)   emit baudRateChanged(baudRate);
    if (dataDiff)   emit dataBitsChanged(dataBits);
    if (stopDiff)   emit stopBitsChanged(stopBits);
    if (parityDiff) emit parityChanged(parity);
    return true;
}

bool QGCAndroidSerialPort::setFlowControl(QGCFlowControl flowControl)
{
    if (_flowControl == flowControl) return true;
    if (isOpen()) {
        if (!_engine) {
            _setError(QGCSerialPortError::NotOpen, tr("Failed to set flow control"));
            return false;
        }
        if (!_engine->setFlowControl(flowControlToWire(flowControl))) {
            _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set flow control"));
            return false;
        }
    }
    _flowControl = flowControl;
    emit flowControlChanged(flowControl);
    return true;
}

// ============================================================================
// Control lines
// ============================================================================

bool QGCAndroidSerialPort::setDataTerminalReady(bool set)
{
    if (!isOpen() || !_engine) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    const bool prev = isDataTerminalReady();
    if (!_engine->setDataTerminalReady(set)) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set DTR"));
        return false;
    }
    if (prev != set) emit dataTerminalReadyChanged(set);
    return true;
}

bool QGCAndroidSerialPort::setRequestToSend(bool set)
{
    if (!isOpen() || !_engine) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    if (_flowControl == QGCFlowControl::HardwareRtsCts) {
        _setError(QGCSerialPortError::UnsupportedOperation,
                  tr("RTS cannot be set when hardware flow control is enabled"));
        return false;
    }
    const bool prev = isRequestToSend();
    if (!_engine->setRequestToSend(set)) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set RTS"));
        return false;
    }
    if (prev != set) emit requestToSendChanged(set);
    return true;
}

bool QGCAndroidSerialPort::isDataTerminalReady()
{
    return pinoutSignals() & QGCPinoutSignals::DTR;
}

bool QGCAndroidSerialPort::isRequestToSend()
{
    return pinoutSignals() & QGCPinoutSignals::RTS;
}

quint32 QGCAndroidSerialPort::pinoutSignals()
{
    if (!isOpen() || !_engine) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return QGCPinoutSignals::None;
    }
    return _engine->controlLinesMask();
}

bool QGCAndroidSerialPort::setBreakEnabled(bool set)
{
    if (!isOpen() || !_engine) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    if (!_engine->setBreak(set)) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Failed to set Break enabled"));
        return false;
    }
    if (_breakEnabled != set) {
        _breakEnabled = set;
        emit breakEnabledChanged(set);
    }
    return true;
}

bool QGCAndroidSerialPort::sendBreak(int duration)
{
    Q_UNUSED(duration);
    // Timed break is not supported over Android USB host API. setBreakEnabled(true/false)
    // is available for untimed break.
    return false;
}

bool QGCAndroidSerialPort::flush()
{
    if (!isOpen()) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    // Non-blocking, matching upstream Qt serial backends — drains via the same async path
    // used after writeData(). Callers needing to block must use waitForBytesWritten().
    _drainWriteBuffer();
    return _writeBuffer.isEmpty();
}

bool QGCAndroidSerialPort::clear(bool input, bool output)
{
    if (!isOpen() || !_engine) {
        _setError(QGCSerialPortError::NotOpen, tr("Device is not open"));
        return false;
    }
    if (input) {
        _readBuffer.clear();
        _readOffset = 0;
        _engine->clearReadStaging();
    }
    if (output) {
        _writeBuffer.clear();
    }
    if (!_engine->purgeBuffers(input, output)) {
        _setError(QGCSerialPortError::Unknown, tr("Failed to purge buffers"));
        return false;
    }
    return true;
}

// ============================================================================
// Error reporting
// ============================================================================

void QGCAndroidSerialPort::_setError(QGCSerialPortError errorCode, const QString& errorString)
{
    _error = errorCode;
    if (!errorString.isEmpty()) {
        setErrorString(errorString);
    }
    emit errorOccurred(errorCode);
}

void QGCAndroidSerialPort::clearError()
{
    _error = QGCSerialPortError::NoError;
    setErrorString({});
}
