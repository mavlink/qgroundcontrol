#include "AndroidSerialFake.h"

AndroidSerialFake& AndroidSerialFake::instance()
{
    static AndroidSerialFake fake;
    return fake;
}

void AndroidSerialFake::reset()
{
    instance() = AndroidSerialFake();
}

bool AndroidSerialFake::OpenPort::newDataArrived(const char* bytes, int length) const
{
    return AndroidSerial::deliverNewData(handle, bytes, length);
}

void AndroidSerialFake::OpenPort::bytesWritten(int count) const
{
    AndroidSerial::deliverBytesWritten(handle, count);
}

void AndroidSerialFake::OpenPort::resourceErrorArrived(AndroidSerial::ResourceErrorReason reason,
                                                       const QString& detail) const
{
    AndroidSerial::deliverResourceError(handle, reason, detail);
}

namespace AndroidSerial {

QList<QSerialPortInfo> availablePorts()
{
    return AndroidSerialFake::instance().ports;
}

bool hasPermission(const QString& portName)
{
    return AndroidSerialFake::instance().permittedPorts.contains(portName);
}

bool requestPermission(const QString& portName)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    fake.permissionRequests.append(portName);
    return fake.requestPermissionResult;
}

bool isPermissionDenied(const QString& portName)
{
    return AndroidSerialFake::instance().deniedPorts.contains(portName);
}

QString getLastErrorDetail()
{
    return AndroidSerialFake::instance().lastErrorDetail;
}

int open(const QString& portName, PortHandle handle)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.openCalls;
    fake.openedLocation = portName;
    if (fake.openResult != Result::Ok) {
        return static_cast<int>(fake.openResult);
    }
    fake.port = AndroidSerialFake::OpenPort{handle};
    return fake.fileDescriptor;
}

Result close(PortHandle handle)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.closeCalls;
    if (fake.port && (fake.port->handle == handle)) {
        fake.port.reset();
    }
    return fake.closeResult;
}

Result setParameters(PortHandle, qint32 baudRate, QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                     QSerialPort::Parity parity)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.setParametersCalls;
    fake.parameters = AndroidSerialFake::Parameters{baudRate, dataBits, stopBits, parity};
    return fake.setParametersResult;
}

Result setFlowControl(PortHandle, QSerialPort::FlowControl flowControl)
{
    AndroidSerialFake::instance().flowControl = flowControl;
    return Result::Ok;
}

Result purgeBuffers(PortHandle, QSerialPort::Directions directions)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    fake.purgedDirections = directions;
    if (fake.purgeBuffersHook) {
        fake.purgeBuffersHook();
    }
    return fake.purgeBuffersResult;
}

Result startReading(PortHandle)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.startReadingCalls;
    if (fake.startReadingHook) {
        fake.startReadingHook();
    }
    return fake.startReadingResult;
}

Result stopReading(PortHandle)
{
    return Result::Ok;
}

Result setDataTerminalReady(PortHandle, bool set)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    fake.dataTerminalReady = set;
    return fake.dtrResult;
}

Result setRequestToSend(PortHandle, bool set)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    fake.requestToSend = set;
    return fake.rtsResult;
}

Result setBreak(PortHandle, bool set)
{
    AndroidSerialFake::instance().breakEnabled = set;
    return Result::Ok;
}

QSerialPort::PinoutSignals getControlLines(PortHandle)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.getControlLinesCalls;
    return fake.controlLines;
}

int writeAsync(PortHandle handle, const char* data, int length)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.writeCalls;
    const int written = fake.writeStep(length);
    if (written < 0) {
        return written;
    }
    fake.acceptedOutput.append(data, length);
    if (written > 0) {
        deliverBytesWritten(handle, written);
    }
    return length;
}

}  // namespace AndroidSerial
