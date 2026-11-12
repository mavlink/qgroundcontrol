#pragma once

#include <memory>

#include <QtCore/QIODevice>
#include <QtCore/QString>

#include "qserialport.h"

QT_BEGIN_NAMESPACE

class QDeadlineTimer;
class QSerialPortPrivate;

/// The device behind a QSerialPort: a USB serial adapter or a device node. The port opens the one that its
/// name calls for and calls it on its own thread.
///
/// A function that returns false, or startWrite() that returns -1, has set the port's error, and did so last:
/// the port's owner may close the port in response before the function returns.
class QSerialPortBackend
{
public:
    virtual ~QSerialPortBackend() = default;

    /// Opens the device and sets the port's descriptor. The port applies its settings afterwards.
    virtual bool open(QIODevice::OpenMode mode) = 0;
    /// Closes the device, after which no more of its input reaches the port. Does nothing if it is closed.
    virtual void close() = 0;

    /// Starts passing the device's input to the port's newDataArrived(), or resumes it after stopReading() or
    /// after newDataArrived() has answered that the port has no room for more. On that answer the backend reads
    /// the device no further, so that the device or the system holds its input back instead of the port dropping
    /// it. Once the device is lost it fails without setting the error again.
    virtual bool startReading() = 0;
    virtual bool stopReading() = 0;
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
};

/// A USB serial adapter, which Java drives (AndroidSerial).
std::unique_ptr<QSerialPortBackend> createUsbSerialPortBackend(QSerialPortPrivate& port);
/// A serial device node, such as /dev/ttyS1.
std::unique_ptr<QSerialPortBackend> createPosixSerialPortBackend(QSerialPortPrivate& port);

QT_END_NAMESPACE
