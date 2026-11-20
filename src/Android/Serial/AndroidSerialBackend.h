#pragma once

#include <memory>

#include <QtCore/QIODevice>
#include <QtCore/QString>

#include "AndroidSerialPort.h"

class QDeadlineTimer;

/// A QSerialPort as its backend sees it: what the backend asks of the port and reports to it.
class AndroidSerialBackendHost
{
public:
    /// The port itself, for its settings, its error and its thread.
    QSerialPort& serialPort() const { return _port; }

    QString systemLocation() const { return _port._systemLocation; }

    /// Sets the port's error and tells the port's owner, who may close the port before this returns. An error
    /// without a text is described by the text of its kind.
    void setError(QSerialPort::SerialPortError error, const QString& text = QString()) { _port._setError(error, text); }

    /// Takes input from the backend, on any thread, and returns whether the port has room for more. If not, the
    /// backend stops reading the device, which then holds its input back, until the port calls its
    /// startReading().
    bool newDataArrived(const char* bytes, int length) { return _port._newDataArrived(bytes, length); }

    /// Returns whether the port has room for more input, on any thread.
    bool hasRoomForInput() { return _port._hasRoomForInput(); }

    /// Returns whether input has arrived that the port's owner has not been told of and can be told of now.
    bool inputArrived() { return _port._inputArrived(); }

    /// Blocks until newDataArrived(), writeCompleted() or interruptWait() has been called or the deadline has
    /// passed, unless what one of them brought is still waiting.
    void waitForArrivedInput(const QDeadlineTimer& deadline) { _port._waitForArrivedInput(deadline); }

    /// Takes the backend's report, on any thread, that it has written this many of the bytes it took, or with 0
    /// that it can take more.
    void writeCompleted(qint64 bytes) { _port._writeCompleted(bytes); }

    /// Blocks until writeCompleted() or interruptWait() has been called or the deadline has passed, unless the
    /// report of either is still waiting.
    void waitForWriteProgress(const QDeadlineTimer& deadline) { _port._waitForWriteProgress(deadline); }

    /// Ends the wait that waitForArrivedInput() or waitForWriteProgress() is in, or else the next one, on any
    /// thread. For a backend that has something else to tell the port's thread, which runs no event loop while
    /// it waits.
    void interruptWait() { _port._interruptWait(); }

private:
    friend class QSerialPort;

    explicit AndroidSerialBackendHost(QSerialPort& port)
        : _port(port)
    {}

    QSerialPort& _port;
};

/// The device behind a QSerialPort: a USB serial adapter or a device node. The port opens the one that its
/// name calls for and calls it on its own thread.
///
/// A function that returns false, or startWrite() that returns -1, has set the port's error, and did so last:
/// the port's owner may close the port in response before the function returns.
class AndroidSerialBackend
{
public:
    virtual ~AndroidSerialBackend() = default;

    /// Opens the device. The port applies its settings afterwards.
    virtual bool open(QIODevice::OpenMode mode) = 0;
    /// Closes the device, after which no more of its input reaches the port. Does nothing if it is closed.
    virtual void close() = 0;
    /// The file descriptor of the open device, or -1.
    virtual int descriptor() const = 0;

    /// Starts passing the device's input to the port's newDataArrived(), or resumes it after newDataArrived() has
    /// answered that the port has no room for more. On that answer the backend reads the device no further, so
    /// that the device or the system holds its input back instead of the port dropping it. Once the device is
    /// lost it fails without setting the error again.
    virtual bool startReading() = 0;
    /// Blocks until input may have reached the port, its writeCompleted() may have been called, or the deadline
    /// has passed.
    virtual bool waitForInput(const QDeadlineTimer& deadline) = 0;

    /// Takes up to maxSize bytes to write and returns how many, or -1, without waiting for the device. It
    /// reports the bytes to the port's writeCompleted() as they are written, perhaps before it returns, and calls
    /// that with 0 when it took fewer than it was offered and can take more. The port offers nothing more until
    /// all that was taken is reported.
    virtual qint64 startWrite(const char* data, qint64 maxSize) = 0;
    /// Blocks until the port's writeCompleted() may have been called, or the deadline has passed.
    virtual bool waitForWritable(const QDeadlineTimer& deadline) = 0;

    /// A backend that cannot tell why the device refused describes the failure by failureText.
    virtual bool setParameters(qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                               QSerialPort::Parity parity, const QString& failureText) = 0;
    virtual bool setFlowControl(QSerialPort::FlowControl flowControl) = 0;
    virtual bool setBreakEnabled(bool set) = 0;
    virtual bool clear(QSerialPort::Directions directions) = 0;

    virtual QSerialPort::PinoutSignals pinoutSignals() = 0;
    virtual bool setDataTerminalReady(bool set) = 0;
    virtual bool setRequestToSend(bool set) = 0;

protected:
    AndroidSerialBackend() = default;

private:
    // The port holds a pointer to its backend, and a backend may own the device's descriptor.
    Q_DISABLE_COPY_MOVE(AndroidSerialBackend)
};

/// A USB serial adapter, which Java drives (AndroidSerial).
std::unique_ptr<AndroidSerialBackend> createUsbSerialBackend(AndroidSerialBackendHost port);
/// A serial device node, such as /dev/ttyS1.
std::unique_ptr<AndroidSerialBackend> createPosixSerialBackend(AndroidSerialBackendHost port);
