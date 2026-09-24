#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSProvider.h"

class GPSRtk;

/// Decides when the RTK receiver connects: user connections from the saved settings, serial
/// auto-discovery, and retries after a connection is lost. GPSRtk runs the receiver sessions.
class RTKConnectionPolicy : public QObject
{
    Q_OBJECT
    friend class GPSRtkTest;
    friend class RTKConnectionPolicyTest;

public:
    explicit RTKConnectionPolicy(GPSRtk* receiver);

    /// Connects from the saved settings and turns auto-connect off. Flash-save consent is one-use.
    bool connectConfigured(bool allowPersistentChanges);
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

    bool _connectConfigured(bool allowPersistentChanges);
    void _retryManual();
    void _scheduleRetry();
    bool _autoConnectEnabled() const;
    void _updateAutoConnection();

    GPSRtk* const _receiver;
    Owner _owner = Owner::None;
    // A manual connection that reached the receiver once is retried after a loss.
    bool _established = false;
    bool _waitingForPort = false;
    QString _autoPort;
    QMap<QString, QElapsedTimer> _waitingPorts;
    QDeadlineTimer _retryDeadline = QDeadlineTimer::Forever;
    int _retryDelayMs = kInitialRetryDelayMs;
    quint64 _revision = 0;
#ifdef Q_OS_WIN
    int _connectDelayMs = 6000;
#else
    int _connectDelayMs = 1000;
#endif

    static constexpr int kInitialRetryDelayMs = 1000;
    static constexpr int kMaxRetryDelayMs = 30000;
};
