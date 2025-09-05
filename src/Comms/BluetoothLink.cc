/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BluetoothLink.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QThread>

#include "DeviceInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(BluetoothLinkLog, "qgc.comms.bluetoothlink")

/*===========================================================================*/

BluetoothConfiguration::BluetoothConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(BluetoothLinkLog) << this;

    _initDeviceDiscoveryAgent();
}

BluetoothConfiguration::BluetoothConfiguration(const BluetoothConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _mode(copy->mode())
    , _device(copy->device())
    , _serviceUuid(copy->_serviceUuid)
    , _readCharacteristicUuid(copy->readCharacteristicUuid())
    , _writeCharacteristicUuid(copy->writeCharacteristicUuid())
{
    qCDebug(BluetoothLinkLog) << this;

    BluetoothConfiguration::copyFrom(copy);
    _initDeviceDiscoveryAgent();
}

BluetoothConfiguration::~BluetoothConfiguration()
{
    stopScan();

    qCDebug(BluetoothLinkLog) << this;
}

void BluetoothConfiguration::_initDeviceDiscoveryAgent()
{
    if (!_deviceDiscoveryAgent) {
        _deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    }

    // Configure discovery timeout for BLE
    _deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(15000);

    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                   this, &BluetoothConfiguration::_deviceDiscovered);
    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::canceled,
                   this, &BluetoothConfiguration::scanningChanged);
    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::finished,
                   this, &BluetoothConfiguration::_onDiscoveryFinished);
    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::errorOccurred,
                   this, &BluetoothConfiguration::_onDiscoveryErrorOccurred);

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::deviceUpdated,
                       this, [this](const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields) {
            qCDebug(BluetoothLinkLog) << "Device Updated:" << info.name() << "Fields:" << updatedFields;
        });
    }
}

void BluetoothConfiguration::setMode(BluetoothMode mode)
{
    if (_mode != mode) {
        _mode = mode;
        emit modeChanged();

        // Clear device list when switching modes
        _deviceList.clear();
        _nameList.clear();
        emit nameListChanged();
    }
}

void BluetoothConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);
    LinkConfiguration::copyFrom(source);

    const BluetoothConfiguration *const bluetoothSource = qobject_cast<const BluetoothConfiguration*>(source);
    Q_ASSERT(bluetoothSource);

    _mode = bluetoothSource->mode();
    _device = bluetoothSource->device();
    _serviceUuid = bluetoothSource->_serviceUuid;
    _readCharacteristicUuid = bluetoothSource->readCharacteristicUuid();
    _writeCharacteristicUuid = bluetoothSource->writeCharacteristicUuid();

    emit modeChanged();
    emit deviceChanged();
    emit serviceUuidChanged();
}

void BluetoothConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    _mode = static_cast<BluetoothMode>(settings.value("mode", static_cast<int>(BluetoothMode::ModeClassic)).toInt());

    const QString deviceName = settings.value("deviceName").toString();
    const QBluetoothAddress address(settings.value("address").toString());

    if (!deviceName.isEmpty() && !address.isNull()) {
        _device = QBluetoothDeviceInfo(address, deviceName, 0);
    }

    _serviceUuid = QBluetoothUuid(settings.value("serviceUuid", _serviceUuid.toString()).toString());
    _readCharacteristicUuid = QBluetoothUuid(settings.value("readCharUuid", _readCharacteristicUuid.toString()).toString());
    _writeCharacteristicUuid = QBluetoothUuid(settings.value("writeCharUuid", _writeCharacteristicUuid.toString()).toString());

    settings.endGroup();
}

void BluetoothConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue("mode", static_cast<int>(_mode));
    settings.setValue("deviceName", _device.name());
    settings.setValue("address", _device.address().toString());
    settings.setValue("serviceUuid", _serviceUuid.toString());
    settings.setValue("readCharUuid", _readCharacteristicUuid.toString());
    settings.setValue("writeCharUuid", _writeCharacteristicUuid.toString());

    settings.endGroup();
}

QString BluetoothConfiguration::settingsTitle() const
{
    if (QGCDeviceInfo::isBluetoothAvailable()) {
        return (_mode == BluetoothMode::ModeLowEnergy) ? tr("Bluetooth Low Energy Link Settings") : tr("Bluetooth Link Settings");
    }
    return tr("Bluetooth Not Available");
}

void BluetoothConfiguration::startScan()
{
    if (!_deviceDiscoveryAgent) {
        return;
    }

    _deviceList.clear();
    _nameList.clear();
    emit nameListChanged();

    // Start discovery based on mode
    if (_mode == BluetoothMode::ModeLowEnergy) {
        _deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    } else {
        _deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
    }

    emit scanningChanged();
}

void BluetoothConfiguration::stopScan()
{
    if (_deviceDiscoveryAgent && scanning()) {
        _deviceDiscoveryAgent->stop();
    }
}

void BluetoothConfiguration::setDevice(const QString &name)
{
    for (const QBluetoothDeviceInfo &info : _deviceList) {
        if (info.name() == name) {
            _device = info;

            // For BLE devices, try to extract service UUID
            if (_mode == BluetoothMode::ModeLowEnergy) {
                const QList<QBluetoothUuid> serviceUuids = info.serviceUuids();
                if (!serviceUuids.isEmpty()) {
                    // Look for Nordic UART service first
                    static const QBluetoothUuid nordicUartService(QStringLiteral("6e400001-b5a3-f393-e0a9-e50e24dcca9e"));

                    _serviceUuid = QBluetoothUuid();
                    for (const QBluetoothUuid &uuid : serviceUuids) {
                        if (uuid == nordicUartService) {
                            _serviceUuid = uuid;
                            break;
                        }
                    }

                    // If no known service, use first available
                    if (_serviceUuid.isNull() && !serviceUuids.isEmpty()) {
                        _serviceUuid = serviceUuids.first();
                    }

                    emit serviceUuidChanged();
                }
            }

            emit deviceChanged();
            return;
        }
    }
}

void BluetoothConfiguration::setServiceUuid(const QString &uuid)
{
    const QBluetoothUuid newUuid(uuid);
    if (_serviceUuid != newUuid) {
        _serviceUuid = newUuid;
        emit serviceUuidChanged();
    }
}

bool BluetoothConfiguration::scanning() const
{
    return _deviceDiscoveryAgent && _deviceDiscoveryAgent->isActive();
}

void BluetoothConfiguration::_deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (info.name().isEmpty() || !info.isValid()) {
        return;
    }

    // Filter based on mode
    if (_mode == BluetoothMode::ModeLowEnergy) {
        if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) {
            return;  // Skip non-BLE devices
        }
    } else {
        if (!(info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateCoreConfiguration) &&
            !(info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateAndLowEnergyCoreConfiguration)) {
            return;  // Skip non-classic devices
        }
    }

    // Check if device already in list
    bool found = false;
    for (const QBluetoothDeviceInfo &existing : _deviceList) {
        if (existing.address() == info.address()) {
            found = true;
            break;
        }
    }

    if (!found) {
        _deviceList.append(info);
        _nameList.append(info.name());
        emit nameListChanged();

        qCDebug(BluetoothLinkLog) << (_mode == BluetoothMode::ModeLowEnergy ? "BLE" : "Classic")
                                  << "device discovered:" << info.name()
                                  << "Address:" << info.address().toString();
    }
}

// void BluetoothConfiguration::_deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
// {
//     if (!info.isValid() || (updatedFields == QBluetoothDeviceInfo::Field::None)) {
//         return;
//     }

//     for (QBluetoothDeviceInfo &dev: _deviceList) {
//         if (dev.address() == info.address()) {
//             if (updatedFields & QBluetoothDeviceInfo::Field::RSSI) {
//                 dev.setRssi(info.rssi());
//             }
//             if (updatedFields & QBluetoothDeviceInfo::Field::ManufacturerData) {
//                 const QMultiHash<quint16, QByteArray> data = info.manufacturerData();
//                 for (quint16 id : info.manufacturerIds()) {
//                     dev.setManufacturerData(id, data.value(id));
//                 }
//             }
//             if (updatedFields & QBluetoothDeviceInfo::Field::ServiceData) {
//                 const QMultiHash<QBluetoothUuid, QByteArray> data = info.serviceData();
//                 for (QBluetoothUuid uuid : info.serviceIds()) {
//                     dev.setServiceData(uuid, data.value(uuid));
//                 }
//             }
//             if (updatedFields & QBluetoothDeviceInfo::Field::All) {
//                 dev = info;
//             }
//             break;
//         }
//     }
// }

void BluetoothConfiguration::_onDiscoveryFinished()
{
    emit scanningChanged();
    _updateDeviceList();
}

void BluetoothConfiguration::_updateDeviceList()
{
    // Update name list from device list
    _nameList.clear();
    for (const QBluetoothDeviceInfo &info : _deviceList) {
        _nameList.append(info.name());
    }
    emit nameListChanged();
}

void BluetoothConfiguration::_onDiscoveryErrorOccurred(QBluetoothDeviceDiscoveryAgent::Error error)
{
    QString errorString;
    if (_deviceDiscoveryAgent) {
        errorString = _deviceDiscoveryAgent->errorString();
    } else {
        errorString = tr("Discovery error: %1").arg(error);
    }

    qCWarning(BluetoothLinkLog) << "Bluetooth discovery error:" << error << errorString;
    emit errorOccurred(errorString);
}

/*===========================================================================*/

BluetoothWorker::BluetoothWorker(const BluetoothConfiguration *config, QObject *parent)
    : QObject(parent)
    , _config(config)
    , _reconnectTimer(new QTimer(this))
{
    qCDebug(BluetoothLinkLog) << this;

    _reconnectTimer->setInterval(5000);
    _reconnectTimer->setSingleShot(true);
    (void) connect(_reconnectTimer, &QTimer::timeout, this, &BluetoothWorker::_reconnectTimeout);
}

BluetoothWorker::~BluetoothWorker()
{
    _intentionalDisconnect = true;
    disconnectLink();
    qCDebug(BluetoothLinkLog) << this;
}

bool BluetoothWorker::isConnected() const
{
    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        return (_controller && _controller->state() == QLowEnergyController::ConnectedState &&
                _service && _service->state() == QLowEnergyService::RemoteServiceDiscovered &&
                _writeCharacteristic.isValid());
    } else {
        return (_socket && _socket->isOpen() &&
                (_socket->state() == QBluetoothSocket::SocketState::ConnectedState));
    }
}

void BluetoothWorker::setupConnection()
{
    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        _setupBleController();
    } else {
        _setupClassicSocket();
    }
}

void BluetoothWorker::_setupClassicSocket()
{
    Q_ASSERT(!_socket);
    _socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

    (void) connect(_socket.data(), &QBluetoothSocket::connected, this, &BluetoothWorker::_onSocketConnected);
    (void) connect(_socket.data(), &QBluetoothSocket::disconnected, this, &BluetoothWorker::_onSocketDisconnected);
    (void) connect(_socket.data(), &QBluetoothSocket::readyRead, this, &BluetoothWorker::_onSocketReadyRead);
    (void) connect(_socket.data(), &QBluetoothSocket::errorOccurred, this, &BluetoothWorker::_onSocketErrorOccurred);

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_socket.data(), &QBluetoothSocket::bytesWritten, this, &BluetoothWorker::_onSocketBytesWritten);
        (void) connect(_socket.data(), &QBluetoothSocket::stateChanged, this, [](QBluetoothSocket::SocketState state) {
            qCDebug(BluetoothLinkLog) << "Bluetooth Socket State Changed:" << state;
        });
    }
}

void BluetoothWorker::_setupBleController()
{
    Q_ASSERT(!_controller);

    _controller = QLowEnergyController::createCentral(_config->device(), this);

    if (!_controller) {
        emit errorOccurred(tr("Failed to create BLE controller"));
        return;
    }

    (void) connect(_controller.data(), &QLowEnergyController::connected,
                   this, &BluetoothWorker::_onControllerConnected);
    (void) connect(_controller.data(), &QLowEnergyController::disconnected,
                   this, &BluetoothWorker::_onControllerDisconnected);
    (void) connect(_controller.data(), &QLowEnergyController::errorOccurred,
                   this, &BluetoothWorker::_onControllerErrorOccurred);
    (void) connect(_controller.data(), &QLowEnergyController::serviceDiscovered,
                   this, &BluetoothWorker::_onServiceDiscovered);
    (void) connect(_controller.data(), &QLowEnergyController::discoveryFinished,
                   this, &BluetoothWorker::_onServiceDiscoveryFinished);

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_controller.data(), &QLowEnergyController::stateChanged,
                       this, [](QLowEnergyController::ControllerState state) {
            qCDebug(BluetoothLinkLog) << "BLE Controller State Changed:" << state;
        });
        (void) connect(_controller.data(), &QLowEnergyController::mtuChanged, this, [this](int mtu) { _mtu = mtu; });
        (void) connect(_controller.data(), &QLowEnergyController::rssiRead, this, [this](qint16 rssi) { _rssi = rssi; });
    }
}

void BluetoothWorker::connectLink()
{
    _intentionalDisconnect = false;

    if (isConnected()) {
        qCWarning(BluetoothLinkLog) << "Already connected to" << _config->device().name();
        return;
    }

    qCDebug(BluetoothLinkLog) << "Attempting to connect to" << _config->device().name()
                              << "Mode:" << (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy ? "BLE" : "Classic");

    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        if (!_controller) {
            _setupBleController();
        }

        if (!_controller) {
            emit errorOccurred(tr("BLE controller not available"));
            return;
        }

        _controller->connectToDevice();
    } else {
        if (!_socket) {
            _setupClassicSocket();
        }

        static constexpr QBluetoothUuid uuid = QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
        _socket->connectToService(_config->device().address(), uuid);
    }
}

void BluetoothWorker::disconnectLink()
{
    _intentionalDisconnect = true;
    _reconnectTimer->stop();

    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        if (!_controller) {
            qCWarning(BluetoothLinkLog) << "No BLE controller to disconnect";
            return;
        }

        if (_service) {
            _service->deleteLater();
            _service = nullptr;
        }

        _controller->disconnectFromDevice();
    } else {
        if (!_socket) {
            qCWarning(BluetoothLinkLog) << "No socket to disconnect";
            return;
        }

        _socket->disconnectFromService();
    }
}

void BluetoothWorker::writeData(const QByteArray &data)
{
    if (data.isEmpty()) {
        emit errorOccurred(tr("Data to Send is Empty"));
        return;
    }

    if (!isConnected()) {
        emit errorOccurred(tr("Device is not connected"));
        return;
    }

    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        _writeBleData(data);
    } else {
        _writeClassicData(data);
    }
}

void BluetoothWorker::_writeClassicData(const QByteArray &data)
{
    if (!_socket || !_socket->isWritable()) {
        emit errorOccurred(tr("Socket is not writable"));
        return;
    }

    qint64 totalBytesWritten = 0;
    while (totalBytesWritten < data.size()) {
        const qint64 bytesWritten = _socket->write(data.constData() + totalBytesWritten,
                                                   data.size() - totalBytesWritten);
        if (bytesWritten == -1) {
            emit errorOccurred(tr("Write failed: %1").arg(_socket->errorString()));
            return;
        } else if (bytesWritten == 0) {
            emit errorOccurred(tr("Write returned 0 bytes"));
            return;
        }
        totalBytesWritten += bytesWritten;
    }

    emit dataSent(data.first(totalBytesWritten));
}

void BluetoothWorker::_writeBleData(const QByteArray &data)
{
    if (!_writeCharacteristic.isValid()) {
        emit errorOccurred(tr("Write characteristic is not valid"));
        return;
    }

    if (!_service) {
        emit errorOccurred(tr("BLE service not available"));
        return;
    }

    // BLE has packet size limitations (typically 20-512 bytes depending on MTU)
    static constexpr int BLE_PACKET_SIZE = 20;
    // ATT MTU payload is (MTU - 3). If we don't know MTU yet, assume 20.
    const int pdu = (_mtu > 3) ? (_mtu - 3) : BLE_PACKET_SIZE;
    // keep it sane
    const int pdu_size = qBound(1, pdu, 244);

    for (int i = 0; i < data.size(); i += pdu_size) {
        const QByteArray chunk = data.mid(i, pdu_size);

        if (_writeCharacteristic.properties() & QLowEnergyCharacteristic::WriteNoResponse) {
            _service->writeCharacteristic(_writeCharacteristic, chunk, QLowEnergyService::WriteWithoutResponse);
        } else {
            _service->writeCharacteristic(_writeCharacteristic, chunk);
        }
    }

    emit dataSent(data);
}

// Classic Bluetooth slots
void BluetoothWorker::_onSocketConnected()
{
    qCDebug(BluetoothLinkLog) << "Socket connected to device:" << _config->device().name();
    emit connected();
}

void BluetoothWorker::_onSocketDisconnected()
{
    qCDebug(BluetoothLinkLog) << "Socket disconnected from device:" << _config->device().name();
    emit disconnected();

    if (!_intentionalDisconnect && _socket) {
        qCDebug(BluetoothLinkLog) << "Starting reconnect timer";
        _reconnectTimer->start();
    }
}

void BluetoothWorker::_onSocketReadyRead()
{
    if (!_socket) {
        return;
    }

    const QByteArray data = _socket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void BluetoothWorker::_onSocketBytesWritten(qint64 bytes)
{
    qCDebug(BluetoothLinkLog) << _config->device().name() << "Wrote" << bytes << "bytes";
}

void BluetoothWorker::_onSocketErrorOccurred(QBluetoothSocket::SocketError socketError)
{
    QString errorString;
    if (_socket) {
        errorString = _socket->errorString();
    } else {
        errorString = tr("Null Socket Error");
    }

    qCWarning(BluetoothLinkLog) << "Socket error:" << socketError << errorString;
    emit errorOccurred(errorString);
}

// BLE slots
void BluetoothWorker::_onControllerConnected()
{
    qCDebug(BluetoothLinkLog) << "BLE Controller connected to device:" << _config->device().name();

    if (_controller) {
        _controller->discoverServices();
    }
}

void BluetoothWorker::_onControllerDisconnected()
{
    qCDebug(BluetoothLinkLog) << "BLE Controller disconnected from device:" << _config->device().name();

    if (_service) {
        _service->deleteLater();
        _service = nullptr;
    }

    _readCharacteristic = QLowEnergyCharacteristic();
    _writeCharacteristic = QLowEnergyCharacteristic();

    emit disconnected();

    if (!_intentionalDisconnect && _controller) {
        qCDebug(BluetoothLinkLog) << "Starting reconnect timer";
        _reconnectTimer->start();
    }
}

void BluetoothWorker::_onControllerErrorOccurred(QLowEnergyController::Error error)
{
    QString errorString;
    if (_controller) {
        errorString = _controller->errorString();
    } else {
        errorString = tr("Controller error: %1").arg(error);
    }

    qCWarning(BluetoothLinkLog) << "BLE Controller error:" << error << errorString;
    emit errorOccurred(errorString);
}

void BluetoothWorker::_onServiceDiscovered(const QBluetoothUuid &uuid)
{
    qCDebug(BluetoothLinkLog) << "Service discovered:" << uuid.toString();

    // Check if this is our target service
    const QBluetoothUuid targetUuid = QBluetoothUuid(_config->serviceUuid());
    if ((uuid == targetUuid) || (targetUuid.isNull())) {
        _setupBleService();
    }
}

void BluetoothWorker::_onServiceDiscoveryFinished()
{
    qCDebug(BluetoothLinkLog) << "Service discovery finished";

    if (!_service && _controller) {
        // If no specific service was found, try to use the first available service
        const QList<QBluetoothUuid> services = _controller->services();
        if (!services.isEmpty()) {
            qCDebug(BluetoothLinkLog) << "Using first available service";
            _setupBleService();
        } else {
            emit errorOccurred(tr("No services found on BLE device"));
        }
    }
}

void BluetoothWorker::_setupBleService()
{
    if (!_controller) {
        emit errorOccurred(tr("Controller not available"));
        return;
    }

    QBluetoothUuid serviceUuid = QBluetoothUuid(_config->serviceUuid());

    if (serviceUuid.isNull()) {
        const QList<QBluetoothUuid> services = _controller->services();
        if (services.isEmpty()) {
            emit errorOccurred(tr("No services available"));
            return;
        }
        serviceUuid = services.first();
    }

    _service = _controller->createServiceObject(serviceUuid, this);

    if (!_service) {
        emit errorOccurred(tr("Failed to create service object"));
        return;
    }

    (void) connect(_service.data(), &QLowEnergyService::stateChanged,
                   this, &BluetoothWorker::_onServiceStateChanged);
    (void) connect(_service.data(), &QLowEnergyService::characteristicChanged,
                   this, &BluetoothWorker::_onCharacteristicChanged);
    (void) connect(_service.data(), &QLowEnergyService::characteristicWritten,
                   this, &BluetoothWorker::_onCharacteristicWritten);
    // characteristicRead, descriptorRead, descriptorWritten
    (void) connect(_service.data(), &QLowEnergyService::errorOccurred,
                   this, &BluetoothWorker::_onServiceError);

    _discoverServiceDetails();
}

void BluetoothWorker::_discoverServiceDetails()
{
    if (!_service) {
        return;
    }

    qCDebug(BluetoothLinkLog) << "Discovering service details";
    _service->discoverDetails();
}

void BluetoothWorker::_onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qCDebug(BluetoothLinkLog) << "Service state changed:" << state;

    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        if (!_service) {
            return;
        }

        // Find read and write characteristics
        const QList<QLowEnergyCharacteristic> characteristics = _service->characteristics();

        for (const QLowEnergyCharacteristic &c : characteristics) {
            if (c.uuid() == _config->readCharacteristicUuid() ||
                (c.properties() & QLowEnergyCharacteristic::Notify)) { // Indicate?
                _readCharacteristic = c;
                qCDebug(BluetoothLinkLog) << "Read characteristic found:" << c.uuid().toString();
            }

            if (c.uuid() == _config->writeCharacteristicUuid() ||
                (c.properties() & (QLowEnergyCharacteristic::Write |
                                  QLowEnergyCharacteristic::WriteNoResponse))) {
                _writeCharacteristic = c;
                qCDebug(BluetoothLinkLog) << "Write characteristic found:" << c.uuid().toString();
            }
        }

        if (!_writeCharacteristic.isValid()) {
            emit errorOccurred(tr("Write characteristic not found"));
            return;
        }

        // Enable notifications if available
        if (_readCharacteristic.isValid()) {
            _enableNotifications();
        }

        emit connected();
    }
}

void BluetoothWorker::_enableNotifications()
{
    if (!_service || !_readCharacteristic.isValid()) {
        return;
    }

    const QLowEnergyDescriptor notificationDescriptor = _readCharacteristic.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

    if (notificationDescriptor.isValid()) {
        // Enable notifications
        _service->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
        qCDebug(BluetoothLinkLog) << "Notifications enabled for read characteristic";
    }
}

void BluetoothWorker::_onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                               const QByteArray &value)
{
    if (characteristic.uuid() == _readCharacteristic.uuid() && !value.isEmpty()) {
        emit dataReceived(value);
    }
}

void BluetoothWorker::_onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic,
                                               const QByteArray &value)
{
    Q_UNUSED(characteristic)
    Q_UNUSED(value)
}

void BluetoothWorker::_onServiceError(QLowEnergyService::ServiceError error)
{
    const QString errorString = tr("Service error: %1").arg(error);
    qCWarning(BluetoothLinkLog) << "BLE Service error:" << error;
    emit errorOccurred(errorString);
}

void BluetoothWorker::_reconnectTimeout()
{
    if (!_intentionalDisconnect) {
        qCDebug(BluetoothLinkLog) << "Attempting to reconnect";
        connectLink();
    }
}

/*===========================================================================*/

BluetoothLink::BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _bluetoothConfig(qobject_cast<const BluetoothConfiguration*>(config.get()))
    , _worker(new BluetoothWorker(_bluetoothConfig))
    , _workerThread(new QThread(this))
{
    qCDebug(BluetoothLinkLog) << this;

    _checkPermission();

    const QString threadName = (_bluetoothConfig->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy)
        ? QStringLiteral("BLE_%1") : QStringLiteral("Bluetooth_%1");
    _workerThread->setObjectName(threadName.arg(_bluetoothConfig->name()));

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &BluetoothWorker::setupConnection);
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

    qCDebug(BluetoothLinkLog) << this;
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
    const QString linkType = (_bluetoothConfig->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy)
        ? tr("Bluetooth Low Energy") : tr("Bluetooth");

    qCWarning(BluetoothLinkLog) << "Communication error:" << errorString;
    emit communicationError(tr("%1 Link Error").arg(linkType),
                           tr("Link %1: (Device: %2) %3").arg(_bluetoothConfig->name(),
                                                              _bluetoothConfig->device().name(),
                                                              errorString));
}

void BluetoothLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void BluetoothLink::_onDataSent(const QByteArray &data)
{
    emit bytesSent(this, data);
}

void BluetoothLink::_writeBytes(const QByteArray &bytes)
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
