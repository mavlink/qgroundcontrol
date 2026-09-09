#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>

/// Shared connection intent, lifecycle, and retry policy; transports own their I/O.
class GPSConnectionState : public QObject
{
    Q_OBJECT

    friend class GPSConnectionStateTest;
    friend class NMEASourceManagerTest;
    friend class RTKAutoConnectTest;

public:
    enum State
    {
        Disconnected,
        Connecting,
        Configuring,
        Ready,
        Retrying,
        Stopping,
    };
    Q_ENUM(State)

    explicit GPSConnectionState(QObject* parent = nullptr);
    ~GPSConnectionState() override;

    State state() const { return _state; }

    bool active() const { return _active; }

    bool paused() const { return _paused; }

    bool shouldConnect(bool automatic) const;
    bool updateIntent(bool automatic);
    void requestConnect();
    void pause();
    void stop();
    void resetIntent();

    bool canAttempt() const;
    bool beginAttempt();
    void configuring();
    void ready();
    void failed();
    void stopping();
    void stopped();
    void resetRetry();

signals:
    void changed();

private:
    void _setState(State state);

    State _state = Disconnected;
    bool _active = false;
    bool _manualRequested = false;
    bool _paused = false;
    QDeadlineTimer _retryDeadline = QDeadlineTimer::Forever;
    int _retryDelayMs = 1000;
};
