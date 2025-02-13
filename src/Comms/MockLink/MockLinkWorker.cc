/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MockLinkWorker.h"
#include "MockLink.h"

#include <QtCore/QTimer>

MockLinkWorker::MockLinkWorker(MockLink *link, QObject *parent)
    : QObject(parent)
    , _mockLink(link)
{

}

MockLinkWorker::~MockLinkWorker()
{
    stopWork();
}

void MockLinkWorker::startWork()
{
    _timer1Hz = new QTimer(this);
    _timer10Hz = new QTimer(this);
    _timer500Hz = new QTimer(this);
    _timerStatusText = new QTimer(this);

    (void) connect(_timer1Hz, &QTimer::timeout, this, &MockLinkWorker::run1HzTasks);
    (void) connect(_timer10Hz, &QTimer::timeout, this, &MockLinkWorker::run10HzTasks);
    (void) connect(_timer500Hz, &QTimer::timeout, this, &MockLinkWorker::run500HzTasks);
    (void) connect(_timerStatusText, &QTimer::timeout, this, &MockLinkWorker::sendStatusTextMessages);

    _timer1Hz->start(1000);
    _timer10Hz->start(100);
    _timer500Hz->start(2);

    if (_mockLink->shouldSendStatusText()) {
        _timerStatusText->setSingleShot(true);
        _timerStatusText->start(10000);
    }

    run1HzTasks();
    run10HzTasks();
    run500HzTasks();
}

void MockLinkWorker::stopWork()
{
    _timer1Hz->stop();
    _timer10Hz->stop();
    _timer500Hz->stop();
    _timerStatusText->stop();
}

void MockLinkWorker::run1HzTasks()
{
    _mockLink->run1HzTasks();
}

void MockLinkWorker::run10HzTasks()
{
    _mockLink->run10HzTasks();
}

void MockLinkWorker::run500HzTasks()
{
    _mockLink->run500HzTasks();
}

void MockLinkWorker::sendStatusTextMessages()
{
    _mockLink->sendStatusTextMessages();
}
