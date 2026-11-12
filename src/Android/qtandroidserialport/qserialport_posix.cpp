// The POSIX backend of QSerialPort: a serial device node, such as the built-in UART that a rooted device
// exposes as /dev/ttyS1.

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QPointer>
#include <QtCore/QSocketNotifier>

#include "qserialport_p.h"

QT_BEGIN_NAMESPACE

namespace {

speed_t baudRateToSpeed(qint32 baudRate)
{
    switch (baudRate) {
        case 50:
            return B50;
        case 75:
            return B75;
        case 110:
            return B110;
        case 134:
            return B134;
        case 150:
            return B150;
        case 200:
            return B200;
        case 300:
            return B300;
        case 600:
            return B600;
        case 1200:
            return B1200;
        case 1800:
            return B1800;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 500000:
            return B500000;
        case 576000:
            return B576000;
        case 921600:
            return B921600;
        case 1000000:
            return B1000000;
        case 1152000:
            return B1152000;
        case 1500000:
            return B1500000;
        case 2000000:
            return B2000000;
        case 2500000:
            return B2500000;
        case 3000000:
            return B3000000;
        case 3500000:
            return B3500000;
        case 4000000:
            return B4000000;
        default:
            return B0;
    }
}

// The kernel's interface for the baud rates that termios has no constant for. It is declared here, as in Qt's
// own serial port, because the header that declares it, <asm/termbits.h>, also defines struct termios again.
struct KernelTermios2
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

constexpr unsigned long kGetTermios2 = _IOR('T', 0x2A, KernelTermios2);
constexpr unsigned long kSetTermios2 = _IOW('T', 0x2B, KernelTermios2);
// BOTHER: the rate is the number in c_ispeed and c_ospeed.
constexpr tcflag_t kOtherBaudRate = 0010000;

int pollTimeoutMs(const QDeadlineTimer& deadline)
{
    return deadline.isForever() ? -1 : static_cast<int>(qMax<qint64>(0, deadline.remainingTime()));
}

class PosixSerialPortBackend final : public QSerialPortBackend
{
public:
    explicit PosixSerialPortBackend(QSerialPortPrivate& port)
        : _port(port)
    {}

    ~PosixSerialPortBackend() override { close(); }

    bool open(QIODevice::OpenMode mode) override;
    void close() override;
    bool startReading() override;
    bool stopReading() override;
    bool waitForInput(const QDeadlineTimer& deadline) override;
    qint64 startWrite(const char* data, qint64 maxSize) override;
    bool waitForWritable(const QDeadlineTimer& deadline) override;
    bool setParameters(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                       QSerialPort::Parity parity, const QString& failureText) override;
    bool setFlowControl(QSerialPort::FlowControl flowControl) override;
    bool setBreakEnabled(bool set) override;
    bool clear(QSerialPort::Directions directions) override;
    QSerialPort::PinoutSignals pinoutSignals() override;
    bool setDataTerminalReady(bool set) override;
    bool setRequestToSend(bool set) override;

private:
    /// Sets a baud rate that termios has no constant for.
    bool _setCustomBaudRate(qint32 baudRate);
    /// Sets the device's whole configuration, as termios takes it.
    bool _applySettings(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                        QSerialPort::Parity parity, QSerialPort::FlowControl flowControl);
    bool _setControlLine(int line, bool set, const char* lineName, const QString& failureText);
    /// Passes what the device has to read on to the port. Returns false if it reported an error instead.
    bool _readActivated();
    /// The device takes output again.
    void _writeActivated();
    void _readFailed(int readError);

    QSerialPortPrivate& _port;
    int _descriptor = -1;
    QPointer<QSocketNotifier> _readNotifier;
    // Whether the port has answered that it has no room for more input. The device is not read until the port
    // starts reading again, so the kernel holds its input meanwhile, as it does for desktop QSerialPort.
    bool _readPaused = false;
    // Enabled while the device takes no more output, to tell the port when it does again.
    QPointer<QSocketNotifier> _writeNotifier;
};

bool PosixSerialPortBackend::open(QIODevice::OpenMode mode)
{
    int flags = O_NOCTTY | O_NONBLOCK;
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        flags |= O_RDWR;
    } else if (mode & QIODevice::WriteOnly) {
        flags |= O_WRONLY;
    } else {
        flags |= O_RDONLY;
    }

    const int descriptor = ::open(_port.systemLocation.toLocal8Bit().constData(), flags);
    if (descriptor == -1) {
        const int openErrno = errno;
        qCWarning(AndroidSerialPortLog) << "Error opening" << _port.systemLocation << ":" << strerror(openErrno);
        const QSerialPort::SerialPortError openError = (openErrno == ENOENT)   ? QSerialPort::DeviceNotFoundError
                                                       : (openErrno == EACCES) ? QSerialPort::PermissionError
                                                                               : QSerialPort::OpenError;
        _port.setError(QSerialPortErrorInfo(openError, QString::fromLocal8Bit(strerror(openErrno))));
        return false;
    }

    if (::isatty(descriptor) == 0) {
        qCWarning(AndroidSerialPortLog) << _port.systemLocation << "is not a tty device";
        (void) ::close(descriptor);
        _port.setError(QSerialPortErrorInfo(QSerialPort::OpenError, QSerialPort::tr("Not a tty device")));
        return false;
    }

    // Request exclusive access. Failure is not fatal.
    if (::ioctl(descriptor, TIOCEXCL) == -1) {
        qCDebug(AndroidSerialPortLog) << "Failed to set exclusive access on" << _port.systemLocation << ":"
                                      << strerror(errno);
    }

    _descriptor = descriptor;
    _port.descriptor = descriptor;

    QSerialPort* const q = _port.q_func();
    if (mode & QIODevice::ReadOnly) {
        _readNotifier = new QSocketNotifier(descriptor, QSocketNotifier::Read, q);
        // The port starts reading once it has applied its settings.
        _readNotifier->setEnabled(false);
        QObject::connect(_readNotifier, &QSocketNotifier::activated, q, [this]() { (void) _readActivated(); });
    }
    if (mode & QIODevice::WriteOnly) {
        _writeNotifier = new QSocketNotifier(descriptor, QSocketNotifier::Write, q);
        _writeNotifier->setEnabled(false);
        QObject::connect(_writeNotifier, &QSocketNotifier::activated, q, [this]() { _writeActivated(); });
    }

    return true;
}

void PosixSerialPortBackend::close()
{
    delete _readNotifier.data();
    delete _writeNotifier.data();
    _readPaused = false;

    if (_descriptor != -1) {
        // The exclusive access that open() took outlives this descriptor while anyone else holds the device
        // open, and would refuse the next open.
        (void) ::ioctl(_descriptor, TIOCNXCL);
        if (::close(_descriptor) == -1) {
            qCWarning(AndroidSerialPortLog) << "Failed to close" << _port.systemLocation << ":" << strerror(errno);
        }
        _descriptor = -1;
    }
}

bool PosixSerialPortBackend::startReading()
{
    _readPaused = false;
    if (_readNotifier) {
        _readNotifier->setEnabled(true);
    }

    return true;
}

bool PosixSerialPortBackend::stopReading()
{
    if (_readNotifier) {
        _readNotifier->setEnabled(false);
    }

    return true;
}

bool PosixSerialPortBackend::waitForInput(const QDeadlineTimer& deadline)
{
    // While the device takes no more output, also wake when it does, so that the port can go on writing.
    const bool writeBlocked = _writeNotifier && _writeNotifier->isEnabled();
    // Input that the port has no room for stays with the device, and does not end the wait.
    const short events = static_cast<short>((_readPaused ? 0 : POLLIN) | (writeBlocked ? POLLOUT : 0));
    struct pollfd pfd = {_descriptor, events, 0};
    const int ready = ::poll(&pfd, 1, pollTimeoutMs(deadline));
    if (ready < 0) {
        if (errno == EINTR) {
            return true;
        }
        const int pollErrno = errno;
        qCWarning(AndroidSerialPortLog) << "Poll error on" << _port.systemLocation << ":" << strerror(pollErrno);
        _port.setError(QSerialPortErrorInfo(QSerialPort::ReadError, QString::fromLocal8Bit(strerror(pollErrno))));
        return false;
    }
    if (ready == 0) {
        return true;
    }
    if (pfd.revents & POLLOUT) {
        _writeActivated();
    }
    if (!(pfd.revents & ~POLLOUT)) {
        return true;
    }

    // Read on any other wakeup, an error or hangup too: a read that fails reports the real error.
    const bool hungUp = pfd.revents & (POLLERR | POLLHUP | POLLNVAL);
    const bool readSucceeded = _readActivated();
    if (_port.inputArrived()) {
        return true;
    }
    if (!readSucceeded) {
        return false;
    }
    if (hungUp) {
        // Nothing to read from a descriptor that hung up: report it rather than poll it until the deadline.
        _port.setError(QSerialPortErrorInfo(QSerialPort::ResourceError, QSerialPort::tr("Device disconnected")));
        return false;
    }

    return true;
}

bool PosixSerialPortBackend::_readActivated()
{
    char readBuf[4096];
    bool gotData = false;

    for (;;) {
        const ssize_t bytesRead = ::read(_descriptor, readBuf, sizeof(readBuf));
        if (bytesRead > 0) {
            gotData = true;
            if (!_port.newDataArrived(readBuf, static_cast<int>(bytesRead))) {
                _readPaused = true;
                if (_readNotifier) {
                    _readNotifier->setEnabled(false);
                }
                return true;
            }
            if (bytesRead < static_cast<ssize_t>(sizeof(readBuf))) {
                return true;
            }
            continue;
        }

        if ((bytesRead < 0) && (errno == EINTR)) {
            continue;
        }

        if (bytesRead < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                return true;
            }
            _readFailed(errno);
            return false;
        }

        if (gotData) {
            return true;
        }
        // The descriptor was reported readable, so having nothing to read means the device hung up.
        _readFailed(0);
        return false;
    }
}

void PosixSerialPortBackend::_readFailed(int readError)
{
    const bool deviceGone =
        (readError == 0) || (readError == EIO) || (readError == ENODEV) || (readError == ENXIO) || (readError == EBADF);
    if (!deviceGone) {
        qCWarning(AndroidSerialPortLog) << "Read error on" << _port.systemLocation << ":" << strerror(readError);
        _port.setError(QSerialPortErrorInfo(QSerialPort::ReadError, QString::fromLocal8Bit(strerror(readError))));
        return;
    }

    // Like desktop QSerialPort: a vanished device is a ResourceError, which tells the owner to close the port.
    // Its descriptor stays readable for good, so it must not keep waking the event loop.
    if (_readNotifier) {
        _readNotifier->setEnabled(false);
    }
    qCWarning(AndroidSerialPortLog) << "Device lost on" << _port.systemLocation << ":"
                                    << (readError == 0 ? "hung up" : strerror(readError));
    _port.setError(QSerialPortErrorInfo(QSerialPort::ResourceError, QSerialPort::tr("Device disconnected")));
}

qint64 PosixSerialPortBackend::startWrite(const char* data, qint64 maxSize)
{
    ssize_t written;
    do {
        written = ::write(_descriptor, data, static_cast<size_t>(maxSize));
    } while ((written < 0) && (errno == EINTR));

    if ((written < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        const int writeErrno = errno;
        qCWarning(AndroidSerialPortLog) << "Write error on" << _port.systemLocation << ":" << strerror(writeErrno);
        // Like the USB backend and desktop QSerialPort: a vanished device is a ResourceError.
        const bool deviceGone = (writeErrno == EIO) || (writeErrno == ENODEV) || (writeErrno == ENXIO);
        _port.setError(deviceGone
                           ? QSerialPortErrorInfo(QSerialPort::ResourceError, QSerialPort::tr("Device disconnected"))
                           : QSerialPortErrorInfo(QSerialPort::WriteError, QSerialPort::tr("Failed to write to port")));
        return -1;
    }

    const qint64 taken = (written > 0) ? written : 0;
    if ((taken < maxSize) && _writeNotifier) {
        // The device takes no more for now.
        _writeNotifier->setEnabled(true);
    }
    if (taken > 0) {
        _port.writeCompleted(taken);
    }

    return taken;
}

void PosixSerialPortBackend::_writeActivated()
{
    if (_writeNotifier) {
        _writeNotifier->setEnabled(false);
    }
    _port.writeCompleted(0);
}

bool PosixSerialPortBackend::waitForWritable(const QDeadlineTimer& deadline)
{
    struct pollfd pfd = {_descriptor, POLLOUT, 0};
    if (::poll(&pfd, 1, pollTimeoutMs(deadline)) > 0) {
        _writeActivated();
    }

    return true;
}

bool PosixSerialPortBackend::setParameters(qint32 baudRate, QSerialPort::DataBits dataBits,
                                           QSerialPort::StopBits stopBits, QSerialPort::Parity parity, const QString&)
{
    return _applySettings(baudRate, dataBits, stopBits, parity, _port.flowControl);
}

bool PosixSerialPortBackend::setFlowControl(QSerialPort::FlowControl flowControl)
{
    return _applySettings(_port.inputBaudRate, _port.dataBits, _port.stopBits, _port.parity, flowControl);
}

bool PosixSerialPortBackend::_applySettings(qint32 baudRate, QSerialPort::DataBits dataBits,
                                            QSerialPort::StopBits stopBits, QSerialPort::Parity parity,
                                            QSerialPort::FlowControl flowControl)
{
    struct termios tio;
    if (::tcgetattr(_descriptor, &tio) == -1) {
        const int getErrno = errno;
        qCWarning(AndroidSerialPortLog) << "tcgetattr failed on" << _port.systemLocation << ":" << strerror(getErrno);
        _port.setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(getErrno))));
        return false;
    }

    ::cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    // A closed port keeps any rate it is given. One without a constant goes to the kernel as a number, which
    // takes 0 for a hang-up and a negative one for a huge rate.
    if (baudRate <= 0) {
        qCWarning(AndroidSerialPortLog) << "Unsupported baud rate" << baudRate << "on" << _port.systemLocation;
        _port.setError(
            QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported baud rate")));
        return false;
    }

    // A rate without a constant is set afterwards, on its own.
    const speed_t speed = baudRateToSpeed(baudRate);
    if (speed != B0) {
        (void) ::cfsetispeed(&tio, speed);
        (void) ::cfsetospeed(&tio, speed);
    }

    tio.c_cflag &= ~CSIZE;
    switch (dataBits) {
        case QSerialPort::Data5:
            tio.c_cflag |= CS5;
            break;
        case QSerialPort::Data6:
            tio.c_cflag |= CS6;
            break;
        case QSerialPort::Data7:
            tio.c_cflag |= CS7;
            break;
        case QSerialPort::Data8:
        default:
            tio.c_cflag |= CS8;
            break;
    }

    // CMSPAR makes the parity bit constant: a mark with PARODD, otherwise a space.
    tio.c_cflag &= ~(PARENB | PARODD | CMSPAR);
    switch (parity) {
        case QSerialPort::NoParity:
            break;
        case QSerialPort::EvenParity:
            tio.c_cflag |= PARENB;
            break;
        case QSerialPort::OddParity:
            tio.c_cflag |= (PARENB | PARODD);
            break;
        case QSerialPort::SpaceParity:
            tio.c_cflag |= (PARENB | CMSPAR);
            break;
        case QSerialPort::MarkParity:
            tio.c_cflag |= (PARENB | CMSPAR | PARODD);
            break;
        default:
            qCWarning(AndroidSerialPortLog) << "Unsupported parity:" << parity;
            _port.setError(
                QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported parity")));
            return false;
    }

    switch (stopBits) {
        case QSerialPort::OneStop:
            tio.c_cflag &= ~CSTOPB;
            break;
        case QSerialPort::TwoStop:
            tio.c_cflag |= CSTOPB;
            break;
        default:
            qCWarning(AndroidSerialPortLog) << "Unsupported stop bits:" << stopBits;
            _port.setError(
                QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported stop bits")));
            return false;
    }

    switch (flowControl) {
        case QSerialPort::NoFlowControl:
            tio.c_cflag &= ~CRTSCTS;
            tio.c_iflag &= ~(IXON | IXOFF);
            break;
        case QSerialPort::HardwareControl:
            tio.c_cflag |= CRTSCTS;
            tio.c_iflag &= ~(IXON | IXOFF);
            break;
        case QSerialPort::SoftwareControl:
            tio.c_cflag &= ~CRTSCTS;
            tio.c_iflag |= (IXON | IXOFF);
            break;
        default:
            qCWarning(AndroidSerialPortLog) << "Unsupported flow control:" << flowControl;
            _port.setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError,
                                                QSerialPort::tr("Unsupported flow control")));
            return false;
    }

    if (::tcsetattr(_descriptor, TCSANOW, &tio) == -1) {
        const int setErrno = errno;
        qCWarning(AndroidSerialPortLog) << "tcsetattr failed on" << _port.systemLocation << ":" << strerror(setErrno);
        _port.setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(setErrno))));
        return false;
    }

    return (speed != B0) || _setCustomBaudRate(baudRate);
}

bool PosixSerialPortBackend::_setCustomBaudRate(qint32 baudRate)
{
    KernelTermios2 tio;
    if (::ioctl(_descriptor, kGetTermios2, &tio) != -1) {
        tio.c_cflag &= ~CBAUD;
        tio.c_cflag |= kOtherBaudRate;
        tio.c_ispeed = static_cast<speed_t>(baudRate);
        tio.c_ospeed = static_cast<speed_t>(baudRate);
        if (::ioctl(_descriptor, kSetTermios2, &tio) != -1) {
            return true;
        }
    }

    const int setErrno = errno;
    qCWarning(AndroidSerialPortLog) << "Unsupported baud rate" << baudRate << "on" << _port.systemLocation << ":"
                                    << strerror(setErrno);
    _port.setError(
        QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported baud rate")));
    return false;
}

bool PosixSerialPortBackend::clear(QSerialPort::Directions directions)
{
    int queues = 0;
    if (directions == QSerialPort::AllDirections) {
        queues = TCIOFLUSH;
    } else if (directions & QSerialPort::Input) {
        queues = TCIFLUSH;
    } else if (directions & QSerialPort::Output) {
        queues = TCOFLUSH;
    } else {
        return true;
    }

    if (::tcflush(_descriptor, queues) == -1) {
        const int flushErrno = errno;
        qCWarning(AndroidSerialPortLog) << "tcflush failed on" << _port.systemLocation << ":" << strerror(flushErrno);
        _port.setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(flushErrno))));
        return false;
    }

    return true;
}

QSerialPort::PinoutSignals PosixSerialPortBackend::pinoutSignals()
{
    int arg = 0;
    if (::ioctl(_descriptor, TIOCMGET, &arg) == -1) {
        const int getErrno = errno;
        qCWarning(AndroidSerialPortLog) << "TIOCMGET failed on" << _port.systemLocation << ":" << strerror(getErrno);
        _port.setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(getErrno))));
        return QSerialPort::NoSignal;
    }

    QSerialPort::PinoutSignals signals_ = QSerialPort::NoSignal;
    if (arg & TIOCM_DTR) {
        signals_ |= QSerialPort::DataTerminalReadySignal;
    }
    if (arg & TIOCM_RTS) {
        signals_ |= QSerialPort::RequestToSendSignal;
    }
    if (arg & TIOCM_CTS) {
        signals_ |= QSerialPort::ClearToSendSignal;
    }
    if (arg & TIOCM_DSR) {
        signals_ |= QSerialPort::DataSetReadySignal;
    }
    if (arg & TIOCM_CAR) {
        signals_ |= QSerialPort::DataCarrierDetectSignal;
    }
    if (arg & TIOCM_RNG) {
        signals_ |= QSerialPort::RingIndicatorSignal;
    }
    if (arg & TIOCM_ST) {
        signals_ |= QSerialPort::SecondaryTransmittedDataSignal;
    }
    if (arg & TIOCM_SR) {
        signals_ |= QSerialPort::SecondaryReceivedDataSignal;
    }

    return signals_;
}

bool PosixSerialPortBackend::_setControlLine(int line, bool set, const char* lineName, const QString& failureText)
{
    if (::ioctl(_descriptor, set ? TIOCMBIS : TIOCMBIC, &line) != -1) {
        return true;
    }

    const int lineErrno = errno;
    const auto code = (lineErrno == ENOTTY) || (lineErrno == EOPNOTSUPP) ? QSerialPort::UnsupportedOperationError
                                                                         : QSerialPort::UnknownError;
    qCDebug(AndroidSerialPortLog) << "Failed to set" << lineName << "on" << _port.systemLocation << ":"
                                  << strerror(lineErrno);
    _port.setError(QSerialPortErrorInfo(code, failureText));
    return false;
}

bool PosixSerialPortBackend::setDataTerminalReady(bool set)
{
    return _setControlLine(TIOCM_DTR, set, "DTR", QSerialPort::tr("Failed to set DTR"));
}

bool PosixSerialPortBackend::setRequestToSend(bool set)
{
    return _setControlLine(TIOCM_RTS, set, "RTS", QSerialPort::tr("Failed to set RTS"));
}

bool PosixSerialPortBackend::setBreakEnabled(bool set)
{
    if (::ioctl(_descriptor, set ? TIOCSBRK : TIOCCBRK) == -1) {
        qCWarning(AndroidSerialPortLog) << "Failed to set break on" << _port.systemLocation << ":" << strerror(errno);
        _port.setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set Break Enabled")));
        return false;
    }

    return true;
}

}  // namespace

std::unique_ptr<QSerialPortBackend> createPosixSerialPortBackend(QSerialPortPrivate& port)
{
    return std::make_unique<PosixSerialPortBackend>(port);
}

QT_END_NAMESPACE
