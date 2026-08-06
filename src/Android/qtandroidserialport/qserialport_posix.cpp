// POSIX serial backend for Android devices which expose accessible /dev/tty* nodes
// (e.g. rooted devices with built-in serial ports). Selected at startup via
// AndroidSerial::setUsePosixSerial().

#include <QtCore/QDeadlineTimer>
#include <QtCore/QSocketNotifier>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

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

}  // namespace

bool QSerialPortPrivate::_posixOpen(QIODevice::OpenMode mode)
{
    Q_Q(QSerialPort);

    qCDebug(AndroidSerialPortLog) << "Opening (POSIX)" << systemLocation;

    int flags = O_NOCTTY | O_NONBLOCK;
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        flags |= O_RDWR;
    } else if (mode & QIODevice::WriteOnly) {
        flags |= O_WRONLY;
    } else {
        flags |= O_RDONLY;
    }

    descriptor = ::open(systemLocation.toLocal8Bit().constData(), flags);
    if (descriptor == -1) {
        qCWarning(AndroidSerialPortLog) << "Error opening" << systemLocation << ":" << strerror(errno);
        const QSerialPort::SerialPortError error = (errno == ENOENT)   ? QSerialPort::DeviceNotFoundError
                                                   : (errno == EACCES) ? QSerialPort::PermissionError
                                                                       : QSerialPort::OpenError;
        setError(QSerialPortErrorInfo(error, QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    if (::isatty(descriptor) == 0) {
        qCWarning(AndroidSerialPortLog) << systemLocation << "is not a tty device";
        setError(QSerialPortErrorInfo(QSerialPort::OpenError, QSerialPort::tr("Not a tty device")));
        _posixClose();
        return false;
    }

    // Request exclusive access. Failure is not fatal.
    if (::ioctl(descriptor, TIOCEXCL) == -1) {
        qCDebug(AndroidSerialPortLog) << "Failed to set exclusive access on" << systemLocation << ":"
                                      << strerror(errno);
    }

    if (!_posixApplyPortSettings(inputBaudRate, dataBits, stopBits, parity, flowControl)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set serial port parameters for" << systemLocation;
        _posixClose();
        return false;
    }

    (void) _posixClear(QSerialPort::AllDirections);

    if (mode & QIODevice::ReadOnly) {
        _posixReadNotifier = new QSocketNotifier(descriptor, QSocketNotifier::Read, q);
        QObject::connect(_posixReadNotifier, &QSocketNotifier::activated, q, [this]() { _posixReadActivated(); });
    }

    return true;
}

void QSerialPortPrivate::_posixClose()
{
    qCDebug(AndroidSerialPortLog) << "Closing (POSIX)" << systemLocation;

    delete _posixReadNotifier;
    _posixReadNotifier = nullptr;

    if (descriptor != -1) {
        if (::close(descriptor) == -1) {
            qCWarning(AndroidSerialPortLog) << "Failed to close" << systemLocation << ":" << strerror(errno);
        }
        descriptor = -1;
    }
}

bool QSerialPortPrivate::_posixStartAsyncRead()
{
    if (_posixReadNotifier) {
        _posixReadNotifier->setEnabled(true);
    }

    // Deliver any data still pending in the intermediate buffer.
    _scheduleReadyRead();

    return true;
}

bool QSerialPortPrivate::_posixStopAsyncRead()
{
    if (_posixReadNotifier) {
        _posixReadNotifier->setEnabled(false);
    }

    return true;
}

void QSerialPortPrivate::_posixReadActivated()
{
    char readBuf[4096];

    for (;;) {
        const ssize_t bytesRead = ::read(descriptor, readBuf, sizeof(readBuf));
        if (bytesRead > 0) {
            newDataArrived(readBuf, static_cast<int>(bytesRead));
            if (bytesRead < static_cast<ssize_t>(sizeof(readBuf))) {
                break;
            }
            continue;
        }

        if ((bytesRead < 0) && (errno == EINTR)) {
            continue;
        }

        if ((bytesRead < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            qCWarning(AndroidSerialPortLog) << "Read error on" << systemLocation << ":" << strerror(errno);
            setError(QSerialPortErrorInfo(QSerialPort::ReadError, QString::fromLocal8Bit(strerror(errno))));
        }

        break;
    }
}

qint64 QSerialPortPrivate::_posixWrite(const char* data, qint64 maxSize, int timeoutMs)
{
    qint64 totalWritten = 0;
    QDeadlineTimer deadline(timeoutMs);

    while (totalWritten < maxSize) {
        const ssize_t written = ::write(descriptor, data + totalWritten, static_cast<size_t>(maxSize - totalWritten));
        if (written > 0) {
            totalWritten += written;
            continue;
        }

        if ((written < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR)) {
            qCWarning(AndroidSerialPortLog) << "Write error on" << systemLocation << ":" << strerror(errno);
            return -1;
        }

        if (deadline.hasExpired()) {
            qCWarning(AndroidSerialPortLog) << "Write timeout on" << systemLocation;
            return -1;
        }

        struct pollfd pfd = {descriptor, POLLOUT, 0};
        const int pollTimeout = deadline.isForever() ? -1 : static_cast<int>(qMax<qint64>(0, deadline.remainingTime()));
        (void) ::poll(&pfd, 1, pollTimeout);
    }

    return totalWritten;
}

bool QSerialPortPrivate::_posixApplyPortSettings(qint32 baudRate, QSerialPort::DataBits dataBits_,
                                                 QSerialPort::StopBits stopBits_, QSerialPort::Parity parity_,
                                                 QSerialPort::FlowControl flowControl_)
{
    struct termios tio;
    if (::tcgetattr(descriptor, &tio) == -1) {
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    ::cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    const speed_t speed = baudRateToSpeed(baudRate);
    if (speed == B0) {
        qCWarning(AndroidSerialPortLog) << "Unsupported baud rate:" << baudRate;
        setError(
            QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported baud rate")));
        return false;
    }
    (void) ::cfsetispeed(&tio, speed);
    (void) ::cfsetospeed(&tio, speed);

    tio.c_cflag &= ~CSIZE;
    switch (dataBits_) {
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

    switch (parity_) {
        case QSerialPort::NoParity:
            tio.c_cflag &= ~PARENB;
            break;
        case QSerialPort::EvenParity:
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
            break;
        case QSerialPort::OddParity:
            tio.c_cflag |= (PARENB | PARODD);
            break;
        default:
            qCWarning(AndroidSerialPortLog) << "Unsupported parity:" << parity_;
            setError(
                QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported parity")));
            return false;
    }

    switch (stopBits_) {
        case QSerialPort::OneStop:
            tio.c_cflag &= ~CSTOPB;
            break;
        case QSerialPort::TwoStop:
            tio.c_cflag |= CSTOPB;
            break;
        default:
            qCWarning(AndroidSerialPortLog) << "Unsupported stop bits:" << stopBits_;
            setError(
                QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Unsupported stop bits")));
            return false;
    }

    switch (flowControl_) {
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
            qCWarning(AndroidSerialPortLog) << "Unsupported flow control:" << flowControl_;
            setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError,
                                          QSerialPort::tr("Unsupported flow control")));
            return false;
    }

    if (::tcsetattr(descriptor, TCSANOW, &tio) == -1) {
        qCWarning(AndroidSerialPortLog) << "tcsetattr failed on" << systemLocation << ":" << strerror(errno);
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    return true;
}

bool QSerialPortPrivate::_posixClear(QSerialPort::Directions directions)
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

    if (::tcflush(descriptor, queues) == -1) {
        qCWarning(AndroidSerialPortLog) << "tcflush failed on" << systemLocation << ":" << strerror(errno);
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    return true;
}

QSerialPort::PinoutSignals QSerialPortPrivate::_posixPinoutSignals()
{
    int arg = 0;
    if (::ioctl(descriptor, TIOCMGET, &arg) == -1) {
        qCWarning(AndroidSerialPortLog) << "TIOCMGET failed on" << systemLocation << ":" << strerror(errno);
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QString::fromLocal8Bit(strerror(errno))));
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

static bool setControlLine(int descriptor, int line, bool set)
{
    return (::ioctl(descriptor, set ? TIOCMBIS : TIOCMBIC, &line) != -1);
}

bool QSerialPortPrivate::_posixSetDataTerminalReady(bool set)
{
    if (!setControlLine(descriptor, TIOCM_DTR, set)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set DTR on" << systemLocation << ":" << strerror(errno);
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set DTR")));
        return false;
    }

    return true;
}

bool QSerialPortPrivate::_posixSetRequestToSend(bool set)
{
    if (!setControlLine(descriptor, TIOCM_RTS, set)) {
        qCWarning(AndroidSerialPortLog) << "Failed to set RTS on" << systemLocation << ":" << strerror(errno);
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set RTS")));
        return false;
    }

    return true;
}

bool QSerialPortPrivate::_posixSetBreakEnabled(bool set)
{
    if (::ioctl(descriptor, set ? TIOCSBRK : TIOCCBRK) == -1) {
        qCWarning(AndroidSerialPortLog) << "Failed to set break on" << systemLocation << ":" << strerror(errno);
        setError(QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to set Break Enabled")));
        return false;
    }

    return true;
}

bool QSerialPortPrivate::_posixWaitForReadyRead(int msecs)
{
    {
        QMutexLocker locker(&_readMutex);
        if (!buffer.isEmpty()) {
            return true;
        }
        if (_pendingSizeLocked() > 0) {
            (void) _drainPendingDataLocked();
            return true;
        }
    }

    QDeadlineTimer deadline(msecs);
    for (;;) {
        struct pollfd pfd = {descriptor, POLLIN, 0};
        const int pollTimeout = deadline.isForever() ? -1 : static_cast<int>(qMax<qint64>(0, deadline.remainingTime()));
        const int ret = ::poll(&pfd, 1, pollTimeout);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            qCWarning(AndroidSerialPortLog) << "Poll error on" << systemLocation << ":" << strerror(errno);
            setError(QSerialPortErrorInfo(QSerialPort::ReadError, QString::fromLocal8Bit(strerror(errno))));
            return false;
        }

        if (ret > 0) {
            // Attempt the read on any wakeup (POLLIN/POLLERR/POLLHUP); a failed read
            // surfaces the real error via _posixReadActivated.
            _posixReadActivated();

            {
                QMutexLocker locker(&_readMutex);
                if (_pendingSizeLocked() > 0) {
                    (void) _drainPendingDataLocked();
                }
                if (!buffer.isEmpty()) {
                    return true;
                }
            }

            // A read error was reported with no data delivered: surface it instead
            // of looping until the deadline stamps TimeoutError over it.
            if (error != QSerialPort::NoError) {
                return false;
            }

            // No data delivered on an error/hangup wakeup: bail out rather than
            // busy-spinning on a dead descriptor until the deadline.
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (error == QSerialPort::NoError) {
                    setError(QSerialPortErrorInfo(QSerialPort::ResourceError, QSerialPort::tr("Device disconnected")));
                }
                return false;
            }
        }

        if (deadline.hasExpired()) {
            break;
        }
    }

    setError(QSerialPortErrorInfo(QSerialPort::TimeoutError, QSerialPort::tr("Timeout while waiting for ready read")));
    return false;
}

QT_END_NAMESPACE
