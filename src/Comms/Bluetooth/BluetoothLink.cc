#include "BluetoothLink.h"
#include "BluetoothWorker.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QThread>

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

    _worker = BluetoothWorker::create(_bluetoothConfig);

    qCDebug(BluetoothLinkLog) << this;

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

    _checkPermission();
}

BluetoothLink::~BluetoothLink()
{
    if (_workerThread) {
        if (_workerThread->isRunning() && _connectedCache && _worker) {
            (void) QMetaObject::invokeMethod(_worker, "disconnectLink", Qt::QueuedConnection);
        }

        _workerThread->quit();
        if (!_workerThread->wait(5000)) {
            qCWarning(BluetoothLinkLog) << "Worker thread did not stop within timeout, terminating";
            _workerThread->terminate();
            (void) _workerThread->wait(1000);
        }
    }

    qCDebug(BluetoothLinkLog) << this;
}

bool BluetoothLink::isConnected() const
{
    return _connectedCache;
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
    _connectedCache = true;
    _disconnectedEmitted = false;
    emit connected();
}

void BluetoothLink::_onDisconnected()
{
    _connectedCache = false;
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
        QCoreApplication::instance()->requestPermission(permission, this, [this](const QPermission &perm) {
            _handlePermissionStatus(perm.status());
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
        return;
    }

    qCDebug(BluetoothLinkLog) << "Bluetooth Permission Granted";

    if (_workerThread && !_workerThread->isRunning()) {
        _workerThread->start();
        if (!_workerThread->isRunning()) {
            qCCritical(BluetoothLinkLog) << "Failed to start worker thread";
            _onErrorOccurred(tr("Failed to start Bluetooth worker thread"));
            _worker->deleteLater();
            _worker = nullptr;
        }
    }
}
