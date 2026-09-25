#include "RTKConnectionPolicy.h"

#include <algorithm>
#include <iterator>

#include <QtCore/QSet>

#include "AutoConnectSettings.h"
#include "GPSNotificationQueue.h"
#include "GPSReceiverConfig.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

QGC_LOGGING_CATEGORY(RTKConnectionPolicyLog, "GPS.RTK.RTKConnectionPolicy")

namespace {
GPSRtk::ConnectionType selectedConnection(RTKSettings* settings)
{
    const auto saved = static_cast<GPSRtk::ConnectionType>(settings->connectionType()->rawValue().toInt());
#ifdef QGC_NO_SERIAL_LINK
    return saved == GPSRtk::Udp ? GPSRtk::Udp : GPSRtk::Tcp;
#else
    return saved == GPSRtk::Tcp || saved == GPSRtk::Udp ? saved : GPSRtk::Serial;
#endif
}

GPSRtk::ReceiverRole selectedRole(RTKSettings* settings)
{
    const auto saved = static_cast<GPSRtk::ReceiverRole>(settings->receiverRole()->rawValue().toInt());
    return saved == GPSRtk::PositionOnly || saved == GPSRtk::Passive ? saved : GPSRtk::ConfiguredBase;
}
}  // namespace

RTKConnectionPolicy::RTKConnectionPolicy(RTKConnectionTarget& receiver, RTKSettings* settings,
                                         AutoConnectSettings* autoConnectSettings, QObject* parent)
    : QObject(parent)
    , _receiver(receiver)
    , _settings(settings)
    , _autoConnect(autoConnectSettings->autoConnectRTKGPS())
{
    // Retries reuse the saved connection, so changing it ends them.
    for (Fact* fact : {settings->receiverRole(), settings->connectionType(), settings->tcpHost(), settings->tcpPort(),
                       settings->udpPort(), settings->serialDevice(), settings->serialBaudRate(),
                       settings->baseReceiverManufacturers(), settings->useFixedBasePosition()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() {
            if (_owner == Owner::Manual && !_receiver.hasReceiver()) {
                reset();
            }
        });
    }
    connect(_autoConnect, &Fact::rawValueChanged, this, [this](const QVariant& value) {
        if (value.toBool() && _owner == Owner::Manual) {
            reset();
        }
    });
}

bool RTKConnectionPolicy::reconnecting() const
{
    return _owner == Owner::Manual && _established && !_receiver.hasReceiver();
}

void RTKConnectionPolicy::reset()
{
    const bool wasReconnecting = reconnecting();
    _revision.invalidate();
    _owner = Owner::None;
    _established = false;
    _waitingForPort = false;
    _autoPort.clear();
    _waitingPorts.clear();
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = kInitialRetryDelayMs;
    if (wasReconnecting) {
        qCDebug(RTKConnectionPolicyLog) << "Automatic reconnect cancelled";
        emit reconnectingChanged();
    }
}

bool RTKConnectionPolicy::connectConfigured(bool allowPersistentChanges)
{
    const GPSNotificationQueue::Scope publish(_receiver.notifications());
    reset();
    const auto operation = _revision.current(this);
    if (!_connectConfigured(allowPersistentChanges, true) || !operation.isCurrent()) {
        return false;
    }
    _owner = Owner::Manual;
    return true;
}

void RTKConnectionPolicy::connectSaved()
{
    const GPSNotificationQueue::Scope publish(_receiver.notifications());
    reset();
    if (_autoConnectEnabled()) {
        // Waiting for the saved receiver would suspend discovery, so only a receiver present now is connected.
        const auto operation = _revision.current(this);
        if (_connectConfigured(false, false) && operation.isCurrent()) {
            _owner = Owner::Manual;
        }
        return;
    }
    // Waiting for an absent receiver is the same as waiting for a lost one to return.
    _owner = Owner::Manual;
    _established = true;
    _retryManual();
    emit reconnectingChanged();
}

void RTKConnectionPolicy::disconnectConfigured()
{
    const GPSNotificationQueue::Scope publish(_receiver.notifications());
    reset();
    _disableAutoConnect();
    _receiver.disconnectReceiver(true);
}

void RTKConnectionPolicy::stop()
{
    const GPSNotificationQueue::Scope publish(_receiver.notifications());
    const bool autoSession = _owner == Owner::Auto;
    reset();
    if (autoSession) {
        _receiver.disconnectReceiver(false);
    }
}

void RTKConnectionPolicy::receiverReady()
{
    if (_owner == Owner::Manual && !std::exchange(_established, true)) {
        qCDebug(RTKConnectionPolicyLog) << "Automatic reconnect armed for the manual receiver connection";
    }
    _waitingForPort = false;
    _retryDeadline = QDeadlineTimer::Forever;
    _retryDelayMs = kInitialRetryDelayMs;
}

QString RTKConnectionPolicy::sessionEnded(GPSConnectionError error, const QString& detail, bool portRemoved)
{
    switch (_owner) {
        case Owner::None:
            return {};
        case Owner::Auto:
            if (portRemoved) {
                // Discovery connects the receiver again when it returns.
                reset();
                return tr("Receiver unplugged.");
            }
            _scheduleRetry();
            return {};
        case Owner::Manual:
            if (!_established) {
                reset();
                return {};
            }
            if (portRemoved) {
                _waitingForPort = true;
                return tr("Receiver unplugged. Reconnecting when it is plugged back in.");
            }
            _scheduleRetry();
            return error == GPSConnectionError::ConfigFailed && !detail.isEmpty()
                       ? tr("Receiver configuration failed: %1. Reconnecting automatically.").arg(detail)
                       : tr("Receiver connection lost. Reconnecting automatically.");
    }
    return {};
}

void RTKConnectionPolicy::_scheduleRetry()
{
    qCDebug(RTKConnectionPolicyLog) << "Retrying the receiver connection in" << _retryDelayMs << "ms";
    _retryDeadline.setRemainingTime(_retryDelayMs);
    _retryDelayMs = (std::min) (_retryDelayMs * 2, kMaxRetryDelayMs);
}

void RTKConnectionPolicy::update()
{
    const GPSNotificationQueue::Scope publish(_receiver.notifications());
    if (_owner != Owner::Manual) {
        _updateAutoConnection();
        return;
    }
    if (!_established || _receiver.hasReceiver()) {
        return;
    }
    if (!_waitingForPort) {
        if (_retryDeadline.hasExpired()) {
            _retryManual();
        }
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    auto* serialPorts = _receiver.serialPorts();
    if (!serialPorts) {
        return;
    }
    const auto operation = _revision.current(this);
    const auto ports = serialPorts->availablePorts();
    if (!operation.isCurrent() || !_waitingForPort) {
        return;
    }
    const QString device = _settings->serialDevice()->rawValue().toString().trimmed();
    if (std::any_of(ports.cbegin(), ports.cend(),
                    [&device](const auto& port) { return port.systemLocation == device; })) {
        // Retry as soon as the receiver returns instead of waiting out the backoff.
        _retryManual();
    }
#endif
}

void RTKConnectionPolicy::_retryManual()
{
    const auto operation = _revision.current(this);
    _waitingForPort = false;
    _retryDeadline = QDeadlineTimer::Forever;
    // Flash-save consent is one-use and never reused by automatic attempts.
    if (_connectConfigured(false, false) || !operation.isCurrent()) {
        return;
    }
    _waitingForPort = selectedConnection(_settings) == GPSRtk::Serial &&
                      _receiver.connectionError() == GPSConnectionError::OpenFailed;
    _scheduleRetry();
}

bool RTKConnectionPolicy::_connectConfigured(bool allowPersistentChanges, bool userRequested)
{
    const auto operation = _revision.current(this);
    const auto role = selectedRole(_settings);
    const auto type = role == GPSRtk::ConfiguredBase
                          ? GPSRtk::typeForManufacturer(_settings->baseReceiverManufacturers()->rawValue().toInt())
                          : std::optional(GPSType::passive);
    if (!type) {
        _receiver.setConnectionError(GPSConnectionError::ConfigFailed,
                                     tr("Select a specific receiver type before connecting."));
        return false;
    }
    if (_receiver.hasReceiver()) {
        _receiver.setConnectionError(GPSConnectionError::OpenFailed,
                                     tr("Disconnect the current receiver before connecting another."));
        return false;
    }
    const auto connection = selectedConnection(_settings);
    const bool tcp = connection == GPSRtk::Tcp;
    const QString host = _settings->tcpHost()->rawValue().toString().trimmed();
    const uint tcpPort = _settings->tcpPort()->rawValue().toUInt();
    if (tcp && (host.isEmpty() || tcpPort == 0 || tcpPort > 65535)) {
        _receiver.setConnectionError(GPSConnectionError::OpenFailed, tr("Enter the receiver's TCP host and port."));
        return false;
    }
    if (connection == GPSRtk::Udp) {
        const uint udpPort = _settings->udpPort()->rawValue().toUInt();
        if (role == GPSRtk::ConfiguredBase) {
            _receiver.setConnectionError(
                GPSConnectionError::ConfigFailed,
                tr("A configured base needs a serial or TCP connection. UDP only receives data."));
            return false;
        }
        if (udpPort == 0 || udpPort > 65535) {
            _receiver.setConnectionError(GPSConnectionError::OpenFailed,
                                         tr("Enter the UDP port that receives the data."));
            return false;
        }
        if (userRequested) {
            _disableAutoConnect();
        }
        return _receiver.connectUdp(static_cast<quint16>(udpPort), *type);
    }
#ifndef QGC_NO_SERIAL_LINK
    const QString device = _settings->serialDevice()->rawValue().toString().trimmed();
    // Zero asks configurable receivers to detect the rate.
    const auto baud = _settings->serialBaudRate()->rawValue().toULongLong();
    if (!tcp) {
        auto* serialPorts = _receiver.serialPorts();
        const auto ports = serialPorts ? serialPorts->availablePorts() : QList<SerialPortManager::Port>{};
        if (!operation.isCurrent()) {
            return false;
        }
        const auto port = std::find_if(ports.cbegin(), ports.cend(),
                                       [&device](const auto& candidate) { return candidate.systemLocation == device; });
        if (device.isEmpty() || port == ports.cend() || port->bootloader) {
            _receiver.setConnectionError(GPSConnectionError::OpenFailed,
                                         tr("Select an available serial device that is not a bootloader."));
            return false;
        }
        if ((baud != 0 && (baud < 1200 || baud > 4000000)) || (baud == 0 && *type == GPSType::passive)) {
            _receiver.setConnectionError(GPSConnectionError::ConfigFailed,
                                         gpsReceiverConfigErrorText(GPSReceiverConfigError::InvalidBaudRate));
            return false;
        }
    }
#endif
    if (userRequested) {
        _disableAutoConnect();
    }
    if (tcp) {
        return _receiver.connectTcp(host, static_cast<quint16>(tcpPort), *type, allowPersistentChanges);
    }
#ifndef QGC_NO_SERIAL_LINK
    return _receiver.connectSerial(device, *type, static_cast<uint32_t>(baud), allowPersistentChanges);
#else
    return false;
#endif
}

void RTKConnectionPolicy::_disableAutoConnect()
{
    _receiver.notifications().post(reinterpret_cast<quintptr>(_autoConnect),
                                   [fact = _autoConnect]() { fact->setRawValue(false); });
}

bool RTKConnectionPolicy::_autoConnectEnabled() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    // Discovery configures known base receivers; it would replace a network or passive receiver.
    return _autoConnect->rawValue().toBool() && selectedConnection(_settings) == GPSRtk::Serial &&
           selectedRole(_settings) == GPSRtk::ConfiguredBase;
#endif
}

void RTKConnectionPolicy::_updateAutoConnection()
{
#ifndef QGC_NO_SERIAL_LINK
    auto* serialPorts = _receiver.serialPorts();
    if (!serialPorts || !_autoConnectEnabled()) {
        stop();
        return;
    }
    const auto operation = _revision.advance(this);
    const auto ports = serialPorts->availablePorts();
    if (!operation.isCurrent() || _owner == Owner::Manual) {
        return;
    }
    if (!_autoConnectEnabled()) {
        stop();
        return;
    }
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    if (_owner == Owner::Auto && !present.contains(_autoPort)) {
        stop();
        return;
    }
    for (auto it = _waitingPorts.begin(); it != _waitingPorts.end();) {
        it = !present.contains(it.key()) ? _waitingPorts.erase(it) : std::next(it);
    }
    if (_receiver.hasReceiver()) {
        return;
    }
    const auto connectPort = [this](const SerialPortManager::Port& port) {
        const auto attempt = _revision.current(this);
        _owner = Owner::Auto;
        _autoPort = port.systemLocation;
        _waitingPorts.clear();
        _retryDeadline = QDeadlineTimer::Forever;
        if (!_receiver.connectDiscovered(port.systemLocation, port.boardName) && attempt.isCurrent() &&
            !_receiver.hasReceiver()) {
            _scheduleRetry();
        }
    };
    if (_owner == Owner::Auto) {
        if (!_retryDeadline.hasExpired()) {
            return;
        }
        for (const auto& port : ports) {
            if (port.systemLocation == _autoPort && !port.bootloader &&
                port.boardType == QGCSerialPortInfo::BoardTypeRTKGPS &&
                serialPorts->canReservePort(port.systemLocation)) {
                connectPort(port);
                break;
            }
        }
        return;
    }
    QSet<QString> seenDevices;
    for (const auto& port : ports) {
        if (port.boardType != QGCSerialPortInfo::BoardTypeRTKGPS || port.bootloader) {
            _waitingPorts.remove(port.systemLocation);
            continue;
        }
        // A labelled NMEA interface remains a receiver candidate on composite GPS devices.
        const bool duplicate = !port.physicalDeviceId.isEmpty() && seenDevices.contains(port.physicalDeviceId);
        if (!port.physicalDeviceId.isEmpty()) {
            seenDevices.insert(port.physicalDeviceId);
        }
        if ((duplicate && !port.description.contains(QStringLiteral("NMEA"))) ||
            !serialPorts->canReservePort(port.systemLocation)) {
            _waitingPorts.remove(port.systemLocation);
            continue;
        }
        auto it = _waitingPorts.find(port.systemLocation);
        if (it == _waitingPorts.end()) {
            _waitingPorts[port.systemLocation].start();
        } else if (it->elapsed() >= _connectDelayMs) {
            connectPort(port);
            return;
        }
    }
#endif
}
