// The USB backend of QSerialPort: a serial adapter that Java drives, reached through AndroidSerial.

#include <algorithm>
#include <atomic>
#include <optional>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>

#include "AndroidSerialBackend.h"
#include "AndroidSerialJni.h"

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

/// Whether a port was not opened because QGC may not use its device. Each of these results tells what has
/// become of the permission.
bool isPermissionRefusal(AndroidSerial::Result openResult)
{
    switch (openResult) {
        case AndroidSerial::Result::PermissionDenied:
        case AndroidSerial::Result::PermissionRequested:
        case AndroidSerial::Result::PermissionPending:
        case AndroidSerial::Result::PermissionAwaitsAndroid:
        case AndroidSerial::Result::PermissionAwaitsFocus:
            return true;
        default:
            return false;
    }
}

struct OpenFailure
{
    QSerialPort::SerialPortError code;
    QString text;
};

OpenFailure openFailure(AndroidSerial::Result openResult)
{
    switch (openResult) {
        case AndroidSerial::Result::DeviceNotFound:
            return {QSerialPort::DeviceNotFoundError, QSerialPort::tr("No such device")};
        // The port's owner shows the text to the user, so each refusal for lack of permission has a whole text
        // of its own, and Java keeps no detail of it, which would follow the text untranslated.
        case AndroidSerial::Result::PermissionDenied:
            return {QSerialPort::PermissionError,
                    QSerialPort::tr("USB permission was denied. Replug the device or connect to it by hand to be "
                                    "asked again")};
        case AndroidSerial::Result::PermissionRequested:
            return {QSerialPort::PermissionError,
                    QSerialPort::tr("USB permission has not been granted. You have been asked to allow it")};
        case AndroidSerial::Result::PermissionPending:
            return {QSerialPort::PermissionError,
                    QSerialPort::tr("USB permission has not been granted. The request has not been answered yet")};
        case AndroidSerial::Result::PermissionAwaitsAndroid:
            return {QSerialPort::PermissionError,
                    QSerialPort::tr("USB permission has not been granted. The device was just attached, and "
                                    "Android asks first")};
        case AndroidSerial::Result::PermissionAwaitsFocus:
            return {QSerialPort::PermissionError,
                    //: %1 is the name of the app, such as QGroundControl
                    QSerialPort::tr("USB permission has not been granted. %1 asks once its window has the focus")
                        .arg(QCoreApplication::applicationName())};
        case AndroidSerial::Result::AlreadyOpen:
            // Desktop QSerialPort reports a device that another owner holds as PermissionError too.
            return {QSerialPort::PermissionError, QSerialPort::tr("Device is already open")};
        case AndroidSerial::Result::AmbiguousName:
            // Adapters of a kind that report the same serial number, or none. One of them alone has the name
            // to itself, and each is listed under the name that Android gave it.
            return {QSerialPort::UnknownError,
                    QSerialPort::tr("More than one attached USB device has this name. Unplug the others, or choose "
                                    "the port again")};
        case AndroidSerial::Result::Ok:
        case AndroidSerial::Result::Failed:
        case AndroidSerial::Result::IoError:
        case AndroidSerial::Result::Unsupported:
        case AndroidSerial::Result::NotOpen:
            break;
    }

    // Also a result that only a newer Java side lists.
    return {QSerialPort::UnknownError, QSerialPort::tr("Failed to open the USB device")};
}

class UsbSerialBackend final : public AndroidSerialBackend, private AndroidSerial::PortListener
{
public:
    explicit UsbSerialBackend(AndroidSerialBackendHost port)
        : _port(port)
    {}

    ~UsbSerialBackend() override { close(); }

    bool open(QIODevice::OpenMode mode) override;
    void close() override;

    int descriptor() const override { return _descriptor; }

    bool startReading() override;
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
    /// Sets the error of a port whose device Java no longer has open, as it is once the device is detached.
    void _setDeviceGoneError();

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

    AndroidSerialBackendHost _port;
    // Names the port to Java while the device is open.
    AndroidSerial::PortHandle _handle = AndroidSerial::kNoPort;
    int _descriptor = -1;
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

bool UsbSerialBackend::open(QIODevice::OpenMode)
{
    _handle = AndroidSerial::registerPort(this);

    const int opened = AndroidSerial::open(_port.systemLocation(), _handle);
    if (opened < 0) {
        const auto openResult = static_cast<AndroidSerial::Result>(opened);
        OpenFailure error = openFailure(openResult);
        error.text = withDetail(error.text, AndroidSerial::getLastErrorDetail());
        if (isPermissionRefusal(openResult)) {
            // Auto-connect opens the port again and again while its device waits for the user's permission. Java
            // warns of the first refusal.
            qCDebug(AndroidSerialPortLog) << "No permission to open" << _port.systemLocation() << ":" << error.text;
        } else {
            qCWarning(AndroidSerialPortLog) << "Error opening" << _port.systemLocation() << ":" << error.text;
        }
        AndroidSerial::unregisterPort(_handle);
        _handle = AndroidSerial::kNoPort;
        _port.setError(error.code, error.text);
        return false;
    }

    _descriptor = opened;
    return true;
}

void UsbSerialBackend::close()
{
    if (_handle == AndroidSerial::kNoPort) {
        return;
    }

    // Closing the device also stops its read thread.
    const AndroidSerial::Result closeResult = AndroidSerial::close(_handle);
    // Java reports nothing more to this backend once this returns.
    AndroidSerial::unregisterPort(_handle);
    _handle = AndroidSerial::kNoPort;
    _descriptor = -1;
    _readThreadStarted = false;
    _readingPaused.store(false);
    _deviceGone = false;
    _deviceParameters.reset();

    if (closeResult == AndroidSerial::Result::Ok) {
        return;
    }

    // A device whose loss the port has reported may fail to close: its owner, which closes the port over that
    // loss, hears of no other.
    if (_port.serialPort().error() == QSerialPort::ResourceError) {
        qCDebug(AndroidSerialPortLog) << "Device of" << _port.systemLocation() << "was gone before it was closed";
        return;
    }

    (void) _commandSucceeded(closeResult, QSerialPort::tr("Closing device failed"));
}

bool UsbSerialBackend::startReading()
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
        if (!_reportLoss() && (_port.serialPort().error() != QSerialPort::ResourceError)) {
            qCDebug(AndroidSerialPortLog) << "Device is gone, cannot read port" << _port.systemLocation();
            _setDeviceGoneError();
        }
        return false;
    }

    _readThreadStarted = (result == AndroidSerial::Result::Ok);
    return _commandSucceeded(result, QSerialPort::tr("Failed to start async read"));
}

bool UsbSerialBackend::waitForInput(const QDeadlineTimer& deadline)
{
    // Java's threads deliver the input and report what was written; there is no descriptor to poll. They also end
    // the wait when the device is lost.
    _port.waitForArrivedInput(deadline);
    return !_reportLoss();
}

qint64 UsbSerialBackend::startWrite(const char* data, qint64 maxSize)
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
        qCDebug(AndroidSerialPortLog) << "Device is gone, failed to write to port" << _port.systemLocation();
        if (!_reportLoss() && (_port.serialPort().error() != QSerialPort::ResourceError)) {
            _setDeviceGoneError();
        }
    } else {
        const QString detail = AndroidSerial::getLastErrorDetail();
        qCWarning(AndroidSerialPortLog).noquote()
            << withDetail(QStringLiteral("Failed to write to port ") + _port.systemLocation(), detail);
        _port.setError(QSerialPort::WriteError, withDetail(QSerialPort::tr("Failed to write to port"), detail));
    }

    return -1;
}

bool UsbSerialBackend::waitForWritable(const QDeadlineTimer& deadline)
{
    _port.waitForWriteProgress(deadline);
    return !_reportLoss();
}

bool UsbSerialBackend::setParameters(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                                     QSerialPort::Parity parity, const QString& failureText)
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

bool UsbSerialBackend::setFlowControl(QSerialPort::FlowControl flowControl)
{
    return _commandSucceeded(AndroidSerial::setFlowControl(_handle, flowControl),
                             QSerialPort::tr("Failed to set flow control"));
}

bool UsbSerialBackend::setBreakEnabled(bool set)
{
    return _commandSucceeded(AndroidSerial::setBreak(_handle, set), QSerialPort::tr("Failed to set break"));
}

bool UsbSerialBackend::clear(QSerialPort::Directions directions)
{
    return _commandSucceeded(AndroidSerial::purgeBuffers(_handle, directions),
                             QSerialPort::tr("Failed to purge buffers"));
}

QSerialPort::PinoutSignals UsbSerialBackend::pinoutSignals()
{
    return AndroidSerial::getControlLines(_handle);
}

bool UsbSerialBackend::setDataTerminalReady(bool set)
{
    return _commandSucceeded(AndroidSerial::setDataTerminalReady(_handle, set), QSerialPort::tr("Failed to set DTR"));
}

bool UsbSerialBackend::setRequestToSend(bool set)
{
    return _commandSucceeded(AndroidSerial::setRequestToSend(_handle, set), QSerialPort::tr("Failed to set RTS"));
}

bool UsbSerialBackend::newDataArrived(const char* bytes, int length)
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

void UsbSerialBackend::bytesWritten(int count)
{
    _port.writeCompleted(count);
}

void UsbSerialBackend::resourceErrorArrived(AndroidSerial::ResourceErrorReason reason, const QString& detail)
{
    {
        QMutexLocker locker(&_lossMutex);
        _loss = Loss{_handle, reason, detail};
    }
    // A blocking wait on the port's thread runs no event loop, so the wait is ended and reports the loss itself.
    _port.interruptWait();

    // Queued for the port's thread, in case it is not waiting. This one is Java's, and AndroidSerial holds its
    // registry lock while it reports, which the close that this error prompts has to take.
    QMetaObject::invokeMethod(&_port.serialPort(), [this]() { (void) _reportLoss(); }, Qt::QueuedConnection);
}

bool UsbSerialBackend::_reportLoss()
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
    qCWarning(AndroidSerialPortLog) << "Resource error on" << _port.systemLocation() << ":"
                                    << withDetail(text, loss->detail);
    // Java reports this when its read thread has ended, so the next read starts it again.
    _readThreadStarted = false;
    _port.setError(QSerialPort::ResourceError, text);
    return true;
}

void UsbSerialBackend::_setDeviceGoneError()
{
    _port.setError(QSerialPort::ResourceError, resourceErrorText(AndroidSerial::ResourceErrorReason::DeviceDetached));
}

bool UsbSerialBackend::_commandSucceeded(AndroidSerial::Result result, const QString& failureText)
{
    QSerialPort::SerialPortError errorCode = QSerialPort::UnknownError;
    switch (result) {
        case AndroidSerial::Result::Ok:
            return true;
        case AndroidSerial::Result::NotOpen:
            // Java closes its side of the port when the device is detached, and reports the loss on a thread of
            // its own. A command that gets ahead of that report tells the port of the loss in its place.
            qCDebug(AndroidSerialPortLog).noquote()
                << "Device is gone:" << failureText << "on" << _port.systemLocation();
            _deviceGone = true;
            if (!_reportLoss()) {
                _setDeviceGoneError();
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
        case AndroidSerial::Result::PermissionRequested:
        case AndroidSerial::Result::PermissionPending:
        case AndroidSerial::Result::PermissionAwaitsAndroid:
        case AndroidSerial::Result::PermissionAwaitsFocus:
        case AndroidSerial::Result::AmbiguousName:
            // Java's other failures say nothing a caller could act on differently.
            break;
    }

    const QString detail = AndroidSerial::getLastErrorDetail();
    qCWarning(AndroidSerialPortLog).noquote()
        << withDetail(failureText + QStringLiteral(" on ") + _port.systemLocation(), detail);
    _port.setError(errorCode, withDetail(failureText, detail));
    return false;
}

}  // namespace

std::unique_ptr<AndroidSerialBackend> createUsbSerialBackend(AndroidSerialBackendHost port)
{
    return std::make_unique<UsbSerialBackend>(port);
}
