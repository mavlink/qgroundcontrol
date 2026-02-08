#include "BluetoothWorker.h"
#include "BluetoothBleWorker.h"
#include "BluetoothClassicWorker.h"

BluetoothWorker::BluetoothWorker(const BluetoothConfiguration *config, QObject *parent)
    : QObject(parent)
    , _device(config->device())
    , _serviceUuid(QBluetoothUuid(config->serviceUuid()))
    , _readCharacteristicUuid(config->readCharacteristicUuid())
    , _writeCharacteristicUuid(config->writeCharacteristicUuid())
    , _reconnectTimer(new QTimer(this))
    , _serviceDiscoveryTimer(new QTimer(this))
{
    qCDebug(BluetoothLinkLog) << this;

    _reconnectTimer->setInterval(RECONNECT_BASE_INTERVAL_MS);
    _reconnectTimer->setSingleShot(true);
    (void) connect(_reconnectTimer.data(), &QTimer::timeout, this, &BluetoothWorker::_reconnectTimeout);

    _serviceDiscoveryTimer->setInterval(SERVICE_DISCOVERY_TIMEOUT_MS);
    _serviceDiscoveryTimer->setSingleShot(true);
    (void) connect(_serviceDiscoveryTimer.data(), &QTimer::timeout, this, &BluetoothWorker::_serviceDiscoveryTimeout);
}

BluetoothWorker::~BluetoothWorker()
{
    _intentionalDisconnect = true;

    // Note: subclass destructors handle their own resource cleanup (socket/controller)
    // since virtual dispatch is not available in base destructors.

    QObject::disconnect();

    if (_reconnectTimer) {
        _reconnectTimer->stop();
    }
    if (_serviceDiscoveryTimer) {
        _serviceDiscoveryTimer->stop();
    }

    qCDebug(BluetoothLinkLog) << this;
}

bool BluetoothWorker::isConnected() const
{
    return _connected.load();
}

BluetoothWorker *BluetoothWorker::create(const BluetoothConfiguration *config, QObject *parent)
{
    if (config->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy) {
        return new BluetoothBleWorker(config, parent);
    }
    return new BluetoothClassicWorker(config, parent);
}

void BluetoothWorker::setupConnection()
{
    onSetupConnection();
}

void BluetoothWorker::connectLink()
{
    _intentionalDisconnect = false;

    if (isConnected()) {
        qCWarning(BluetoothLinkLog) << "Already connected to" << _device.name();
        return;
    }

    onConnectLink();
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

    onDisconnectLink();
}

void BluetoothWorker::writeData(const QByteArray &data)
{
    if (data.isEmpty()) {
        qCWarning(BluetoothLinkLog) << "Write called with empty data";
        return;
    }

    if (!isConnected()) {
        emit errorOccurred(tr("Device is not connected"));
        return;
    }

    onWriteData(data);
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

    if (_consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
        onResetAfterConsecutiveFailures();
        _consecutiveFailures = 0;
    }

    connectLink();
}

void BluetoothWorker::_serviceDiscoveryTimeout()
{
    qCWarning(BluetoothLinkLog) << "Service discovery timed out after" << SERVICE_DISCOVERY_TIMEOUT_MS << "ms";
    _consecutiveFailures++;

    onServiceDiscoveryTimeout();

    emit errorOccurred(tr("Service discovery timed out"));
}
