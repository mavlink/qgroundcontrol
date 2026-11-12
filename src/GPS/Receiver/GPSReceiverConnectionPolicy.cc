#include "GPSReceiverConnectionPolicy.h"

#include <limits>
#include <type_traits>
#include <utility>
#include <variant>

#include "GPSReceiverConfig.h"
#include "GPSReceiverSerialPorts.h"
#include "GPSReceiverSession.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSReceiverConnectionPolicyLog, "GPS.Receiver.GPSReceiverConnectionPolicy")

GPSReceiverConnectionPolicy::GPSReceiverConnectionPolicy(GPSReceiverConnector& receiver,
                                                         const GPSReceiverConfiguration& configuration,
                                                         const GPSReceiverConnectability& connectability,
                                                         GPSReceiverSerialPorts& serial, RuntimeScheduler* scheduler)
    : _receiver(receiver)
    , _configuration(configuration)
    , _connectability(connectability)
    , _serial(serial)
    , _scheduler(scheduler)
{}

GPSReceiverConnectability GPSReceiverConnectionPolicy::connectability(const GPSReceiverConfiguration& configuration,
                                                                      bool serialLinks, bool allowPersistentChanges)
{
    const bool configuredBase = configuration.receiverRole == RTKSettings::ConfiguredBase;
    const bool network =
        configuration.connectionType == RTKSettings::Tcp || configuration.connectionType == RTKSettings::Udp;
    const auto connection = network       ? configuration.connectionType
                            : serialLinks ? RTKSettings::Serial
                                          : RTKSettings::Tcp;
    GPSReceiverConnectability result;
    result.connection = connection;
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
        // A UDP connection only receives, so it never carries the consent.
        result.request = gpsReceiverConfigFor(configuration.base, *result.type, baudRate,
                                              allowPersistentChanges && connection != RTKSettings::Udp, &result.error);
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
        // The changed configuration is tried as at startup, and the message about the previous one is stale. A
        // refused Connect's message stays while the configuration has its reason.
        if (!_refusal) {
            _showError({});
        }
        _startPending = true;
    }
    if (_refusal && !_refusalApplies()) {
        _showError({});
    }
}

void GPSReceiverConnectionPolicy::_showError(const QString& error)
{
    _refusal.reset();
    _receiver.setConnectionError(error);
}

void GPSReceiverConnectionPolicy::_refuse(Refusal refusal)
{
    _receiver.setConnectionError(refusal.error);
    _refusal = std::move(refusal);
}

bool GPSReceiverConnectionPolicy::_refusalApplies() const
{
    if (_refusal->discovery) {
        return _discoveryEnabled();
    }
    if (_refusal->serialDevice.isEmpty()) {
        return _connectability.error == _refusal->error;
    }
    return _connectability.connection == RTKSettings::Serial &&
           _configuration.serialDevice.trimmed() == _refusal->serialDevice;
}

void GPSReceiverConnectionPolicy::_cancelRetries()
{
    if (_keepConnected) {
        qCDebug(GPSReceiverConnectionPolicyLog) << "Automatic reconnect cancelled";
    }
    // Nothing reconnects any more, so a message about the pending reconnect would be stale.
    if ((_keepConnected || _retryDeadlineUs) && !_receiver.hasReceiver()) {
        _showError({});
    }
    _keepConnected = false;
    _waitingForPort = false;
    _autoPort.clear();
    _serial.forgetRtkReceivers();
    _retryDeadlineUs.reset();
    _retryBackoff.reset();
}

void GPSReceiverConnectionPolicy::_rejectConfiguration(const QString& error)
{
    qCDebug(GPSReceiverConnectionPolicyLog) << "The configuration cannot connect:" << error;
    _cancelRetries();
    _owner = Owner::None;
    _unconnectable = true;
    _showError(error);
}

void GPSReceiverConnectionPolicy::reset()
{
    _cancelRetries();
    _owner = Owner::None;
    _startPending = false;
    _unconnectable = false;
    _refusal.reset();
}

bool GPSReceiverConnectionPolicy::connectConfigured(bool allowPersistentChanges)
{
    // A rejected request leaves the current connection's ownership and retries as they are.
    if (_receiver.hasReceiver()) {
        _showError(tr("Disconnect the current receiver before connecting another."));
        return false;
    }
    reset();
    _paused = false;
    if (!_connectConfigured(allowPersistentChanges)) {
        // As after an automatic attempt, a saved receiver whose configuration cannot connect is tried again once
        // the configuration changes.
        _unconnectable = _hasSavedReceiver() && !_connectability.error.isEmpty();
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
    return _connectability.hasEndpoint;
}

bool GPSReceiverConnectionPolicy::_discoveryEnabled() const
{
    // Discovery configures known base receivers, so it never replaces a saved, network or passive receiver. Without
    // serial links every connection is a network one.
    return _configuration.autoConnect && !_paused && _configuration.receiverRole == RTKSettings::ConfiguredBase &&
           _connectability.connection == RTKSettings::Serial && _configuration.serialDevice.trimmed().isEmpty();
}

void GPSReceiverConnectionPolicy::_updateSaved()
{
    // Neither its port nor a retry is waited for when the configuration cannot connect.
    if (!_connectability.error.isEmpty()) {
        _rejectConfiguration(_connectability.error);
        return;
    }
    if (_connectability.connection == RTKSettings::Serial) {
        if (!_serial.available(_configuration.serialDevice.trimmed())) {
            if (!std::exchange(_waitingForPort, true)) {
                //: %1 is a serial device, such as /dev/ttyACM0 or COM3
                _showError(tr("Waiting for the receiver on %1.").arg(_configuration.serialDevice.trimmed()));
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

bool GPSReceiverConnectionPolicy::_connectConfigured(bool allowPersistentChanges)
{
    // The consent is part of the request, so the request with it is built and checked for this attempt.
    const GPSReceiverConnectability check =
        allowPersistentChanges ? connectability(_configuration, _serial.supported(), true) : _connectability;
    if (!check.type || !check.error.isEmpty()) {
        _refuse({.error = check.error, .serialDevice = {}});
        return false;
    }
    // What a connection shows from here on is its own.
    _refusal.reset();
    const GPSType type = *check.type;
    if (check.connection == RTKSettings::Tcp) {
        return _receiver.connectTcp(_configuration.tcpHost.trimmed(), static_cast<quint16>(_configuration.tcpPort),
                                    type, check.request);
    }
    if (check.connection == RTKSettings::Udp) {
        // connectability() lets only a passive receiver through.
        return _receiver.connectUdp(static_cast<quint16>(_configuration.udpPort), check.request);
    }
    // What follows depends on the devices present, so automatic connection retries it.
    const QString device = _configuration.serialDevice.trimmed();
    if (!_serial.available(device)) {
        _refuse({.error = tr("Select an available serial device that is not a bootloader."), .serialDevice = device});
        return false;
    }
    GPSSerialClaim claim = _serial.claim(device);
    if (!claim) {
        // A device that nothing holds is refused where only one can be open at a time, while another is.
        _refuse({.error = _serial.held(device)
                              ? tr("The selected serial device is already in use.")
                              : tr("Another connection has a serial device open, and only one can be open at a time. "
                                   "Disconnect it first."),
                 .serialDevice = device});
        return false;
    }
    return _receiver.connectSerial(device, type, check.request, std::move(claim));
}

void GPSReceiverConnectionPolicy::_updateDiscovery()
{
    if (_owner == Owner::Discovered) {
        if (_serial.available(_autoPort)) {
            if (!_retryDeadlineUs || _scheduler->nowUs() >= *_retryDeadlineUs) {
                _connectDiscovered(_autoPort);
            }
            return;
        }
        // The receiver being retried was unplugged; discovery finds it again when it returns.
        _owner = Owner::None;
        _cancelRetries();
    }
    if (const QString device = _serial.settledRtkReceiver(_scheduler->nowUs()); !device.isEmpty()) {
        _connectDiscovered(device);
        return;
    }
    // Nothing is connected while another connection keeps the receiver closed, which is said for as long as it lasts.
    const bool said = _refusal && _refusal->discovery;
    if (_serial.rtkReceiverKeptClosed()) {
        if (!said) {
            _refuse({.error = tr("An RTK receiver is on USB, but another connection has a serial device open, and "
                                 "only one can be open at a time. Disconnect it to connect the receiver."),
                     .serialDevice = {},
                     .discovery = true});
        }
    } else if (said) {
        _showError({});
    }
}

void GPSReceiverConnectionPolicy::_connectDiscovered(const QString& device)
{
    // The USB identity only chose the port; the saved manufacturer chooses the family, detecting it when Automatic,
    // and the receiver detects the rate.
    GPSReceiverConfiguration discovered = _configuration;
    discovered.serialDevice = device;
    discovered.serialBaudRate = 0;
    const GPSReceiverConnectability check = connectability(discovered, _serial.supported());
    if (!check.type || !check.error.isEmpty()) {
        _rejectConfiguration(check.error);
        return;
    }
    GPSSerialClaim claim = _serial.claim(device);
    if (!claim) {
        return;
    }
    _owner = Owner::Discovered;
    _autoPort = device;
    _serial.forgetRtkReceivers();
    _retryDeadlineUs.reset();
    _refusal.reset();
    if (!_receiver.connectSerial(device, *check.type, check.request, std::move(claim))) {
        _scheduleRetry();
    }
}
