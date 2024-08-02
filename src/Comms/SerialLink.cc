/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SerialLink.h"
#include "QGCLoggingCategory.h"
#include "QGCSerialPortInfo.h"
#ifdef Q_OS_ANDROID
#include "qserialportinfo.h"
#else
#include <QtSerialPort/QSerialPortInfo>
#endif
#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(SerialLinkLog, "qgc.comms.seriallink")

const QString SerialLink::kTitle = QStringLiteral("Serial Link Error");
const QString SerialLink::kError = QStringLiteral("Link %1: %2.");

SerialLink::SerialLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _serialConfig(qobject_cast<const SerialConfiguration*>(config.get()))
    , _port(new QSerialPort(_serialConfig->portName(), this))
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");

    (void) connect(_port, &QIODevice::aboutToClose, this, &SerialLink::disconnected, Qt::AutoConnection);

    (void) QObject::connect(_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        switch (error) {
        case QSerialPort::NoError:
            return;
        case QSerialPort::ResourceError:
            _connectionRemoved();
            break;
        default:
            break;
        }
        qCWarning(SerialLinkLog) << "Serial Link Error:" << error;
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), _port->errorString()));
    }, Qt::AutoConnection);
}

SerialLink::~SerialLink()
{
    SerialLink::disconnect();

    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

void SerialLink::disconnect()
{
    (void) QObject::disconnect(_port, &QIODevice::readyRead, this, &SerialLink::_readBytes);
    _port->close();
}

bool SerialLink::_connect()
{
    if (isConnected()) {
        return true;
    }

    if (_hardwareConnect()) {
        emit connected();
        return true;
    }

    if (!_serialConfig->isAutoConnect() || (_port->error() != QSerialPort::PermissionError)) {
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Failed to Connect.")));
    }

    return false;
}

bool SerialLink::_hardwareConnect()
{
    const QGCSerialPortInfo portInfo(*_port);
    if (portInfo.isBootloader()) {
        if (!_serialConfig->isAutoConnect()) {
            emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Not Connecting to a Bootloader.")));
        }
        return false;
    }

    if (!_port->open(QIODevice::ReadWrite)) {
        if (!_serialConfig->isAutoConnect()) {
            emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Could Not Open Port.")));
        }
        return false;
    }

    (void) QObject::connect(_port, &QIODevice::readyRead, this, &SerialLink::_readBytes, Qt::AutoConnection);

    _port->setDataTerminalReady(true);

    _port->setBaudRate(_serialConfig->baud());
    _port->setDataBits(static_cast<QSerialPort::DataBits>(_serialConfig->dataBits()));
    _port->setFlowControl(static_cast<QSerialPort::FlowControl>(_serialConfig->flowControl()));
    _port->setStopBits(static_cast<QSerialPort::StopBits>(_serialConfig->stopBits()));
    _port->setParity(static_cast<QSerialPort::Parity>(_serialConfig->parity()));

    return true;
}

void SerialLink::_writeBytes(const QByteArray &data)
{
    if (!isConnected()) {
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Could Not Send Data - Link is Disconnected!")));
        return;
    }

    if (_port->write(data) <= 0) {
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Could Not Send Data - Write Failed!")));
        return;
    }

    emit bytesSent(this, data);
}

void SerialLink::_readBytes()
{
    if (!isConnected()) {
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Could Not Read Data - link is Disconnected!")));
        return;
    }

    const qint64 byteCount = _port->bytesAvailable();
    if (byteCount <= 0) {
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Could Not Read Data - No Data Available!")));
        return;
    }

    QByteArray buffer(byteCount, Qt::Initialization::Uninitialized);
    if (_port->read(buffer.data(), buffer.size()) < 0) {
        emit communicationError(kTitle, kError.arg(_serialConfig->name(), QStringLiteral("Could Not Read Data - Read Failed!")));
        return;
    }

    emit bytesReceived(this, buffer);
}

/*===========================================================================*/

SerialConfiguration::SerialConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

SerialConfiguration::SerialConfiguration(const SerialConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(source);

    SerialConfiguration::copyFrom(source);
}

SerialConfiguration::~SerialConfiguration()
{
    // qCDebug(SerialLinkLog) << Q_FUNC_INFO << this;
}

void SerialConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_CHECK_PTR(source);
    LinkConfiguration::copyFrom(source);

    const SerialConfiguration* const serialSource = qobject_cast<const SerialConfiguration*>(source);
    Q_CHECK_PTR(serialSource);

    setBaud(serialSource->baud());
    setDataBits(serialSource->dataBits());
    setFlowControl(serialSource->flowControl());
    setStopBits(serialSource->stopBits());
    setParity(serialSource->parity());
    setPortName(serialSource->portName());
    setPortDisplayName(serialSource->portDisplayName());
    setUsbDirect(serialSource->usbDirect());
}

void SerialConfiguration::setPortName(const QString &name)
{
    const QString portName = name.trimmed();
    if (portName.isEmpty()) {
        return;
    }

    if (portName != _portName) {
        _portName = portName;
        emit portNameChanged();
    }

    const QString portDisplayName = cleanPortDisplayName(portName);
    setPortDisplayName(portDisplayName);
}

void SerialConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setBaud(settings.value("baud", _baud).toInt());
    setDataBits(static_cast<QSerialPort::DataBits>(settings.value("dataBits", _dataBits).toInt()));
    setFlowControl(static_cast<QSerialPort::FlowControl>(settings.value("flowControl", _flowControl).toInt()));
    setStopBits(static_cast<QSerialPort::StopBits>(settings.value("stopBits", _stopBits).toInt()));
    setParity(static_cast<QSerialPort::Parity>(settings.value("parity", _parity).toInt()));
    setPortName(settings.value("portName", _portName).toString());
    _portDisplayName = settings.value("portDisplayName", _portDisplayName).toString();

    settings.endGroup();
}

void SerialConfiguration::saveSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    settings.setValue("baud", _baud);
    settings.setValue("dataBits", _dataBits);
    settings.setValue("flowControl", _flowControl);
    settings.setValue("stopBits", _stopBits);
    settings.setValue("parity", _parity);
    settings.setValue("portName", _portName);
    settings.setValue("portDisplayName", _portDisplayName);

    settings.endGroup();
}

QStringList SerialConfiguration::supportedBaudRates()
{
    QStringList supportBaudRateStrings;
    for (qint32 rate : QSerialPortInfo::standardBaudRates()) {
        (void) supportBaudRateStrings.append(QString::number(rate));
    }

    return supportBaudRateStrings;
}

QString SerialConfiguration::cleanPortDisplayName(const QString &name)
{
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.systemLocation() == name) {
            return info.portName();
        }
    }

    return QString();
}
