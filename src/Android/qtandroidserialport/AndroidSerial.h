#pragma once

#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <qserialport.h>
#include <qserialportinfo.h>

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialLog)

/// The USB serial ports of the Android device, as QGCUsbSerialManager.java provides them. A function that
/// calls a Java method has the name of that method.
namespace AndroidSerial {
/// The flow control a USB serial driver may have, by the code that flowControlCode() in
/// QGCUsbSerialManager.java gives it. QSerialPort::FlowControl names only three of them.
enum class FlowControl
{
    None = 0,
    RtsCts,
    DtrDsr,
    XonXoff,
    XonXoffInline,
};

constexpr const char* kJniUsbSerialManagerClassName = "org/mavlink/qgroundcontrol/serial/QGCUsbSerialManager";

/// How an operation on a port went. QGCUsbSerialResult.java lists the same values. A function that returns a
/// count as an int returns one of the negative values instead when it fails.
enum class Result : int
{
    Ok = 0,
    Failed = -1,            ///< The call could not be made, or failed for a reason not listed here.
    IoError = -2,           ///< The device did not carry it out, and may be gone.
    Unsupported = -3,       ///< The port or its driver cannot do this.
    NotOpen = -4,           ///< No port is open under this handle.
    DeviceNotFound = -5,    ///< Opening: there is no such device.
    PermissionDenied = -6,  ///< Opening: the user has not let QGC use the device.
    AlreadyOpen = -7,       ///< Opening: the port is already open.
};

/// Why a port can no longer transfer data. QGCUsbSerialResourceError.java lists the same values.
enum class ResourceErrorReason : int
{
    DeviceDetached = 1,  ///< The device was unplugged.
    ReadFailed = 2,      ///< A read from the device failed.
    WriteFailed = 3,     ///< A write to the device failed.
};

/// Names an open port in every call to Java and in Java's callbacks. registerPort() makes one.
using PortHandle = qint64;
constexpr PortHandle kNoPort = 0;

/// What a registered port hears from Java. Java calls on a thread of its own, and AndroidSerial holds a lock
/// meanwhile that unregisterPort() takes, so neither function may wait for the port's thread or unregister.
class PortListener
{
public:
    /// Takes the port's input and returns whether the port has room for more. If not, Java's read thread stops
    /// reading the device, which holds its input back, until startReading() is called for the port.
    virtual bool newDataArrived(const char* bytes, int length) = 0;
    /// This many of the bytes handed to writeAsync() are done with: the device has taken them, or purgeBuffers()
    /// discarded them unsent.
    virtual void bytesWritten(int count) = 0;
    /// The port can no longer transfer data. The detail is what the failure itself says, such as the message of
    /// Java's exception, or nothing.
    virtual void resourceErrorArrived(ResourceErrorReason reason, const QString& detail) = 0;

protected:
    PortListener() = default;
    ~PortListener() = default;

private:
    // The registry holds a pointer to each listener.
    Q_DISABLE_COPY_MOVE(PortListener)
};

/// Tells when the ports QGC may open may have changed: a device was attached or detached, or the user granted
/// permission. An Android thread emits the signal, so connect a receiver object to it, whose thread then runs
/// the slot. The watcher itself belongs to the application's thread.
class DeviceWatcher : public QObject
{
    Q_OBJECT

public:
    static DeviceWatcher* instance();

signals:
    void devicesChanged();

private:
    DeviceWatcher();
};

/// Registers the callbacks that Java calls on its threads. Returns false, after a warning, if Java does not
/// declare them as they are registered here: a callback would then throw on its Java thread.
[[nodiscard]] bool setNativeMethods();
/// Proves, for QGC's boot test, that C++ and Java reach each other in the app as it is packaged: Java has every
/// method and field that C++ binds, passes arguments that C++ gave it to each callback, and answers calls about
/// a port that does not exist. Returns false after it has logged what failed, as critical messages of the
/// category Android.AndroidSerial.SelfTest.
[[nodiscard]] bool selfTest();

/// Returns a handle under which Java's callbacks reach the listener until unregisterPort() has returned.
PortHandle registerPort(PortListener* listener);
void unregisterPort(PortHandle handle);
/// Pass what Java reports for the port with this handle to its listener, or drop it if no port is registered
/// under the handle any more. Java's callbacks call these on Java's threads; a test calls them in Java's place.
/// deliverNewData() returns what the listener answers, or true if there is none.
bool deliverNewData(PortHandle handle, const char* bytes, int length);
void deliverBytesWritten(PortHandle handle, int count);
void deliverResourceError(PortHandle handle, ResourceErrorReason reason, const QString& detail);

/// The ports of the attached USB serial devices, also of those that still need USB permission. Android
/// withholds the serial number of such a device, so its ports have no stable name either.
QList<QSerialPortInfo> availablePorts();
/// The ports of attached devices that still need USB permission.
QList<QSerialPortInfo> portsAwaitingPermission();
/// Whether QGC may open the device of this port.
bool hasPermission(const QString& portName);
/// Asks the user to let QGC open the device of this port, unless it already may. The user answers later.
/// Unlike open(), this asks whenever it is called, also after the user has denied the permission. Returns false
/// if there is no such device.
bool requestPermission(const QString& portName);
/// Whether an attached device has a port with this name.
bool isPortNameValid(const QString& portName);
bool isPortOpen(const QString& portName);
/// The handle of the open port with this name, or kNoPort.
PortHandle getPortHandle(const QString& portName);

/// What the failure of this thread's last call to Java says of itself beyond its Result, such as the message of
/// Java's exception, or nothing. Ask only right after a call that returned a failure, on its thread: a later
/// call replaces the detail, and a call that fails before it reaches Java leaves that of the one before.
QString getLastErrorDetail();

/// Opens the port under a handle from registerPort(). For a device that still needs USB permission the result is
/// PermissionDenied, and getLastErrorDetail() tells what has become of the permission. Such an open also asks the
/// user for it, who answers later; DeviceWatcher tells of a grant. Callers open ports without being told to and
/// try again every few seconds, so the open asks once for each time the device is attached, not in the first
/// seconds after that, which Android's own prompt for the device takes to appear, and only while QGC's window has
/// the focus. It asks once more if Android closes the dialog without an answer. After a denial, the user is
/// asked again once the device has been replugged, or by requestPermission(). A stable name that no device with
/// permission has stands for the only attached device with its vendor and product IDs that still needs
/// permission, if there is exactly one.
Result open(const QString& portName, PortHandle handle);
Result close(PortHandle handle);
/// The file descriptor of the port's USB connection, or -1.
int getFileDescriptor(PortHandle handle);

/// Starts delivering the port's input to its listener, or has Java's read thread read on after the listener
/// answered that the port had no room for more.
Result startReading(PortHandle handle);
/// Stops delivering the port's input until startReading(); what arrives meanwhile is dropped.
Result stopReading(PortHandle handle);
bool isReading(PortHandle handle);

/// Reads up to length bytes from the device, waiting no longer than timeout. Usable only before startReading()
/// has first run on the port, which QSerialPort does as it opens a port for reading: from then on input arrives
/// through the port instead, and this returns nothing.
QByteArray read(PortHandle handle, int length, std::chrono::milliseconds timeout);
/// Returns length, or a Result: Failed if the write timed out, otherwise what writeWithProgress() reports.
int write(PortHandle handle, const char* data, int length, std::chrono::milliseconds timeout);
/// Returns the bytes written, fewer than length on timeout, or a Result: IoError or NotOpen if the port can no
/// longer transfer data, as when its device is unplugged. A zero timeout waits without limit.
int writeWithProgress(PortHandle handle, const char* data, int length, std::chrono::milliseconds timeout);
/// Hands the data to the port's write thread and returns length at once, or a Result: Failed if the thread still
/// has too much to write, IoError if the listener's resourceErrorArrived() has been called since startReading()
/// last ran, NotOpen if the port is not open. The listener's bytesWritten() follows for the whole length: once
/// the device has taken the data, however long that takes, or once purgeBuffers() has discarded what was not yet
/// sent. If the device is lost instead, its resourceErrorArrived() follows.
int writeAsync(PortHandle handle, const char* data, int length);

Result setParameters(PortHandle handle, qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                     QSerialPort::Parity parity);
/// None also if the port cannot be asked.
FlowControl getFlowControl(PortHandle handle);
Result setFlowControl(PortHandle handle, QSerialPort::FlowControl flowControl);
/// The flow control the port's driver has.
QList<FlowControl> getSupportedFlowControl(PortHandle handle);
/// Under XON/XOFF flow control, false while the device has paused transmission.
bool getXon(PortHandle handle);
/// Sets how long an FTDI chip holds back a partly filled packet, 1 to 255 ms. No other chip supports it.
Result setLatencyTimer(PortHandle handle, int milliseconds);
Result setBreak(PortHandle handle, bool set);
/// Discards what the device has buffered in the given directions. With Output, it also stops the write thread
/// from sending what is left of the data handed to writeAsync() so far, also where the driver has no purge
/// command. The listener's bytesWritten() still follows for all of that data.
Result purgeBuffers(PortHandle handle, QSerialPort::Directions directions);

Result setDataTerminalReady(PortHandle handle, bool set);
Result setRequestToSend(PortHandle handle, bool set);
/// The control lines that are active.
QSerialPort::PinoutSignals getControlLines(PortHandle handle);
/// Whether this one of the control lines is active.
bool isControlLineActive(PortHandle handle, QSerialPort::PinoutSignal line);
/// The control lines the port has at all. None also if the port cannot be asked.
QSerialPort::PinoutSignals getSupportedControlLines(PortHandle handle);
/// Whether the port has this one of the control lines.
bool isControlLineSupported(PortHandle handle, QSerialPort::PinoutSignal line);
}  // namespace AndroidSerial
