#include "BluetoothLink.h"
#include "BluetoothWorker.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(BluetoothLinkLog, "Comms.Bluetooth.BluetoothLink")

/*===========================================================================*/

BluetoothLink::BluetoothLink(SharedLinkConfigurationPtr& config, QObject* parent)
    : LinkInterface(config, parent)
    , _bluetoothConfig(qobject_cast<BluetoothConfiguration*>(config.get()))
#if defined(Q_OS_MACOS)
    , _workerThread(nullptr)
#else
    , _workerThread(new QThread(this))
#endif
{
    if (!_bluetoothConfig) {
        qCCritical(BluetoothLinkLog) << "Invalid BluetoothConfiguration";
        return;
    }

#if defined(Q_OS_MACOS)
    // Qt's IOBluetooth backend requires a CFRunLoop, which ordinary QThreads lack.
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        qCCritical(BluetoothLinkLog) << "macOS Bluetooth link must be created on the main thread";
        return;
    }
    _worker = BluetoothWorker::create(_bluetoothConfig, this);
#else
    _worker = BluetoothWorker::create(_bluetoothConfig);
#endif

    qCDebug(BluetoothLinkLog) << this;

    const QString threadName = (_bluetoothConfig->mode() == BluetoothConfiguration::BluetoothMode::ModeLowEnergy)
        ? QStringLiteral("BLE_%1") : QStringLiteral("Bluetooth_%1");
    if (_workerThread) {
        _workerThread->setObjectName(threadName.arg(_bluetoothConfig->name()));
        _worker->moveToThread(_workerThread.data());

        (void) connect(_workerThread.data(), &QThread::started, _worker.data(), &BluetoothWorker::setupConnection);
        (void) connect(_workerThread.data(), &QThread::finished, _worker.data(), &QObject::deleteLater);
    }

    (void) connect(_worker.data(), &BluetoothWorker::connected, this, &BluetoothLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::disconnected, this, &BluetoothLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::errorOccurred, this, &BluetoothLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::dataReceived, this, &BluetoothLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::dataSent, this, &BluetoothLink::_onDataSent, Qt::QueuedConnection);
    (void) connect(_worker.data(), &BluetoothWorker::rssiUpdated, this, &BluetoothLink::_onRssiUpdated, Qt::QueuedConnection);

    (void) connect(_bluetoothConfig, &BluetoothConfiguration::errorOccurred, this, &BluetoothLink::_onErrorOccurred);
}

BluetoothLink::~BluetoothLink()
{
    if (_workerThread) {
        if (!_workerReady && _worker && !_workerThread->isRunning()) {
            // No event loop has run yet to process the worker's deferred deletion.
            delete _worker.data();
        }
        if (_workerThread->isRunning() && _connectedCache && _worker) {
            (void) QMetaObject::invokeMethod(_worker, "disconnectLink", Qt::QueuedConnection);
        }

        _shutdownWorkerThread(_workerThread, BluetoothLinkLog());
    } else if (_worker) {
        _worker->disconnectLink();
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
    if (!_connectPending) {
        _connectPending = true;
        _disconnectedEmitted = false;
        // LinkManager must register the link before a permission failure can disconnect it.
        return QMetaObject::invokeMethod(this, &BluetoothLink::_checkPermission, Qt::QueuedConnection);
    }
    return true;
}

void BluetoothLink::disconnect()
{
    if (_connectPending) {
        _connectPending = false;
        _onDisconnected();
        return;
    }
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
    _connectPending = false;
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
    if (!_connectPending || _permissionRequestPending) {
        return;
    }

    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);

    const Qt::PermissionStatus permissionStatus = QCoreApplication::instance()->checkPermission(permission);
    if (permissionStatus == Qt::PermissionStatus::Undetermined) {
        _permissionRequestPending = true;
        QCoreApplication::instance()->requestPermission(permission, this, [this](const QPermission &perm) {
            _handlePermissionStatus(perm.status());
        });
    } else {
        _handlePermissionStatus(permissionStatus);
    }
}

void BluetoothLink::_handlePermissionStatus(Qt::PermissionStatus permissionStatus)
{
    _permissionRequestPending = false;
    if (!_connectPending) {
        return;
    }

    if (permissionStatus != Qt::PermissionStatus::Granted) {
        qCWarning(BluetoothLinkLog) << "Bluetooth Permission Denied";
        _onErrorOccurred(tr("Bluetooth Permission Denied"));
        _onDisconnected();
        return;
    }

    qCDebug(BluetoothLinkLog) << "Bluetooth Permission Granted";

    if (!_workerReady && !_workerThread) {
        if (!_worker || QThread::currentThread() != _worker->thread()) {
            qCCritical(BluetoothLinkLog) << "Bluetooth worker is unavailable on the current thread";
            _onErrorOccurred(tr("Bluetooth worker is unavailable on the current thread"));
            _onDisconnected();
            return;
        }
        _worker->setupConnection();
    } else if (!_workerReady && !_workerThread->isRunning()) {
        _workerThread->start();
        if (!_workerThread->isRunning()) {
            qCCritical(BluetoothLinkLog) << "Failed to start worker thread";
            _onErrorOccurred(tr("Failed to start Bluetooth worker thread"));
            _worker->deleteLater();
            _worker = nullptr;
            _onDisconnected();
            return;
        }
    }

    _workerReady = true;
    _connectPending = false;
    (void) QMetaObject::invokeMethod(_worker.data(), "connectLink", Qt::QueuedConnection);
}
