#include "BluetoothClassicWorker.h"

BluetoothClassicWorker::BluetoothClassicWorker(const BluetoothConfiguration *config, QObject *parent)
    : BluetoothWorker(config, parent)
{
}

void BluetoothClassicWorker::onSetupConnection()
{
    _setupClassicSocket();
}

void BluetoothClassicWorker::onConnectLink()
{
    qCDebug(BluetoothLinkLog) << "Attempting to connect to" << _device.name() << "Mode: Classic";

    // Destroy stale socket — QBluetoothSocket must be recreated after errors
    if (_socket) {
        _socket->disconnect();
        _socket->deleteLater();
        _socket = nullptr;
    }

    _setupClassicSocket();

    if (!_socket) {
        emit errorOccurred(tr("Socket not available"));
        return;
    }

    _socket->connectToService(_device.address(), SPP_UUID);
}

void BluetoothClassicWorker::onDisconnectLink()
{
    if (_classicDiscovery && _classicDiscovery->isActive()) {
        _classicDiscovery->stop();
    }

    if (_socket) {
        _socket->disconnectFromService();
    }
}

void BluetoothClassicWorker::onWriteData(const QByteArray &data)
{
    _writeClassicData(data);
}

void BluetoothClassicWorker::onServiceDiscoveryTimeout()
{
    if (_classicDiscovery && _classicDiscovery->isActive()) {
        _classicDiscovery->stop();
    }
}

void BluetoothClassicWorker::onResetAfterConsecutiveFailures()
{
    // Classic Bluetooth has no special reset — just let reconnect proceed
}

BluetoothClassicWorker::~BluetoothClassicWorker()
{
    _intentionalDisconnect = true;
    disconnectLink();

    if (_socket) {
        _socket->disconnect();
        if (_socket->state() == QBluetoothSocket::SocketState::ConnectedState) {
            _socket->disconnectFromService();
        }
        _socket->deleteLater();
    }
}

void BluetoothClassicWorker::_setupClassicSocket()
{
    if (_socket) {
        return;
    }

    _socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

    (void) connect(_socket.data(), &QBluetoothSocket::connected, this, &BluetoothClassicWorker::_onSocketConnected);
    (void) connect(_socket.data(), &QBluetoothSocket::disconnected, this, &BluetoothClassicWorker::_onSocketDisconnected);
    (void) connect(_socket.data(), &QBluetoothSocket::readyRead, this, &BluetoothClassicWorker::_onSocketReadyRead);
    (void) connect(_socket.data(), &QBluetoothSocket::errorOccurred, this, &BluetoothClassicWorker::_onSocketErrorOccurred);

    if (BluetoothLinkLog().isDebugEnabled()) {
        (void) connect(_socket.data(), &QBluetoothSocket::bytesWritten, this, &BluetoothClassicWorker::_onSocketBytesWritten);

        (void) connect(_socket.data(), &QBluetoothSocket::stateChanged, this,
                      [](QBluetoothSocket::SocketState state) {
            qCDebug(BluetoothLinkLog) << "Bluetooth Socket State Changed:" << state;
        });
    }
}

void BluetoothClassicWorker::_writeClassicData(const QByteArray &data)
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

void BluetoothClassicWorker::_onSocketConnected()
{
    qCDebug(BluetoothLinkLog) << "=== Classic Bluetooth Connection Established ===" << _device.name();
    qCDebug(BluetoothLinkLog) << "  Address:" << _device.address().toString();
    qCDebug(BluetoothLinkLog) << "  Protocol: RFCOMM (SPP)";

    _connected = true;
    _reconnectAttempts = 0;
    _consecutiveFailures = 0;
    emit connected();
}

void BluetoothClassicWorker::_onSocketDisconnected()
{
    qCDebug(BluetoothLinkLog) << "Socket disconnected from device:" << _device.name();
    _connected = false;
    emit disconnected();

    if (!_intentionalDisconnect.load() && _socket && _reconnectTimer) {
        qCDebug(BluetoothLinkLog) << "Starting reconnect timer";
        _reconnectTimer->start();
    }
}

void BluetoothClassicWorker::_onSocketReadyRead()
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

void BluetoothClassicWorker::_onSocketBytesWritten(qint64 bytes)
{
    qCDebug(BluetoothLinkVerboseLog) << _device.name() << "Wrote" << bytes << "bytes";
}

void BluetoothClassicWorker::_onSocketErrorOccurred(QBluetoothSocket::SocketError socketError)
{
    QString errorString;
    if (_socket) {
        errorString = _socket->errorString();
    } else {
        errorString = tr("Socket error: Null Socket");
    }

    qCWarning(BluetoothLinkLog) << "Socket error:" << socketError << errorString;
    emit errorOccurred(errorString);

    _consecutiveFailures++;

    // If we never successfully connected, emit disconnected to reset UI state
    if (!_connected.load()) {
        qCDebug(BluetoothLinkLog) << "Connection attempt failed, emitting disconnected signal";
        emit disconnected();
    }

    // Attempt service discovery for service-related errors only
    if ((socketError == QBluetoothSocket::SocketError::ServiceNotFoundError) ||
        (socketError == QBluetoothSocket::SocketError::UnsupportedProtocolError)) {
        qCDebug(BluetoothLinkLog) << "Service-related error, attempting fallback discovery";
        _startClassicServiceDiscovery();
    }
}

void BluetoothClassicWorker::_startClassicServiceDiscovery()
{
    if (_classicDiscovery && _classicDiscovery->isActive()) {
        return;
    }

    if (!_classicDiscovery) {
        _classicDiscovery = new QBluetoothServiceDiscoveryAgent(_device.address(), this);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::serviceDiscovered,
                       this, &BluetoothClassicWorker::_onClassicServiceDiscovered);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::finished,
                       this, &BluetoothClassicWorker::_onClassicServiceDiscoveryFinished);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::canceled,
                       this, &BluetoothClassicWorker::_onClassicServiceDiscoveryCanceled);
        (void) connect(_classicDiscovery.data(), &QBluetoothServiceDiscoveryAgent::errorOccurred,
                       this, &BluetoothClassicWorker::_onClassicServiceDiscoveryError);
    }

    qCDebug(BluetoothLinkLog) << "Starting classic service discovery on" << _device.name();
    _classicDiscoveredService = QBluetoothServiceInfo();
    _classicDiscovery->start(QBluetoothServiceDiscoveryAgent::FullDiscovery);

    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
        _serviceDiscoveryTimer->start();
    }
}

void BluetoothClassicWorker::_onClassicServiceDiscovered(const QBluetoothServiceInfo &serviceInfo)
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

void BluetoothClassicWorker::_onClassicServiceDiscoveryFinished()
{
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

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

void BluetoothClassicWorker::_onClassicServiceDiscoveryCanceled()
{
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    qCDebug(BluetoothLinkLog) << "Classic service discovery canceled";
}

void BluetoothClassicWorker::_onClassicServiceDiscoveryError(QBluetoothServiceDiscoveryAgent::Error error)
{
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    const QString e = _classicDiscovery ? _classicDiscovery->errorString() : tr("Service discovery error: %1").arg(error);
    qCWarning(BluetoothLinkLog) << e;
    emit errorOccurred(e);
}
