/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>

class MockLink;
class QTimer;

class MockLinkWorker : public QObject
{
    Q_OBJECT

public:
    explicit MockLinkWorker(MockLink *link, QObject *parent = nullptr);
    ~MockLinkWorker();

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
