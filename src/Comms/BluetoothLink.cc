/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BluetoothLink.h"

#include <QtBluetooth/QBluetoothHostInfo>
#include <QtBluetooth/QLowEnergyConnectionParameters>
#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QThread>
#include <QtCore/QVariantMap>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(BluetoothLinkLog, "Comms.BluetoothLink")

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
    , _connectedRssi(copy->connectedRssi())
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
    // Delete old local device if exists
    if (_localDevice) {
        _localDevice->deleteLater();
        _localDevice = nullptr;
    }

    // Create new local device with specified adapter
    _localDevice = new QBluetoothLocalDevice(address, this);

    if (!_localDevice->isValid()) {
        qCWarning(BluetoothLinkLog) << "Failed to initialize Bluetooth adapter:" << address.toString();
        _localDevice->deleteLater();
        _localDevice = nullptr;
        return false;
    }

    // Connect all signals
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

    // Monitor adapter host mode changes (powered on/off, discoverable, etc.)
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::hostModeStateChanged,
                   this, &BluetoothConfiguration::_onHostModeStateChanged);

    // Monitor when devices connect/disconnect from the local adapter
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::deviceConnected,
                   this, &BluetoothConfiguration::_onDeviceConnected);
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::deviceDisconnected,
                   this, &BluetoothConfiguration::_onDeviceDisconnected);

    // Monitor pairing events for Classic Bluetooth
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::pairingFinished,
                   this, &BluetoothConfiguration::_onPairingFinished);

    // Monitor local device errors
    (void) connect(_localDevice.data(), &QBluetoothLocalDevice::errorOccurred,
                   this, &BluetoothConfiguration::_onLocalDeviceErrorOccurred);
}

void BluetoothConfiguration::_initDeviceDiscoveryAgent()
{
    if (!_deviceDiscoveryAgent) {
        _deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    }

    // Create local device instance for adapter state monitoring/power control
    if (!_localDevice) {
        const QList<QBluetoothHostInfo> hosts = QBluetoothLocalDevice::allDevices();
        if (!hosts.isEmpty()) {
            _createLocalDevice(hosts.first().address());
        }
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

    // Receive RSSI/manufacturer/service updates during discovery
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

    // If device isn't in list yet, add it on update when we have a usable signal (BLE only)
    if (!found) {
        if (_mode == BluetoothMode::ModeLowEnergy) {
            if (info.rssi() != 0 &&
                (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)) {
                _deviceList.append(info);
                _nameList.append(info.name());
                emit nameListChanged();
                emit devicesModelChanged();
                if (_device.address() == info.address()) {
                    emit selectedRssiChanged();
                }
            }
        } else {
            if ((info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateCoreConfiguration) ||
                (info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateAndLowEnergyCoreConfiguration)) {
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
}

void BluetoothConfiguration::setMode(BluetoothMode mode)
{
    if (_mode != mode) {
        // Abort any ongoing discovery when switching modes
        stopScan();
        _mode = mode;
        emit modeChanged();

        // Clear device list when switching modes
        _deviceList.clear();
        _nameList.clear();
        emit nameListChanged();
        emit devicesModelChanged();
    }
}

void BluetoothConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const BluetoothConfiguration *const bluetoothSource = qobject_cast<const BluetoothConfiguration*>(source);
    Q_ASSERT(bluetoothSource);

    _mode = bluetoothSource->mode();
    _device = bluetoothSource->device();
    _serviceUuid = bluetoothSource->_serviceUuid;
    _readCharacteristicUuid = bluetoothSource->readCharacteristicUuid();
    _writeCharacteristicUuid = bluetoothSource->writeCharacteristicUuid();
    _connectedRssi = bluetoothSource->connectedRssi();

    emit modeChanged();
    emit deviceChanged();
    emit serviceUuidChanged();
    emit readUuidChanged();
    emit writeUuidChanged();
    emit connectedRssiChanged();
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
    const QList<QBluetoothHostInfo> devices = QBluetoothLocalDevice::allDevices();
    return !devices.isEmpty();
}

QVariantList BluetoothConfiguration::devicesModel() const
{
    QVariantList model;
    for (const QBluetoothDeviceInfo &info : _deviceList) {
        QVariantMap device;
        device["name"] = info.name();
        device["address"] = info.address().toString();
        // Only report RSSI if non-zero. Many platforms return 0 when unknown/unavailable.
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
    if (_serviceUuid != newUuid) {
        _serviceUuid = newUuid;
        emit serviceUuidChanged();
    }
}

void BluetoothConfiguration::setReadUuid(const QString &uuid)
{
    const QBluetoothUuid newUuid(uuid);
    if (_readCharacteristicUuid != newUuid) {
        _readCharacteristicUuid = newUuid;
        emit readUuidChanged();
    }
}

void BluetoothConfiguration::setWriteUuid(const QString &uuid)
{
    const QBluetoothUuid newUuid(uuid);
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

    // Ensure permission before starting discovery
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

    // Ensure adapter is powered on if available
    if (_localDevice && _localDevice->isValid() && (_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff)) {
        _localDevice->powerOn();
    }

    _deviceList.clear();
    _nameList.clear();
    emit nameListChanged();
    emit devicesModelChanged();

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
    for (const QBluetoothDeviceInfo &info : std::as_const(_deviceList)) {
        if (info.name() == name) {
            // Stop scanning once a device is selected
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
            // Stop scanning once a device is selected
            stopScan();
            _applySelectedDevice(info);
            return;
        }
    }
}

void BluetoothConfiguration::_applySelectedDevice(const QBluetoothDeviceInfo &info)
{
    _device = info;

    // For BLE devices, try to extract service/characteristic UUIDs
    if (_mode == BluetoothMode::ModeLowEnergy) {
        const QList<QBluetoothUuid> serviceUuids = info.serviceUuids();
        if (!serviceUuids.isEmpty()) {
            _serviceUuid = QBluetoothUuid();
            for (const QBluetoothUuid &uuid : serviceUuids) {
                if ((uuid == NORDIC_UART_SERVICE) || (uuid == TI_SENSORTAG_SERVICE)) {
                    _serviceUuid = uuid;

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

            // If no known service, use first available and allow auto-detect characteristics
            if (_serviceUuid.isNull()) {
                _serviceUuid = serviceUuids.first();
                _readCharacteristicUuid = QBluetoothUuid();
                _writeCharacteristicUuid = QBluetoothUuid();
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

    // Check if device already in list by address
    int existingIndex = -1;
    for (int i = 0; i < _deviceList.size(); ++i) {
        if (_deviceList[i].address() == info.address()) {
            existingIndex = i;
            break;
        }
    }

    if (existingIndex >= 0) {
        // Update stored info to capture RSSI/name changes
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
    // Update name list from device list
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

    // Notify UI of adapter state change
    emit adapterStateChanged();

    // If adapter is powered off during scanning, stop the scan
    if ((mode == QBluetoothLocalDevice::HostPoweredOff) && scanning()) {
        stopScan();
        emit errorOccurred(tr("Bluetooth adapter powered off"));
    }
}

void BluetoothConfiguration::_onDeviceConnected(const QBluetoothAddress &address)
{
    qCDebug(BluetoothLinkLog) << "Device connected to adapter:" << address.toString();

    // Update device info in list if it exists
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

    // Update device info in list if it exists
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

    // Find device name if available
    QString deviceName;
    for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
        if (dev.address() == address) {
            deviceName = dev.name();
            break;
        }
    }

    // Notify user of pairing result
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

    // Trigger UI update to reflect new pairing status
    emit devicesModelChanged();
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

    // Get all devices from the local device's paired list
    for (const QBluetoothDeviceInfo &dev : std::as_const(_deviceList)) {
        const QBluetoothLocalDevice::Pairing pairingStatus = _localDevice->pairingStatus(dev.address());
        if (pairingStatus != QBluetoothLocalDevice::Unpaired) {
            QVariantMap device;
            device["name"] = dev.name();
            device["address"] = dev.address().toString();
            device["paired"] = true;

            // Add pairing status
            if (pairingStatus == QBluetoothLocalDevice::AuthorizedPaired) {
                device["pairingStatus"] = tr("Authorized");
            } else {
                device["pairingStatus"] = tr("Paired");
            }

            // Check if currently connected
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

        // Try to find device name from our device list
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

    // Stop any ongoing scan before powering off
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

        // Check if this is the currently selected adapter
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

    // Check if the adapter exists
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

    // Stop any ongoing scan before switching adapters
    if (scanning()) {
        stopScan();
    }

    // Create new local device with selected adapter (deletes old one if exists)
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
    , _config(config)
    , _reconnectTimer(new QTimer(this))
{
    qCDebug(BluetoothLinkLog) << this;

    _reconnectTimer->setInterval(5000);
    _reconnectTimer->setSingleShot(true);
    (void) connect(_reconnectTimer.data(), &QTimer::timeout, this, &BluetoothWorker::_reconnectTimeout);

    // Periodic RSSI polling for BLE connections
    _rssiTimer = new QTimer(this);
    _rssiTimer->setInterval(3000);
    _rssiTimer->setSingleShot(false);
    // Connect once; start/stop occurs on connect/disconnect
    (void) connect(_rssiTimer.data(), &QTimer::timeout, this, [this]() {
        if (_controller && (_controller->state() == QLowEnergyController::ConnectedState)) {
            _controller->readRssi();
        }
    });
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
        return ((_controller && (_controller->state() == QLowEnergyController::ConnectedState)) &&
                (_service && (_service->state() == QLowEnergyService::RemoteServiceDiscovered)) &&
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

        // Capture this as context ensures auto-disconnect on destruction
        (void) connect(_socket.data(), &QBluetoothSocket::stateChanged, this,
                      [this](QBluetoothSocket::SocketState state) {
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

    (void) connect(_controller.data(), &QLowEnergyController::connected, this, &BluetoothWorker::_onControllerConnected);
    (void) connect(_controller.data(), &QLowEnergyController::disconnected, this, &BluetoothWorker::_onControllerDisconnected);
    (void) connect(_controller.data(), &QLowEnergyController::errorOccurred, this, &BluetoothWorker::_onControllerErrorOccurred);
    (void) connect(_controller.data(), &QLowEnergyController::serviceDiscovered, this, &BluetoothWorker::_onServiceDiscovered);
    (void) connect(_controller.data(), &QLowEnergyController::discoveryFinished, this, &BluetoothWorker::_onServiceDiscoveryFinished);

    // Capture this; receiver context auto-disconnects safely
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
        qCWarning(BluetoothLinkLog) << "Already connected to" << _config->device().name();
        return;
    }

    qCDebug(BluetoothLinkLog) << "Attempting to connect to" << _config->device().name()
                              << "Mode:" << ((_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) ? "BLE" : "Classic");

    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        if (!_controller) {
            _setupBleController();
        }

        if (!_controller) {
            emit errorOccurred(tr("BLE controller not available"));
            return;
        }

        _controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
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
        _socket->connectToService(_config->device().address(), uuid);
    }
}

void BluetoothWorker::disconnectLink()
{
    _intentionalDisconnect = true;

    if (_reconnectTimer) {
        _reconnectTimer->stop();
        _reconnectTimer->setInterval(5000); // Reset to initial interval
    }
    _reconnectAttempts = 0; // Reset reconnection attempts

    if (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        if (_service) {
            _service->deleteLater();
            _service = nullptr;
        }

        if (_controller) {
            _controller->disconnectFromDevice();
        }
    } else {
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
    // ATT MTU payload is (MTU - 3). If we don't know MTU yet, assume minimum
    const int effectiveMtu = (_mtu > 3) ? (_mtu - 3) : BLE_MIN_PACKET_SIZE;
    const int packetSize = qBound(BLE_MIN_PACKET_SIZE, effectiveMtu, BLE_MAX_PACKET_SIZE);

    const bool useWriteWithoutResponse = (_writeCharacteristic.properties() & QLowEnergyCharacteristic::WriteNoResponse);

    // For WriteWithoutResponse, send all chunks immediately (no flow control needed)
    if (useWriteWithoutResponse) {
        for (int i = 0; i < data.size(); i += packetSize) {
            const QByteArray chunk = data.mid(i, packetSize);
            _service->writeCharacteristic(_writeCharacteristic, chunk, QLowEnergyService::WriteWithoutResponse);
        }
        emit dataSent(data);
    } else {
        // For WriteWithResponse, use queue-based approach to wait for confirmations
        _currentBleWrite = data;
        for (int i = 0; i < data.size(); i += packetSize) {
            _bleWriteQueue.enqueue(data.mid(i, packetSize));
        }

        // Start sending the first chunk if not already writing
        if (!_bleWriteInProgress) {
            _processNextBleWrite();
        }
    }
}

void BluetoothWorker::_processNextBleWrite()
{
    if (_bleWriteQueue.isEmpty()) {
        _bleWriteInProgress = false;
        // All chunks sent successfully, emit the complete data
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

// Classic Bluetooth slots
void BluetoothWorker::_onSocketConnected()
{
    qCDebug(BluetoothLinkLog) << "Socket connected to device:" << _config->device().name();
    _connected = true;
    _reconnectAttempts = 0; // Reset on successful connection
    emit connected();
}

void BluetoothWorker::_onSocketDisconnected()
{
    qCDebug(BluetoothLinkLog) << "Socket disconnected from device:" << _config->device().name();
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
        errorString = tr("Socket error: Null Socket");
    }

    qCWarning(BluetoothLinkLog) << "Socket error:" << socketError << errorString;
    emit errorOccurred(errorString);

    // If we never successfully connected, emit disconnected to reset UI state
    if (!_connected.load()) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
        emit disconnected();
    }

    // Attempt classic service discovery as a fallback only for service-related errors
    // Don't try discovery for device availability issues, connection drops, or permission errors
    if (_config && (_config->mode() == BluetoothConfiguration::BluetoothMode::ModeClassic)) {
        if ((socketError == QBluetoothSocket::SocketError::ServiceNotFoundError) ||
            (socketError == QBluetoothSocket::SocketError::UnsupportedProtocolError)) {
            qCDebug(BluetoothLinkLog) << "Service-related error, attempting fallback discovery";
            _startClassicServiceDiscovery();
        }
    }
}

void BluetoothWorker::_startClassicServiceDiscovery()
{
    if (!_config) {
        return;
    }

    if (_classicDiscovery && _classicDiscovery->isActive()) {
        return; // already running
    }

    if (!_classicDiscovery) {
        _classicDiscovery = new QBluetoothServiceDiscoveryAgent(_config->device().address(), this);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::serviceDiscovered,
                       this, &BluetoothWorker::_onClassicServiceDiscovered);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::finished,
                       this, &BluetoothWorker::_onClassicServiceDiscoveryFinished);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::canceled,
                       this, &BluetoothWorker::_onClassicServiceDiscoveryCanceled);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::errorOccurred,
                       this, &BluetoothWorker::_onClassicServiceDiscoveryError);
    }

    qCDebug(BluetoothLinkLog) << "Starting classic service discovery on" << _config->device().name();
    _classicDiscoveredService = QBluetoothServiceInfo();
    // Discover all available services; we'll pick a suitable RFCOMM service below
    _classicDiscovery->start(QBluetoothServiceDiscoveryAgent::FullDiscovery);
}

void BluetoothWorker::_onClassicServiceDiscovered(const QBluetoothServiceInfo &serviceInfo)
{
    qCDebug(BluetoothLinkLog) << "Classic service discovered: UUIDs=" << serviceInfo.serviceClassUuids()
                              << "RFCOMM channel=" << serviceInfo.serverChannel()
                              << "L2CAP PSM=" << serviceInfo.protocolServiceMultiplexer();

    // Prefer Serial Port Profile, otherwise accept any service exposing an RFCOMM server channel
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

// BLE slots
void BluetoothWorker::_onControllerConnected()
{
    qCDebug(BluetoothLinkLog) << "BLE Controller connected to device:" << _config->device().name();

    if (_controller) {
        _controller->discoverServices();
        if (_rssiTimer) {
            _rssiTimer->start();
        }
    }
}

void BluetoothWorker::_onControllerDisconnected()
{
    qCDebug(BluetoothLinkLog) << "BLE Controller disconnected from device:" << _config->device().name();

    if (_rssiTimer) {
        _rssiTimer->stop();
        _rssi = 0;
        emit rssiUpdated(_rssi);
    }

    // Clear any pending write queue
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
    QString errorString;
    if (_controller) {
        errorString = _controller->errorString();
    } else {
        errorString = tr("Controller error: %1").arg(error);
    }

    qCWarning(BluetoothLinkLog) << "BLE Controller error:" << error << errorString;
    emit errorOccurred(errorString);

    // If we never successfully connected, emit disconnected to reset UI state
    if (!_connected.load()) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
        emit disconnected();
    }
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

        // Prefer known UART services when auto-selecting
        if (services.contains(BluetoothConfiguration::NORDIC_UART_SERVICE)) {
            serviceUuid = BluetoothConfiguration::NORDIC_UART_SERVICE;
            qCDebug(BluetoothLinkLog) << "Auto-selected Nordic UART service";
        } else if (services.contains(BluetoothConfiguration::TI_SENSORTAG_SERVICE)) {
            serviceUuid = BluetoothConfiguration::TI_SENSORTAG_SERVICE;
            qCDebug(BluetoothLinkLog) << "Auto-selected TI SensorTag service";
        } else {
            // Use first available service - will validate characteristics later
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

void BluetoothWorker::_findCharacteristics()
{
    if (!_service) {
        return;
    }

    const QList<QLowEnergyCharacteristic> characteristics = _service->characteristics();

    // Try to match by UUID first if specified
    const QBluetoothUuid readUuid = _config->readCharacteristicUuid();
    const QBluetoothUuid writeUuid = _config->writeCharacteristicUuid();

    for (const QLowEnergyCharacteristic &c : characteristics) {
        // Match read characteristic
        if (!readUuid.isNull() && (c.uuid() == readUuid)) {
            _readCharacteristic = c;
            qCDebug(BluetoothLinkLog) << "Read characteristic found by UUID:" << c.uuid().toString();
        }
        // Match write characteristic
        if (!writeUuid.isNull() && (c.uuid() == writeUuid)) {
            _writeCharacteristic = c;
            qCDebug(BluetoothLinkLog) << "Write characteristic found by UUID:" << c.uuid().toString();
        }
    }

    // Auto-detect if not found by UUID
    if (!_readCharacteristic.isValid() || !_writeCharacteristic.isValid()) {
        for (const QLowEnergyCharacteristic &c : characteristics) {
            // Auto-detect read characteristic
            if (!_readCharacteristic.isValid() &&
                (c.properties() & (QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify))) {
                _readCharacteristic = c;
                qCDebug(BluetoothLinkLog) << "Read characteristic auto-detected:" << c.uuid().toString();
            }

            // Auto-detect write characteristic
            if (!_writeCharacteristic.isValid() &&
                (c.properties() & (QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::WriteNoResponse))) {
                _writeCharacteristic = c;
                qCDebug(BluetoothLinkLog) << "Write characteristic auto-detected:" << c.uuid().toString();
            }
        }
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

        // Enable notifications if available
        if (_readCharacteristic.isValid()) {
            _enableNotifications();
        }

        // Request optimized connection parameters for better throughput
        // MTU is negotiated automatically by the BLE stack (we monitor via mtuChanged signal)
        if (_controller) {
            QLowEnergyConnectionParameters params;
            // Request faster connection interval for lower latency (7.5ms - 15ms)
            params.setIntervalRange(6, 12); // Units of 1.25ms
            params.setLatency(0); // No latency
            params.setSupervisionTimeout(500); // 5 seconds
            _controller->requestConnectionUpdate(params);
        }

        _connected = true;
        _reconnectAttempts = 0; // Reset on successful connection
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
        // Enable notifications (0x0100) or indications (0x0200) based on characteristic properties
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

    // Process next chunk in write queue if this was our write characteristic
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

    // If we never successfully connected, emit disconnected to reset UI state
    if (!_connected.load()) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
        emit disconnected();
    }

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

    // Exponential backoff: 5s, 10s, 20s, 40s, ... (capped at 60s)
    const int nextInterval = qMin(5000 * (1 << (_reconnectAttempts - 1)), 60000);
    if (_reconnectTimer) {
        _reconnectTimer->setInterval(nextInterval);
    }

    connectLink();
}

/*===========================================================================*/

BluetoothLink::BluetoothLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _bluetoothConfig(qobject_cast<BluetoothConfiguration*>(config.get()))
    , _worker(new BluetoothWorker(_bluetoothConfig))
    , _workerThread(new QThread(this))
{
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
    BluetoothLink::disconnect();

    if (_workerThread) {
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
    // Stop any ongoing discovery to avoid contention while connecting
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

void BluetoothLink::_onRssiUpdated(qint16 rssi)
{
    _bluetoothConfig->setConnectedRssi(rssi);
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
