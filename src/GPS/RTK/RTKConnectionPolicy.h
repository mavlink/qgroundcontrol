#pragma once

#include <optional>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSProvider.h"
#include "GPSRevision.h"
#include "GPSRtk.h"
#include "RTKConnectionTarget.h"

class RuntimeScheduler;

/// What the policy does after a receiver session ends; the receiver words the user message.
enum class RTKSessionOutcome
{
    /// Nothing to announce; the receiver's own message for the error applies.
    None,
    /// An auto-connected receiver was unplugged; discovery connects it again when it returns.
    Unplugged,
    /// A manual receiver was unplugged; it reconnects when its port returns.
    WaitingForPort,
    /// A manual connection was lost; a retry is scheduled.
    Retrying,
};

/// Decides when the receiver connects: user and startup connections from the saved settings, serial
/// auto-discovery of known base receivers, and retries after a connection is lost. The target runs the sessions.
class RTKConnectionPolicy : public QObject
{
    Q_OBJECT
    friend class GPSRtkTest;

public:
    explicit RTKConnectionPolicy(RTKConnectionTarget& receiver, QObject* parent = nullptr,
                                 RuntimeScheduler* scheduler = nullptr);

    void setConfiguration(const GPSRtk::Configuration& configuration);

    /// Connects from the saved settings and turns auto-connect off. Flash-save consent is one-use.
    bool connectConfigured(bool allowPersistentChanges);
    /// Connects the saved receiver at startup and leaves auto-connect unchanged. While discovery is on, only a
    /// receiver present now is connected; otherwise it keeps retrying until reached, as after a lost connection.
    void connectSaved();

    /// Disconnects and turns auto-connect off.
    void disconnectConfigured();
    /// Periodic tick: serial discovery and due retries.
    void update();
    /// Ends retries and any auto-connected session.
    void stop();
    /// Forgets connection ownership without disconnecting, before another connection replaces it.
    void reset();

    /// A manual connection was lost and a retry is pending.
    bool reconnecting() const;
    bool retryPending() const;

    void receiverReady();
    [[nodiscard]] RTKSessionOutcome sessionEnded(bool portRemoved);

signals:
    void reconnectingChanged();
    void autoConnectDisabled();

private:
    enum class Owner
    {
        None,
        Manual,
        Auto,
    };

    /// Only a user's connection turns auto-connect off.
    bool _connectConfigured(bool allowPersistentChanges, bool userRequested);
    void _retryManual();
    void _scheduleRetry();
    /// Published with the receiver's notifications, after the current operation.
    void _disableAutoConnect();
    bool _autoConnectEnabled() const;
    void _updateAutoConnection();

    RTKConnectionTarget& _receiver;
    RuntimeScheduler* const _scheduler;
    GPSRtk::Configuration _configuration;
    Owner _owner = Owner::None;
    // A manual connection that reached the receiver once is retried after a loss.
    bool _established = false;
    bool _waitingForPort = false;
    QString _autoPort;
    QMap<QString, quint64> _waitingPorts;
    std::optional<quint64> _retryDeadlineUs;
    int _retryDelayMs = kInitialRetryDelayMs;
    GPSRevision _revision;
#ifdef Q_OS_WIN
    static constexpr int kConnectDelayMs = 6000;
#else
    static constexpr int kConnectDelayMs = 1000;
#endif

    static constexpr int kInitialRetryDelayMs = 1000;
    static constexpr int kMaxRetryDelayMs = 30000;
};
