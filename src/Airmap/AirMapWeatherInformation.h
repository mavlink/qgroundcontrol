/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"

#include "AirspaceWeatherInfoProvider.h"
#include "AirMapSharedState.h"

#include <QGeoCoordinate>
#include <QTime>

#include "airmap/status.h"

/**
 * @file AirMapWeatherInformation.h
 * Weather information provided by AirMap.
 */

class AirMapWeatherInformation : public AirspaceWeatherInfoProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapWeatherInformation(AirMapSharedState &shared, QObject *parent = nullptr);

    bool        valid           () override { return _valid; }
    QString     condition       () override { return QString::fromStdString(_weather.condition); }
    QString     icon            () override { return _icon; }
    quint32     windHeading     () override { return _weather.wind.heading; }
    quint32     windSpeed       () override { return _weather.wind.speed; }
    quint32     windGusting     () override { return _weather.wind.gusting; }
    qint32      temperature     () override { return _weather.temperature; }
    float       humidity        () override { return _weather.humidity; }
    quint32     visibility      () override { return _weather.visibility; }
    quint32     precipitation   () override { return _weather.precipitation; }

    void        setROI          (const QGeoCoordinate& center) override;

signals:
    void        error           (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    void        _requestWeatherUpdate   (const QGeoCoordinate& coordinate);

private:
    bool                    _valid;
    QString                 _icon;
    airmap::Status::Weather _weather;
    //-- Don't check the weather every time the user moves the map
    AirMapSharedState&      _shared;
    QGeoCoordinate          _lastRoiCenter;
    QTime                   _weatherTime;
};
