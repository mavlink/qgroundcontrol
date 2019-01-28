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

#include "LandingPadPosition.h"

LandingPadPosition::LandingPadPosition()
    : QGeoPositionInfoSource(NULL)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();

    qsrand(currentDateTime.toTime_t());

    connect(&update_timer, &QTimer::timeout, this, &LandingPadPosition::updatePosition);
}

QGeoPositionInfo LandingPadPosition::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
    return lastPosition;
}

LandingPadPosition::PositioningMethods LandingPadPosition::supportedPositioningMethods() const
{
    return AllPositioningMethods;
}

int LandingPadPosition::minimumUpdateInterval() const
{
    return 1000; // 1Hz
}

void LandingPadPosition::startUpdates()
{    
    int interval = updateInterval();
    if (interval < minimumUpdateInterval())
        interval = minimumUpdateInterval();

    update_timer.setSingleShot(false);
    update_timer.start(interval);
}

void LandingPadPosition::stopUpdates()
{
    update_timer.stop();
}

void LandingPadPosition::requestUpdate(int /*timeout*/)
{
    emit updateTimeout();
}

void LandingPadPosition::updatePosition()
{
    double rando = QRandomGenerator::global()->generateDouble() * 10;
    double latitude = ((double)(lat_int + rando))*1e-7;
    double longitude = ((double)(lon_int + rando))*1e-7;

    QDateTime timestamp = QDateTime::currentDateTime();

    QGeoCoordinate position(latitude, longitude);
    QGeoPositionInfo info(position, timestamp);

    lastPosition = info;

    emit positionUpdated(info);
}

QGeoPositionInfoSource::Error LandingPadPosition::error() const
{
    return QGeoPositionInfoSource::NoError;
}
