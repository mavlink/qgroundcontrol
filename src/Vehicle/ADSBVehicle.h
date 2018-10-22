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
#include <QElapsedTimer>

#include "QGCMAVLink.h"

class ADSBVehicle : public QObject
{
    Q_OBJECT

public:
    ADSBVehicle(mavlink_adsb_vehicle_t& adsbVehicle, QObject* parent = NULL);

    ADSBVehicle(const QGeoCoordinate& location, float heading, bool alert = false, QObject* parent = NULL);

    Q_PROPERTY(int              icaoAddress READ icaoAddress    CONSTANT)
    Q_PROPERTY(QString          callsign    READ callsign       NOTIFY callsignChanged)
    Q_PROPERTY(QGeoCoordinate   coordinate  READ coordinate     NOTIFY coordinateChanged)
    Q_PROPERTY(double           altitude    READ altitude       NOTIFY altitudeChanged)     // NaN for not available
    Q_PROPERTY(double           heading     READ heading        NOTIFY headingChanged)      // NaN for not available
    Q_PROPERTY(bool             alert       READ alert          NOTIFY alertChanged)        // Collision path

    int             icaoAddress (void) const { return _icaoAddress; }
    QString         callsign    (void) const { return _callsign; }
    QGeoCoordinate  coordinate  (void) const { return _coordinate; }
    double          altitude    (void) const { return _altitude; }
    double          heading     (void) const { return _heading; }
    bool            alert       (void) const { return _alert; }

    /// Update the vehicle with new information
    void update(mavlink_adsb_vehicle_t& adsbVehicle);

    void update(bool alert, const QGeoCoordinate& location, float heading);

    /// check if the vehicle is expired and should be removed
    bool expired();

signals:
    void coordinateChanged  ();
    void callsignChanged    ();
    void altitudeChanged    ();
    void headingChanged     ();
    void alertChanged       ();

private:
    /* According with Thomas Voß, we should be using 2 minutes for the time being
    static constexpr qint64 expirationTimeoutMs = 5000; ///< timeout with no update in ms after which the vehicle is removed.
                                                        ///< AirMap sends updates for each vehicle every second.
    */
    static constexpr qint64 expirationTimeoutMs = 120000;

    uint32_t        _icaoAddress;
    QString         _callsign;
    QGeoCoordinate  _coordinate;
    double          _altitude;
    double          _heading;
    bool            _alert;

    QElapsedTimer   _lastUpdateTimer;
};
