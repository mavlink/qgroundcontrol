#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

#include "ExponentialBackoff.h"
#include "GPSReceiverConnector.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

class RuntimeScheduler;

/// How a receiver session ended.
enum class GPSReceiverSessionEnd
{
    /// The session reported its error or its worker's exit.
    Reported,
    /// A scan no longer lists the session's serial device.
    Unplugged,
};

/// What the policy does after a receiver session ends; the receiver words the message for it.
enum class GPSReceiverSessionOutcome
{
    /// Nothing reconnects; the receiver's own message for the error applies.
    None,
    /// A discovered receiver was unplugged; discovery connects it again when it returns.
    Unplugged,
    /// A discovered receiver failed or was lost; discovery retries it.
    AutoRetrying,
    /// The saved receiver was unplugged; it reconnects when its port returns.
    WaitingForPort,
    /// The saved receiver's connection was lost; a retry is scheduled.
    Retrying,
};

/// Whether a receiver configuration can connect, whatever devices are plugged in or reachable.
struct GPSReceiverConnectability
{
    /// The receiver family to connect; empty when no receiver type is selected.
    std::optional<GPSType> type;
    /// The family supports the selected base mode; a passive receiver has none to support.
    bool baseModeSupported = false;
    /// UDP only receives, so a receiver QGroundControl configures needs serial or TCP.
    bool connectionSupported = false;
    /// The serial device, TCP address or UDP port to connect is entered, so a receiver is saved.
    bool hasEndpoint = false;
    /// Why the configuration cannot connect, as shown to the user; empty when it can. Never empty without a type, a
    /// supported base mode, a supported connection and an endpoint.
    QString error;
};

/// Decides when the receiver connects. With Configuration::autoConnect on, it keeps the saved receiver connected: it
/// connects it at startup and when the setting is turned on, reconnects it with backoff after a lost connection, and
/// reconnects an unplugged serial receiver as soon as its port returns. A connection the user starts is kept once it
/// reaches the receiver. A configured base without a saved serial device is discovered on USB instead, trying only
/// ports whose USB identity is a known RTK receiver adapter, with the saved manufacturer. The user's Disconnect pauses
/// both until the user connects again or turns the setting back on. With the setting off, receivers connect only when
/// the user asks, and a lost connection is reported. Automatic attempts never pass flash-save consent. A saved or
/// discovered receiver whose configuration cannot connect is reported once and not retried until the configuration
/// changes or the user connects. The policy reports why a connection cannot start or is waiting for its port; the
/// receiver words how a session ended.
class GPSReceiverConnectionPolicy
{
    Q_DECLARE_TR_FUNCTIONS(GPSReceiverConnectionPolicy)

    friend class GPSReceiverConnectionPolicyTest;

public:
    /// Drives @a receiver as @a configuration, the receiver's, selects; all three must outlive the policy, and
    /// @a scheduler times the retries.
    GPSReceiverConnectionPolicy(GPSReceiverConnector& receiver, const GPSReceiverConfiguration& configuration,
                                RuntimeScheduler* scheduler);

    /// The one check of whether @a configuration can connect, which Connect, automatic connection and the settings
    /// view share.
    [[nodiscard]] static GPSReceiverConnectability connectability(const GPSReceiverConfiguration& configuration);

    /// The receiver's configuration changed from @a previous.
    void configurationChanged(const GPSReceiverConfiguration& previous);

    /// Connects the saved receiver at the user's request and resumes automatic connection.
    bool connectConfigured(bool allowPersistentChanges);
    /// Disconnects at the user's request and pauses automatic connection.
    void disconnectConfigured();
    /// Periodic tick: due automatic connections and USB discovery.
    void update();
    /// Forgets connection ownership and pending retries, before another connection replaces it.
    void reset();

    /// The saved receiver is being connected automatically and is not connected yet.
    bool reconnecting() const { return _keepConnected && !_receiver.hasReceiver(); }

    void receiverReady();
    [[nodiscard]] GPSReceiverSessionOutcome sessionEnded(GPSReceiverSessionEnd end);

private:
    enum class Owner
    {
        None,
        Saved,
        Discovered,
    };

    bool _hasSavedReceiver() const;
    bool _discoveryEnabled() const;
    bool _connectConfigured(bool allowPersistentChanges);
    void _updateSaved();
    void _attemptSaved();
    void _scheduleRetry();
    void _cancelRetries();
    /// Shows why the configuration cannot connect and stops connecting it automatically.
    void _rejectConfiguration(const QString& error);
    bool _serialDeviceAvailable(const QString& device) const;
    void _updateDiscovery();
#ifndef QGC_NO_SERIAL_LINK
    /// Connects a discovered receiver; while another connection holds its port, a later update tries again without
    /// advancing the backoff.
    void _connectDiscovered(const QString& device);
#endif

    GPSReceiverConnector& _receiver;
    const GPSReceiverConfiguration& _configuration;
    RuntimeScheduler* const _scheduler;
    Owner _owner = Owner::None;
    /// Connect the saved receiver at the next update: set at startup and when the setting is turned on.
    bool _startPending = true;
    /// Retry the saved receiver until it is connected.
    bool _keepConnected = false;
    /// The user disconnected; nothing connects automatically until the user connects again.
    bool _paused = false;
    /// The configuration cannot connect; nothing connects automatically until it changes or the user connects.
    bool _unconnectable = false;
    bool _waitingForPort = false;
    QString _autoPort;
#ifndef QGC_NO_SERIAL_LINK
    SerialPortSettleTracker _waitingPorts;
#endif
    std::optional<quint64> _retryDeadlineUs;
    ExponentialBackoff _retryBackoff{std::chrono::seconds(1), 2, std::chrono::seconds(30)};
};
