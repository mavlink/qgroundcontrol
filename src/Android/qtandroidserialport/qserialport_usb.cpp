// The USB backend of QSerialPort: a serial adapter that Java drives, reached through AndroidSerial.

#include <algorithm>
#include <atomic>
#include <optional>
#include <utility>

#include <QtCore/QMetaObject>
#include <QtCore/QMutex>

#include "AndroidSerial.h"
#include "qserialport_p.h"

QT_BEGIN_NAMESPACE

namespace {

/// The text, followed by what Java's failure says of itself, if anything.
QString withDetail(const QString& text, const QString& detail)
{
    return detail.isEmpty() ? text : text + QStringLiteral(": ") + detail;
}

QString resourceErrorText(AndroidSerial::ResourceErrorReason reason)
{
    switch (reason) {
        case AndroidSerial::ResourceErrorReason::DeviceDetached:
            return QSerialPort::tr("Device disconnected");
        case AndroidSerial::ResourceErrorReason::ReadFailed:
            return QSerialPort::tr("Reading from the device failed");
        case AndroidSerial::ResourceErrorReason::WriteFailed:
            return QSerialPort::tr("Writing to the device failed");
    }

    // A reason that only a newer Java side lists.
    return QSerialPort::tr("Device stopped transferring data");
}

QSerialPortErrorInfo openErrorInfo(AndroidSerial::Result openResult)
{
    switch (openResult) {
        case AndroidSerial::Result::DeviceNotFound:
            return QSerialPortErrorInfo(QSerialPort::DeviceNotFoundError, QSerialPort::tr("No such device"));
        case AndroidSerial::Result::PermissionDenied:
            return QSerialPortErrorInfo(QSerialPort::PermissionError,
                                        QSerialPort::tr("USB permission has not been granted"));
        case AndroidSerial::Result::AlreadyOpen:
            // Desktop QSerialPort reports a device that another owner holds as PermissionError too.
            return QSerialPortErrorInfo(QSerialPort::PermissionError, QSerialPort::tr("Device is already open"));
        case AndroidSerial::Result::Ok:
        case AndroidSerial::Result::Failed:
        case AndroidSerial::Result::IoError:
        case AndroidSerial::Result::Unsupported:
        case AndroidSerial::Result::NotOpen:
            break;
    }

    // Also a result that only a newer Java side lists.
    return QSerialPortErrorInfo(QSerialPort::UnknownError, QSerialPort::tr("Failed to open the USB device"));
}

/// The error of a port whose device Java no longer has open, as it is once the device is detached.
QSerialPortErrorInfo deviceGoneError()
{
    return QSerialPortErrorInfo(QSerialPort::ResourceError,
                                resourceErrorText(AndroidSerial::ResourceErrorReason::DeviceDetached));
}

class UsbSerialPortBackend final : public QSerialPortBackend, private AndroidSerial::PortListener
{
public:
    explicit UsbSerialPortBackend(QSerialPortPrivate& port)
        : _port(port)
    {}

    ~UsbSerialPortBackend() override { close(); }

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
    bool newDataArrived(const char* bytes, int length) override;
    void bytesWritten(int count) override;
    void resourceErrorArrived(AndroidSerial::ResourceErrorReason reason, const QString& detail) override;

    /// Returns whether a command succeeded. If not, sets the error its result stands for: the loss of the device
    /// if Java no longer has the port open, otherwise one described by failureText and by what Java's failure
    /// says of itself. Call it right after the command.
    bool _commandSucceeded(AndroidSerial::Result result, const QString& failureText);
    /// Sets the port's error for the loss of the device that Java has reported, unless the port has been told
    /// of it or has closed since. Returns whether it did.
    bool _reportLoss();

    struct DeviceParameters
    {
        qint32 baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::StopBits stopBits;
        QSerialPort::Parity parity;

        bool operator==(const DeviceParameters&) const = default;
    };

    struct Loss
    {
        AndroidSerial::PortHandle handle;
        AndroidSerial::ResourceErrorReason reason;
        QString detail;
    };

    QSerialPortPrivate& _port;
    // Names the port to Java while the device is open.
    AndroidSerial::PortHandle _handle = AndroidSerial::kNoPort;
    // What the device was last set to since it was opened. Each QSerialPort setter passes all four parameters
    // down again, and setting the device costs USB transfers, so an unchanged set is not resent. Nothing after a
    // set that failed: a chip that takes the baud rate and the framing in separate transfers may have taken one.
    std::optional<DeviceParameters> _deviceParameters;
    // Whether Java's read thread was started and has not since been stopped or reported a resource error. Kept
    // here because every read that empties the port's buffer asks, and asking Java is a JNI call.
    bool _readThreadStarted = false;
    // Whether Java's read thread was told that the port has no room for more input, on which it waits to be
    // started again. That thread writes it.
    std::atomic<bool> _readingPaused{false};
    // Whether Java has answered that it no longer has the port open. It is not asked to read again, since it
    // logs every such call.
    bool _deviceGone = false;
    // The loss of the device that Java has reported, on its own thread, and of which the port's thread has not
    // told the port yet.
    QMutex _lossMutex;
    std::optional<Loss> _loss;
};

bool UsbSerialPortBackend::open(QIODevice::OpenMode)
{
    _handle = AndroidSerial::registerPort(this);

    const AndroidSerial::Result openResult = AndroidSerial::open(_port.systemLocation, _handle);
    if (openResult != AndroidSerial::Result::Ok) {
        QSerialPortErrorInfo error = openErrorInfo(openResult);
        error.errorString = withDetail(error.errorString, AndroidSerial::getLastErrorDetail());
        if (openResult == AndroidSerial::Result::PermissionDenied) {
            // Auto-connect opens the port again and again while its device waits for the user's permission. Java
            // warns of the first refusal.
            qCDebug(AndroidSerialPortLog)
                << "No permission to open" << _port.systemLocation << ":" << error.errorString;
        } else {
            qCWarning(AndroidSerialPortLog) << "Error opening" << _port.systemLocation << ":" << error.errorString;
        }
        AndroidSerial::unregisterPort(_handle);
        _handle = AndroidSerial::kNoPort;
        _port.setError(error);
        return false;
    }

    _port.descriptor = AndroidSerial::getFileDescriptor(_handle);
    if (_port.descriptor == -1) {
        qCWarning(AndroidSerialPortLog) << "Failed to get device handle for" << _port.systemLocation;
        close();
        _port.setError(QSerialPortErrorInfo(QSerialPort::OpenError));
        return false;
    }

    return true;
}

void UsbSerialPortBackend::close()
{
    if (_handle == AndroidSerial::kNoPort) {
        return;
    }

    // Closing the device also stops its read thread.
    const AndroidSerial::Result closeResult = AndroidSerial::close(_handle);
    // Java reports nothing more to this backend once this returns.
    AndroidSerial::unregisterPort(_handle);
    _handle = AndroidSerial::kNoPort;
    _readThreadStarted = false;
    _readingPaused.store(false);
    _deviceGone = false;
    _deviceParameters.reset();

    if (closeResult == AndroidSerial::Result::Ok) {
        return;
    }

    // A port that Java no longer has open is closed, which is all that was asked. And a device whose loss the
    // port has reported may fail to close: its owner, which closes the port over that loss, hears of no other.
    if ((closeResult == AndroidSerial::Result::NotOpen) || (_port.error == QSerialPort::ResourceError)) {
        qCDebug(AndroidSerialPortLog) << "Device of" << _port.systemLocation << "was gone before it was closed";
        return;
    }

    (void) _commandSucceeded(closeResult, QSerialPort::tr("Closing device failed"));
}

bool UsbSerialPortBackend::startReading()
{
    const bool paused = _readingPaused.exchange(false);
    if (_readThreadStarted && !paused) {
        return true;
    }

    if (_deviceGone) {
        return false;
    }

    const AndroidSerial::Result result = AndroidSerial::startReading(_handle);
    if (result == AndroidSerial::Result::NotOpen) {
        // Java closed its side of the port when the device was detached, and said so. If its report of the loss
        // has not reached the port yet, the port hears that. An owner that leaves the port open comes here on
        // every read(), so it hears of the loss again only if it cleared that error.
        _deviceGone = true;
        if (!_reportLoss() && (_port.error != QSerialPort::ResourceError)) {
            qCDebug(AndroidSerialPortLog) << "Device is gone, cannot read port" << _port.systemLocation;
            _port.setError(deviceGoneError());
        }
        return false;
    }

    _readThreadStarted = (result == AndroidSerial::Result::Ok);
    return _commandSucceeded(result, QSerialPort::tr("Failed to start async read"));
}

bool UsbSerialPortBackend::stopReading()
{
    if (!_readThreadStarted) {
        return true;
    }

    _readThreadStarted = false;
    return _commandSucceeded(AndroidSerial::stopReading(_handle), QSerialPort::tr("Failed to stop async read"));
}

bool UsbSerialPortBackend::waitForInput(const QDeadlineTimer& deadline)
{
    // Java's threads deliver the input and report what was written; there is no descriptor to poll. They also end
    // the wait when the device is lost.
    _port.waitForArrivedInput(deadline);
    return !_reportLoss();
}

qint64 UsbSerialPortBackend::startWrite(const char* data, qint64 maxSize)
{
    // Java reports a chunk only once all of it is written, so a large one would be a long time without
    // progress, and it is copied whole for the call.
    constexpr qint64 kMaxChunkBytes = 16 * 1024;

    const int length = static_cast<int>(std::min(maxSize, kMaxChunkBytes));
    const int result = AndroidSerial::writeAsync(_handle, data, length);
    if (result >= 0) {
        return std::min(result, length);
    }

    if (result == static_cast<int>(AndroidSerial::Result::IoError) ||
        result == static_cast<int>(AndroidSerial::Result::NotOpen)) {
        // Like desktop QSerialPort when the device disappears; the port's owner is expected to close it. The
        // port hears Java's own report of the loss if that has not reached it yet, and nothing more of a loss that
        // it has been told of: Java refuses output from then on.
        qCDebug(AndroidSerialPortLog) << "Device is gone, failed to write to port" << _port.systemLocation;
        if (!_reportLoss() && (_port.error != QSerialPort::ResourceError)) {
            _port.setError(deviceGoneError());
        }
    } else {
        const QString detail = AndroidSerial::getLastErrorDetail();
        qCWarning(AndroidSerialPortLog).noquote()
            << withDetail(QStringLiteral("Failed to write to port ") + _port.systemLocation, detail);
        _port.setError(QSerialPortErrorInfo(QSerialPort::WriteError,
                                            withDetail(QSerialPort::tr("Failed to write to port"), detail)));
    }

    return -1;
}

bool UsbSerialPortBackend::waitForWritable(const QDeadlineTimer& deadline)
{
    _port.waitForWriteProgress(deadline);
    return !_reportLoss();
}

bool UsbSerialPortBackend::setParameters(qint32 baudRate, QSerialPort::DataBits dataBits,
                                         QSerialPort::StopBits stopBits, QSerialPort::Parity parity,
                                         const QString& failureText)
{
    const DeviceParameters requested{baudRate, dataBits, stopBits, parity};
    if (_deviceParameters == requested) {
        return true;
    }

    const AndroidSerial::Result result = AndroidSerial::setParameters(_handle, baudRate, dataBits, stopBits, parity);
    if (result == AndroidSerial::Result::Ok) {
        _deviceParameters = requested;
    } else {
        _deviceParameters.reset();
    }

    return _commandSucceeded(result, failureText);
}

bool UsbSerialPortBackend::setFlowControl(QSerialPort::FlowControl flowControl)
{
    return _commandSucceeded(AndroidSerial::setFlowControl(_handle, flowControl),
                             QSerialPort::tr("Failed to set flow control"));
}

bool UsbSerialPortBackend::setBreakEnabled(bool set)
{
    return _commandSucceeded(AndroidSerial::setBreak(_handle, set), QSerialPort::tr("Failed to set break"));
}

bool UsbSerialPortBackend::clear(QSerialPort::Directions directions)
{
    return _commandSucceeded(AndroidSerial::purgeBuffers(_handle, directions),
                             QSerialPort::tr("Failed to purge buffers"));
}

QSerialPort::PinoutSignals UsbSerialPortBackend::pinoutSignals()
{
    return AndroidSerial::getControlLines(_handle);
}

bool UsbSerialPortBackend::setDataTerminalReady(bool set)
{
    return _commandSucceeded(AndroidSerial::setDataTerminalReady(_handle, set), QSerialPort::tr("Failed to set DTR"));
}

bool UsbSerialPortBackend::setRequestToSend(bool set)
{
    return _commandSucceeded(AndroidSerial::setRequestToSend(_handle, set), QSerialPort::tr("Failed to set RTS"));
}

bool UsbSerialPortBackend::newDataArrived(const char* bytes, int length)
{
    if (_port.newDataArrived(bytes, length)) {
        return true;
    }

    _readingPaused.store(true);
    // The port's thread may have made room, and asked to read on, before the note above was there to tell it
    // that Java pauses. Nothing would then end the pause, so Java pauses only if there is still no room.
    if (_port.hasRoomForInput()) {
        _readingPaused.store(false);
        return true;
    }

    return false;
}

void UsbSerialPortBackend::bytesWritten(int count)
{
    _port.writeCompleted(count);
}

void UsbSerialPortBackend::resourceErrorArrived(AndroidSerial::ResourceErrorReason reason, const QString& detail)
{
    {
        QMutexLocker locker(&_lossMutex);
        _loss = Loss{_handle, reason, detail};
    }
    // A blocking wait on the port's thread runs no event loop, so the wait is ended and reports the loss itself.
    _port.interruptWait();

    // Queued for the port's thread, in case it is not waiting. This one is Java's, and AndroidSerial holds its
    // registry lock while it reports, which the close that this error prompts has to take.
    QMetaObject::invokeMethod(_port.q_func(), [this]() { (void) _reportLoss(); }, Qt::QueuedConnection);
}

bool UsbSerialPortBackend::_reportLoss()
{
    std::optional<Loss> loss;
    {
        QMutexLocker locker(&_lossMutex);
        loss = std::exchange(_loss, std::nullopt);
    }
    if (!loss || (loss->handle != _handle)) {
        // Nothing to tell, or the port closed, and may have opened again, before the loss reached its thread.
        return false;
    }

    // The port's owner shows the text to the user, so Java's detail, which is not translated, goes to the log
    // only.
    const QString text = resourceErrorText(loss->reason);
    qCWarning(AndroidSerialPortLog) << "Resource error on" << _port.systemLocation << ":"
                                    << withDetail(text, loss->detail);
    // Java reports this when its read thread has ended, so the next read starts it again.
    _readThreadStarted = false;
    _port.setError(QSerialPortErrorInfo(QSerialPort::ResourceError, text));
    return true;
}

bool UsbSerialPortBackend::_commandSucceeded(AndroidSerial::Result result, const QString& failureText)
{
    QSerialPort::SerialPortError errorCode = QSerialPort::UnknownError;
    switch (result) {
        case AndroidSerial::Result::Ok:
            return true;
        case AndroidSerial::Result::NotOpen:
            // Java closes its side of the port when the device is detached, and reports the loss on a thread of
            // its own. A command that gets ahead of that report tells the port of the loss in its place.
            qCDebug(AndroidSerialPortLog).noquote() << "Device is gone:" << failureText << "on" << _port.systemLocation;
            _deviceGone = true;
            if (!_reportLoss()) {
                _port.setError(deviceGoneError());
            }
            return false;
        case AndroidSerial::Result::IoError:
            // The device did not carry the command out. If Java has reported its loss, that is why, and all
            // that the owner needs to hear. Otherwise the owner hears what failed: a link closes its port
            // without a word over a ResourceError, which would hide a device that only refuses this command.
            if (_reportLoss()) {
                return false;
            }
            break;
        case AndroidSerial::Result::Unsupported:
            errorCode = QSerialPort::UnsupportedOperationError;
            break;
        case AndroidSerial::Result::Failed:
        case AndroidSerial::Result::DeviceNotFound:
        case AndroidSerial::Result::PermissionDenied:
        case AndroidSerial::Result::AlreadyOpen:
            // Java's other failures say nothing a caller could act on differently.
            break;
    }

    const QString detail = AndroidSerial::getLastErrorDetail();
    qCWarning(AndroidSerialPortLog).noquote()
        << withDetail(failureText + QStringLiteral(" on ") + _port.systemLocation, detail);
    _port.setError(QSerialPortErrorInfo(errorCode, withDetail(failureText, detail)));
    return false;
}

}  // namespace

std::unique_ptr<QSerialPortBackend> createUsbSerialPortBackend(QSerialPortPrivate& port)
{
    return std::make_unique<UsbSerialPortBackend>(port);
}

QT_END_NAMESPACE
