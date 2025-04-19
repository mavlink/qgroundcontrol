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

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(BluetoothLinkLog, "qgc.comms.bluetoothlink")

/*===========================================================================*/

BluetoothConfiguration::BluetoothConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
    , _deviceDiscoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;

    _initDeviceDiscoveryAgent();
}

BluetoothConfiguration::BluetoothConfiguration(const BluetoothConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _device(copy->device())
    , _deviceDiscoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;

    BluetoothConfiguration::copyFrom(copy);

    _initDeviceDiscoveryAgent();
}

BluetoothConfiguration::~BluetoothConfiguration()
{
    stopScan();

    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;
}

void BluetoothConfiguration::_initDeviceDiscoveryAgent()
{
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BluetoothConfiguration::_deviceDiscovered);
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &BluetoothConfiguration::scanningChanged);
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BluetoothConfiguration::scanningChanged);
    (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &BluetoothConfiguration::_onSocketErrorOccurred);

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this, [this](const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields) {
            qCDebug(BluetoothLinkLog) << "Device Updated";
        });
    }
}

void BluetoothConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);
    LinkConfiguration::copyFrom(source);

    const BluetoothConfiguration *const bluetoothSource = qobject_cast<const BluetoothConfiguration*>(source);
    Q_ASSERT(bluetoothSource);

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

void BluetoothConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue("deviceName", _device.name);
#ifdef Q_OS_IOS
    settings.setValue("uuid", _device.uuid.toString());
#else
    settings.setValue("address", _device.address.toString());
#endif

    settings.endGroup();
}

QString BluetoothConfiguration::settingsTitle() const
{
    if (QGCDeviceInfo::isBluetoothAvailable()) {
        return tr("Bluetooth Link Settings");
    }

    return tr("Bluetooth Not Available");
}

void BluetoothConfiguration::startScan()
{
    _deviceList.clear();

    _nameList.clear();
    emit nameListChanged();

    _deviceDiscoveryAgent->start();
    emit scanningChanged();
}

void BluetoothConfiguration::stopScan() const
{
    if (scanning()) {
        _deviceDiscoveryAgent->stop();
    }
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

bool BluetoothConfiguration::scanning() const
{
    return _deviceDiscoveryAgent->isActive();
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
            _deviceList.append(data);
            _nameList.append(data.name);
            emit nameListChanged();
        }
    }
}

void BluetoothConfiguration::_onSocketErrorOccurred(QBluetoothDeviceDiscoveryAgent::Error error)
{
    const QString errorString = _deviceDiscoveryAgent->errorString();
    qCWarning(BluetoothLinkLog) << "Bluetooth Configuration error:" << error << errorString;
    emit errorOccurred(errorString);
}

/*===========================================================================*/

BluetoothWorker::BluetoothWorker(const BluetoothConfiguration *config, QObject *parent)
    : QObject(parent)
    , _config(config)
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;
}

BluetoothWorker::~BluetoothWorker()
{
    disconnectLink();

    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;
}

bool BluetoothWorker::isConnected() const
{
    return (_socket && _socket->isOpen() && (_socket->state() == QBluetoothSocket::SocketState::ConnectedState));
}

void BluetoothWorker::setupSocket()
{
    Q_ASSERT(!_socket);
    _socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

#ifdef Q_OS_IOS
    Q_ASSERT(!_serviceDiscoveryAgent);
    _serviceDiscoveryAgent = new QBluetoothServiceDiscoveryAgent(this);
#endif

    (void) connect(_socket, &QBluetoothSocket::connected, this, &BluetoothWorker::_onSocketConnected);
    (void) connect(_socket, &QBluetoothSocket::disconnected, this, &BluetoothWorker::_onSocketDisconnected);
    (void) connect(_socket, &QBluetoothSocket::readyRead, this, &BluetoothWorker::_onSocketReadyRead);
    (void) connect(_socket, &QBluetoothSocket::errorOccurred, this, &BluetoothWorker::_onSocketErrorOccurred);

#ifdef Q_OS_IOS
    (void) connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothWorker::_serviceDiscovered);
    (void) connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::finished, this, &BluetoothWorker::_discoveryFinished);
    (void) connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::canceled, this, &BluetoothWorker::_discoveryFinished);
    (void) connect(_serviceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::errorOccurred, this, &BluetoothWorker::_onServiceErrorOccurred);
#endif

    if (BluetoothLinkLog().isDebugEnabled()) {
        // (void) connect(_socket, &QBluetoothSocket::bytesWritten, this, &BluetoothWorker::_onSocketBytesWritten);

        (void) QObject::connect(_socket, &QBluetoothSocket::stateChanged, this, [](QBluetoothSocket::SocketState state) {
            qCDebug(BluetoothLinkLog) << "Bluetooth State Changed:" << state;
        });
    }
}

void BluetoothWorker::connectLink()
{
    if (isConnected()) {
        qCWarning(BluetoothLinkLog) << "Already connected to" << _config->device().name;
        return;
    }

    qCDebug(BluetoothLinkLog) << "Attempting to connect to" << _config->device().name;

#ifdef Q_OS_IOS
    if (_serviceDiscoveryAgent && _serviceDiscoveryAgent->isActive()) {
        _serviceDiscoveryAgent->start();
    }
#else
    static constexpr QBluetoothUuid uuid = QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
    _socket->connectToService(_config->device().address, uuid);
#endif
}

void BluetoothWorker::disconnectLink()
{
    if (!isConnected()) {
        qCWarning(BluetoothLinkLog) << "Already disconnected from device:" << _config->device().name;
        return;
    }

    qCDebug(BluetoothLinkLog) << "Attempting to disconnect from device:" << _config->device().name;

#ifdef Q_OS_IOS
    if (_serviceDiscoveryAgent && _serviceDiscoveryAgent->isActive()) {
        _serviceDiscoveryAgent->stop();
    }
#endif
    _socket->disconnectFromService();
}

void BluetoothWorker::writeData(const QByteArray &data)
{
    if (data.isEmpty()) {
        emit errorOccurred(tr("Data to Send is Empty"));
        return;
    }

    if (!isConnected()) {
        emit errorOccurred(tr("Socket is not connected"));
        return;
    }

    if (!_socket->isWritable()) {
        emit errorOccurred(tr("Socket is not Writable"));
        return;
    }

    qint64 totalBytesWritten = 0;
    while (totalBytesWritten < data.size()) {
        const qint64 bytesWritten = _socket->write(data.constData() + totalBytesWritten, data.size() - totalBytesWritten);
        if (bytesWritten == -1) {
            emit errorOccurred(tr("Could Not Send Data - Write Failed: %1").arg(_socket->errorString()));
            return;
        } else if (bytesWritten == 0) {
            emit errorOccurred(tr("Could Not Send Data - Write Returned 0 Bytes"));
            return;
        }
        totalBytesWritten += bytesWritten;
    }

    const QByteArray sent = data.first(totalBytesWritten);
    emit dataSent(sent);
}

void BluetoothWorker::_onSocketConnected()
{
    qCDebug(BluetoothLinkLog) << "Socket connected to device:" << _config->device().name;
    emit connected();
}

void BluetoothWorker::_onSocketDisconnected()
{
    qCDebug(BluetoothLinkLog) << "Socket disconnected from device:" << _config->device().name;
    emit disconnected();
}

void BluetoothWorker::_onSocketReadyRead()
{
    const QByteArray data = _socket->readAll();
    if (!data.isEmpty()) {
        // qCDebug(BluetoothLinkLog) << "_onSocketReadyRead:" << data.size();
        emit dataReceived(data);
    }
}

void BluetoothWorker::_onSocketBytesWritten(qint64 bytes)
{
    qCDebug(BluetoothLinkLog) << _config->device().name << "Wrote" << bytes << "bytes";
}

void BluetoothWorker::_onSocketErrorOccurred(QBluetoothSocket::SocketError socketError)
{
    const QString errorString = _socket->errorString();
    qCWarning(BluetoothLinkLog) << "Socket error:" << socketError << errorString;
    emit errorOccurred(errorString);
}

#ifdef Q_OS_IOS
void BluetoothWorker::_onServiceErrorOccurred(QBluetoothServiceDiscoveryAgent::Error error)
{
    const QString errorString = _serviceDiscoveryAgent->errorString();
    qCWarning(BluetoothLinkLog) << "Socket error:" << error << errorString;
    emit errorOccurred(errorString);
}

void BluetoothWorker::_serviceDiscovered(const QBluetoothServiceInfo &info)
{
    if (isConnected()) {
        qCWarning(BluetoothLinkLog) << "Already connected to" << _config->device().name;
        return;
    }

    if (info.device().name().isEmpty() || !info.isValid()) {
        return;
    }

    if ((_config->device().uuid == info.device().deviceUuid()) && (_config->device().name == info.device().name)) {
        _socket->connectToService(info);
    }
}

void BluetoothWorker::_discoveryFinished()
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("Discovery Error: Could Not Locate Device!"));
    }
}
#endif

/*===========================================================================*/

BluetoothLink::BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _bluetoothConfig(qobject_cast<const BluetoothConfiguration*>(config.get()))
    , _worker(new BluetoothWorker(_bluetoothConfig))
    , _workerThread(new QThread(this))
{
    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;

    _checkPermission();

    _workerThread->setObjectName(QStringLiteral("Bluetooth_%1").arg(_bluetoothConfig->name()));

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &BluetoothWorker::setupSocket);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &BluetoothWorker::connected, this, &BluetoothLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &BluetoothWorker::disconnected, this, &BluetoothLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &BluetoothWorker::errorOccurred, this, &BluetoothLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker, &BluetoothWorker::dataReceived, this, &BluetoothLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &BluetoothWorker::dataSent, this, &BluetoothLink::_onDataSent, Qt::QueuedConnection);

    (void) connect(_bluetoothConfig, &BluetoothConfiguration::errorOccurred, this, &BluetoothLink::_onErrorOccurred);

    _workerThread->start();
}

BluetoothLink::~BluetoothLink()
{
    BluetoothLink::disconnect();

    _workerThread->quit();
    if (!_workerThread->wait()) {
        qCWarning(BluetoothLinkLog) << "Failed to wait for Bluetooth Thread to close";
    }

    // qCDebug(BluetoothLinkLog) << Q_FUNC_INFO << this;
}

bool BluetoothLink::isConnected() const
{
    return _worker->isConnected();
}

bool BluetoothLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectLink", Qt::QueuedConnection);
}

void BluetoothLink::disconnect()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectLink", Qt::QueuedConnection);
}

void BluetoothLink::_onConnected()
{
    emit connected();
}

void BluetoothLink::_onDisconnected()
{
    emit disconnected();
}

void BluetoothLink::_onErrorOccurred(const QString &errorString)
{
    qCWarning(BluetoothLinkLog) << "Communication error:" << errorString;
    emit communicationError(tr("Bluetooth Link Error"), tr("Link %1: (Device: %2) %3").arg(_bluetoothConfig->name(), _bluetoothConfig->device().name, errorString));
}

void BluetoothLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void BluetoothLink::_onDataSent(const QByteArray &data)
{
    emit bytesSent(this, data);
}

void BluetoothLink::_writeBytes(const QByteArray& bytes)
{
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, bytes));
}

void BluetoothLink::_checkPermission()
{
    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);

    const Qt::PermissionStatus permissionStatus = QCoreApplication::instance()->checkPermission(permission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        QCoreApplication::instance()->requestPermission(permission, this, [this](const QPermission &permission) {
            _handlePermissionStatus(permission.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void BluetoothLink::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    if (permissionStatus != Qt::PermissionStatus::Granted) {
        qCWarning(BluetoothLinkLog) << "Bluetooth Permission Denied";
        _onErrorOccurred("Bluetooth Permission Denied");
        _onDisconnected();
    } else {
        qCDebug(BluetoothLinkLog) << "Bluetooth Permission Granted";
    }
}
