#include "BluetoothConfiguration.h"

#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

#include <QtBluetooth/QBluetoothHostInfo>
#include <QtBluetooth/QBluetoothUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QTimer>
#include <QtCore/QVariantMap>

QGC_LOGGING_CATEGORY(BluetoothLinkLog, "Comms.BluetoothLink")
QGC_LOGGING_CATEGORY(BluetoothLinkVerboseLog, "Comms.BluetoothLink:verbose")

/*===========================================================================*/

BluetoothConfiguration::BluetoothConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(BluetoothLinkLog) << this;
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
}

BluetoothConfiguration::~BluetoothConfiguration()
{
    stopScan();

    if (_adapterWatcher) {
        _adapterWatcher->cancel();
        _adapterWatcher->waitForFinished();
    }

    qCDebug(BluetoothLinkLog) << this;
}

bool BluetoothConfiguration::_createLocalDevice(const QBluetoothAddress &address)
{
    if (_localDevice) {
        _localDevice->disconnect();
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

    if (!_localDevice) {
        if (!_availableAdapters.isEmpty()) {
            const QVariantMap adapter = _availableAdapters.first().toMap();
            const QBluetoothAddress address(adapter.value("address").toString());
            if (!address.isNull()) {
                (void) _createLocalDevice(address);
            }
        } else {
            _refreshAvailableAdaptersAsync();
        }
    }
}

void BluetoothConfiguration::_refreshAvailableAdaptersAsync()
{
    if (_adapterEnumerationInProgress) {
        return;
    }

    _adapterEnumerationInProgress = true;

    if (!_adapterWatcher) {
        _adapterWatcher = new QFutureWatcher<QList<QBluetoothHostInfo>>(this);
        (void) connect(_adapterWatcher.data(), &QFutureWatcher<QList<QBluetoothHostInfo>>::finished, this, [this]() {
            _adapterEnumerationInProgress = false;
            _updateAvailableAdapters(_adapterWatcher->future().result());
        });
    }

    _adapterWatcher->setFuture(QtConcurrent::run([]() {
        return QBluetoothLocalDevice::allDevices();
    }));
}

void BluetoothConfiguration::_updateAvailableAdapters(const QList<QBluetoothHostInfo> &hosts)
{
    QVariantList adapters;
    const QBluetoothAddress selectedAddress = (_localDevice && _localDevice->isValid())
        ? _localDevice->address()
        : QBluetoothAddress();

    for (const QBluetoothHostInfo &host : hosts) {
        QVariantMap adapter;
        adapter["name"] = host.name();
        adapter["address"] = host.address().toString();
        adapter["selected"] = (!selectedAddress.isNull() && (host.address() == selectedAddress));
        adapters.append(adapter);
    }

    if (_availableAdapters != adapters) {
        _availableAdapters = adapters;
        emit adapterStateChanged();
    }
}

bool BluetoothConfiguration::_ensureLocalDevice()
{
    if (_localDevice && _localDevice->isValid()) {
        return true;
    }

    // Prefer known adapter entries first.
    for (const QVariant &entry : std::as_const(_availableAdapters)) {
        const QBluetoothAddress address(entry.toMap().value("address").toString());
        if (!address.isNull() && _createLocalDevice(address)) {
            return true;
        }
    }

    // Refresh synchronously as a fallback for platforms where async refresh may not have completed yet.
    const QList<QBluetoothHostInfo> hosts = QBluetoothLocalDevice::allDevices();
    if (!hosts.isEmpty()) {
        _updateAvailableAdapters(hosts);
        for (const QBluetoothHostInfo &host : hosts) {
            if (!host.address().isNull() && _createLocalDevice(host.address())) {
                return true;
            }
        }
    }

    // Some platforms may expose only the default adapter.
    _localDevice = new QBluetoothLocalDevice(this);
    if (_localDevice->isValid()) {
        _connectLocalDeviceSignals();
        emit adapterStateChanged();
        return true;
    }

    _localDevice->deleteLater();
    _localDevice = nullptr;
    return false;
}

void BluetoothConfiguration::_deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{
    qCDebug(BluetoothLinkLog) << "Device Updated:" << info.name() << "Fields:" << updatedFields;

    if (!info.isValid() || (updatedFields == QBluetoothDeviceInfo::Field::None)) {
        qCDebug(BluetoothLinkVerboseLog) << "Ignoring device update: invalid or no updated fields"
                                         << info.name() << info.address().toString() << updatedFields;
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
            _updateDeviceList();
            if (_device.address() == dev.address()) {
                emit selectedRssiChanged();
            }
            break;
        }
    }

    if (!found && _isDeviceCompatibleWithMode(info)) {
        _deviceList.append(info);
        _updateDeviceList();
        if (_device.address() == info.address()) {
            emit selectedRssiChanged();
        }
    }
}

bool BluetoothConfiguration::_isDeviceCompatibleWithMode(const QBluetoothDeviceInfo &info) const
{
    // Some stacks temporarily report unknown core configuration while discovery is active.
    // Treat unknown as compatible so devices appear during live scans.
    if (info.coreConfigurations() == QBluetoothDeviceInfo::UnknownCoreConfiguration) {
        return true;
    }

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
        _updateDeviceList();
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
    return _devicesModelCache;
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
    _initDeviceDiscoveryAgent();

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
    _updateDeviceList();

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

void BluetoothConfiguration::_requestHostMode(QBluetoothLocalDevice::HostMode mode)
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    if (_localDevice->hostMode() == mode) {
        return;
    }

    // BlueZ rejects overlapping setHostMode() calls while a previous one is pending.
    if (_hostModeRequestPending) {
        _queuedHostMode = mode;
        _hasQueuedHostModeRequest = true;
        if (_requestedHostMode != mode) {
            qCDebug(BluetoothLinkLog) << "Host mode change already pending; queueing request"
                                      << _requestedHostMode << "->" << mode;
        }
        return;
    }

    _requestedHostMode = mode;
    _hasQueuedHostModeRequest = false;
    _hostModeRequestPending = true;
    _localDevice->setHostMode(mode);
}

void BluetoothConfiguration::_deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!info.isValid()) {
        qCDebug(BluetoothLinkVerboseLog) << "Ignoring discovered device: invalid device info"
                                         << info.name() << info.address().toString();
        return;
    }

    if (!_isDeviceCompatibleWithMode(info)) {
        qCDebug(BluetoothLinkVerboseLog) << "Ignoring discovered device: incompatible with mode"
                                         << ((_mode == BluetoothMode::ModeLowEnergy) ? "BLE" : "Classic")
                                         << info.name() << info.address().toString()
                                         << "CoreCfg:" << info.coreConfigurations();
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
        _updateDeviceList();
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
            const bool replace = (_deviceList[sameNameIndex].address() != info.address());
            if (replace) {
                _deviceList[sameNameIndex] = info;

                // If the selected device is this BLE peripheral, track its latest private address.
                if ((_device.name() == info.name()) && (_device.address() != info.address())) {
                    _device = info;
                    emit deviceChanged();
                    emit selectedRssiChanged();
                }

                _updateDeviceList();
            }
            return;
        }
    }

    _deviceList.append(info);
    _updateDeviceList();

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
    QVariantList model;
    for (const QBluetoothDeviceInfo &info : std::as_const(_deviceList)) {
        QVariantMap device;
        device["name"] = info.name();
        device["address"] = info.address().toString();
        if (info.rssi() != 0) {
            device["rssi"] = info.rssi();
        }
        model.append(device);
    }
    if (_devicesModelCache != model) {
        _devicesModelCache = model;
        qCDebug(BluetoothLinkVerboseLog) << "Bluetooth devices model updated. Count:" << _devicesModelCache.size();
        emit devicesModelChanged();
    }
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
    _hostModeRequestPending = false;

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

    if (_hasQueuedHostModeRequest) {
        const QBluetoothLocalDevice::HostMode queuedMode = _queuedHostMode;
        _hasQueuedHostModeRequest = false;
        if (queuedMode != mode) {
            QTimer::singleShot(0, this, [this, queuedMode]() {
                _requestHostMode(queuedMode);
            });
        }
    }

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

    _updateDeviceList();
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

bool BluetoothConfiguration::isPaired(const QString &address) const
{
    if (!_localDevice || !_localDevice->isValid() || _mode != BluetoothMode::ModeClassic) {
        return false;
    }

    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        return false;
    }

    const QBluetoothLocalDevice::Pairing status = _localDevice->pairingStatus(addr);
    return (status == QBluetoothLocalDevice::Paired || status == QBluetoothLocalDevice::AuthorizedPaired);
}

bool BluetoothConfiguration::isDiscoverable() const
{
    if (!_localDevice || !_localDevice->isValid()) {
        return false;
    }

    const QBluetoothLocalDevice::HostMode mode = _localDevice->hostMode();
    return (mode == QBluetoothLocalDevice::HostDiscoverable || mode == QBluetoothLocalDevice::HostDiscoverableLimitedInquiry);
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

    const QList<QBluetoothAddress> connectedAddresses = _localDevice->connectedDevices();

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

            device["connected"] = connectedAddresses.contains(dev.address());

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
    if (!_ensureLocalDevice()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        _refreshAvailableAdaptersAsync();
        return;
    }

    const bool wasPoweredOff = (_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff);

    qCDebug(BluetoothLinkLog) << "Powering on Bluetooth adapter";

    // powerOn() triggers an async host-mode change on BlueZ. Mark this as pending
    // so explicit setHostMode() requests are queued instead of rejected.
    if (wasPoweredOff) {
        _requestedHostMode = QBluetoothLocalDevice::HostConnectable;
        _hostModeRequestPending = true;
    }

    _localDevice->powerOn();

    // On some platforms powerOn can remain powered off; defer explicit mode request
    // to avoid colliding with BlueZ's in-flight state transition.
    if (wasPoweredOff && !_deferredPowerOnFixPending) {
        _deferredPowerOnFixPending = true;
        QTimer::singleShot(1500, this, [this]() {
            _deferredPowerOnFixPending = false;
            if (!_localDevice || !_localDevice->isValid()) {
                return;
            }

            if (_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
                _hostModeRequestPending = false;
                qCDebug(BluetoothLinkLog) << "Power-on fallback: requesting connectable host mode";
                _requestHostMode(QBluetoothLocalDevice::HostConnectable);
            } else if (_hostModeRequestPending) {
                // Some stacks update hostMode without emitting hostModeStateChanged.
                _hostModeRequestPending = false;

                if (_hasQueuedHostModeRequest) {
                    const QBluetoothLocalDevice::HostMode queuedMode = _queuedHostMode;
                    _hasQueuedHostModeRequest = false;
                    if (queuedMode != _localDevice->hostMode()) {
                        _requestHostMode(queuedMode);
                    }
                }
            }
        });
    }

    emit adapterStateChanged();
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

    if (_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        return;
    }

    qCDebug(BluetoothLinkLog) << "Powering off Bluetooth adapter";
    _requestHostMode(QBluetoothLocalDevice::HostPoweredOff);
}

void BluetoothConfiguration::setAdapterDiscoverable(bool discoverable)
{
    if (!_localDevice || !_localDevice->isValid()) {
        emit errorOccurred(tr("Bluetooth adapter not available"));
        return;
    }

    if (discoverable) {
        if (_localDevice->hostMode() == QBluetoothLocalDevice::HostDiscoverable) {
            return;
        }
        qCDebug(BluetoothLinkLog) << "Making Bluetooth adapter discoverable";
        _requestHostMode(QBluetoothLocalDevice::HostDiscoverable);
    } else {
        if (_localDevice->hostMode() == QBluetoothLocalDevice::HostConnectable) {
            return;
        }
        qCDebug(BluetoothLinkLog) << "Making Bluetooth adapter connectable only";
        _requestHostMode(QBluetoothLocalDevice::HostConnectable);
    }
}

QVariantList BluetoothConfiguration::getAllAvailableAdapters()
{
    if (_availableAdapters.isEmpty()) {
        _refreshAvailableAdaptersAsync();
    }
    return _availableAdapters;
}

void BluetoothConfiguration::selectAdapter(const QString &address)
{
    const QBluetoothAddress addr(address);
    if (addr.isNull()) {
        emit errorOccurred(tr("Invalid adapter address"));
        return;
    }

    if (!_availableAdapters.isEmpty()) {
        bool found = false;
        for (const QVariant &entry : std::as_const(_availableAdapters)) {
            const QVariantMap adapter = entry.toMap();
            if (QBluetoothAddress(adapter.value("address").toString()) == addr) {
                found = true;
                break;
            }
        }
        if (!found) {
            emit errorOccurred(tr("Adapter not found"));
            _refreshAvailableAdaptersAsync();
            return;
        }
    }

    if (scanning()) {
        stopScan();
    }

    if (!_createLocalDevice(addr)) {
        emit errorOccurred(tr("Failed to initialize adapter"));
        _refreshAvailableAdaptersAsync();
        return;
    }

    bool selectionUpdated = false;
    for (QVariant &entry : _availableAdapters) {
        QVariantMap adapter = entry.toMap();
        if (adapter.isEmpty()) {
            continue;
        }

        const bool selected = (QBluetoothAddress(adapter.value("address").toString()) == addr);
        if (adapter.value("selected").toBool() != selected) {
            adapter["selected"] = selected;
            entry = adapter;
            selectionUpdated = true;
        }
    }

    if (_availableAdapters.isEmpty() && _localDevice && _localDevice->isValid()) {
        QVariantMap adapter;
        adapter["name"] = _localDevice->name();
        adapter["address"] = _localDevice->address().toString();
        adapter["selected"] = true;
        _availableAdapters.append(adapter);
        selectionUpdated = true;
    }

    if (selectionUpdated) {
        emit adapterStateChanged();
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
