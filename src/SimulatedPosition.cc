#include <QtCore>

#include "SimulatedPosition.h"

SimulatedPosition::simulated_motion_s SimulatedPosition::_simulated_motion[4] = {{0,500},{500,0},{0, -500},{-500, 0}};

SimulatedPosition::SimulatedPosition()
    : QGeoPositionInfoSource(NULL)
{
    _simulate_motion_timer = 0;
    _simulate_motion_index = 0;
    _simulate_motion = true;

    update_timer.setSingleShot(false);
    connect(&update_timer, &QTimer::timeout, this, &SimulatedPosition::readNextPosition);

    update_timer.start(minimumUpdateInterval());
}

QGeoPositionInfo SimulatedPosition::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
    return lastPosition;
}

SimulatedPosition::PositioningMethods SimulatedPosition::supportedPositioningMethods() const
{
    return AllPositioningMethods;
}

int SimulatedPosition::minimumUpdateInterval() const
{
    return 1000;
}

void SimulatedPosition::startUpdates()
{
    int interval = updateInterval();
    if (interval < minimumUpdateInterval())
        interval = minimumUpdateInterval();
    update_timer.start(interval);
}

void SimulatedPosition::stopUpdates()
{
    update_timer.stop();
}

void SimulatedPosition::requestUpdate(int /*timeout*/)
{
    emit updateTimeout();
}

void SimulatedPosition::readNextPosition()
{
    int32_t lat_int = 47.3977420*1e7;
    int32_t lon_int = 8.5455941*1e7;

    _createSimulatedMotion(lat_int, lon_int);

    _simulate_motion_timer++;
    double latitude = ((double) lat_int)*1e-7;
    double longitude = ((double) lon_int)*1e-7;

    QDateTime timestamp = QDateTime::currentDateTime();

    QGeoCoordinate position(latitude, longitude);
    QGeoPositionInfo info(position, timestamp);
    lastPosition = info;

    emit lastPositionInfoUpdated(info);
    emit lastPositionUpdated(info.isValid(), QVariant::fromValue(position));
}

void SimulatedPosition::_createSimulatedMotion(int32_t & lat, int32_t & lon)
{
    static int f_lon = 0;
    static int f_lat = 0;
    static float rot = 0;

    rot += (float) .1;

    if(!(_simulate_motion_timer++%50)) {
        _simulate_motion_index++;
        if(_simulate_motion_index > 3) {
            _simulate_motion_index = 0;
        }
    }

    f_lon = f_lon + _simulated_motion[_simulate_motion_index].lon*sin(rot);
    f_lat = f_lat + _simulated_motion[_simulate_motion_index].lat;

    lat += f_lon;
    lon += f_lat;
}

QGeoPositionInfoSource::Error SimulatedPosition::error() const
{
    return QGeoPositionInfoSource::NoError;
}

QGeoPositionInfoSource * QGCPositionManager::positionSource()
{
    return &_simulatedPosition;
}
