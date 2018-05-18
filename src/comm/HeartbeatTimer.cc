/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "HeartbeatTimer.h"

#include <QDebug>

HeartbeatTimer::HeartbeatTimer(int vehicle_id, bool high_latency) :
    _active(true),
    _timer(new QTimer),
    _vehicleID(vehicle_id),
    _high_latency(high_latency)
{
    if (!high_latency) {
        _timer->setInterval(_heartbeatReceivedTimeoutMSecs);
        _timer->setSingleShot(true);
        _timer->start();
    }
    emit activeChanged(true, _vehicleID);
    QObject::connect(_timer, &QTimer::timeout, this, &HeartbeatTimer::timerTimeout);
}

HeartbeatTimer::~HeartbeatTimer() {
    if (_timer) {
        QObject::disconnect(_timer, &QTimer::timeout, this, &HeartbeatTimer::timerTimeout);
        _timer->stop();
        delete _timer;
        _timer = nullptr;
    }

    emit activeChanged(false, _vehicleID);
}

void HeartbeatTimer::restartTimer()
{
    if (!_active) {
        _active = true;
        emit activeChanged(true, _vehicleID);
    }

    _timer->start();
}

void HeartbeatTimer::timerTimeout()
{
    if (!_high_latency) {
        if (_active) {
            _active = false;
            emit activeChanged(false, _vehicleID);
        }
        emit heartbeatTimeout(_vehicleID);
    }
}
