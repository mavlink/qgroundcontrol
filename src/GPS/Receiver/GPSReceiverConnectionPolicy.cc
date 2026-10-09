#include "GPSReceiverConnectionPolicy.h"

#include <utility>

#include <QtCore/QStringList>

#include "GPSReceiverConfig.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

QGC_LOGGING_CATEGORY(GPSReceiverConnectionPolicyLog, "GPS.Receiver.GPSReceiverConnectionPolicy")

GPSReceiverConnectionPolicy::GPSReceiverConnectionPolicy(GPSReceiverConnector& receiver,
                                                         const GPSReceiverConfiguration& configuration,
                                                         RuntimeScheduler* scheduler)
    : _receiver(receiver)
    , _configuration(configuration)
    , _scheduler(scheduler)
{}

void GPSReceiverConnectionPolicy::configurationChanged(const GPSReceiverConfiguration& previous)
{
    const bool connectionChanged = !previous.sameConnection(_configuration);
    const bool turnedOn = !previous.autoConnect && _configuration.autoConnect;
    const bool turnedOff = previous.autoConnect && !_configuration.autoConnect;
    if (connectionChanged || turnedOff) {
        _cancelRetries();
    }
    if (turnedOn) {
        _paused = false;
        _startPending = true;
        // A saved receiver the user already connected is kept connected from now on.
        _keepConnected = _owner == Owner::Saved && !connectionChanged;
    }
}

void GPSReceiverConnectionPolicy::_cancelRetries()
{
    if (_keepConnected) {
        qCDebug(GPSReceiverConnectionPolicyLog) << "Automatic reconnect cancelled";
    }
    // Nothing reconnects any more, so a message about the pending reconnect would be stale.
    if ((_keepConnected || _retryDeadlineUs) && !_receiver.hasReceiver()) {
        _receiver.setConnectionError({});
    }
    _keepConnected = false;
    _waitingForPort = false;
    _autoPort.clear();
#ifndef QGC_NO_SERIAL_LINK
    _waitingPorts.clear();
#endif
    _retryDeadlineUs.reset();
    _retryBackoff.reset();
}

void GPSReceiverConnectionPolicy::reset()
{
    _cancelRetries();
    _owner = Owner::None;
    _startPending = false;
}

bool GPSReceiverConnectionPolicy::connectConfigured(bool allowPersistentChanges)
{
    // A rejected request leaves the current connection's ownership and retries as they are.
    if (_receiver.hasReceiver()) {
        _receiver.setConnectionError(tr("Disconnect the current receiver before connecting another."));
        return false;
    }
    reset();
    _paused = false;
    if (!_connectConfigured(allowPersistentChanges)) {
        return false;
    }
    _owner = Owner::Saved;
    return true;
}

void GPSReceiverConnectionPolicy::disconnectConfigured()
{
    reset();
    _paused = true;
    _receiver.endConnection(true);
}

void GPSReceiverConnectionPolicy::receiverReady()
{
    if (_owner == Owner::Saved && _configuration.autoConnect && !std::exchange(_keepConnected, true)) {
        qCDebug(GPSReceiverConnectionPolicyLog) << "Automatic reconnect armed for the saved receiver";
    }
    _waitingForPort = false;
    _retryDeadlineUs.reset();
    _retryBackoff.reset();
}

GPSReceiverSessionOutcome GPSReceiverConnectionPolicy::sessionEnded(bool portRemoved)
{
    if (_owner == Owner::Discovered && _discoveryEnabled()) {
        if (portRemoved) {
            // Discovery connects the receiver again when it returns.
            _owner = Owner::None;
            _cancelRetries();
            return GPSReceiverSessionOutcome::Unplugged;
        }
        _scheduleRetry();
        return GPSReceiverSessionOutcome::AutoRetrying;
    }
    if (_owner == Owner::Saved && _keepConnected) {
        if (portRemoved) {
            _waitingForPort = true;
            return GPSReceiverSessionOutcome::WaitingForPort;
        }
        _scheduleRetry();
        return GPSReceiverSessionOutcome::Retrying;
    }
    _owner = Owner::None;
    _cancelRetries();
    return GPSReceiverSessionOutcome::None;
}

void GPSReceiverConnectionPolicy::_scheduleRetry()
{
    const auto delay = _retryBackoff.next();
    qCDebug(GPSReceiverConnectionPolicyLog) << "Retrying the receiver connection in" << delay.count() << "ms";
    _retryDeadlineUs = _scheduler->nowUs() + static_cast<quint64>(std::chrono::microseconds(delay).count());
}

void GPSReceiverConnectionPolicy::update()
{
    if (!_configuration.autoConnect || _paused || _receiver.hasReceiver()) {
        return;
    }
    if (std::exchange(_startPending, false) && _hasSavedReceiver()) {
        _owner = Owner::Saved;
        _keepConnected = true;
    }
    if (_keepConnected) {
        _updateSaved();
    } else if (_discoveryEnabled()) {
        _updateDiscovery();
    }
}

bool GPSReceiverConnectionPolicy::_hasSavedReceiver() const
{
    switch (_configuration.effectiveConnectionType()) {
        case RTKSettings::Serial:
            return !_configuration.serialDevice.trimmed().isEmpty();
        case RTKSettings::Tcp:
            return !_configuration.tcpHost.trimmed().isEmpty() && _configuration.tcpPort != 0;
        case RTKSettings::Udp:
            // UDP only receives, so it cannot carry a configured base.
            return _configuration.receiverRole != RTKSettings::ConfiguredBase;
    }
    return false;
}

bool GPSReceiverConnectionPolicy::_discoveryEnabled() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    // Discovery configures known base receivers, so it never replaces a saved, network or passive receiver.
    return _configuration.autoConnect && !_paused && _configuration.receiverRole == RTKSettings::ConfiguredBase &&
           _configuration.effectiveConnectionType() == RTKSettings::Serial &&
           _configuration.serialDevice.trimmed().isEmpty();
#endif
}

void GPSReceiverConnectionPolicy::_updateSaved()
{
    if (_configuration.effectiveConnectionType() == RTKSettings::Serial) {
        if (!_serialDeviceAvailable(_configuration.serialDevice.trimmed())) {
            if (!std::exchange(_waitingForPort, true)) {
                //: %1 is a serial device, such as /dev/ttyACM0 or COM3
                _receiver.setConnectionError(
                    tr("Waiting for the receiver on %1.").arg(_configuration.serialDevice.trimmed()));
            }
            return;
        }
        if (std::exchange(_waitingForPort, false)) {
            // Retry as soon as the receiver returns instead of waiting out the backoff.
            _attemptSaved();
            return;
        }
    }
    if (!_retryDeadlineUs || _scheduler->nowUs() >= *_retryDeadlineUs) {
        _attemptSaved();
    }
}

void GPSReceiverConnectionPolicy::_attemptSaved()
{
    _retryDeadlineUs.reset();
    // Flash-save consent is one-use and never passed by automatic attempts.
    if (!_connectConfigured(false)) {
        _scheduleRetry();
    }
}

bool GPSReceiverConnectionPolicy::_serialDeviceAvailable([[maybe_unused]] const QString& device) const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    return _receiver.serialPortAvailable(device);
#endif
}

bool GPSReceiverConnectionPolicy::_connectConfigured(bool allowPersistentChanges)
{
    const bool configuredBase = _configuration.receiverRole == RTKSettings::ConfiguredBase;
    const auto type = configuredBase ? gpsReceiverTypeForManufacturer(_configuration.baseReceiverManufacturer)
                                     : std::optional(GPSType::passive);
    if (!type) {
        _receiver.setConnectionError(tr("Select a receiver type before connecting."));
        return false;
    }
    const auto connection = _configuration.effectiveConnectionType();
    if (connection == RTKSettings::Tcp) {
        const QString host = _configuration.tcpHost.trimmed();
        const uint tcpPort = _configuration.tcpPort;
        if (host.isEmpty() || tcpPort == 0 || tcpPort > 65535) {
            _receiver.setConnectionError(tr("Enter the receiver's TCP host and port."));
            return false;
        }
        return _receiver.connectTcp(host, static_cast<quint16>(tcpPort), *type, allowPersistentChanges);
    }
    if (connection == RTKSettings::Udp) {
        const uint udpPort = _configuration.udpPort;
        if (configuredBase) {
            _receiver.setConnectionError(
                tr("A configured base needs a serial or TCP connection. UDP only receives data."));
            return false;
        }
        if (udpPort == 0 || udpPort > 65535) {
            _receiver.setConnectionError(tr("Enter the UDP port that receives the data."));
            return false;
        }
        return _receiver.connectUdp(static_cast<quint16>(udpPort), *type);
    }
#ifndef QGC_NO_SERIAL_LINK
    const QString device = _configuration.serialDevice.trimmed();
    if (!_serialDeviceAvailable(device)) {
        _receiver.setConnectionError(tr("Select an available serial device that is not a bootloader."));
        return false;
    }
    // Zero asks configurable receivers to detect the rate.
    const auto baud = _configuration.serialBaudRate;
    if (!gpsValidBaudRate(baud, *type == GPSType::passive)) {
        _receiver.setConnectionError(gpsReceiverConfigErrorText(GPSReceiverConfigError::InvalidBaudRate));
        return false;
    }
    auto claim = _receiver.claimSerialPort(device);
    if (!claim) {
        _receiver.setConnectionError(tr("The selected serial device is already in use."));
        return false;
    }
    return _receiver.connectSerial(device, *type, baud, allowPersistentChanges, std::move(claim));
#else
    return false;
#endif
}

void GPSReceiverConnectionPolicy::_updateDiscovery()
{
#ifndef QGC_NO_SERIAL_LINK
    if (_owner == Owner::Discovered) {
        if (_serialDeviceAvailable(_autoPort)) {
            if (!_retryDeadlineUs || _scheduler->nowUs() >= *_retryDeadlineUs) {
                _connectDiscovered(_autoPort);
            }
            return;
        }
        // The receiver being retried was unplugged; discovery finds it again when it returns.
        _owner = Owner::None;
        _cancelRetries();
    }
    const QStringList receivers = _receiver.rtkReceiverPorts();
    _waitingPorts.removeIf([&receivers](const QString& device) { return !receivers.contains(device); });
    const quint64 nowUs = _scheduler->nowUs();
    for (const QString& device : receivers) {
        if (_waitingPorts.settled(device, nowUs)) {
            _connectDiscovered(device);
            return;
        }
    }
#endif
}

#ifndef QGC_NO_SERIAL_LINK
void GPSReceiverConnectionPolicy::_connectDiscovered(const QString& device)
{
    auto claim = _receiver.claimSerialPort(device);
    if (!claim) {
        return;
    }
    _owner = Owner::Discovered;
    _autoPort = device;
    _waitingPorts.clear();
    _retryDeadlineUs.reset();
    // The USB identity only chose the port; the saved manufacturer chooses the family, detecting it when Automatic.
    const auto type = gpsReceiverTypeForManufacturer(_configuration.baseReceiverManufacturer);
    if (!type) {
        _receiver.setConnectionError(tr("Select a receiver type before connecting."));
    }
    if (!type || !_receiver.connectSerial(device, *type, 0, false, std::move(claim))) {
        _scheduleRetry();
    }
}
#endif
