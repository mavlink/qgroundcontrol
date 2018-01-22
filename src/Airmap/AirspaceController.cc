/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirspaceController.h"
#include "AirspaceManagement.h"
#include "QGCApplication.h"
#include "QGCQGeoCoordinate.h"

#define WEATHER_UPDATE_DISTANCE 50000                   //-- 50km threshold for weather updates
#define WEATHER_UPDATE_TIME     30 * 60 * 60 * 1000     //-- 30 minutes threshold for weather updates

AirspaceController::AirspaceController(QObject* parent)
    : QObject(parent)
    , _manager(qgcApp()->toolbox()->airspaceManager())
    , _weatherTemp(0)
    , _hasWeather(false)
{
    connect(_manager, &AirspaceManager::weatherUpdate, this, &AirspaceController::_weatherUpdate);
}

void AirspaceController::setROI(QGeoCoordinate center, double radius)
{
    _manager->setROI(center, radius);
    //-- If first time or we've moved more than WEATHER_UPDATE_DISTANCE, ask for weather updates.
    if(!_lastRoiCenter.isValid() || _lastRoiCenter.distanceTo(center) > WEATHER_UPDATE_DISTANCE) {
        _lastRoiCenter = center;
        _manager->requestWeatherUpdate(center);
        _weatherTime.start();
    } else {
        //-- Check weather once every WEATHER_UPDATE_TIME
        if(_weatherTime.elapsed() > WEATHER_UPDATE_TIME) {
            _manager->requestWeatherUpdate(center);
            _weatherTime.start();
        }
    }
}

void AirspaceController::_weatherUpdate(bool success, QGeoCoordinate, WeatherInformation weather)
{
    qCDebug(AirMapManagerLog)<<"Weather Info:"<< success << weather.condition << weather.temperature;
    _hasWeather     = success;
    _weatherIcon    = QStringLiteral("qrc:/airmapweather/") + weather.condition + QStringLiteral(".svg");
    _weatherTemp    = weather.temperature;
    emit weatherChanged();
}
