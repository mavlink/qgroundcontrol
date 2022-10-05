/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkMessagesTimer.h"

#include <QDebug>

MavlinkMessagesTimer::MavlinkMessagesTimer(int vehicle_id, bool high_latency) :
    _active(true),
    _timer(new QTimer),
    _vehicleID(vehicle_id),
    _high_latency(high_latency)
{
}

void MavlinkMessagesTimer::init()
{
    if (!_high_latency) {
        _timer->setInterval(_messageReceivedTimeoutMSecs);
        _timer->setSingleShot(false);
        _timer->start();
    }
    emit activeChanged(true, _vehicleID);
    QObject::connect(_timer, &QTimer::timeout, this, &MavlinkMessagesTimer::timerTimeout);
}


MavlinkMessagesTimer::~MavlinkMessagesTimer()
{
    if (_timer) {
        QObject::disconnect(_timer, &QTimer::timeout, this, &MavlinkMessagesTimer::timerTimeout);
        _timer->stop();
        delete _timer;
        _timer = nullptr;
    }

    emit activeChanged(false, _vehicleID);
}

void MavlinkMessagesTimer::restartTimer()
{
    if (!_active) {
        _active = true;
        emit activeChanged(true, _vehicleID);
    }

    _timer->start();
}

void MavlinkMessagesTimer::timerTimeout()
{
    if (!_high_latency) {
        if (_active) {
            _active = false;
            emit activeChanged(false, _vehicleID);
        }
        emit heartbeatTimeout(_vehicleID);
    }
}
