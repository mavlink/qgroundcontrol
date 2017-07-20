/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>

#include "QGCMAVLink.h"

class ADSBVehicle : public QObject
{
    Q_OBJECT

public:
    ADSBVehicle(mavlink_adsb_vehicle_t& adsbVehicle, QObject* parent = NULL);

    Q_PROPERTY(int              icaoAddress READ icaoAddress    CONSTANT)
    Q_PROPERTY(QString          callsign    READ callsign       NOTIFY callsignChanged)
    Q_PROPERTY(QGeoCoordinate   coordinate  READ coordinate     NOTIFY coordinateChanged)
    Q_PROPERTY(double           altitude    READ altitude       NOTIFY altitudeChanged)     // NaN for not available
    Q_PROPERTY(double           heading     READ heading        NOTIFY headingChanged)      // NaN for not available

    int             icaoAddress (void) const { return _icaoAddress; }
    QString         callsign    (void) const { return _callsign; }
    QGeoCoordinate  coordinate  (void) const { return _coordinate; }
    double          altitude    (void) const { return _altitude; }
    double          heading     (void) const { return _heading; }

    /// Update the vehicle with new information
    void update(mavlink_adsb_vehicle_t& adsbVehicle);

signals:
    void coordinateChanged(QGeoCoordinate coordinate);
    void callsignChanged(QString callsign);
    void altitudeChanged(double altitude);
    void headingChanged(double heading);

private:
    uint32_t        _icaoAddress;
    QString         _callsign;
    QGeoCoordinate  _coordinate;
    double          _altitude;
    double          _heading;
};
