#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

#include "ExponentialBackoff.h"
#include "GPSReceiverConnector.h"

class GPSReceiverSerialPorts;
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

/// Decides when the receiver connects. With Configuration::autoConnect on, it keeps the saved receiver connected: it
/// connects it at startup and when the setting is turned on, reconnects it with backoff after a lost connection, and
/// reconnects an unplugged serial receiver as soon as its port returns. A connection the user starts is kept once it
/// reaches the receiver. A configured base without a saved serial device is discovered on USB instead, trying only
/// ports whose USB identity is a known RTK receiver adapter, with the saved manufacturer. The user's Disconnect pauses
/// both until the user connects again or turns the setting back on. With the setting off, receivers connect only when
/// the user asks, and a lost connection is reported. Automatic attempts never pass flash-save consent. A saved or
/// discovered receiver whose configuration cannot connect is reported once and not retried until the configuration
/// changes or the user connects. The policy reports why a connection cannot start or is waiting for its port, and
/// withdraws the reason a connection could not start once the configuration no longer has it; the receiver words how a
/// session ended.
class GPSReceiverConnectionPolicy
{
    Q_DECLARE_TR_FUNCTIONS(GPSReceiverConnectionPolicy)

    friend class GPSReceiverConnectionPolicyTest;

public:
    /// Drives @a receiver as @a configuration, the receiver's, selects. @a connectability is the receiver's check of
    /// that configuration, renewed before configurationChanged(), and @a serial the receiver's serial devices. All
    /// five must outlive the policy, and @a scheduler times the retries.
    GPSReceiverConnectionPolicy(GPSReceiverConnector& receiver, const GPSReceiverConfiguration& configuration,
                                const GPSReceiverConnectability& connectability, GPSReceiverSerialPorts& serial,
                                RuntimeScheduler* scheduler);

    /// The one place that checks whether @a configuration can connect and builds the request its connection sends,
    /// for Connect, automatic connection and the settings view. Without @a serialLinks, a serial selection connects
    /// over TCP. Only the user's Connect passes @a allowPersistentChanges, the flash-save consent.
    [[nodiscard]] static GPSReceiverConnectability connectability(const GPSReceiverConfiguration& configuration,
                                                                  bool serialLinks,
                                                                  bool allowPersistentChanges = false);

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

    /// Why a connection could not start.
    struct Refusal
    {
        QString error;
        /// The serial device that was missing or in use; empty when the configuration itself cannot connect.
        QString serialDevice;
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
    /// Shows @a error in place of the connection error on display; an empty one clears it.
    void _showError(const QString& error);
    /// Shows why the connection cannot start, until a change of the configuration removes the reason.
    void _refuse(Refusal refusal);
    /// The configuration still has the reason of _refusal.
    bool _refusalApplies() const;
    void _updateDiscovery();
    /// Connects a discovered receiver; while another connection holds its port, a later update tries again without
    /// advancing the backoff.
    void _connectDiscovered(const QString& device);

    GPSReceiverConnector& _receiver;
    const GPSReceiverConfiguration& _configuration;
    /// The check of _configuration, without flash-save consent.
    const GPSReceiverConnectability& _connectability;
    GPSReceiverSerialPorts& _serial;
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
    /// Set while its error is the one on display: nothing else was shown and no connection started since.
    std::optional<Refusal> _refusal;
    QString _autoPort;
    std::optional<quint64> _retryDeadlineUs;
    ExponentialBackoff _retryBackoff{std::chrono::seconds(1), 2, std::chrono::seconds(30)};
};
