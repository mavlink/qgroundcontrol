#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSProvider.h"
#include "GPSRevision.h"

class AutoConnectSettings;
class Fact;
class GPSRtk;
class RTKSettings;

/// Decides when the receiver connects: user and startup connections from the saved settings, serial
/// auto-discovery of known base receivers, and retries after a connection is lost. GPSRtk runs the sessions.
class RTKConnectionPolicy : public QObject
{
    Q_OBJECT
    friend class GPSRtkTest;
    friend class RTKConnectionPolicyTest;

public:
    /// The settings must outlive the policy.
    RTKConnectionPolicy(GPSRtk* receiver, RTKSettings* settings, AutoConnectSettings* autoConnectSettings);

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

    void receiverReady();
    /// Returns the user message for a lost session, or empty when the receiver's default applies.
    QString sessionEnded(GPSConnectionError error, const QString& detail, bool portRemoved);

signals:
    void reconnectingChanged();

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
    bool _autoConnectEnabled() const;
    void _updateAutoConnection();

    GPSRtk* const _receiver;
    RTKSettings* const _settings;
    Fact* const _autoConnect;
    Owner _owner = Owner::None;
    // A manual connection that reached the receiver once is retried after a loss.
    bool _established = false;
    bool _waitingForPort = false;
    QString _autoPort;
    QMap<QString, QElapsedTimer> _waitingPorts;
    QDeadlineTimer _retryDeadline = QDeadlineTimer::Forever;
    int _retryDelayMs = kInitialRetryDelayMs;
    GPSRevision _revision;
#ifdef Q_OS_WIN
    int _connectDelayMs = 6000;
#else
    int _connectDelayMs = 1000;
#endif

    static constexpr int kInitialRetryDelayMs = 1000;
    static constexpr int kMaxRetryDelayMs = 30000;
};
