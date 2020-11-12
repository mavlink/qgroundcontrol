/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"

#include "AirspaceWeatherInfoProvider.h"
#include "AirMapSharedState.h"
#include "QGCGeoBoundingCube.h"

#include <QGeoCoordinate>
#include <QTime>

#include "airmap/advisory.h"

/**
 * @file AirMapWeatherInfoManager.h
 * Weather information provided by AirMap.
 */

class AirMapWeatherInfoManager : public AirspaceWeatherInfoProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapWeatherInfoManager(AirMapSharedState &shared, QObject *parent = nullptr);

    bool        valid           () override { return _valid; }
    QString     condition       () override { return QString::fromStdString(_weather.condition); }
    QString     icon            () override { return _icon; }
    quint32     windHeading     () override { return _weather.wind.heading; }
    float       windSpeed       () override { return _weather.wind.speed; }
    quint32     windGusting     () override { return _weather.wind.gusting; }
    float       temperature     () override { return _weather.temperature; }
    float       humidity        () override { return _weather.humidity; }
    float       visibility      () override { return _weather.visibility; }
    float       precipitation   () override { return _weather.precipitation; }

    void        setROI          (const QGCGeoBoundingCube& roi, bool reset = false) override;

signals:
    void        error           (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    void        _requestWeatherUpdate   (const QGeoCoordinate& coordinate);

private:
    bool                    _valid;
    QString                 _icon;
    airmap::Advisory::Weather _weather;
    //-- Don't check the weather every time the user moves the map
    AirMapSharedState&      _shared;
    QGeoCoordinate          _lastRoiCenter;
    QTime                   _weatherTime;
};
