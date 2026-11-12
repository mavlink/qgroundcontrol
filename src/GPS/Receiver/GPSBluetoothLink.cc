#include "GPSBluetoothLink.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QPermissions>

#include "BluetoothConfiguration.h"
#include "BluetoothWorker.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSBluetoothLinkLog, "GPS.Receiver.GPSBluetoothLink")

namespace {

QObject* createBluetoothDiscovery(QObject* parent)
{
    // The configuration of a Bluetooth link scans for the devices of its mode, which is classic unless it is set.
    return new BluetoothConfiguration(QStringLiteral("GNSS receiver"), parent);
}

BluetoothWorker* createBluetoothWorker(const QBluetoothDeviceInfo& device, QObject* parent)
{
    // The worker takes its device from a link's configuration, whose classic mode selects the classic worker.
    BluetoothConfiguration configuration(device.name());
    configuration.setDeviceInfo(device);
    return BluetoothWorker::create(&configuration, parent);
}

/// The permission flow of the application's Bluetooth links.
void requestBluetoothPermission(QObject* context, std::function<void(bool)> answer)
{
    QBluetoothPermission permission;
    permission.setCommunicationModes(QBluetoothPermission::Access);
    switch (QCoreApplication::instance()->checkPermission(permission)) {
        case Qt::PermissionStatus::Undetermined:
            QCoreApplication::instance()->requestPermission(permission, context,
                                                            [answer = std::move(answer)](const QPermission& asked) {
                                                                answer(asked.status() == Qt::PermissionStatus::Granted);
                                                            });
            break;
        case Qt::PermissionStatus::Granted:
            answer(true);
            break;
        case Qt::PermissionStatus::Denied:
            answer(false);
            break;
    }
}

}  // namespace

GPSBluetoothPlatform GPSBluetoothPlatform::system()
{
    return {
#ifdef Q_OS_IOS
        // Qt has no Bluetooth sockets on iOS, which gives applications no serial port profile.
        .supported = false,
#else
        .supported = true,
#endif
        .createDiscovery = createBluetoothDiscovery,
        .createWorker = createBluetoothWorker,
        .requestPermission = requestBluetoothPermission};
}

GPSBluetoothLink::GPSBluetoothLink(std::shared_ptr<GPSPipe> pipe, QBluetoothDeviceInfo device,
                                   GPSBluetoothPlatform platform, QObject* parent)
    : GPSPipeLink(std::move(pipe), parent)
    , _device(std::move(device))
    , _platform(std::move(platform))
{
    qCDebug(GPSBluetoothLinkLog) << this << _device.address().toString();
}

GPSBluetoothLink::~GPSBluetoothLink()
{
    qCDebug(GPSBluetoothLinkLog) << this;
}

void GPSBluetoothLink::openLink()
{
    if (!_platform.requestPermission) {
        _connect();
        return;
    }
    _platform.requestPermission(this, [this](bool granted) {
        if (granted) {
            _connect();
        } else {
            _end(tr("Bluetooth permission was denied"));
        }
    });
}

void GPSBluetoothLink::_connect()
{
    // The session may have ended while the user was asked for the permission.
    if (_ended) {
        return;
    }
    _worker = _platform.createWorker ? _platform.createWorker(_device, this) : nullptr;
    if (!_worker) {
        _end(tr("Bluetooth is not available"));
        return;
    }
    (void) connect(_worker, &BluetoothWorker::connected, this, [this]() { linkOpened(); });
    (void) connect(_worker, &BluetoothWorker::dataReceived, this,
                   [this](const QByteArray& data) { linkReceived(data); });
    // The worker reports what its socket took, and what it could not send as an error.
    (void) connect(_worker, &BluetoothWorker::dataSent, this,
                   [this](const QByteArray& data) { linkWrote(data.size()); });
    (void) connect(_worker, &BluetoothWorker::errorOccurred, this, &GPSBluetoothLink::_end);
    // A failure reports its error first, which is then the reason the user reads.
    (void) connect(_worker, &BluetoothWorker::disconnected, this,
                   [this]() { _end(tr("the Bluetooth device disconnected")); });
    _worker->connectLink();
}

void GPSBluetoothLink::_end(const QString& detail)
{
    if (std::exchange(_ended, true)) {
        return;
    }
    qCDebug(GPSBluetoothLinkLog) << "Link ended:" << detail;
    linkClosed(detail);
    if (_worker) {
        // Queued, as the worker reports from inside its own handlers: once they return it would reconnect by itself,
        // or search the device for another service, which stopping it prevents.
        (void) QMetaObject::invokeMethod(_worker.data(), &BluetoothWorker::disconnectLink, Qt::QueuedConnection);
    }
}

void GPSBluetoothLink::writeLink(const QByteArray& bytes)
{
    if (_worker && !_ended) {
        _worker->writeData(bytes);
    }
}

void GPSBluetoothLink::closeLink()
{
    _ended = true;
    if (_worker) {
        _worker->disconnectLink();
    }
}
