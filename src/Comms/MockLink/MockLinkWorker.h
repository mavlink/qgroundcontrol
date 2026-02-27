#pragma once

#include <QtCore/QObject>

class MockLink;
class QTimer;

/// Worker class that runs periodic tasks for MockLink simulation.
/// Operates on a separate thread to avoid blocking the main thread.
class MockLinkWorker : public QObject
{
    Q_OBJECT

public:
    explicit MockLinkWorker(MockLink *link, QObject *parent = nullptr);
    ~MockLinkWorker();

    /// Timer intervals in milliseconds
    static constexpr int kTimer1HzIntervalMs = 1000;        ///< 1 Hz tasks (battery, sys status, ADSB, etc.)
    static constexpr int kTimer10HzIntervalMs = 100;        ///< 10 Hz tasks (heartbeat, GPS, position)
    static constexpr int kTimer500HzIntervalMs = 2;         ///< 500 Hz tasks (param list, log download)
    static constexpr int kStatusTextDelayMs = 10000;        ///< Delay before sending status text messages

public slots:
    void startWork();
    void stopWork();

private slots:
    void run1HzTasks();
    void run10HzTasks();
    void run500HzTasks();

    void sendStatusTextMessages();

private:
    MockLink *_mockLink = nullptr;
    QTimer *_timer1Hz = nullptr;
    QTimer *_timer10Hz = nullptr;
    QTimer *_timer500Hz = nullptr;
    QTimer *_timerStatusText = nullptr;
};
