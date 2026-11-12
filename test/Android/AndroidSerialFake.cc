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

QString getLastErrorDetail()
{
    return AndroidSerialFake::instance().lastErrorDetail;
}

Result open(const QString& portName, PortHandle handle)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.openCalls;
    fake.openedLocation = portName;
    if (fake.openResult == Result::Ok) {
        fake.port = AndroidSerialFake::OpenPort{handle};
    }
    return fake.openResult;
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

int getFileDescriptor(PortHandle)
{
    return AndroidSerialFake::instance().fileDescriptor;
}

Result setParameters(PortHandle, qint32, QSerialPort::DataBits, QSerialPort::StopBits, QSerialPort::Parity)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.setParametersCalls;
    return fake.setParametersResult;
}

Result setFlowControl(PortHandle, QSerialPort::FlowControl)
{
    return Result::Ok;
}

Result purgeBuffers(PortHandle, QSerialPort::Directions)
{
    if (const auto& hook = AndroidSerialFake::instance().purgeBuffersHook) {
        hook();
    }
    return Result::Ok;
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
    ++AndroidSerialFake::instance().stopReadingCalls;
    return Result::Ok;
}

Result setDataTerminalReady(PortHandle, bool)
{
    return AndroidSerialFake::instance().dtrResult;
}

Result setRequestToSend(PortHandle, bool)
{
    return AndroidSerialFake::instance().rtsResult;
}

Result setBreak(PortHandle, bool)
{
    return Result::Ok;
}

QSerialPort::PinoutSignals getControlLines(PortHandle)
{
    return {};
}

int writeAsync(PortHandle handle, const char*, int length)
{
    AndroidSerialFake& fake = AndroidSerialFake::instance();
    ++fake.writeCalls;
    const int written = fake.writeStep(length);
    if (written < 0) {
        return written;
    }
    if (written > 0) {
        deliverBytesWritten(handle, written);
    }
    return length;
}

}  // namespace AndroidSerial
