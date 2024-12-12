/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BluetoothLink.h"

#include "DeviceInfo.h"
#include "QGCLoggingCategory.h"

#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothServiceInfo>
#include <QtBluetooth/QBluetoothSocket>
#ifdef Q_OS_IOS
#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#else
#include <QtBluetooth/QBluetoothUuid>
#endif

QGC_LOGGING_CATEGORY(BluetoothLinkLog, "qgc.comms.bluetoothlink")

BluetoothLink::BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _bluetoothConfig(qobject_cast<const BluetoothConfiguration*>(config.get()))
    , _socket(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this))
#ifdef Q_OS_IOS
    , _serviceDiscoveryAgent(new QBluetoothServiceDiscoveryAgent(this))
#endif
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QBluetoothServiceInfo>("QBluetoothServiceInfo");
    (void) qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");

    (void) QObject::connect(_socket, &QBluetoothSocket::connected, this, &BluetoothLink::connected, Qt::AutoConnection);
    (void) QObject::connect(_socket, &QBluetoothSocket::disconnected, this, &BluetoothLink::disconnected, Qt::AutoConnection);
#ifdef Q_OS_IOS
    (void) QObject::connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothLink::_serviceDiscovered, Qt::AutoConnection);
    (void) QObject::connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::finished, this, &BluetoothLink::_discoveryFinished, Qt::AutoConnection);
    (void) QObject::connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::canceled, this, &BluetoothLink::_discoveryFinished, Qt::AutoConnection);
#endif

    (void) QObject::connect(_socket, &QBluetoothSocket::errorOccurred, this, [this](QBluetoothSocket::SocketError error) {
        qCWarning(BluetoothLinkLog) << "Bluetooth Link Error:" << error << _socket->errorString();
        emit communicationError(QStringLiteral("Bluetooth Link Error"), QStringLiteral("Link: %1, %2.").arg(_bluetoothConfig->name(), _socket->errorString()));
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QBluetoothSocket::stateChanged, this, [](QBluetoothSocket::SocketState state) {
        qCDebug(BluetoothLinkLog) << "Bluetooth State Changed:" << state;
    }, Qt::AutoConnection);
}

BluetoothLink::~BluetoothLink()
{
    BluetoothLink::disconnect();

    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;
}

bool BluetoothLink::isConnected() const
{
    return ((_socket->state() == QBluetoothSocket::SocketState::ConnectedState) || (_socket->state() == QBluetoothSocket::SocketState::ConnectingState));
}

void BluetoothLink::disconnect()
{
    (void) QObject::disconnect(_socket, &QBluetoothSocket::readyRead, this, &BluetoothLink::_readBytes);

#ifdef Q_OS_IOS
    (void) QMetaObject::invokeMethod(_serviceDiscoveryAgent, "stop", Qt::AutoConnection);
#else
    _socket->disconnectFromService();
    _socket->abort();
    _socket->close();
#endif
}

bool BluetoothLink::_connect()
{
    if (isConnected()) {
        return true;
    }

#ifdef Q_OS_IOS
    (void) QMetaObject::invokeMethod(_serviceDiscoveryAgent, "start", Qt::AutoConnection);
#else
    (void) QObject::connect(_socket, &QBluetoothSocket::readyRead, this, &BluetoothLink::_readBytes, Qt::AutoConnection);
    static constexpr QBluetoothUuid uuid = QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
    _socket->connectToService(_bluetoothConfig->device().address, uuid);
#endif

    return true;
}

void BluetoothLink::_writeBytes(const QByteArray &bytes)
{
    static const QString title = tr("Bluetooth Link Write Error");
    static const QString error = tr("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(title, error.arg(_bluetoothConfig->name(), "Could Not Send Data - Link is Disconnected!"));
        return;
    }

    if (_socket->write(bytes) <= 0) {
        emit communicationError(title, error.arg(_bluetoothConfig->name(), "Could Not Send Data - Write Failed!"));
        return;
    }

    emit bytesSent(this, bytes);
}

void BluetoothLink::_readBytes()
{
    static const QString title = tr("Bluetooth Link Read Error");
    static const QString error = tr("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(title, error.arg(_bluetoothConfig->name(), QStringLiteral("Could Not Read Data - link is Disconnected!")));
        return;
    }

    const qint64 byteCount = _socket->bytesAvailable();
    if (byteCount <= 0) {
        emit communicationError(title, error.arg(_bluetoothConfig->name(), QStringLiteral("Could Not Read Data - No Data Available!")));
        return;
    }

    QByteArray buffer(byteCount, Qt::Initialization::Uninitialized);
    if (_socket->read(buffer.data(), buffer.size()) < 0) {
        emit communicationError(title, error.arg(_bluetoothConfig->name(), QStringLiteral("Could Not Read Data - Read Failed!")));
        return;
    }

    emit bytesReceived(this, buffer);
}

#ifdef Q_OS_IOS
void BluetoothLink::_serviceDiscovered(const QBluetoothServiceInfo &info)
{
    if (info.device().name().isEmpty() || !info.isValid()) {
        return;
    }

    if ((_bluetoothConfig->device().uuid == info.device().deviceUuid()) && (_bluetoothConfig->device().name == info.device().name())) {
        (void) QObject::connect(_socket, &QBluetoothSocket::readyRead, this, &BluetoothLink::_readBytes, Qt::AutoConnection);
        _socket->connectToService(info);
    }
}

void BluetoothLink::_discoveryFinished()
{
    static const QString title = tr("Bluetooth Link Discovery Error");
    static const QString error = tr("Link %1: %2.");

    if (!isConnected()) {
        _socket->disconnectFromService();
        emit communicationError(title, error.arg(_bluetoothConfig->device().name, QStringLiteral("Could Not Locate Device!")));
    }
}
#endif

//------------------------------------------------------------------------------

BluetoothConfiguration::BluetoothConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
    , _deviceDiscoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;

    _initDeviceDiscoveryAgent();
}

BluetoothConfiguration::BluetoothConfiguration(const BluetoothConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
    , _device(source->device())
    , _deviceDiscoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(source);

    BluetoothConfiguration::copyFrom(source);

    _initDeviceDiscoveryAgent();
}

BluetoothConfiguration::~BluetoothConfiguration()
{
    stopScan();

    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;
}

void BluetoothConfiguration::_initDeviceDiscoveryAgent()
{
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BluetoothConfiguration::_deviceDiscovered, Qt::AutoConnection);
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &BluetoothConfiguration::scanningChanged, Qt::AutoConnection);
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BluetoothConfiguration::scanningChanged, Qt::AutoConnection);
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, [this](QBluetoothDeviceDiscoveryAgent::Error error) {
        qCWarning(BluetoothLinkLog) << "Bluetooth Configuration error:" << error << _deviceDiscoveryAgent->errorString();
    }, Qt::AutoConnection);
}

void BluetoothConfiguration::startScan()
{
    _deviceList.clear();

    _nameList.clear();
    emit nameListChanged();

    (void) QMetaObject::invokeMethod(_deviceDiscoveryAgent, "start", Qt::AutoConnection);
    emit scanningChanged();
}

void BluetoothConfiguration::stopScan()
{
    (void) QMetaObject::invokeMethod(_deviceDiscoveryAgent, "stop", Qt::AutoConnection);
}

bool BluetoothConfiguration::scanning() const
{
    return _deviceDiscoveryAgent->isActive();
}

void BluetoothConfiguration::setDevice(const QString &name)
{
    for (const BluetoothData &data: _deviceList) {
        if (data.name == name) {
            _device = data;
            emit deviceChanged();
            return;
        }
    }
}

void BluetoothConfiguration::_deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!info.name().isEmpty() && info.isValid()) {
        BluetoothData data;
        data.name = info.name();
#ifdef Q_OS_IOS
        data.uuid = info.deviceUuid();
#else
        data.address = info.address();
#endif

        if (!_deviceList.contains(data)) {
            (void) _deviceList.append(data);
            (void) _nameList.append(data.name);
            emit nameListChanged();
        }
    }
}

void BluetoothConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_CHECK_PTR(source);
    LinkConfiguration::copyFrom(source);

    const BluetoothConfiguration* const bluetoothSource = qobject_cast<const BluetoothConfiguration*>(source);
    Q_CHECK_PTR(bluetoothSource);

    _device = bluetoothSource->device();
    emit deviceChanged();
}

void BluetoothConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    _device.name = settings.value("deviceName", _device.name).toString();
#ifdef Q_OS_IOS
    _device.uuid = QUuid(settings.value("uuid", _device.uuid.toString()).toString());
#else
    _device.address = QBluetoothAddress(settings.value("address", _device.address.toString()).toString());
#endif

    settings.endGroup();
}

void BluetoothConfiguration::saveSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    settings.setValue("deviceName", _device.name);
#ifdef Q_OS_IOS
    settings.setValue("uuid", _device.uuid.toString());
#else
    settings.setValue("address",_device.address.toString());
#endif

    settings.endGroup();
}

QString BluetoothConfiguration::settingsTitle()
{
    if (QGCDeviceInfo::isBluetoothAvailable()) {
        return QStringLiteral("Bluetooth Link Settings");
    }

    return QStringLiteral("Bluetooth Not Available");
}
