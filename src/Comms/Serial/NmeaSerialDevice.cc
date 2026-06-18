#include "NmeaSerialDevice.h"

#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>
#include <QtCore/private/qiodevice_p.h>
#include <memory>

#include "QGCSerialPort.h"
#include "QGCSerialPortTypes.h"
#include "SerialPlatform.h"
#include "WorkerThread.h"

namespace {
// Bounds the GUI-thread block in close() if the worker is wedged in a JNI/USB call; past it we detach, never terminate().
constexpr int kReaderStopTimeoutMs = 2000;
}  // namespace

// Push-model device: the worker appends bytes straight into QIODevicePrivate's read buffer, so QIODevice's own read()/bytesAvailable()/canReadLine() work on primed data (QIOPipe pattern, matching AndroidSerialPort).
class NmeaSerialDevicePrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(NmeaSerialDevice)

public:
    NmeaSerialDevicePrivate(QString portName, qint32 baud) : _portName(std::move(portName)), _baud(baud) {}

    const QString _portName;
    const qint32 _baud;
    NmeaSerialReader* _reader = nullptr;  // owned by _workerThread
    std::unique_ptr<WorkerThread> _workerThread;
};

NmeaSerialReader::NmeaSerialReader(QString portName, qint32 baud, QObject* parent)
    : QObject(parent), _portName(std::move(portName)), _baud(baud)
{}

NmeaSerialReader::~NmeaSerialReader() = default;

void NmeaSerialReader::process()
{
    // Created here so the port's thread affinity is this worker, not the GUI thread (mirrors GPSProvider).
    _serial.reset(SerialPlatform::makeSerialPort(_portName, nullptr));
    SerialPortConfig cfg{};
    cfg.baud = _baud;
    if (!_serial || !_serial->openConfigured(QIODevice::ReadOnly, cfg)) {
        _serial.reset();
        emit finished();
        return;
    }

    (void) connect(_serial.get(), &QIODevice::readyRead, this, &NmeaSerialReader::_onReadyRead);
    // Port self-close (USB unplug, ResourceError) after a successful open: tell the device the source is gone.
    (void) connect(_serial.get(), &QIODevice::aboutToClose, this, &NmeaSerialReader::_onPortAboutToClose);

    // Drain anything that arrived between open() and the readyRead connect above.
    _onReadyRead();
}

void NmeaSerialReader::_onReadyRead()
{
    if (!_serial) {
        return;
    }
    const QByteArray chunk = _serial->readAll();
    if (!chunk.isEmpty()) {
        emit dataReceived(chunk);
    }
}

void NmeaSerialReader::_onPortAboutToClose()
{
    if (_finished) {
        return;
    }
    _finished = true;
    emit sourceLost();  // the device closes itself; owner-driven cleanup follows via stop()
    emit finished();
}

void NmeaSerialReader::stop()
{
    if (_finished) {
        return;  // already torn down via a port self-close, or a second explicit stop()
    }
    _finished = true;
    if (_serial) {
        // Drop our slots first so closing the port here doesn't re-enter as a sourceLost().
        disconnect(_serial.get(), nullptr, this, nullptr);
        _serial->close();
        _serial.reset();
    }
    emit finished();
}

NmeaSerialDevice::NmeaSerialDevice(QString portName, qint32 baud, QObject* parent)
    : QIODevice(*new NmeaSerialDevicePrivate(std::move(portName), baud), parent)
{}

NmeaSerialDevice::~NmeaSerialDevice()
{
    NmeaSerialDevice::close();
}

bool NmeaSerialDevice::open(OpenMode mode)
{
    Q_D(NmeaSerialDevice);
    if (isOpen()) {
        return false;
    }
    // Read-only NMEA source: require a read request and reject any write capability.
    if (!(mode & QIODeviceBase::ReadOnly) || (mode & QIODeviceBase::WriteOnly)) {
        return false;
    }

    d->_reader = new NmeaSerialReader(d->_portName, d->_baud, nullptr);
    d->_workerThread = std::make_unique<WorkerThread>(d->_reader, QStringLiteral("NmeaSerialReader"));

    (void) connect(d->_workerThread->thread(), &QThread::started, d->_reader, &NmeaSerialReader::process);
    // Cross-thread (worker emits, this lives on the GUI thread) -> auto-queued.
    (void) connect(d->_reader, &NmeaSerialReader::dataReceived, this, &NmeaSerialDevice::_appendFromReader);
    // A failed open leaves the device open-but-idle (no auto-close); only a post-open source loss closes it.
    // close() drops the ->this connections first so owner-initiated teardown doesn't re-enter.
    (void) connect(d->_reader, &NmeaSerialReader::sourceLost, this, &NmeaSerialDevice::close, Qt::QueuedConnection);
    (void) connect(d->_reader, &NmeaSerialReader::finished, d->_workerThread->thread(), &QThread::quit);
    d->_workerThread->start();

    return QIODevice::open(mode);
}

void NmeaSerialDevice::close()
{
    Q_D(NmeaSerialDevice);
    const bool wasOpen = isOpen();
    if (d->_workerThread) {
        if (d->_reader) {
            disconnect(d->_reader, nullptr, this, nullptr);
            // Close the port + emit finished on the worker's own thread; the readyRead-driven loop unwinds there.
            (void) QMetaObject::invokeMethod(d->_reader, "stop", Qt::QueuedConnection);
            // Wait for the worker-driven quit (finished()->quit(), see open()) so the port is closed on the worker
            // thread before stopAndWait()'s owner-thread quit() can race it shut.
            (void) d->_workerThread->thread()->wait(QDeadlineTimer(kReaderStopTimeoutMs));
        }
        // Already quiescent on the normal path; bounded detach only if the USB stack wedged the worker.
        (void) d->_workerThread->stopAndWait(0);
        d->_reader = nullptr;
        d->_workerThread.reset();
    }
    d->buffer.clear();
    if (wasOpen) {
        QIODevice::close();  // emits aboutToClose(); the consumer learns the source is gone
    }
}

void NmeaSerialDevice::_appendFromReader(const QByteArray& chunk)
{
    Q_D(NmeaSerialDevice);
    if (!isOpen()) {
        return;  // a chunk queued before close() drained the event loop
    }
    d->buffer.append(chunk);
    emit readyRead();
}

qint64 NmeaSerialDevice::readData(char* data, qint64 maxlen)
{
    // Async-fed: bytes are appended directly into d->buffer, which QIODevice drains before calling here.
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;  // 0 = "no data now, not EOF" for a sequential device
}

qint64 NmeaSerialDevice::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}
