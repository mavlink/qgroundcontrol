#include "AndroidSerialPortInfo.h"

#include <iterator>

#include "AndroidSerialAccess.h"
#include "AndroidSerialJni.h"
#include "AndroidSerialPort.h"

namespace {

// A name that still opens a USB port after its device is replugged starts with this, as QGCUsbSerialPortName.java
// builds it.
constexpr QLatin1String kStableUsbNamePrefix("usb:");

constexpr qint32 kStandardBaudRates[] = {
    50,     75,     110,     134,     150,     200,     300,     600,     1200,    1800,
    2400,   4800,   9600,    19200,   38400,   57600,   115200,  230400,  460800,  500000,
    576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000,
};

}  // namespace

QSerialPortInfo::QSerialPortInfo(const QString& name)
{
    const QList<QSerialPortInfo> ports = availablePorts();
    for (const QSerialPortInfo& port : ports) {
        // A port that was opened by its stable name goes by that name.
        if ((name == port.portName()) || (!name.isEmpty() && (name == port.stableName()))) {
            *this = port;
            break;
        }
    }
}

QSerialPortInfo::QSerialPortInfo(const QSerialPort& port)
    : QSerialPortInfo(port.portName())
{}

QSerialPortInfo::QSerialPortInfo(const Data& data)
    : _data(data)
{}

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QList<qint32>(std::begin(kStandardBaudRates), std::end(kStandardBaudRates));
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    return availablePorts(QStringLiteral("/dev"), QStringLiteral("/sys/class/tty"));
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts(const QString& devDir, const QString& sysTtyDir)
{
    QList<QSerialPortInfo> ports = AndroidSerial::availablePorts();
    if (AndroidSerialAccess::usePosixSerial()) {
        ports += posixPorts(devDir, sysTtyDir, ports);
    }

    return ports;
}

QString QSerialPortInfo::portNameToSystemLocation(const QString& portName)
{
    // A stable name of a USB port is not the name of a device node, and Java opens the port by it as it is.
    return (portName.startsWith(QLatin1Char('/')) || portName.startsWith(QLatin1String("./")) ||
            portName.startsWith(QLatin1String("../")) || portName.startsWith(kStableUsbNamePrefix))
               ? portName
               : (QLatin1String("/dev/") + portName);
}

QString QSerialPortInfo::portNameFromSystemLocation(const QString& systemLocation)
{
    return systemLocation.startsWith(QLatin1String("/dev/")) ? systemLocation.mid(5) : systemLocation;
}

bool QSerialPortInfo::isUsbLocation(const QString& systemLocation)
{
    return systemLocation.startsWith(QLatin1String("/dev/bus/usb/")) || systemLocation.startsWith(kStableUsbNamePrefix);
}
