#include "BluetoothLink.h"

#include <QtCore/QLoggingCategory>
#include "QGCNetworkHelper.h"

#include <QtBluetooth/QBluetoothHostInfo>
#include <QtBluetooth/QLowEnergyConnectionParameters>
#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QThread>
#include <QtCore/QVariantMap>

Q_STATIC_LOGGING_CATEGORY(BluetoothLinkLog, "Comms.BluetoothLink")
Q_STATIC_LOGGING_CATEGORY(BluetoothLinkVerboseLog, "Comms.BluetoothLink:verbose")

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
    , _connectedRssi(copy->connectedRssi())
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

bool BluetoothConfiguration::_createLocalDevice(const QBluetoothAddress &address)
{
    if (_localDevice) {
        _localDevice->deleteLater();
        _localDevice = nullptr;
    }

    _localDevice = new QBluetoothLocalDevice(address, this);

    if (!_localDevice->isValid()) {
        qCWarning(BluetoothLinkLog) << "Failed to initialize Bluetooth adapter:" << address.toString();
        _localDevice->deleteLater();
        _localDevice = nullptr;
        return false;
    }

    _connectLocalDeviceSignals();

    qCDebug(BluetoothLinkLog) << "Initialized Bluetooth adapter:" << _localDevice->name() << _localDevice->address().toString();
    emit adapterStateChanged();
    return true;
}

void BluetoothConfiguration::_connectLocalDeviceSignals()
{
    if (!_localDevice || !_localDevice->isValid()) {
        return;
    }

    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::hostModeStateChanged,
                   this, &BluetoothConfiguration::_onHostModeStateChanged);
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::deviceConnected,
                   this, &BluetoothConfiguration::_onDeviceConnected);
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::deviceDisconnected,
                   this, &BluetoothConfiguration::_onDeviceDisconnected);
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::pairingFinished,
                   this, &BluetoothConfiguration::_onPairingFinished);
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::errorOccurred,
                   this, &BluetoothConfiguration::_onLocalDeviceErrorOccurred);
}

void BluetoothConfiguration::_initDeviceDiscoveryAgent()
{
    if (!_deviceDiscoveryAgent) {
        _deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    }

    if (!_localDevice) {
        const QList<QBluetoothHostInfo> hosts = QBluetoothLocalDevice::allDevices();
        if (!hosts.isEmpty()) {
            _createLocalDevice(hosts.first().address());
        }
    }

    _deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(15000);

    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                   this, &BluetoothConfiguration::_deviceDiscovered);
    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::canceled,
                   this, &BluetoothConfiguration::scanningChanged);
    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::finished,
                   this, &BluetoothConfiguration::_onDiscoveryFinished);
    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::errorOccurred,
                   this, &BluetoothConfiguration::_onDiscoveryErrorOccurred);

    (void) connect(_deviceDiscoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::deviceUpdated,
                   this, &BluetoothConfiguration::_deviceUpdated);
}

void BluetoothConfiguration::_deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{
    qCDebug(BluetoothLinkLog) << "Device Updated:" << info.name() << "Fields:" << updatedFields;

    if (!info.isValid() || (updatedFields == QBluetoothDeviceInfo::Field::None)) {
        return;
    }

    bool found = false;
    for (QBluetoothDeviceInfo &dev : _deviceList) {
        if (dev.address() == info.address()) {
            found = true;
            if (updatedFields & QBluetoothDeviceInfo::Field::RSSI) {
                dev.setRssi(info.rssi());
            }
            if (updatedFields & QBluetoothDeviceInfo::Field::ManufacturerData) {
                const QMultiHash<quint16, QByteArray> data = info.manufacturerData();
                for (quint16 id : info.manufacturerIds()) {
                    dev.setManufacturerData(id, data.value(id));
                }
            }
            if (updatedFields & QBluetoothDeviceInfo::Field::ServiceData) {
                const QMultiHash<QBluetoothUuid, QByteArray> data = info.serviceData();
                for (const QBluetoothUuid &uuid : info.serviceIds()) {
                    dev.setServiceData(uuid, data.value(uuid));
                }
            }
            if (updatedFields & QBluetoothDeviceInfo::Field::All) {
                dev = info;
            }
            emit devicesModelChanged();
            if (_device.address() == dev.address()) {
                emit selectedRssiChanged();
            }
            break;
        }
    }

    // For BLE, require valid RSSI to avoid stale cached devices
    if (!found && _isDeviceCompatibleWithMode(info)) {
        if (_mode != BluetoothMode::ModeLowEnergy || info.rssi() != 0) {
            _deviceList.append(info);
            _nameList.append(info.name());
            emit nameListChanged();
            emit devicesModelChanged();
            if (_device.address() == info.address()) {
                emit selectedRssiChanged();
            }
        }
    }
}

bool BluetoothConfiguration::_isDeviceCompatibleWithMode(const QBluetoothDeviceInfo &info) const
{
    if (_mode == BluetoothMode::ModeLowEnergy) {
        return (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
    } else {
        return (info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateCoreConfiguration) ||
               (info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateAndLowEnergyCoreConfiguration);
    }
}

void BluetoothConfiguration::setMode(BluetoothMode mode)
{
    if (_mode != mode) {
        stopScan();
        _mode = mode;
        emit modeChanged();

        _deviceList.clear();
        _nameList.clear();
        emit nameListChanged();
        emit devicesModelChanged();
    }
}

void BluetoothConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const BluetoothConfiguration *bluetoothSource = qobject_cast<const BluetoothConfiguration*>(source);
    if (!bluetoothSource) {
        qCWarning(BluetoothLinkLog) << "Invalid source configuration type";
        return;
    }

    if (_mode != bluetoothSource->mode()) {
        _mode = bluetoothSource->mode();
        emit modeChanged();
    }

    if (_device.address() != bluetoothSource->device().address()) {
        _device = bluetoothSource->device();
        emit deviceChanged();
    }

    if (_serviceUuid != bluetoothSource->_serviceUuid) {
        _serviceUuid = bluetoothSource->_serviceUuid;
        emit serviceUuidChanged();
    }

    if (_readCharacteristicUuid != bluetoothSource->readCharacteristicUuid()) {
        _readCharacteristicUuid = bluetoothSource->readCharacteristicUuid();
        emit readUuidChanged();
    }

    if (_writeCharacteristicUuid != bluetoothSource->writeCharacteristicUuid()) {
        _writeCharacteristicUuid = bluetoothSource->writeCharacteristicUuid();
        emit writeUuidChanged();
    }

    if (_connectedRssi != bluetoothSource->connectedRssi()) {
        _connectedRssi = bluetoothSource->connectedRssi();
        emit connectedRssiChanged();
    }
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
    if (isBluetoothAvailable()) {
        return (_mode == BluetoothMode::ModeLowEnergy) ? tr("Bluetooth Low Energy Link Settings") : tr("Bluetooth Link Settings");
    }
    return tr("Bluetooth Not Available");
}

bool BluetoothConfiguration::isBluetoothAvailable()
{
    return QGCNetworkHelper::isBluetoothAvailable();
}

QVariantList BluetoothConfiguration::devicesModel() const
{
    QVariantList model;
    for (const QBluetoothDeviceInfo &info : _deviceList) {
        QVariantMap device;
        device["name"] = info.name();
        device["address"] = info.address().toString();
        if (info.rssi() != 0) {
            device["rssi"] = info.rssi();
        }
        model.append(device);
    }
    return model;
}

void BluetoothConfiguration::setConnectedRssi(qint16 rssi)
{
    if (_connectedRssi != rssi) {
        _connectedRssi = rssi;
        emit connectedRssiChanged();
        emit selectedRssiChanged();
    }
}

qint16 BluetoothConfiguration::selectedRssi() const
{
    const QBluetoothAddress selAddr = _device.address();
    if (!selAddr.isNull()) {
        for (const QBluetoothDeviceInfo &dev : _deviceList) {
            if (dev.address() == selAddr) {
                return dev.rssi();
            }
        }
    }
    return 0;
}

void BluetoothConfiguration::setServiceUuid(const QString &uuid)
{
    const QBluetoothUuid newUuid(uuid);
    if (!uuid.isEmpty() && newUuid.isNull()) {
        emit errorOccurred(tr("Invalid service UUID format: %1").arg(uuid));
        return;
    }
    if (_serviceUuid != newUuid) {
        _serviceUuid = newUuid;
        emit serviceUuidChanged();
    }
}

void BluetoothConfiguration::setReadUuid(const QString &uuid)
{
    const QBluetoothUuid newUuid(uuid);
    if (!uuid.isEmpty() && newUuid.isNull()) {
        emit errorOccurred(tr("Invalid read characteristic UUID format: %1").arg(uuid));
        return;
    }
    if (_readCharacteristicUuid != newUuid) {
        _readCharacteristicUuid = newUuid;
        emit readUuidChanged();
    }
}

void BluetoothConfiguration::setWriteUuid(const QString &uuid)
{
    const QBluetoothUuid newUuid(uuid);
    if (!uuid.isEmpty() && newUuid.isNull()) {
        emit errorOccurred(tr("Invalid write characteristic UUID format: %1").arg(uuid));
        return;
    }
    if (_writeCharacteristicUuid != newUuid) {
        _writeCharacteristicUuid = newUuid;
        emit writeUuidChanged();
    }
}

void BluetoothConfiguration::startScan()
{
    if (!_deviceDiscoveryAgent) {
        return;
    }

    {
        QBluetoothPermission permission;
        permission.setCommunicationModes(QBluetoothPermission::Access);
        const Qt::PermissionStatus status = QCoreApplication::instance()->checkPermission(permission);
        if (status == Qt::PermissionStatus::Undetermined) {
            QCoreApplication::instance()->requestPermission(permission, this, [this](const QPermission &perm) {
                if (perm.status() == Qt::PermissionStatus::Granted) {
                    startScan();
                } else {
                    errorOccurred(tr("Bluetooth Permission Denied"));
                }
            });
            return;
        } else if (status != Qt::PermissionStatus::Granted) {
            emit errorOccurred(tr("Bluetooth Permission Denied"));
            return;
        }
    }

    if (_localDevice && _localDevice->isValid() && (_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff)) {
        _localDevice->powerOn();
    }

    _deviceList.clear();
    _nameList.clear();
    emit nameListChanged();
    emit devicesModelChanged();

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
    for (const QBluetoothDeviceInfo &info : std::as_const(_deviceList)) {
        if (info.name() == name) {
            stopScan();
            _applySelectedDevice(info);
            return;
        }
    }
}

void BluetoothConfiguration::setDeviceByAddress(const QString &address)
{
    if (address.isEmpty()) {
        return;
    }
    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        return;
    }

    for (const QBluetoothDeviceInfo &info : std::as_const(_deviceList)) {
        if (info.address() == addr) {
            stopScan();
            _applySelectedDevice(info);
            return;
        }
    }
}

void BluetoothConfiguration::_applySelectedDevice(const QBluetoothDeviceInfo &info)
{
    _device = info;

    if (_mode == BluetoothMode::ModeLowEnergy) {
        const QList<QBluetoothUuid> serviceUuids = info.serviceUuids();
        if (!serviceUuids.isEmpty()) {
            QBluetoothUuid serviceUuid = QBluetoothUuid();
            for (const QBluetoothUuid &uuid : serviceUuids) {
                if ((uuid == NORDIC_UART_SERVICE) || (uuid == TI_SENSORTAG_SERVICE)) {
                    serviceUuid = uuid;

                    if (uuid == NORDIC_UART_SERVICE) {
                        _readCharacteristicUuid = NORDIC_UART_RX_CHAR;
                        _writeCharacteristicUuid = NORDIC_UART_TX_CHAR;
                    } else {
                        _readCharacteristicUuid = TI_SENSORTAG_CHAR;
                        _writeCharacteristicUuid = TI_SENSORTAG_CHAR;
                    }
                    break;
                }
            }

            if (serviceUuid.isNull()) {
                _serviceUuid = serviceUuids.first();
                _readCharacteristicUuid = QBluetoothUuid();
                _writeCharacteristicUuid = QBluetoothUuid();
            } else {
                _serviceUuid = serviceUuid;
            }

            emit serviceUuidChanged();
            emit readUuidChanged();
            emit writeUuidChanged();
        }
    }

    emit deviceChanged();
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

    // Filter out entries with unknown/invalid RSSI. These often come from cached results
    // and cause turned-off devices to appear with no signal.
    if (info.rssi() == 0) {
        return;
    }

    if (!_isDeviceCompatibleWithMode(info)) {
        return;
    }

    int existingIndex = -1;
    for (int i = 0; i < _deviceList.size(); ++i) {
        if (_deviceList[i].address() == info.address()) {
            existingIndex = i;
            break;
        }
    }

    if (existingIndex >= 0) {
        _deviceList[existingIndex] = info;
        emit devicesModelChanged();
        return;
    }

    // BLE-only: collapse duplicates by common device name when private addresses rotate
    if (_mode == BluetoothMode::ModeLowEnergy) {
        int sameNameIndex = -1;
        for (int i = 0; i < _deviceList.size(); ++i) {
            if (_deviceList[i].name() == info.name()) {
                sameNameIndex = i;
                break;
            }
        }
        if (sameNameIndex >= 0) {
            const bool replace = (_deviceList[sameNameIndex].rssi() == 0 && info.rssi() != 0);
            if (replace) {
                _deviceList[sameNameIndex] = info;
                emit devicesModelChanged();
            }
            return;
        }
    }

    _deviceList.append(info);
    _nameList.append(info.name());
    emit nameListChanged();
    emit devicesModelChanged();

    qCDebug(BluetoothLinkLog) << ((_mode == BluetoothMode::ModeLowEnergy) ? "BLE" : "Classic")
                              << "device discovered:" << info.name()
                              << "Address:" << info.address().toString()
                              << "RSSI:" << info.rssi();
}

void BluetoothConfiguration::_onDiscoveryFinished()
{
    emit scanningChanged();
    _updateDeviceList();
}

void BluetoothConfiguration::_updateDeviceList()
{
    _nameList.clear();
    for (const QBluetoothDeviceInfo &info : std::as_const(_deviceList)) {
        _nameList.append(info.name());
    }
    emit nameListChanged();
    emit devicesModelChanged();
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

void BluetoothConfiguration::_onHostModeStateChanged(QBluetoothLocalDevice::HostMode mode)
{
    QString modeString;
    switch (mode) {
    case QBluetoothLocalDevice::HostPoweredOff:
        modeString = tr("Powered Off");
        break;
    case QBluetoothLocalDevice::HostConnectable:
        modeString = tr("Connectable");
        break;
    case QBluetoothLocalDevice::HostDiscoverable:
        modeString = tr("Discoverable");
        break;
    case QBluetoothLocalDevice::HostDiscoverableLimitedInquiry:
        modeString = tr("Discoverable (Limited Inquiry)");
        break;
    }

    qCDebug(BluetoothLinkLog) << "Bluetooth adapter mode changed to:" << modeString;
    emit adapterStateChanged();

    if ((mode == QBluetoothLocalDevice::HostPoweredOff) && scanning()) {
        stopScan();
        emit errorOccurred(tr("Bluetooth adapter powered off"));
    }
}

void BluetoothConfiguration::_onDeviceConnected(const QBluetoothAddress &address)
{
    qCDebug(BluetoothLinkLog) << "Device connected to adapter:" << address.toString();
    for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
        if (dev.address() == address) {
            qCDebug(BluetoothLinkLog) << "Connected device:" << dev.name();
            break;
        }
    }
}

void BluetoothConfiguration::_onDeviceDisconnected(const QBluetoothAddress &address)
{
    qCDebug(BluetoothLinkLog) << "Device disconnected from adapter:" << address.toString();
    for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
        if (dev.address() == address) {
            qCDebug(BluetoothLinkLog) << "Disconnected device:" << dev.name();
            break;
        }
    }
}

void BluetoothConfiguration::_onPairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing)
{
    QString pairingStatus;
    switch (pairing) {
    case QBluetoothLocalDevice::Unpaired:
        pairingStatus = tr("Unpaired");
        break;
    case QBluetoothLocalDevice::Paired:
        pairingStatus = tr("Paired");
        break;
    case QBluetoothLocalDevice::AuthorizedPaired:
        pairingStatus = tr("Authorized Paired");
        break;
    }

    qCDebug(BluetoothLinkLog) << "Pairing finished for device:" << address.toString() << "Status:" << pairingStatus;

    QString deviceName;
    for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
        if (dev.address() == address) {
            deviceName = dev.name();
            break;
        }
    }

    if (pairing == QBluetoothLocalDevice::Unpaired) {
        const QString msg = deviceName.isEmpty()
            ? tr("Device %1 unpaired").arg(address.toString())
            : tr("Device %1 (%2) unpaired").arg(deviceName, address.toString());
        qCInfo(BluetoothLinkLog) << msg;
    } else {
        const QString msg = deviceName.isEmpty()
            ? tr("Device %1 paired successfully").arg(address.toString())
            : tr("Device %1 (%2) paired successfully").arg(deviceName, address.toString());
        qCInfo(BluetoothLinkLog) << msg;
    }

    emit devicesModelChanged();
    emit pairingStatusChanged();
}

void BluetoothConfiguration::_onLocalDeviceErrorOccurred(QBluetoothLocalDevice::Error error)
{
    QString errorString;
    switch (error) {
    case QBluetoothLocalDevice::PairingError:
        errorString = tr("Pairing Error");
        break;
    case QBluetoothLocalDevice::MissingPermissionsError:
        errorString = tr("Missing Bluetooth Permissions");
        break;
    case QBluetoothLocalDevice::UnknownError:
    default:
        errorString = tr("Unknown Bluetooth Adapter Error");
        break;
    }

    qCWarning(BluetoothLinkLog) << "Local Bluetooth device error:" << error << errorString;
    emit errorOccurred(errorString);
}

void BluetoothConfiguration::requestPairing(const QString &address)
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    if (_mode != BluetoothMode::ModeClassic) {
        emit errorOccurred(tr("Pairing is only supported for Classic Bluetooth"));
        return;
    }

    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        emit errorOccurred(tr("Invalid Bluetooth address"));
        return;
    }

    qCDebug(BluetoothLinkLog) << "Requesting pairing with device:" << address;
    _localDevice->requestPairing(addr, QBluetoothLocalDevice::Paired);
}

void BluetoothConfiguration::removePairing(const QString &address)
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    if (_mode != BluetoothMode::ModeClassic) {
        emit errorOccurred(tr("Unpairing is only supported for Classic Bluetooth"));
        return;
    }

    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        emit errorOccurred(tr("Invalid Bluetooth address"));
        return;
    }

    qCDebug(BluetoothLinkLog) << "Removing pairing with device:" << address;
    _localDevice->requestPairing(addr, QBluetoothLocalDevice::Unpaired);
}

QString BluetoothConfiguration::getPairingStatus(const QString &address) const
{
    if (!_localDevice || !_localDevice->isValid()) {
        return tr("Adapter unavailable");
    }

    if (_mode != BluetoothMode::ModeClassic) {
        return tr("N/A (BLE mode)");
    }

    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        return tr("Invalid address");
    }

    const QBluetoothLocalDevice::Pairing pairingStatus = _localDevice->pairingStatus(addr);
    switch (pairingStatus) {
    case QBluetoothLocalDevice::Unpaired:
        return tr("Unpaired");
    case QBluetoothLocalDevice::Paired:
        return tr("Paired");
    case QBluetoothLocalDevice::AuthorizedPaired:
        return tr("Authorized Paired");
    default:
        return tr("Unknown");
    }
}

bool BluetoothConfiguration::isAdapterAvailable() const
{
    return _localDevice && _localDevice->isValid();
}

QString BluetoothConfiguration::getAdapterAddress() const
{
    if (!_localDevice || !_localDevice->isValid()) {
        return QString();
    }
    return _localDevice->address().toString();
}

QString BluetoothConfiguration::getAdapterName() const
{
    if (!_localDevice || !_localDevice->isValid()) {
        return QString();
    }
    return _localDevice->name();
}

bool BluetoothConfiguration::isAdapterPoweredOn() const
{
    if (!_localDevice || !_localDevice->isValid()) {
        return false;
    }
    return _localDevice->hostMode() != QBluetoothLocalDevice::HostPoweredOff;
}

QVariantList BluetoothConfiguration::getAllPairedDevices() const
{
    QVariantList pairedDevices;

    if (!_localDevice || !_localDevice->isValid()) {
        return pairedDevices;
    }

    const QList<QBluetoothAddress> pairedAddresses = _localDevice->connectedDevices();

    for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
        const QBluetoothLocalDevice::Pairing pairingStatus = _localDevice->pairingStatus(dev.address());
        if (pairingStatus != QBluetoothLocalDevice::Unpaired) {
            QVariantMap device;
            device["name"] = dev.name();
            device["address"] = dev.address().toString();
            device["paired"] = true;

            if (pairingStatus == QBluetoothLocalDevice::AuthorizedPaired) {
                device["pairingStatus"] = tr("Authorized");
            } else {
                device["pairingStatus"] = tr("Paired");
            }

            device["connected"] = pairedAddresses.contains(dev.address());

            if (dev.rssi() != 0) {
                device["rssi"] = dev.rssi();
            }

            pairedDevices.append(device);
        }
    }

    return pairedDevices;
}

QVariantList BluetoothConfiguration::getConnectedDevices() const
{
    QVariantList connectedDevices;

    if (!_localDevice || !_localDevice->isValid()) {
        return connectedDevices;
    }

    const QList<QBluetoothAddress> connectedAddresses = _localDevice->connectedDevices();

    for (const QBluetoothAddress &addr : connectedAddresses) {
        QVariantMap device;
        device["address"] = addr.toString();
        device["connected"] = true;

        QString deviceName;
        for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
            if (dev.address() == addr) {
                deviceName = dev.name();
                if (dev.rssi() != 0) {
                    device["rssi"] = dev.rssi();
                }
                break;
            }
        }

        device["name"] = deviceName.isEmpty() ? tr("Unknown Device") : deviceName;
        connectedDevices.append(device);
    }

    return connectedDevices;
}

void BluetoothConfiguration::powerOnAdapter()
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    qCDebug(BluetoothLinkLog) << "Powering on Bluetooth adapter";
    _localDevice->powerOn();
}

void BluetoothConfiguration::powerOffAdapter()
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    if (scanning()) {
        stopScan();
    }

    qCDebug(BluetoothLinkLog) << "Powering off Bluetooth adapter";
    _localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
}

void BluetoothConfiguration::setAdapterDiscoverable(bool discoverable)
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    if (discoverable) {
        qCDebug(BluetoothLinkLog) << "Making Bluetooth adapter discoverable";
        _localDevice->setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    } else {
        qCDebug(BluetoothLinkLog) << "Making Bluetooth adapter connectable only";
        _localDevice->setHostMode(QBluetoothLocalDevice::HostConnectable);
    }
}

QVariantList BluetoothConfiguration::getAllAvailableAdapters() const
{
    QVariantList adapters;
    const QList<QBluetoothHostInfo> hosts = QBluetoothLocalDevice::allDevices();

    for (const QBluetoothHostInfo &host : hosts) {
        QVariantMap adapter;
        adapter["name"] = host.name();
        adapter["address"] = host.address().toString();

        if (_localDevice && _localDevice->isValid()) {
            adapter["selected"] = (host.address() == _localDevice->address());
        } else {
            adapter["selected"] = false;
        }

        adapters.append(adapter);
    }

    return adapters;
}

void BluetoothConfiguration::selectAdapter(const QString &address)
{
    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        emit errorOccurred(tr("Invalid adapter address"));
        return;
    }

    const QList<QBluetoothHostInfo> hosts = QBluetoothLocalDevice::allDevices();
    bool found = false;
    for (const QBluetoothHostInfo &host : hosts) {
        if (host.address() == addr) {
            found = true;
            break;
        }
    }

    if (!found) {
        emit errorOccurred(tr("Adapter not found"));
        return;
    }

    if (scanning()) {
        stopScan();
    }

    if (!_createLocalDevice(addr)) {
        emit errorOccurred(tr("Failed to initialize adapter"));
    }
}

QString BluetoothConfiguration::getHostMode() const
{
    if (!_localDevice || !_localDevice->isValid()) {
        return tr("Unavailable");
    }

    switch (_localDevice->hostMode()) {
    case QBluetoothLocalDevice::HostPoweredOff:
        return tr("Powered Off");
    case QBluetoothLocalDevice::HostConnectable:
        return tr("Connectable");
    case QBluetoothLocalDevice::HostDiscoverable:
        return tr("Discoverable");
    case QBluetoothLocalDevice::HostDiscoverableLimitedInquiry:
        return tr("Discoverable (Limited)");
    default:
        return tr("Unknown");
    }
}

/*===========================================================================*/

BluetoothWorker::BluetoothWorker(const BluetoothConfiguration *config, QObject *parent)
    : QObject(parent)
    , _mode(config->mode())
    , _device(config->device())
    , _serviceUuid(config->serviceUuid())
    , _readCharacteristicUuid(config->readCharacteristicUuid())
    , _writeCharacteristicUuid(config->writeCharacteristicUuid())
    , _reconnectTimer(new QTimer(this))
{
    qCDebug(BluetoothLinkLog) << this;

    _reconnectTimer->setInterval(RECONNECT_BASE_INTERVAL_MS);
    _reconnectTimer->setSingleShot(true);
    (void) connect(_reconnectTimer.data(), &QTimer::timeout, this, &BluetoothWorker::_reconnectTimeout);

    _rssiTimer = new QTimer(this);
    _rssiTimer->setInterval(RSSI_POLL_INTERVAL_MS);
    _rssiTimer->setSingleShot(false);
    (void) connect(_rssiTimer.data(), &QTimer::timeout, this, [this]() {
        if (_controller && (_controller->state() == QLowEnergyController::ConnectedState)) {
            _controller->readRssi();
        }
    });

    _serviceDiscoveryTimer = new QTimer(this);
    _serviceDiscoveryTimer->setInterval(SERVICE_DISCOVERY_TIMEOUT_MS);
    _serviceDiscoveryTimer->setSingleShot(true);
    (void) connect(_serviceDiscoveryTimer.data(), &QTimer::timeout, this, &BluetoothWorker::_serviceDiscoveryTimeout);
}

BluetoothWorker::~BluetoothWorker()
{
    _intentionalDisconnect = true;

    // Disconnect the Bluetooth device first
    disconnectLink();

    // Disconnect all Qt signals
    QObject::disconnect();

    if (_reconnectTimer) {
        _reconnectTimer->stop();
    }
    if (_rssiTimer) {
        _rssiTimer->stop();
    }
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    if (_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        if (_service) {
            _service->disconnect();
            _service->deleteLater();
        }
        if (_controller) {
            _controller->disconnect();
            if (_controller->state() != QLowEnergyController::UnconnectedState) {
                _controller->disconnectFromDevice();
            }
            _controller->deleteLater();
        }
    } else {
        if (_socket) {
            _socket->disconnect();
            if (_socket->state() == QBluetoothSocket::SocketState::ConnectedState) {
                _socket->disconnectFromService();
            }
            _socket->deleteLater();
        }
    }

    qCDebug(BluetoothLinkLog) << this;
}

bool BluetoothWorker::isConnected() const
{
    return _connected.load();
}

void BluetoothWorker::setupConnection()
{
    if (_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        _setupBleController();
    } else {
        _setupClassicSocket();
    }
}

void BluetoothWorker::_setupClassicSocket()
{
    if (!_socket) {
        _socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    }

    (void) connect(_socket.data(), &QBluetoothSocket::connected, this, &BluetoothWorker::_onSocketConnected);
    (void) connect(_socket.data(), &QBluetoothSocket::disconnected, this, &BluetoothWorker::_onSocketDisconnected);
    (void) connect(_socket.data(), &QBluetoothSocket::readyRead, this, &BluetoothWorker::_onSocketReadyRead);
    (void) connect(_socket.data(), &QBluetoothSocket::errorOccurred, this, &BluetoothWorker::_onSocketErrorOccurred);

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_socket.data(), &QBluetoothSocket::bytesWritten, this, &BluetoothWorker::_onSocketBytesWritten);

        (void) connect(_socket.data(), &QBluetoothSocket::stateChanged, this,
                      [this](QBluetoothSocket::SocketState state) {
            qCDebug(BluetoothLinkLog) << "Bluetooth Socket State Changed:" << state;
        });
    }
}

void BluetoothWorker::_setupBleController()
{
    if (_controller) {
        qCWarning(BluetoothLinkLog) << "BLE controller already exists, skipping setup";
        return;
    }

    _controller = QLowEnergyController::createCentral(_device, this);

    if (!_controller) {
        emit errorOccurred(tr("Failed to create BLE controller"));
        return;
    }

    (void) connect(_controller.data(), &QLowEnergyController::connected, this, &BluetoothWorker::_onControllerConnected);
    (void) connect(_controller.data(), &QLowEnergyController::disconnected, this, &BluetoothWorker::_onControllerDisconnected);
    (void) connect(_controller.data(), &QLowEnergyController::errorOccurred, this, &BluetoothWorker::_onControllerErrorOccurred);
    (void) connect(_controller.data(), &QLowEnergyController::serviceDiscovered, this, &BluetoothWorker::_onServiceDiscovered);
    (void) connect(_controller.data(), &QLowEnergyController::discoveryFinished, this, &BluetoothWorker::_onServiceDiscoveryFinished);

    (void) connect(_controller.data(), &QLowEnergyController::mtuChanged, this,
                  [this](int mtu) {
                      _mtu = mtu;
                      qCDebug(BluetoothLinkLog) << "MTU changed to:" << mtu;
                  });
    (void) connect(_controller.data(), &QLowEnergyController::rssiRead, this,
                  [this](qint16 rssi) {
                      _rssi = rssi;
                      emit rssiUpdated(rssi);
                  });

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_controller.data(), &QLowEnergyController::stateChanged, this,
                      [](QLowEnergyController::ControllerState state) {
            qCDebug(BluetoothLinkLog) << "BLE Controller State Changed:" << state;
        });

        (void) connect(_controller.data(), &QLowEnergyController::connectionUpdated, this,
                  [](const QLowEnergyConnectionParameters &params) {
            qCDebug(BluetoothLinkLog) << "BLE connection updated: min(ms)" << params.minimumInterval()
                                      << "max(ms)" << params.maximumInterval()
                                      << "latency" << params.latency()
                                      << "supervision(ms)" << params.supervisionTimeout();
        });
    }
}

void BluetoothWorker::connectLink()
{
    _intentionalDisconnect = false;

    if (isConnected()) {
        qCWarning(BluetoothLinkLog) << "Already connected to" << _device.name();
        return;
    }

    qCDebug(BluetoothLinkLog) << "Attempting to connect to" << _device.name()
                              << "Mode:" << ((_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) ? "BLE" : "Classic");

    if (_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        if (!_controller) {
            _setupBleController();
        }

        if (!_controller) {
            emit errorOccurred(tr("BLE controller not available"));
            return;
        }

        // Note: Don't call setRemoteAddressType() - Qt 6's createCentral() with
        // QBluetoothDeviceInfo automatically determines the correct address type
        // from discovery. Hardcoding PublicAddress breaks privacy-enabled devices.
        _controller->connectToDevice();
    } else {
        if (!_socket) {
            _setupClassicSocket();
        }

        if (!_socket) {
            emit errorOccurred(tr("Socket not available"));
            return;
        }

        static const QBluetoothUuid uuid = QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
        _socket->connectToService(_device.address(), uuid);
    }
}

void BluetoothWorker::disconnectLink()
{
    _intentionalDisconnect = true;

    if (_reconnectTimer) {
        _reconnectTimer->stop();
        _reconnectTimer->setInterval(RECONNECT_BASE_INTERVAL_MS);
    }
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }
    _reconnectAttempts = 0;

    if (_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        _clearBleWriteQueue();

        if (_service) {
            _service->disconnect();
            _service->deleteLater();
            _service = nullptr;
        }

        _readCharacteristic = QLowEnergyCharacteristic();
        _writeCharacteristic = QLowEnergyCharacteristic();

        if (_controller) {
            _controller->disconnectFromDevice();
        }
    } else {
        if (_classicDiscovery && _classicDiscovery->isActive()) {
            _classicDiscovery->stop();
        }

        if (_socket) {
            _socket->disconnectFromService();
        }
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

    if (_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
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

    // ATT MTU payload is (MTU - 3)
    const int effectiveMtu = (_mtu > 3) ? (_mtu - 3) : BLE_MIN_PACKET_SIZE;
    const int packetSize = qBound(BLE_MIN_PACKET_SIZE, effectiveMtu, BLE_MAX_PACKET_SIZE);

    const bool useWriteWithoutResponse = (_writeCharacteristic.properties() & QLowEnergyCharacteristic::WriteNoResponse);
    const int numChunks = (data.size() + packetSize - 1) / packetSize;

    qCDebug(BluetoothLinkLog) << _device.name() << "BLE Write:" << data.size() << "bytes,"
                              << numChunks << "chunks (MTU:" << _mtu << ", packet:" << packetSize << ")"
                              << (useWriteWithoutResponse ? "WriteNoResponse" : "WriteWithResponse");

    if (useWriteWithoutResponse) {
        for (int i = 0; i < data.size(); i += packetSize) {
            const QByteArray chunk = data.mid(i, packetSize);
            _service->writeCharacteristic(_writeCharacteristic, chunk, QLowEnergyService::WriteWithoutResponse);
        }
        emit dataSent(data);
    } else {
        // WriteWithResponse: queue chunks and wait for confirmations
        const int chunksNeeded = (data.size() + packetSize - 1) / packetSize;
        if (_bleWriteQueue.size() + chunksNeeded > MAX_BLE_QUEUE_SIZE) {
            qCWarning(BluetoothLinkLog) << "BLE write queue full:" << _bleWriteQueue.size() << "chunks queued";
            emit errorOccurred(tr("Write queue full, dropping data"));
            return;
        }

        _currentBleWrite = data;
        for (int i = 0; i < data.size(); i += packetSize) {
            _bleWriteQueue.enqueue(data.mid(i, packetSize));
        }

        if (!_bleWriteInProgress) {
            _processNextBleWrite();
        }
    }
}

void BluetoothWorker::_processNextBleWrite()
{
    if (_bleWriteQueue.isEmpty()) {
        _bleWriteInProgress = false;
        if (!_currentBleWrite.isEmpty()) {
            emit dataSent(_currentBleWrite);
            _currentBleWrite.clear();
        }
        return;
    }

    if (!_service || !_writeCharacteristic.isValid()) {
        _clearBleWriteQueue();
        return;
    }

    _bleWriteInProgress = true;
    const QByteArray chunk = _bleWriteQueue.dequeue();
    _service->writeCharacteristic(_writeCharacteristic, chunk);
}

void BluetoothWorker::_clearBleWriteQueue()
{
    _bleWriteQueue.clear();
    _currentBleWrite.clear();
    _bleWriteInProgress = false;
}

void BluetoothWorker::_onSocketConnected()
{
    qCDebug(BluetoothLinkLog) << "=== Classic Bluetooth Connection Established ===" << _device.name();
    qCDebug(BluetoothLinkLog) << "  Address:" << _device.address().toString();
    qCDebug(BluetoothLinkLog) << "  Protocol: RFCOMM (SPP)";

    _connected = true;
    _reconnectAttempts = 0;
    _consecutiveFailures = 0;
    emit connected();
}

void BluetoothWorker::_onSocketDisconnected()
{
    qCDebug(BluetoothLinkLog) << "Socket disconnected from device:" << _device.name();
    _connected = false;
    emit disconnected();

    if (!_intentionalDisconnect.load() && _socket && _reconnectTimer) {
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
        qCDebug(BluetoothLinkVerboseLog) << _device.name() << "Received" << data.size() << "bytes";
        emit dataReceived(data);
    }
}

void BluetoothWorker::_onSocketBytesWritten(qint64 bytes)
{
    qCDebug(BluetoothLinkVerboseLog) << _device.name() << "Wrote" << bytes << "bytes";
}

void BluetoothWorker::_onSocketErrorOccurred(QBluetoothSocket::SocketError socketError)
{
    QString errorString;
    if (_socket) {
        errorString = _socket->errorString();
    } else {
        errorString = tr("Socket error: Null Socket");
    }

    qCWarning(BluetoothLinkLog) << "Socket error:" << socketError << errorString;
    emit errorOccurred(errorString);

    // Track consecutive failures for Classic Bluetooth too
    _consecutiveFailures++;

    // If we never successfully connected, emit disconnected to reset UI state
    if (!_connected.load()) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
        emit disconnected();
    }

    // Attempt service discovery for service-related errors only
    if (_mode == BluetoothConfiguration::BluetoothMode::ModeClassic) {
        if ((socketError == QBluetoothSocket::SocketError::ServiceNotFoundError) ||
            (socketError == QBluetoothSocket::SocketError::UnsupportedProtocolError)) {
            qCDebug(BluetoothLinkLog) << "Service-related error, attempting fallback discovery";
            _startClassicServiceDiscovery();
        }
    }
}

void BluetoothWorker::_startClassicServiceDiscovery()
{
    if (_classicDiscovery && _classicDiscovery->isActive()) {
        return;
    }

    if (!_classicDiscovery) {
        _classicDiscovery = new QBluetoothServiceDiscoveryAgent(_device.address(), this);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::serviceDiscovered,
                       this, &BluetoothWorker::_onClassicServiceDiscovered);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::finished,
                       this, &BluetoothWorker::_onClassicServiceDiscoveryFinished);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::canceled,
                       this, &BluetoothWorker::_onClassicServiceDiscoveryCanceled);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::errorOccurred,
                       this, &BluetoothWorker::_onClassicServiceDiscoveryError);
    }

    qCDebug(BluetoothLinkLog) << "Starting classic service discovery on" << _device.name();
    _classicDiscoveredService = QBluetoothServiceInfo();
    _classicDiscovery->start(QBluetoothServiceDiscoveryAgent::FullDiscovery);
}

void BluetoothWorker::_onClassicServiceDiscovered(const QBluetoothServiceInfo &serviceInfo)
{
    qCDebug(BluetoothLinkLog) << "Classic service discovered: UUIDs=" << serviceInfo.serviceClassUuids()
                              << "RFCOMM channel=" << serviceInfo.serverChannel()
                              << "L2CAP PSM=" << serviceInfo.protocolServiceMultiplexer();

    const QList<QBluetoothUuid> serviceUuids = serviceInfo.serviceClassUuids();
    const bool isSerial = serviceUuids.contains(QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort));
    const bool hasRfcommChannel = serviceInfo.serverChannel() > 0;

    if (isSerial || (hasRfcommChannel && !_classicDiscoveredService.isValid())) {
        _classicDiscoveredService = serviceInfo;
    }
}

void BluetoothWorker::_onClassicServiceDiscoveryFinished()
{
    if (!_classicDiscoveredService.isValid()) {
        qCWarning(BluetoothLinkLog) << "No suitable classic service found";
        return;
    }

    if (!_socket) {
        _setupClassicSocket();
    }

    qCDebug(BluetoothLinkLog) << "Connecting using discovered service";
    _socket->connectToService(_classicDiscoveredService);
}

void BluetoothWorker::_onClassicServiceDiscoveryCanceled()
{
    qCDebug(BluetoothLinkLog) << "Classic service discovery canceled";
}

void BluetoothWorker::_onClassicServiceDiscoveryError(QBluetoothServiceDiscoveryAgent::Error error)
{
    const QString e = _classicDiscovery ? _classicDiscovery->errorString() : tr("Service discovery error: %1").arg(error);
    qCWarning(BluetoothLinkLog) << e;
    emit errorOccurred(e);
}

void BluetoothWorker::_onControllerConnected()
{
    qCDebug(BluetoothLinkLog) << "BLE Controller connected to device:" << _device.name();

    if (_controller) {
        if (_serviceDiscoveryTimer) {
            _serviceDiscoveryTimer->start();
        }
        _controller->discoverServices();
        if (_rssiTimer) {
            _rssiTimer->start();
        }
    }
}

void BluetoothWorker::_onControllerDisconnected()
{
    qCDebug(BluetoothLinkLog) << "BLE Controller disconnected from device:" << _device.name();

    if (_rssiTimer) {
        _rssiTimer->stop();
        _rssi = 0;
        emit rssiUpdated(_rssi);
    }

    _clearBleWriteQueue();

    if (_service) {
        _service->deleteLater();
        _service = nullptr;
    }

    _readCharacteristic = QLowEnergyCharacteristic();
    _writeCharacteristic = QLowEnergyCharacteristic();
    _connected = false;

    emit disconnected();

    if (!_intentionalDisconnect.load() && _controller && _reconnectTimer) {
        qCDebug(BluetoothLinkLog) << "Starting reconnect timer";
        _reconnectTimer->start();
    }
}

void BluetoothWorker::_onControllerErrorOccurred(QLowEnergyController::Error error)
{
    // Stop service discovery timer to prevent timeout firing after error
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    QString errorString;
    if (_controller) {
        errorString = _controller->errorString();
    } else {
        errorString = tr("Controller error: %1").arg(error);
    }

    qCWarning(BluetoothLinkLog) << "BLE Controller error:" << error << errorString;
    emit errorOccurred(errorString);

    if (!_connected.load()) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
        emit disconnected();
    }
}

void BluetoothWorker::_onServiceDiscovered(const QBluetoothUuid &uuid)
{
    qCDebug(BluetoothLinkLog) << "Service discovered:" << uuid.toString();

    const QBluetoothUuid targetUuid = QBluetoothUuid(_serviceUuid);
    if ((uuid == targetUuid) || (targetUuid.isNull())) {
        if (_serviceDiscoveryTimer) {
            _serviceDiscoveryTimer->stop();
        }
        _setupBleService();
    }
}

void BluetoothWorker::_onServiceDiscoveryFinished()
{
    qCDebug(BluetoothLinkLog) << "Service discovery finished";

    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    if (!_service && _controller) {
        const QList<QBluetoothUuid> services = _controller->services();
        if (!services.isEmpty()) {
            qCDebug(BluetoothLinkLog) << "Using first available service";
            _setupBleService();
        } else {
            _consecutiveFailures++;
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

    QBluetoothUuid serviceUuid = QBluetoothUuid(_serviceUuid);

    if (serviceUuid.isNull()) {
        const QList<QBluetoothUuid> services = _controller->services();
        if (services.isEmpty()) {
            emit errorOccurred(tr("No services available"));
            return;
        }

        if (services.contains(BluetoothConfiguration::NORDIC_UART_SERVICE)) {
            serviceUuid = BluetoothConfiguration::NORDIC_UART_SERVICE;
            qCDebug(BluetoothLinkLog) << "Auto-selected Nordic UART service";
        } else if (services.contains(BluetoothConfiguration::TI_SENSORTAG_SERVICE)) {
            serviceUuid = BluetoothConfiguration::TI_SENSORTAG_SERVICE;
            qCDebug(BluetoothLinkLog) << "Auto-selected TI SensorTag service";
        } else {
            serviceUuid = services.first();
            qCDebug(BluetoothLinkLog) << "Auto-selected first available service:" << serviceUuid.toString();
        }
    }

    _service = _controller->createServiceObject(serviceUuid, this);

    if (!_service) {
        emit errorOccurred(tr("Failed to create service object"));
        return;
    }

    (void) connect(_service.data(), &QLowEnergyService::stateChanged, this, &BluetoothWorker::_onServiceStateChanged);
    (void) connect(_service.data(), &QLowEnergyService::characteristicChanged, this, &BluetoothWorker::_onCharacteristicChanged);
    (void) connect(_service.data(), &QLowEnergyService::characteristicRead, this, &BluetoothWorker::_onCharacteristicRead);
    (void) connect(_service.data(), &QLowEnergyService::characteristicWritten, this, &BluetoothWorker::_onCharacteristicWritten);
    (void) connect(_service.data(), &QLowEnergyService::descriptorRead, this, &BluetoothWorker::_onDescriptorRead);
    (void) connect(_service.data(), &QLowEnergyService::descriptorWritten, this, &BluetoothWorker::_onDescriptorWritten);
    (void) connect(_service.data(), &QLowEnergyService::errorOccurred, this, &BluetoothWorker::_onServiceError);

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

static QString characteristicPropertiesToString(QLowEnergyCharacteristic::PropertyTypes props)
{
    QStringList propList;
    if (props & QLowEnergyCharacteristic::Read) propList << "Read";
    if (props & QLowEnergyCharacteristic::Write) propList << "Write";
    if (props & QLowEnergyCharacteristic::WriteNoResponse) propList << "WriteNoResponse";
    if (props & QLowEnergyCharacteristic::Notify) propList << "Notify";
    if (props & QLowEnergyCharacteristic::Indicate) propList << "Indicate";
    if (props & QLowEnergyCharacteristic::WriteSigned) propList << "WriteSigned";
    if (props & QLowEnergyCharacteristic::ExtendedProperty) propList << "Extended";
    return propList.isEmpty() ? "None" : propList.join("|");
}

void BluetoothWorker::_findCharacteristics()
{
    if (!_service) {
        return;
    }

    const QList<QLowEnergyCharacteristic> characteristics = _service->characteristics();
    qCDebug(BluetoothLinkLog) << "Service has" << characteristics.size() << "characteristics";

    for (const QLowEnergyCharacteristic &c : characteristics) {
        qCDebug(BluetoothLinkLog) << "  Characteristic:" << c.uuid().toString()
                                  << "Properties:" << characteristicPropertiesToString(c.properties());
    }

    const QBluetoothUuid readUuid = _readCharacteristicUuid;
    const QBluetoothUuid writeUuid = _writeCharacteristicUuid;

    for (const QLowEnergyCharacteristic &c : characteristics) {
        if (!readUuid.isNull() && (c.uuid() == readUuid)) {
            _readCharacteristic = c;
            qCDebug(BluetoothLinkLog) << "Read characteristic found by UUID:" << c.uuid().toString();
        }
        if (!writeUuid.isNull() && (c.uuid() == writeUuid)) {
            _writeCharacteristic = c;
            qCDebug(BluetoothLinkLog) << "Write characteristic found by UUID:" << c.uuid().toString();
        }
    }

    if (!_readCharacteristic.isValid() || !_writeCharacteristic.isValid()) {
        for (const QLowEnergyCharacteristic &c : characteristics) {
            if (!_readCharacteristic.isValid() &&
                (c.properties() & (QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify))) {
                _readCharacteristic = c;
                qCDebug(BluetoothLinkLog) << "Read characteristic auto-detected:" << c.uuid().toString();
            }

            if (!_writeCharacteristic.isValid() &&
                (c.properties() & (QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::WriteNoResponse))) {
                _writeCharacteristic = c;
                qCDebug(BluetoothLinkLog) << "Write characteristic auto-detected:" << c.uuid().toString();
            }
        }
    }

    // Summary of what we found
    if (_readCharacteristic.isValid()) {
        qCDebug(BluetoothLinkLog) << "Using read characteristic:" << _readCharacteristic.uuid().toString()
                                  << "Properties:" << characteristicPropertiesToString(_readCharacteristic.properties());
    } else {
        qCDebug(BluetoothLinkLog) << "No read characteristic found";
    }

    if (_writeCharacteristic.isValid()) {
        qCDebug(BluetoothLinkLog) << "Using write characteristic:" << _writeCharacteristic.uuid().toString()
                                  << "Properties:" << characteristicPropertiesToString(_writeCharacteristic.properties());
    } else {
        qCDebug(BluetoothLinkLog) << "No write characteristic found";
    }

    // Warn if read and write resolved to the same characteristic
    if (_readCharacteristic.isValid() && _writeCharacteristic.isValid() &&
        _readCharacteristic.uuid() == _writeCharacteristic.uuid()) {
        qCWarning(BluetoothLinkLog) << "Read and write characteristics resolved to the same UUID:"
                                    << _readCharacteristic.uuid().toString()
                                    << "- bidirectional communication may not work as intended";
    }
}

void BluetoothWorker::_onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qCDebug(BluetoothLinkLog) << "Service state changed:" << state;

    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        if (!_service) {
            return;
        }

        _findCharacteristics();

        if (!_writeCharacteristic.isValid()) {
            emit errorOccurred(tr("Write characteristic not found"));
            return;
        }

        if (_readCharacteristic.isValid()) {
            _enableNotifications();
        }

        // Request optimized connection parameters (not supported on iOS, may not emit signal on Android)
#ifndef Q_OS_IOS
        if (_controller && _controller->state() == QLowEnergyController::ConnectedState) {
            QLowEnergyConnectionParameters params;
            params.setIntervalRange(6, 12);
            params.setLatency(0);
            params.setSupervisionTimeout(500);
            _controller->requestConnectionUpdate(params);
        }
#endif

        _connected = true;
        _reconnectAttempts = 0;
        _consecutiveFailures = 0;

        qCDebug(BluetoothLinkLog) << "=== BLE Connection Established ===" << _device.name();
        qCDebug(BluetoothLinkLog) << "  Service:" << _service->serviceUuid().toString();
        qCDebug(BluetoothLinkLog) << "  MTU:" << _mtu;
        qCDebug(BluetoothLinkLog) << "  Read:" << (_readCharacteristic.isValid() ? _readCharacteristic.uuid().toString() : "N/A");
        qCDebug(BluetoothLinkLog) << "  Write:" << (_writeCharacteristic.isValid() ? _writeCharacteristic.uuid().toString() : "N/A");

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
        QByteArray value;
        if (_readCharacteristic.properties() & QLowEnergyCharacteristic::Notify) {
            value = QByteArray::fromHex("0100");
            qCDebug(BluetoothLinkLog) << "Enabling notifications for read characteristic";
        } else if (_readCharacteristic.properties() & QLowEnergyCharacteristic::Indicate) {
            value = QByteArray::fromHex("0200");
            qCDebug(BluetoothLinkLog) << "Enabling indications for read characteristic";
        }

        if (!value.isEmpty()) {
            _service->writeDescriptor(notificationDescriptor, value);
        }
    }
}

void BluetoothWorker::_onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                               const QByteArray &value)
{
    if ((characteristic.uuid() == _readCharacteristic.uuid()) && !value.isEmpty()) {
        qCDebug(BluetoothLinkLog) << _device.name() << "BLE Received" << value.size() << "bytes";
        emit dataReceived(value);
    }
}

void BluetoothWorker::_onCharacteristicRead(const QLowEnergyCharacteristic &characteristic,
                                            const QByteArray &value)
{
    qCDebug(BluetoothLinkLog) << "Characteristic read:" << characteristic.uuid().toString()
                              << "Value length:" << value.size();
}

void BluetoothWorker::_onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic,
                                               const QByteArray &value)
{
    Q_UNUSED(value)

    if (characteristic.uuid() == _writeCharacteristic.uuid()) {
        _processNextBleWrite();
    }
}

void BluetoothWorker::_onDescriptorRead(const QLowEnergyDescriptor &descriptor,
                                        const QByteArray &value)
{
    qCDebug(BluetoothLinkLog) << "Descriptor read:" << descriptor.uuid().toString()
                              << "Value:" << value.toHex();
}

void BluetoothWorker::_onDescriptorWritten(const QLowEnergyDescriptor &descriptor,
                                            const QByteArray &value)
{
    qCDebug(BluetoothLinkLog) << "Descriptor written:" << descriptor.uuid().toString()
                              << "Value:" << value.toHex();
}

void BluetoothWorker::_onServiceError(QLowEnergyService::ServiceError error)
{
    const QString errorString = tr("Service error: %1").arg(error);
    qCWarning(BluetoothLinkLog) << "BLE Service error:" << error;
    emit errorOccurred(errorString);

    _clearBleWriteQueue();

    if (_service) {
        _service->deleteLater();
        _service = nullptr;
    }

    _readCharacteristic = QLowEnergyCharacteristic();
    _writeCharacteristic = QLowEnergyCharacteristic();

    const bool wasConnected = _connected.exchange(false);
    if (!wasConnected) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
    }
    emit disconnected();

    if (!_intentionalDisconnect.load() && _reconnectTimer) {
        _reconnectTimer->start();
    }
}

void BluetoothWorker::_reconnectTimeout()
{
    if (_intentionalDisconnect.load()) {
        return;
    }

    if (_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        qCWarning(BluetoothLinkLog) << "Max reconnection attempts (" << MAX_RECONNECT_ATTEMPTS << ") reached. Giving up.";
        emit errorOccurred(tr("Max reconnection attempts reached"));
        return;
    }

    _reconnectAttempts++;
    qCDebug(BluetoothLinkLog) << "Attempting to reconnect (attempt" << _reconnectAttempts << "of" << MAX_RECONNECT_ATTEMPTS << ")";

    const int nextInterval = qMin(RECONNECT_BASE_INTERVAL_MS * (1 << (_reconnectAttempts - 1)), MAX_RECONNECT_INTERVAL_MS);
    if (_reconnectTimer) {
        _reconnectTimer->setInterval(nextInterval);
    }

    if (_mode == BluetoothConfiguration::BluetoothMode::ModeLowEnergy &&
        _consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
        qCDebug(BluetoothLinkLog) << "Recreating BLE controller after" << _consecutiveFailures << "consecutive failures";
        _recreateBleController();
    }

    connectLink();
}

void BluetoothWorker::_serviceDiscoveryTimeout()
{
    qCWarning(BluetoothLinkLog) << "Service discovery timed out after" << SERVICE_DISCOVERY_TIMEOUT_MS << "ms";
    _consecutiveFailures++;

    if (_controller) {
        _controller->disconnectFromDevice();
    }

    emit errorOccurred(tr("Service discovery timed out"));
}

void BluetoothWorker::_recreateBleController()
{
    qCDebug(BluetoothLinkLog) << "Recreating BLE controller";

    if (_service) {
        _service->disconnect();
        _service->deleteLater();
        _service = nullptr;
    }

    if (_controller) {
        _controller->disconnect();
        if (_controller->state() != QLowEnergyController::UnconnectedState) {
            _controller->disconnectFromDevice();
        }
        _controller->deleteLater();
        _controller = nullptr;
    }

    _readCharacteristic = QLowEnergyCharacteristic();
    _writeCharacteristic = QLowEnergyCharacteristic();
    _clearBleWriteQueue();

    _consecutiveFailures = 0;
    _setupBleController();
}

/*===========================================================================*/

BluetoothLink::BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _bluetoothConfig(qobject_cast<BluetoothConfiguration*>(config.get()))
    , _workerThread(new QThread(this))
{
    Q_ASSERT(_bluetoothConfig);
    if (!_bluetoothConfig) {
        qCCritical(BluetoothLinkLog) << "Invalid BluetoothConfiguration";
        return;
    }

    _worker = new BluetoothWorker(_bluetoothConfig);

    qCDebug(BluetoothLinkLog) << this;

    _checkPermission();

    const QString threadName = (_bluetoothConfig->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy)
        ? QStringLiteral("BLE_%1") : QStringLiteral("Bluetooth_%1");
    _workerThread->setObjectName(threadName.arg(_bluetoothConfig->name()));

    _worker->moveToThread(_workerThread.data());

    (void) connect(_workerThread.data(), &QThread::started, _worker.data(), &BluetoothWorker::setupConnection);
    (void) connect(_workerThread.data(), &QThread::finished, _worker.data(), &QObject::deleteLater);

    (void) connect(_worker.data(), &BluetoothWorker::connected, this, &BluetoothLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::disconnected, this, &BluetoothLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::errorOccurred, this, &BluetoothLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::dataReceived, this, &BluetoothLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::dataSent, this, &BluetoothLink::_onDataSent, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::rssiUpdated, this, &BluetoothLink::_onRssiUpdated, Qt::QueuedConnection);

    (void) connect(_bluetoothConfig, &BluetoothConfiguration::errorOccurred, this, &BluetoothLink::_onErrorOccurred);

    _workerThread->start();
}

BluetoothLink::~BluetoothLink()
{
    if (_workerThread) {
        if (_workerThread->isRunning() && isConnected() && _worker) {
            // Use BlockingQueuedConnection to ensure disconnect completes before thread quits
            (void) QMetaObject::invokeMethod(_worker, "disconnectLink", Qt::BlockingQueuedConnection);
        }

        _workerThread->quit();
        if (!_workerThread->wait(5000)) {
            qCWarning(BluetoothLinkLog) << "Failed to wait for Bluetooth Thread to close";
            _workerThread->terminate();
            if (!_workerThread->wait(1000)) {
                qCCritical(BluetoothLinkLog) << "Failed to terminate Bluetooth Thread";
            }
        }
    }

    qCDebug(BluetoothLinkLog) << this;
}

bool BluetoothLink::isConnected() const
{
    return _worker ? _worker->isConnected() : false;
}

bool BluetoothLink::_connect()
{
    if (!_worker) {
        return false;
    }
    if (_bluetoothConfig) {
        _bluetoothConfig->stopScan();
    }
    return QMetaObject::invokeMethod(_worker.data(), "connectLink", Qt::QueuedConnection);
}

void BluetoothLink::disconnect()
{
    if (_worker) {
        (void) QMetaObject::invokeMethod(_worker.data(), "disconnectLink", Qt::QueuedConnection);
    }
}

void BluetoothLink::_onConnected()
{
    _disconnectedEmitted = false;
    emit connected();
}

void BluetoothLink::_onDisconnected()
{
    if (!_disconnectedEmitted.exchange(true)) {
        emit disconnected();
    }
}

void BluetoothLink::_onErrorOccurred(const QString &errorString)
{
    qCWarning(BluetoothLinkLog) << "Communication error:" << errorString;

    if (!_bluetoothConfig) {
        emit communicationError(tr("Bluetooth Link Error"), errorString);
        return;
    }

    const QString linkType = (_bluetoothConfig->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy)
        ? tr("Bluetooth Low Energy") : tr("Bluetooth");

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

void BluetoothLink::_onRssiUpdated(qint16 rssi)
{
    if (_bluetoothConfig) {
        _bluetoothConfig->setConnectedRssi(rssi);
    }
}

void BluetoothLink::_writeBytes(const QByteArray &bytes)
{
    if (_worker) {
        (void) QMetaObject::invokeMethod(_worker.data(), "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, bytes));
    }
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
        _onErrorOccurred(tr("Bluetooth Permission Denied"));
        _onDisconnected();
    } else {
        qCDebug(BluetoothLinkLog) << "Bluetooth Permission Granted";
    }
}
