#include "BluetoothBleWorker.h"

#include <QtBluetooth/QLowEnergyConnectionParameters>

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

BluetoothBleWorker::BluetoothBleWorker(const BluetoothConfiguration *config, QObject *parent)
    : BluetoothWorker(config, parent)
{
    _rssiTimer = new QTimer(this);
    _rssiTimer->setInterval(RSSI_POLL_INTERVAL_MS);
    _rssiTimer->setSingleShot(false);
    (void) connect(_rssiTimer.data(), &QTimer::timeout, this, [this]() {
        if (_controller && _controller->state() != QLowEnergyController::UnconnectedState
                       && _controller->state() != QLowEnergyController::ClosingState) {
            _controller->readRssi();
        }
    });
}

BluetoothBleWorker::~BluetoothBleWorker()
{
    _intentionalDisconnect = true;
    disconnectLink();

    if (_rssiTimer) {
        _rssiTimer->stop();
    }

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
}

void BluetoothBleWorker::onSetupConnection()
{
    _setupBleController();
}

void BluetoothBleWorker::onConnectLink()
{
    qCDebug(BluetoothLinkLog) << "Attempting to connect to" << _device.name() << "Mode: BLE";

    if (!_controller) {
        _setupBleController();
    }

    if (!_controller) {
        emit errorOccurred(tr("BLE controller not available"));
        return;
    }

    // Prefer automatic address-type selection from discovery data.
    // If the backend cannot determine it (for example without CAP_NET_ADMIN),
    // _onControllerErrorOccurred can retry once with RandomAddress.
    _controller->connectToDevice();
}

void BluetoothBleWorker::onDisconnectLink()
{
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
}

void BluetoothBleWorker::onWriteData(const QByteArray &data)
{
    _writeBleData(data);
}

void BluetoothBleWorker::onServiceDiscoveryTimeout()
{
    if (_controller) {
        _controller->disconnectFromDevice();
    }
}

void BluetoothBleWorker::onResetAfterConsecutiveFailures()
{
    qCDebug(BluetoothLinkLog) << "Recreating BLE controller after" << _consecutiveFailures << "consecutive failures";
    _recreateBleController();
}

void BluetoothBleWorker::_setupBleController()
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

    if (_forceRandomAddressType) {
        qCDebug(BluetoothLinkLog) << "Using BLE RandomAddress fallback for" << _device.name();
        _controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    }

    (void) connect(_controller.data(), &QLowEnergyController::connected, this, &BluetoothBleWorker::_onControllerConnected);
    (void) connect(_controller.data(), &QLowEnergyController::disconnected, this, &BluetoothBleWorker::_onControllerDisconnected);
    (void) connect(_controller.data(), &QLowEnergyController::errorOccurred, this, &BluetoothBleWorker::_onControllerErrorOccurred);
    (void) connect(_controller.data(), &QLowEnergyController::serviceDiscovered, this, &BluetoothBleWorker::_onServiceDiscovered);
    (void) connect(_controller.data(), &QLowEnergyController::discoveryFinished, this, &BluetoothBleWorker::_onServiceDiscoveryFinished);

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

void BluetoothBleWorker::_recreateBleController()
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

void BluetoothBleWorker::_writeBleData(const QByteArray &data)
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

    // Queue all chunks and pace writes through _processNextBleWrite
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

void BluetoothBleWorker::_processNextBleWrite()
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
    const bool useWriteWithoutResponse = (_writeCharacteristic.properties() & QLowEnergyCharacteristic::WriteNoResponse);

    if (useWriteWithoutResponse) {
        _service->writeCharacteristic(_writeCharacteristic, chunk, QLowEnergyService::WriteWithoutResponse);
        // No confirmation callback for WriteWithoutResponse — pace via timer
        QTimer::singleShot(1, this, &BluetoothBleWorker::_processNextBleWrite);
    } else {
        _service->writeCharacteristic(_writeCharacteristic, chunk);
        // _onCharacteristicWritten will call _processNextBleWrite
    }
}

void BluetoothBleWorker::_clearBleWriteQueue()
{
    _bleWriteQueue.clear();
    _currentBleWrite.clear();
    _bleWriteInProgress = false;
}

void BluetoothBleWorker::_onControllerConnected()
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

void BluetoothBleWorker::_onControllerDisconnected()
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

void BluetoothBleWorker::_onControllerErrorOccurred(QLowEnergyController::Error error)
{
    // Stop service discovery timer to prevent timeout firing after error
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    // On Linux BlueZ, without CAP_NET_ADMIN the backend may fail to determine
    // remote address type and default to public, which can fail for private BLE devices.
    // Retry once with RandomAddress before surfacing the error.
    if (!_connected.load() && !_intentionalDisconnect.load() && !_forceRandomAddressType &&
        ((error == QLowEnergyController::UnknownRemoteDeviceError) ||
         (error == QLowEnergyController::ConnectionError) ||
         (error == QLowEnergyController::NetworkError))) {
        qCDebug(BluetoothLinkLog) << "BLE connect failed; retrying with RandomAddress fallback:" << error;
        _forceRandomAddressType = true;
        _recreateBleController();
        if (_controller) {
            qCDebug(BluetoothLinkLog) << "Issuing BLE reconnect using RandomAddress fallback";
            _controller->connectToDevice();
            return;
        }
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

void BluetoothBleWorker::_onServiceDiscovered(const QBluetoothUuid &uuid)
{
    qCDebug(BluetoothLinkLog) << "Service discovered:" << uuid.toString();

    // Only auto-select during discovery if user specified a target UUID
    if (!_serviceUuid.isNull() && uuid == _serviceUuid) {
        if (_serviceDiscoveryTimer) {
            _serviceDiscoveryTimer->stop();
        }
        _setupBleService();
    }
}

void BluetoothBleWorker::_onServiceDiscoveryFinished()
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

void BluetoothBleWorker::_setupBleService()
{
    if (!_controller) {
        emit errorOccurred(tr("Controller not available"));
        return;
    }

    QBluetoothUuid serviceUuid = _serviceUuid;

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

    (void) connect(_service.data(), &QLowEnergyService::stateChanged, this, &BluetoothBleWorker::_onServiceStateChanged);
    (void) connect(_service.data(), &QLowEnergyService::characteristicChanged, this, &BluetoothBleWorker::_onCharacteristicChanged);
    (void) connect(_service.data(), &QLowEnergyService::characteristicRead, this, &BluetoothBleWorker::_onCharacteristicRead);
    (void) connect(_service.data(), &QLowEnergyService::characteristicWritten, this, &BluetoothBleWorker::_onCharacteristicWritten);
    (void) connect(_service.data(), &QLowEnergyService::descriptorRead, this, &BluetoothBleWorker::_onDescriptorRead);
    (void) connect(_service.data(), &QLowEnergyService::descriptorWritten, this, &BluetoothBleWorker::_onDescriptorWritten);
    (void) connect(_service.data(), &QLowEnergyService::errorOccurred, this, &BluetoothBleWorker::_onServiceError);

    _discoverServiceDetails();
}

void BluetoothBleWorker::_discoverServiceDetails()
{
    if (!_service) {
        return;
    }

    qCDebug(BluetoothLinkLog) << "Discovering service details";
    _service->discoverDetails();
}

void BluetoothBleWorker::_findCharacteristics()
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

void BluetoothBleWorker::_onServiceStateChanged(QLowEnergyService::ServiceState state)
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

void BluetoothBleWorker::_enableNotifications()
{
    if (!_service || !_readCharacteristic.isValid()) {
        return;
    }

    const QLowEnergyDescriptor notificationDescriptor = _readCharacteristic.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

    if (!notificationDescriptor.isValid()) {
        qCWarning(BluetoothLinkLog) << "CCCD descriptor missing on read characteristic — notifications will not work";
        return;
    }

    QByteArray value;
    if (_readCharacteristic.properties() & QLowEnergyCharacteristic::Notify) {
        value = QLowEnergyCharacteristic::CCCDEnableNotification;
        qCDebug(BluetoothLinkLog) << "Enabling notifications for read characteristic";
    } else if (_readCharacteristic.properties() & QLowEnergyCharacteristic::Indicate) {
        value = QLowEnergyCharacteristic::CCCDEnableIndication;
        qCDebug(BluetoothLinkLog) << "Enabling indications for read characteristic";
    }

    if (!value.isEmpty()) {
        _service->writeDescriptor(notificationDescriptor, value);
    }
}

void BluetoothBleWorker::_onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                                   const QByteArray &value)
{
    if ((characteristic.uuid() == _readCharacteristic.uuid()) && !value.isEmpty()) {
        qCDebug(BluetoothLinkLog) << _device.name() << "BLE Received" << value.size() << "bytes";
        emit dataReceived(value);
    }
}

void BluetoothBleWorker::_onCharacteristicRead(const QLowEnergyCharacteristic &characteristic,
                                                const QByteArray &value)
{
    qCDebug(BluetoothLinkVerboseLog) << "Characteristic read:" << characteristic.uuid().toString()
                                     << "Value length:" << value.size();
}

void BluetoothBleWorker::_onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic,
                                                   const QByteArray &value)
{
    Q_UNUSED(value)

    if (characteristic.uuid() == _writeCharacteristic.uuid()) {
        _processNextBleWrite();
    }
}

void BluetoothBleWorker::_onDescriptorRead(const QLowEnergyDescriptor &descriptor,
                                            const QByteArray &value)
{
    qCDebug(BluetoothLinkVerboseLog) << "Descriptor read:" << descriptor.uuid().toString()
                                     << "Value:" << value.toHex();
}

void BluetoothBleWorker::_onDescriptorWritten(const QLowEnergyDescriptor &descriptor,
                                               const QByteArray &value)
{
    qCDebug(BluetoothLinkLog) << "Descriptor written:" << descriptor.uuid().toString()
                              << "Value:" << value.toHex();
}

void BluetoothBleWorker::_onServiceError(QLowEnergyService::ServiceError error)
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
