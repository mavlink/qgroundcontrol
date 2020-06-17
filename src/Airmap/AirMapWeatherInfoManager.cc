/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapWeatherInfoManager.h"
#include "AirMapManager.h"


#define WEATHER_UPDATE_DISTANCE 50000                   //-- 50km threshold for weather updates
#define WEATHER_UPDATE_TIME     30 * 60 * 60 * 1000     //-- 30 minutes threshold for weather updates

using namespace airmap;

AirMapWeatherInfoManager::AirMapWeatherInfoManager(AirMapSharedState& shared, QObject *parent)
    : AirspaceWeatherInfoProvider(parent)
    , _valid(false)
    , _shared(shared)
{
}

void
AirMapWeatherInfoManager::setROI(const QGCGeoBoundingCube& roi, bool reset)
{
    //-- If first time or we've moved more than WEATHER_UPDATE_DISTANCE, ask for weather updates.
    if(reset || (!_lastRoiCenter.isValid() || _lastRoiCenter.distanceTo(roi.center()) > WEATHER_UPDATE_DISTANCE)) {
        _lastRoiCenter = roi.center();
        _requestWeatherUpdate(_lastRoiCenter);
        _weatherTime.start();
    } else {
        //-- Check weather once every WEATHER_UPDATE_TIME
        if(_weatherTime.elapsed() > WEATHER_UPDATE_TIME) {
            _requestWeatherUpdate(roi.center());
            _weatherTime.start();
        }
    }
}

void
AirMapWeatherInfoManager::_requestWeatherUpdate(const QGeoCoordinate& coordinate)
{
    qCDebug(AirMapManagerLog) << "Weather Request (ROI Changed)";
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Weather information";
        _valid = false;
        emit weatherChanged();
        return;
    }
    Advisory::ReportWeather::Parameters params;
    params.longitude= static_cast<float>(coordinate.longitude());
    params.latitude = static_cast<float>(coordinate.latitude());
    _shared.client()->advisory().report_weather(params, [this, coordinate](const Advisory::ReportWeather::Result& result) {
        if (result) {
            _weather = result.value();
            _valid  = true;
            if(_weather.icon.empty()) {
                _icon = QStringLiteral("qrc:/airmapweather/unknown.svg");
            } else {
                _icon = QStringLiteral("qrc:/airmapweather/") + QString::fromStdString(_weather.icon).replace("-", "_") + QStringLiteral(".svg");
            }
            qCDebug(AirMapManagerLog) << "Weather Info: " << _valid << "Icon:" << QString::fromStdString(_weather.icon) << "Condition:" << QString::fromStdString(_weather.condition) << "Temp:" << _weather.temperature;
        } else {
            _valid  = false;
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            qCDebug(AirMapManagerLog) << "Request Weather Failed" << QString::fromStdString(result.error().message()) << description;
        }
        emit weatherChanged();
    });
}
