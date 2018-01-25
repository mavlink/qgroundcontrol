/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirmapWeatherInformation.h"

#define WEATHER_UPDATE_DISTANCE 50000                   //-- 50km threshold for weather updates
#define WEATHER_UPDATE_TIME     30 * 60 * 60 * 1000     //-- 30 minutes threshold for weather updates

AirMapWeatherInformation::AirMapWeatherInformation(AirMapSharedState& shared, QObject *parent)
    : QObject(parent)
    , _valid(false)
    , _windHeading(0)
    , _windSpeed(0)
    , _windGusting(0)
    , _temperature(0)
    , _humidity(0.0f)
    , _visibility(0)
    , _precipitation(0)
{
}

void
AirMapWeatherInformation::setROI(QGeoCoordinate center)
{
    //-- If first time or we've moved more than WEATHER_UPDATE_DISTANCE, ask for weather updates.
    if(!_lastRoiCenter.isValid() || _lastRoiCenter.distanceTo(center) > WEATHER_UPDATE_DISTANCE) {
        _lastRoiCenter = center;
        _requestWeatherUpdate(center);
        _weatherTime.start();
    } else {
        //-- Check weather once every WEATHER_UPDATE_TIME
        if(_weatherTime.elapsed() > WEATHER_UPDATE_TIME) {
            _requestWeatherUpdate(center);
            _weatherTime.start();
        }
    }
}

void
AirMapWeatherInformation::_requestWeatherUpdate(const QGeoCoordinate& coordinate)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Weather information";
        _valid = false;
        emit weatherChanged();
        return;
    }
    Status::GetStatus::Parameters params;
    params.longitude = coordinate.longitude();
    params.latitude  = coordinate.latitude();
    params.weather   = true;
    _shared.client()->status().get_status_by_point(params, [this, coordinate](const Status::GetStatus::Result& result) {
        if (result) {
            const Status::Weather& weather = result.value().weather;
            AirMapWeatherInformation weatherUpdateInfo;
            _valid          = true;
            _condition      = QString::fromStdString(weather.condition);
            _icon           = QStringLiteral("qrc:/airmapweather/") + QString::fromStdString(weather.icon) + QStringLiteral(".svg");
            _windHeading    = weather.wind.heading;
            _windSpeed      = weather.wind.speed;
            _windGusting    = weather.wind.gusting;
            _temperature    = weather.temperature;
            _humidity       = weather.humidity;
            _visibility     = weather.visibility;
            _precipitation  = weather.precipitation;
        } else {
            _valid          = false;
        }
        emit weatherChanged();
    });
}
