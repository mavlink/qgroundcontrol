#include "RTKConnectionPolicy.h"

#include <algorithm>
#include <iterator>

#include <QtCore/QPointer>
#include <QtCore/QSet>

#include "AutoConnectSettings.h"
#include "GPSReceiverConfig.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

QGC_LOGGING_CATEGORY(RTKConnectionPolicyLog, "GPS.RTK.RTKConnectionPolicy")

namespace {
bool tcpSelected()
{
#ifdef QGC_NO_SERIAL_LINK
    return true;
#else
    return SettingsManager::instance()->rtkSettings()->connectionType()->rawValue().toInt() == GPSRtk::Tcp;
#endif
}
}  // namespace

RTKConnectionPolicy::RTKConnectionPolicy(GPSRtk* receiver)
    : QObject(receiver)
    , _receiver(receiver)
{
    auto* settings = SettingsManager::instance()->rtkSettings();
    // Retries reuse the saved connection, so changing it ends them.
    for (Fact* fact :
         {settings->connectionType(), settings->tcpHost(), settings->tcpPort(), settings->serialDevice(),
          settings->serialBaudRate(), settings->baseReceiverManufacturers(), settings->useFixedBasePosition()}) {
        connect(fact, &Fact::rawValueChanged, this, [this]() {
            if (_owner == Owner::Manual && !_receiver->hasReceiver()) {
                reset();
            }
        });
    }
    connect(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), &Fact::rawValueChanged, this,
            [this](const QVariant& value) {
                if (value.toBool() && _owner == Owner::Manual) {
                    reset();
                }
            });
}

bool RTKConnectionPolicy::reconnecting() const
{
    return _owner == Owner::Manual && _established && !_receiver->hasReceiver();
}

void RTKConnectionPolicy::reset()
{
    const bool wasReconnecting = reconnecting();
    ++_revision;
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
    const GPSNotificationQueue::Scope publish(_receiver->_notifications);
    reset();
    const QPointer<RTKConnectionPolicy> guard(this);
    const quint64 revision = _revision;
    if (!_connectConfigured(allowPersistentChanges) || !guard || revision != _revision) {
        return false;
    }
    _owner = Owner::Manual;
    return true;
}

void RTKConnectionPolicy::disconnectConfigured()
{
    const GPSNotificationQueue::Scope publish(_receiver->_notifications);
    reset();
    _receiver->_stageFact(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), false);
    _receiver->_disconnect(true);
}

void RTKConnectionPolicy::stop()
{
    const GPSNotificationQueue::Scope publish(_receiver->_notifications);
    const bool autoSession = _owner == Owner::Auto;
    reset();
    if (autoSession) {
        _receiver->_disconnect(false);
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
    const GPSNotificationQueue::Scope publish(_receiver->_notifications);
    if (_owner != Owner::Manual) {
        _updateAutoConnection();
        return;
    }
    if (!_established || _receiver->hasReceiver()) {
        return;
    }
    if (!_waitingForPort) {
        if (_retryDeadline.hasExpired()) {
            _retryManual();
        }
        return;
    }
#ifndef QGC_NO_SERIAL_LINK
    auto* serialPorts = _receiver->_serialPorts.data();
    if (!serialPorts) {
        return;
    }
    const QPointer<RTKConnectionPolicy> guard(this);
    const quint64 revision = _revision;
    const auto ports = serialPorts->availablePorts();
    if (!guard || revision != _revision || !_waitingForPort) {
        return;
    }
    const QString device = SettingsManager::instance()->rtkSettings()->serialDevice()->rawValue().toString().trimmed();
    if (std::any_of(ports.cbegin(), ports.cend(),
                    [&device](const auto& port) { return port.systemLocation == device; })) {
        // Retry as soon as the receiver returns instead of waiting out the backoff.
        _retryManual();
    }
#endif
}

void RTKConnectionPolicy::_retryManual()
{
    const QPointer<RTKConnectionPolicy> guard(this);
    const quint64 revision = _revision;
    _waitingForPort = false;
    _retryDeadline = QDeadlineTimer::Forever;
    // Flash-save consent is one-use and never reused by automatic attempts.
    if (_connectConfigured(false) || !guard || revision != _revision) {
        return;
    }
    _waitingForPort = !tcpSelected() && _receiver->_connectionError == GPSConnectionError::OpenFailed;
    _scheduleRetry();
}

bool RTKConnectionPolicy::_connectConfigured(bool allowPersistentChanges)
{
    const QPointer<RTKConnectionPolicy> guard(this);
    const quint64 revision = _revision;
    const auto current = [this, guard, revision]() { return guard && _revision == revision; };
    auto* settings = SettingsManager::instance()->rtkSettings();
    const auto type = GPSRtk::typeForManufacturer(settings->baseReceiverManufacturers()->rawValue().toInt());
    if (!type) {
        _receiver->_setError(GPSConnectionError::ConfigFailed,
                             tr("Select a specific receiver type before connecting."));
        return false;
    }
    if (_receiver->hasReceiver()) {
        _receiver->_setError(GPSConnectionError::OpenFailed,
                             tr("Disconnect the current receiver before connecting another."));
        return false;
    }
    const bool tcp = tcpSelected();
    const QString host = settings->tcpHost()->rawValue().toString().trimmed();
    const uint tcpPort = settings->tcpPort()->rawValue().toUInt();
    if (tcp && (host.isEmpty() || tcpPort == 0 || tcpPort > 65535)) {
        _receiver->_setError(GPSConnectionError::OpenFailed, tr("Enter the receiver's TCP host and port."));
        return false;
    }
#ifndef QGC_NO_SERIAL_LINK
    const QString device = settings->serialDevice()->rawValue().toString().trimmed();
    // Zero asks configurable receivers to detect the rate.
    const auto baud = settings->serialBaudRate()->rawValue().toULongLong();
    if (!tcp) {
        auto* serialPorts = _receiver->_serialPorts.data();
        const auto ports = serialPorts ? serialPorts->availablePorts() : QList<SerialPortManager::Port>{};
        if (!current()) {
            return false;
        }
        const auto port = std::find_if(ports.cbegin(), ports.cend(),
                                       [&device](const auto& candidate) { return candidate.systemLocation == device; });
        if (device.isEmpty() || port == ports.cend() || port->bootloader) {
            _receiver->_setError(GPSConnectionError::OpenFailed,
                                 tr("Select an available serial device that is not a bootloader."));
            return false;
        }
        if ((baud != 0 && (baud < 1200 || baud > 4000000)) || (baud == 0 && *type == GPSType::passive)) {
            _receiver->_setError(GPSConnectionError::ConfigFailed,
                                 gpsReceiverConfigErrorText(GPSReceiverConfigError::InvalidBaudRate));
            return false;
        }
    }
#endif
    _receiver->_stageFact(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), false);
    if (tcp) {
        return _receiver->_connectTcpGPS(host, static_cast<quint16>(tcpPort), *type, allowPersistentChanges);
    }
#ifndef QGC_NO_SERIAL_LINK
    return _receiver->_connectSerialGPS(device, *type, static_cast<uint32_t>(baud), allowPersistentChanges);
#else
    return false;
#endif
}

bool RTKConnectionPolicy::_autoConnectEnabled() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    // Serial discovery would replace a selected TCP receiver.
    return SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->rawValue().toBool() &&
           !tcpSelected();
#endif
}

void RTKConnectionPolicy::_updateAutoConnection()
{
#ifndef QGC_NO_SERIAL_LINK
    auto* serialPorts = _receiver->_serialPorts.data();
    if (!serialPorts || !_autoConnectEnabled()) {
        stop();
        return;
    }
    const QPointer<RTKConnectionPolicy> guard(this);
    const quint64 revision = ++_revision;
    const auto ports = serialPorts->availablePorts();
    if (!guard || revision != _revision || _owner == Owner::Manual) {
        return;
    }
    if (!_autoConnectEnabled()) {
        stop();
        return;
    }
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QSet<QString> present;
    for (const auto& port : ports) {
        present.insert(port.systemLocation);
    }
    const QString nmeaPort = settings->nmeaSource()->rawValue().toInt() == AutoConnectSettings::NmeaSourceSerial
                                 ? settings->autoConnectNmeaPort()->rawValue().toString().trimmed()
                                 : QString();
    if (_owner == Owner::Auto && (!present.contains(_autoPort) || _autoPort == nmeaPort)) {
        stop();
        return;
    }
    for (auto it = _waitingPorts.begin(); it != _waitingPorts.end();) {
        it = !present.contains(it.key()) ? _waitingPorts.erase(it) : std::next(it);
    }
    if (_receiver->hasReceiver()) {
        return;
    }
    const auto connectPort = [this, guard](const SerialPortManager::Port& port) {
        const quint64 attempt = _revision;
        _owner = Owner::Auto;
        _autoPort = port.systemLocation;
        _waitingPorts.clear();
        _retryDeadline = QDeadlineTimer::Forever;
        if (!_receiver->_connectGPS(port.systemLocation, port.boardName) && guard && attempt == _revision &&
            !_receiver->hasReceiver()) {
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
        if (port.boardType != QGCSerialPortInfo::BoardTypeRTKGPS || port.bootloader ||
            port.systemLocation == nmeaPort) {
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
