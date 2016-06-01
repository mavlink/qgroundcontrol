/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtCore>
#include <QDateTime>
#include <QDate>

#include "SimulatedPosition.h"

SimulatedPosition::simulated_motion_s SimulatedPosition::_simulated_motion[5] = {{0,250},{0,0},{0, -250},{-250, 0},{0,0}};

SimulatedPosition::SimulatedPosition()
    : QGeoPositionInfoSource(NULL),
      lat_int(47.3977420*1e7),
      lon_int(8.5455941*1e7),
      _step_cnt(0),
      _simulate_motion_index(0),
      _simulate_motion(true),
      _rotation(0.0F)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();

    qsrand(currentDateTime.toTime_t());

    connect(&update_timer, &QTimer::timeout, this, &SimulatedPosition::updatePosition);
}

QGeoPositionInfo SimulatedPosition::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
    return lastPosition;
}

SimulatedPosition::PositioningMethods SimulatedPosition::supportedPositioningMethods() const
{
    return AllPositioningMethods;
}

int SimulatedPosition::minimumUpdateInterval() const
{
    return 1000;
}

void SimulatedPosition::startUpdates()
{    
    int interval = updateInterval();
    if (interval < minimumUpdateInterval())
        interval = minimumUpdateInterval();

    update_timer.setSingleShot(false);
    update_timer.start(interval);
}

void SimulatedPosition::stopUpdates()
{
    update_timer.stop();
}

void SimulatedPosition::requestUpdate(int /*timeout*/)
{
    emit updateTimeout();
}

int SimulatedPosition::getRandomNumber(int size)
{
    if(size == 0) {
        return 0;
    }

    int num = (qrand()%2 > 1) ? -1 : 1;

    return num*qrand()%size;
}

void SimulatedPosition::updatePosition()
{
    int32_t lat_mov = 0;
    int32_t lon_mov = 0;

    _rotation += (float) .1;

    if(!(_step_cnt++%30)) {
        _simulate_motion_index++;
        if(_simulate_motion_index > 4) {
            _simulate_motion_index = 0;
        }
    }

    lat_mov = _simulated_motion[_simulate_motion_index].lat;
    lon_mov = _simulated_motion[_simulate_motion_index].lon*sin(_rotation);

    lon_int += lat_mov;
    lat_int += lon_mov;

    double latitude = ((double)  (lat_int + getRandomNumber(250)))*1e-7;
    double longitude = ((double) (lon_int + getRandomNumber(250)))*1e-7;

    QDateTime timestamp = QDateTime::currentDateTime();

    QGeoCoordinate position(latitude, longitude);
    QGeoPositionInfo info(position, timestamp);

    if(lat_mov || lon_mov) {
        info.setAttribute(QGeoPositionInfo::Attribute::Direction, 3.14/2);
        info.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, 5);
    }

    lastPosition = info;

    emit positionUpdated(info);
}

QGeoPositionInfoSource::Error SimulatedPosition::error() const
{
    return QGeoPositionInfoSource::NoError;
}


