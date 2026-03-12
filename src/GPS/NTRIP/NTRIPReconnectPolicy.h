#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

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

private:
    QTimer _timer;
    int _attempts = 0;
};
