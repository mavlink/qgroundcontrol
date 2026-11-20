#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>

class QSerialPort;

/// A serial port of the Android device, described as Qt Serial Port's class of this name describes one elsewhere.
/// The ports are the USB serial ports that Java lists and, if the app says so (AndroidSerialAccess), the serial
/// device nodes.
class QSerialPortInfo
{
public:
    /// What is known of a port.
    struct Data
    {
        QString portName;
        QString stableName;
        QString systemLocation;
        QString description;
        QString manufacturer;
        QString serialNumber;
        quint16 vendorIdentifier = 0;
        quint16 productIdentifier = 0;
        bool hasVendorIdentifier = false;
        bool hasProductIdentifier = false;
        bool hasHardwareFlowControl = true;
    };

    /// A null port info, which describes no port.
    QSerialPortInfo() = default;
    /// The listed port of this port name or stable name. Null if there is none.
    explicit QSerialPortInfo(const QString& name);
    /// The listed port that this port was named after.
    explicit QSerialPortInfo(const QSerialPort& port);
    explicit QSerialPortInfo(const Data& data);

    QString portName() const { return _data ? _data->portName : QString(); }

    /// A name that still opens the port after its USB device is unplugged and plugged back in, which gives it
    /// another portName(). Pass it to QSerialPort::setPortName(). Empty if the port has no such name: until the
    /// user lets QGC open the device, and for a device node, whose portName() does not change. Qt Serial Port
    /// has no such name.
    QString stableName() const { return _data ? _data->stableName : QString(); }

    /// Whether the port can be set to hardware flow control. The driver of a USB device may have none, as that
    /// of a CDC-ACM device, and its port then refuses the setting. Qt Serial Port has no such answer.
    bool hasHardwareFlowControl() const { return !_data || _data->hasHardwareFlowControl; }

    QString systemLocation() const { return _data ? _data->systemLocation : QString(); }

    QString description() const { return _data ? _data->description : QString(); }

    QString manufacturer() const { return _data ? _data->manufacturer : QString(); }

    QString serialNumber() const { return _data ? _data->serialNumber : QString(); }

    quint16 vendorIdentifier() const { return _data ? _data->vendorIdentifier : 0; }

    quint16 productIdentifier() const { return _data ? _data->productIdentifier : 0; }

    bool hasVendorIdentifier() const { return _data && _data->hasVendorIdentifier; }

    bool hasProductIdentifier() const { return _data && _data->hasProductIdentifier; }

    bool isNull() const { return !_data; }

    static QList<qint32> standardBaudRates();
    static QList<QSerialPortInfo> availablePorts();

    /// The system location that a port name stands for: the path of a device node, or the name itself if it is
    /// a path or the stable name of a USB port.
    static QString portNameToSystemLocation(const QString& portName);
    static QString portNameFromSystemLocation(const QString& systemLocation);
    /// Whether a system location names a port of a USB device, which Java opens: the name that Android gives the
    /// device, or the one that survives a replug. Any other location is a device node.
    static bool isUsbLocation(const QString& systemLocation);
    /// The serial device nodes in devDir that QGC may read and write: the device's own UARTs and the USB serial
    /// adapters that the kernel drives. sysTtyDir is the kernel's /sys/class/tty, which names the ports that have
    /// a driver, tells a UART without hardware and describes the adapters. The node of an adapter that usbPorts
    /// has a port of is left out, so that no device is listed twice.
    static QList<QSerialPortInfo> posixPorts(const QString& devDir, const QString& sysTtyDir,
                                             const QList<QSerialPortInfo>& usbPorts = {});
    /// What availablePorts() lists on a device that has its serial device nodes in these directories.
    static QList<QSerialPortInfo> availablePorts(const QString& devDir, const QString& sysTtyDir);

private:
    std::optional<Data> _data;
};
