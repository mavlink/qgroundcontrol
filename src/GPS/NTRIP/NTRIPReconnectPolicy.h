#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <chrono>

class QNetworkInformation;

class NTRIPReconnectPolicy : public QObject
{
    Q_OBJECT

public:
    static constexpr int kMinReconnectMs = 1000;
    static constexpr int kMaxReconnectMs = 30000;
    static constexpr int kMaxReconnectAttempts = 100;

    explicit NTRIPReconnectPolicy(QObject* parent = nullptr);

    void scheduleReconnect();
    void cancel();
    bool isPending() const;
    void resetAttempts();
    int attempts() const { return _attempts; }
    int nextBackoffMs() const;

signals:
    void reconnectRequested();
    void gaveUp();

private:
    bool hasGivenUp() const { return _attempts >= kMaxReconnectAttempts; }

    /// Returns true when QNetworkInformation reports we can reach the internet.
    /// Returns true (optimistic) when no backend is available so CI / headless
    /// environments without NetworkManager/DBus behave as before.
    bool _isOnline() const;

    void _armTimer();
    void _onReachabilityChanged();

    QChronoTimer _timer;
    QPointer<QNetworkInformation> _netInfo;
    std::chrono::milliseconds _pendingBackoff{0};
    bool _waitingForNetwork = false;
    int _attempts = 0;
};
