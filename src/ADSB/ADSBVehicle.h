/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QtNumeric>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "ADSB.h"

Q_DECLARE_LOGGING_CATEGORY(ADSBVehicleLog)

class ADSBVehicle : public QObject
{
    Q_OBJECT
    // QML_ELEMENT

    Q_PROPERTY(uint           icaoAddress READ icaoAddress CONSTANT)
    Q_PROPERTY(QString        callsign    READ callsign    NOTIFY callsignChanged)
    Q_PROPERTY(QGeoCoordinate coordinate  READ coordinate  NOTIFY coordinateChanged)
    Q_PROPERTY(double         altitude    READ altitude    NOTIFY altitudeChanged)
    Q_PROPERTY(double         heading     READ heading     NOTIFY headingChanged)
    Q_PROPERTY(bool           alert       READ alert       NOTIFY alertChanged)

public:
    explicit ADSBVehicle(const ADSB::VehicleInfo_t &vehicleInfo, QObject *parent = nullptr);
    ~ADSBVehicle();

    uint32_t icaoAddress() const { return _info.icaoAddress; }
    QString callsign() const { return _info.callsign; }
    QGeoCoordinate coordinate() const { return _info.location; }
    double altitude() const { return _info.altitude; }
    double heading() const { return _info.heading; }
    bool alert() const { return _info.alert; }
    bool expired() const { return _lastUpdateTimer.hasExpired(_expirationTimeoutMs); }
    void update(const ADSB::VehicleInfo_t &vehicleInfo);

signals:
    void coordinateChanged();
    void callsignChanged();
    void altitudeChanged();
    void headingChanged();
    void alertChanged();

private:
    ADSB::VehicleInfo_t _info{};
    QElapsedTimer _lastUpdateTimer;

    static constexpr qint64 _expirationTimeoutMs = 120000; ///< timeout with no update in ms after which the vehicle is removed.
};
