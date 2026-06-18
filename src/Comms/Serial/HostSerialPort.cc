#include "HostSerialPort.h"

#ifndef Q_OS_ANDROID
#include <QtCore/QTimerEvent>

#include "QGCSerialPortInfo.h"
#endif

namespace {

#ifndef Q_OS_ANDROID
// Slow poll — this is only a backstop for drivers that swallow the unplug hangup, so latency is fine.
constexpr int kPresencePollMs = 2000;
// Require consecutive misses — a single enumeration can transiently drop a live port during re-enumeration.
constexpr int kPresenceMissesForMissing = 2;
#endif

QGCSerialPortError mapHostError(QSerialPort::SerialPortError e)
{
    switch (e) {
        case QSerialPort::NoError:
            return QGCSerialPortError::NoError;
        case QSerialPort::DeviceNotFoundError:
        case QSerialPort::OpenError:
            return QGCSerialPortError::OpenFailed;
        case QSerialPort::PermissionError:
            return QGCSerialPortError::PermissionDenied;
        case QSerialPort::ResourceError:
            return QGCSerialPortError::ResourceUnavailable;
        case QSerialPort::ReadError:
            return QGCSerialPortError::Read;
        case QSerialPort::WriteError:
            return QGCSerialPortError::Write;
        case QSerialPort::UnsupportedOperationError:
            return QGCSerialPortError::UnsupportedOperation;
        case QSerialPort::NotOpenError:
            return QGCSerialPortError::NotOpen;
        case QSerialPort::TimeoutError:
            return QGCSerialPortError::Timeout;
        default:
            return QGCSerialPortError::Unknown;
    }
}

// QSerialPort::Parity has a gap at 1 (0,2,3,4,5); QGCParity is contiguous.
constexpr QSerialPort::Parity kQGCParityToHost[] = {
    QSerialPort::NoParity,   QSerialPort::OddParity,   QSerialPort::EvenParity,
    QSerialPort::MarkParity, QSerialPort::SpaceParity,
};

// Guard the numeric QGC↔QSerialPort coupling so reordering either enum is a compile error.
// (DataBits/StopBits are now aliases of QSerialPort's own enums — no remap needed.)
static_assert(static_cast<int>(QGCFlowControl::None) == QSerialPort::NoFlowControl);
static_assert(static_cast<int>(QGCFlowControl::HardwareRtsCts) == QSerialPort::HardwareControl);
static_assert(static_cast<int>(QGCFlowControl::SoftwareXonXoff) == QSerialPort::SoftwareControl);

QSerialPort::FlowControl qgcFlowControlToHost(QGCFlowControl fc)
{
    switch (fc) {
        case QGCFlowControl::None:
            return QSerialPort::NoFlowControl;
        case QGCFlowControl::HardwareRtsCts:
            return QSerialPort::HardwareControl;
        case QGCFlowControl::SoftwareXonXoff:
            return QSerialPort::SoftwareControl;
        // QSerialPort has no DTR/DSR or inline XON/XOFF mode; degrade to no flow control on host.
        case QGCFlowControl::DtrDsr:
        case QGCFlowControl::XonXoffInline:
            return QSerialPort::NoFlowControl;
    }
    return QSerialPort::NoFlowControl;
}

}  // namespace

HostSerialPort::HostSerialPort(const QString& portName, QObject* parent) : QGCSerialPort(parent), _port(this)
{
    _port.setPortName(portName);
    // We reconfigure on every open, so skip QSerialPort's restore-on-close termios round-trip.
    _port.setSettingsRestoredOnClose(false);
    connect(&_port, &QSerialPort::errorOccurred, this,
            [this](QSerialPort::SerialPortError e) { _setError(mapHostError(e), _port.errorString()); });
    connect(&_port, &QIODevice::readyRead, this, &QIODevice::readyRead);
    connect(&_port, &QIODevice::bytesWritten, this, &QIODevice::bytesWritten);
    // Mirror an explicit _port->close() (e.g. error path) into this QIODevice's state — QSerialPort never self-closes on ResourceError, it only emits errorOccurred.
    connect(&_port, &QIODevice::aboutToClose, this, [this]() {
        if (QIODevice::isOpen()) {
            QIODevice::close();
        }
    });
}

QIODevice* HostSerialPort::openHostNmeaSource(const QString& name, qint32 baud, QObject* parent)
{
    auto* port = new HostSerialPort(name, parent);
    SerialPortConfig cfg{};
    cfg.baud = baud;
    if (!port->openConfigured(QIODevice::ReadOnly, cfg)) {
        delete port;
        return nullptr;
    }
    return port;
}

bool HostSerialPort::open(QIODevice::OpenMode mode)
{
    _error = QGCSerialPortError::NoError;
    if (!_port.open(mode)) {
        _setErrorIfPortSilent(QGCSerialPortError::OpenFailed, tr("Failed to open serial port"));
        return false;
    }
    _port.setReadBufferSize(kSerialRxBufferCapBytes);
    QIODevice::open(mode);
#ifndef Q_OS_ANDROID
    _presenceMissCount = 0;
    _presenceTimer.start(kPresencePollMs, this);
#endif
    return true;
}

void HostSerialPort::close()
{
#ifndef Q_OS_ANDROID
    _presenceTimer.stop();
#endif
    _port.close();
    if (QIODevice::isOpen()) {
        QIODevice::close();
    }
}

#ifndef Q_OS_ANDROID
void HostSerialPort::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == _presenceTimer.timerId()) {
        _checkPresence();
        return;
    }
    QGCSerialPort::timerEvent(event);
}

void HostSerialPort::_checkPresence()
{
    const QString name = _port.portName();
    for (const QGCSerialPortInfo& info : QGCSerialPortInfo::availablePorts()) {
        if (info.portName() == name) {
            _presenceMissCount = 0;
            return;
        }
    }
    if (++_presenceMissCount >= kPresenceMissesForMissing) {
        _presenceTimer.stop();
        _setError(QGCSerialPortError::ResourceUnavailable, tr("Serial port is no longer available"));
    }
}
#endif

bool HostSerialPort::reconfigure(const SerialPortConfig& cfg)
{
    if (!cfg.isValid()) {
        _setError(QGCSerialPortError::UnsupportedOperation, tr("Invalid serial configuration"));
        return false;
    }
    // QSerialPort has no batched setter — five separate termios/SetCommState calls.
    const bool ok1 = _port.setBaudRate(cfg.baud);
    const bool ok2 = _port.setDataBits(cfg.dataBits);
    const bool ok3 = _port.setStopBits(cfg.stopBits);
    const bool ok4 = _port.setParity(kQGCParityToHost[static_cast<uint8_t>(cfg.parity)]);
    const bool ok5 = _port.setFlowControl(qgcFlowControlToHost(cfg.flowControl));
    if (ok1 && ok2 && ok3 && ok4 && ok5) {
        return true;
    }

    _setErrorIfPortSilent(QGCSerialPortError::UnsupportedOperation, tr("Failed to apply serial configuration"));
    return false;
}

void HostSerialPort::clearError()
{
    _error = QGCSerialPortError::NoError;
    _port.clearError();
    setErrorString(tr("No error"));
}

QGCSerialPortError HostSerialPort::error() const
{
    if (_error != QGCSerialPortError::NoError) {
        return _error;
    }
    return mapHostError(_port.error());
}

void HostSerialPort::_setError(QGCSerialPortError error, const QString& errorString)
{
    _error = error;
    if (!errorString.isEmpty()) {
        setErrorString(errorString);
    }
    emit errorOccurred(error);
}

void HostSerialPort::_setErrorIfPortSilent(QGCSerialPortError error, const QString& errorString)
{
    if (_port.error() == QSerialPort::NoError) {
        _setError(error, errorString);
    }
}
