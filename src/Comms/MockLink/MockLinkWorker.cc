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
    // Prevent double-start which would leak timers
    if (_timer1Hz) {
        return;
    }

    _timer1Hz = new QTimer(this);
    _timer10Hz = new QTimer(this);
    _timer500Hz = new QTimer(this);
    _timerStatusText = new QTimer(this);

    (void) connect(_timer1Hz, &QTimer::timeout, this, &MockLinkWorker::run1HzTasks);
    (void) connect(_timer10Hz, &QTimer::timeout, this, &MockLinkWorker::run10HzTasks);
    (void) connect(_timer500Hz, &QTimer::timeout, this, &MockLinkWorker::run500HzTasks);
    (void) connect(_timerStatusText, &QTimer::timeout, this, &MockLinkWorker::sendStatusTextMessages);

    _timer1Hz->start(kTimer1HzIntervalMs);
    _timer10Hz->start(kTimer10HzIntervalMs);
    _timer500Hz->start(kTimer500HzIntervalMs);

    if (_mockLink && _mockLink->shouldSendStatusText()) {
        _timerStatusText->setSingleShot(true);
        _timerStatusText->start(kStatusTextDelayMs);
    }

    run1HzTasks();
    run10HzTasks();
    run500HzTasks();
}

void MockLinkWorker::stopWork()
{
    if (_timer1Hz) {
        _timer1Hz->stop();
    }
    if (_timer10Hz) {
        _timer10Hz->stop();
    }
    if (_timer500Hz) {
        _timer500Hz->stop();
    }
    if (_timerStatusText) {
        _timerStatusText->stop();
    }
}

void MockLinkWorker::run1HzTasks()
{
    if (_mockLink) {
        _mockLink->run1HzTasks();
    }
}

void MockLinkWorker::run10HzTasks()
{
    if (_mockLink) {
        _mockLink->run10HzTasks();
    }
}

void MockLinkWorker::run500HzTasks()
{
    if (_mockLink) {
        _mockLink->run500HzTasks();
    }
}

void MockLinkWorker::sendStatusTextMessages()
{
    if (_mockLink) {
        _mockLink->sendStatusTextMessages();
    }
}
