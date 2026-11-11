#include "GPSReceiverConnectionPolicy.h"

#include <limits>
#include <type_traits>
#include <utility>
#include <variant>

#include <QtCore/QStringList>

#include "GPSReceiverConfig.h"
#include "GPSReceiverSession.h"
#include "GPSTransport.h"
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

GPSReceiverConnectability GPSReceiverConnectionPolicy::connectability(const GPSReceiverConfiguration& configuration)
{
    const bool configuredBase = configuration.receiverRole == RTKSettings::ConfiguredBase;
    const auto connection = configuration.effectiveConnectionType();
    GPSReceiverConnectability result;
    result.type = configuredBase ? gpsReceiverTypeForManufacturer(configuration.baseReceiverManufacturer)
                                 : std::optional(GPSType::Passive);
    const GPSReceiverCapabilities family =
        result.type ? gpsReceiverCapabilities(*result.type) : GPSReceiverCapabilities{};
    const auto supportsMode = [&family]<typename Mode>(const Mode&) {
        if constexpr (std::is_same_v<Mode, GPSBaseStationConfig::Fixed>) {
            return family.rtkBase;
        } else if constexpr (std::is_same_v<Mode, GPSBaseStationConfig::SurveyIn>) {
            return family.surveyIn;
        } else {
            return family.receiverAveraging;
        }
    };
    result.baseModeSupported = family.passive || std::visit(supportsMode, configuration.base.mode);
    result.connectionSupported = !configuredBase || connection != RTKSettings::Udp;

    const auto validPort = [](uint32_t port) { return port != 0 && port <= (std::numeric_limits<quint16>::max)(); };
    QString endpointError;
    // A network connection runs at its bridge's rate, whatever serial rate is saved.
    uint32_t baudRate = GPSTransport::BRIDGE_BAUD_RATE;
    switch (connection) {
        case RTKSettings::Serial:
            result.hasEndpoint = !configuration.serialDevice.trimmed().isEmpty();
            endpointError = tr("Select an available serial device that is not a bootloader.");
            baudRate = configuration.serialBaudRate;
            break;
        case RTKSettings::Tcp:
            result.hasEndpoint = !configuration.tcpHost.trimmed().isEmpty() && validPort(configuration.tcpPort);
            endpointError = tr("Enter the receiver's TCP host and port.");
            break;
        case RTKSettings::Udp:
            result.hasEndpoint = validPort(configuration.udpPort);
            endpointError = tr("Enter the UDP port that receives the data.");
            break;
    }

    if (!result.type) {
        result.error = tr("Select a receiver type before connecting.");
    } else if (!result.connectionSupported) {
        result.error = tr("A configured base needs a serial or TCP connection. UDP only receives data.");
    } else if (!result.hasEndpoint) {
        result.error = endpointError;
    } else if (!result.baseModeSupported) {
        result.error = gpsReceiverConfigErrorText(GPSReceiverConfigError::UnsupportedBaseMode);
    } else {
        // The request a connection sends, without the flash-save consent only the user's Connect may add.
        (void) gpsReceiverConfigFor(configuration.base, *result.type, baudRate, false, &result.error);
    }
    return result;
}

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
    if (std::exchange(_unconnectable, false)) {
        // The changed configuration is tried as at startup, and the message about the previous one is stale.
        _receiver.setConnectionError({});
        _startPending = true;
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

void GPSReceiverConnectionPolicy::_rejectConfiguration(const QString& error)
{
    qCDebug(GPSReceiverConnectionPolicyLog) << "The configuration cannot connect:" << error;
    _cancelRetries();
    _owner = Owner::None;
    _unconnectable = true;
    _receiver.setConnectionError(error);
}

void GPSReceiverConnectionPolicy::reset()
{
    _cancelRetries();
    _owner = Owner::None;
    _startPending = false;
    _unconnectable = false;
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
    _receiver.endConnection(GPSReceiverConnector::ShownError::Clear);
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

GPSReceiverSessionOutcome GPSReceiverConnectionPolicy::sessionEnded(GPSReceiverSessionEnd end)
{
    const bool portRemoved = end == GPSReceiverSessionEnd::Unplugged;
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
    if (!_configuration.autoConnect || _paused || _unconnectable || _receiver.hasReceiver()) {
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
    return connectability(_configuration).hasEndpoint;
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
    // Neither its port nor a retry is waited for when the configuration cannot connect.
    if (const QString error = connectability(_configuration).error; !error.isEmpty()) {
        _rejectConfiguration(error);
        return;
    }
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
    const GPSReceiverConnectability check = connectability(_configuration);
    if (!check.type || !check.error.isEmpty()) {
        _receiver.setConnectionError(check.error);
        return false;
    }
    const GPSType type = *check.type;
    const auto connection = _configuration.effectiveConnectionType();
    if (connection == RTKSettings::Tcp) {
        return _receiver.connectTcp(_configuration.tcpHost.trimmed(), static_cast<quint16>(_configuration.tcpPort),
                                    type, allowPersistentChanges);
    }
    if (connection == RTKSettings::Udp) {
        return _receiver.connectUdp(static_cast<quint16>(_configuration.udpPort), type);
    }
#ifndef QGC_NO_SERIAL_LINK
    // What follows depends on the devices present, so automatic connection retries it.
    const QString device = _configuration.serialDevice.trimmed();
    if (!_serialDeviceAvailable(device)) {
        _receiver.setConnectionError(tr("Select an available serial device that is not a bootloader."));
        return false;
    }
    auto claim = _receiver.claimSerialPort(device);
    if (!claim) {
        _receiver.setConnectionError(tr("The selected serial device is already in use."));
        return false;
    }
    // Zero asks configurable receivers to detect the rate.
    return _receiver.connectSerial(device, type, _configuration.serialBaudRate, allowPersistentChanges,
                                   std::move(claim));
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
    // The USB identity only chose the port; the saved manufacturer chooses the family, detecting it when Automatic,
    // and the receiver detects the rate.
    GPSReceiverConfiguration discovered = _configuration;
    discovered.serialDevice = device;
    discovered.serialBaudRate = 0;
    const GPSReceiverConnectability check = connectability(discovered);
    if (!check.type || !check.error.isEmpty()) {
        _rejectConfiguration(check.error);
        return;
    }
    auto claim = _receiver.claimSerialPort(device);
    if (!claim) {
        return;
    }
    _owner = Owner::Discovered;
    _autoPort = device;
    _waitingPorts.clear();
    _retryDeadlineUs.reset();
    if (!_receiver.connectSerial(device, *check.type, discovered.serialBaudRate, false, std::move(claim))) {
        _scheduleRetry();
    }
}
#endif
