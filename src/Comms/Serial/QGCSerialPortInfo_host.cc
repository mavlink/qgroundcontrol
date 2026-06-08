#include "QGCSerialPortInfo.h"
#include "SerialPlatform.h"
#include "QGCSerialPort.h"
#include "HostSerialPort.h"

#include <QtCore/QRegularExpression>
#include <QtSerialPort/QSerialPortInfo>

namespace SerialPlatform {

QGCSerialPort *makeSerialPort(const QString &name, QObject *parent)
{
    if (const auto &factory = portFactoryOverride()) {
        return factory(name, parent);
    }
    return new HostSerialPort(name, parent);
}

QIODevice *makeNmeaSerialSource(const QString &systemLocation, qint32 baud, QObject *parent)
{
    return HostSerialPort::openHostNmeaSource(systemLocation, baud, parent);
}

}  // namespace SerialPlatform

QList<qint32> QGCSerialPortInfo::portSpecificBaudRates(const QString &)
{
    return {};
}

QList<QGCSerialPortInfo> QGCSerialPortInfo::_nativeDevices()
{
    return {};
}

bool QGCSerialPortInfo::_acceptQSerialPortInfo(const QSerialPortInfo &)
{
    return true;
}

QList<QGCSerialPortInfo::BoardRegExpFallback_t> QGCSerialPortInfo::_additionalDescriptionFallbacks()
{
    return {};
}

bool QGCSerialPortInfo::_platformIsSystemPort(const QGCSerialPortInfo &port)
{
#ifdef Q_OS_MACOS
    static const QList<QString> systemPortLocations = {
        QStringLiteral("tty.MALS"),
        QStringLiteral("tty.SOC"),
        QStringLiteral("tty.Bluetooth-Incoming-Port"),
        QStringLiteral("tty.usbserial"),
        QStringLiteral("tty.usbmodem")
    };
    for (const QString &systemPortLocation : systemPortLocations) {
        if (port.systemLocation().contains(systemPortLocation)) {
            return true;
        }
    }
#elif defined(Q_OS_LINUX)
    const QString &portName = port.portName();
    static const QRegularExpression ttySRegExp(QStringLiteral("^ttyS\\d+$"));
    static const QRegularExpression rfcommRegExp(QStringLiteral("^rfcomm\\d*$"));
    static const QRegularExpression ttyAcmRegExp(QStringLiteral("^ttyACM\\d+$"));

    if (ttySRegExp.match(portName).hasMatch() || rfcommRegExp.match(portName).hasMatch()) {
        return true;
    }

    if (ttyAcmRegExp.match(portName).hasMatch()) {
        return !(port.hasVendorIdentifier() && port.hasProductIdentifier());
    }
#else
    Q_UNUSED(port);
#endif

    return false;
}
