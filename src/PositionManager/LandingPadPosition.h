/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtPositioning/qgeopositioninfosource.h>
#include "QGCToolbox.h"
#include <QTimer>

class LandingPadPosition : public QGeoPositionInfoSource
{
   Q_OBJECT

public:
    LandingPadPosition();

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const;

    PositioningMethods supportedPositioningMethods() const;
    int minimumUpdateInterval() const;
    Error error() const;

    void setPosition(int lat, int lon)
    {
        lat_int = lat;
        lon_int = lon;
    }

public slots:
    virtual void startUpdates();
    virtual void stopUpdates();

    virtual void requestUpdate(int timeout = 5000);

private slots:
    void updatePosition();

private:
    QTimer update_timer;

    QGeoPositionInfo lastPosition;

    int32_t lat_int;
    int32_t lon_int;
};
