/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include "AirspaceWeatherInfoProvider.h"

/**
 * @file AirMapWeatherInformation.h
 * Weather information provided by AirMap.
 */

class AirMapWeatherInformation : public AirspaceWeatherInfoProvider, public LifetimeChecker
{
    Q_OBJECT
    friend class AirMapManager;
public:
    AirMapWeatherInformation(AirMapSharedState &shared, QObject *parent = nullptr);

    bool        valid           () override { return _valid; }
    QString     condition       () override { return _condition; }
    QString     icon            () override { return _icon; }
    quint32     windHeading     () override { return _windHeading; }
    quint32     windSpeed       () override { return _windSpeed; }
    quint32     windGusting     () override { return _windGusting; }
    qint32      temperature     () override { return _temperature; }
    float       humidity        () override { return _humidity; }
    quint32     visibility      () override { return _visibility; }
    quint32     precipitation   () override { return _precipitation; }

    void        setROI          (const QGeoCoordinate& center) override;

private:
    void        _requestWeatherUpdate   (const QGeoCoordinate& coordinate);

private:
    bool            _valid;
    QString         _condition;
    QString         _icon;
    quint32         _windHeading;
    quint32         _windSpeed;
    quint32         _windGusting;
    qint32          _temperature;
    float           _humidity;
    quint32         _visibility;
    quint32         _precipitation;
    //-- Don't check the weather every time the user moves the map
    QGeoCoordinate  _lastRoiCenter;
    QTime           _weatherTime;
};
