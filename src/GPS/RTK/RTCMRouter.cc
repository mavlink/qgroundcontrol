#include "RTCMRouter.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"

QGC_LOGGING_CATEGORY(RTCMRouterLog, "GPS.RTCMRouter")

RTCMRouter::RTCMRouter(RTCMMavlink *mavlink, QObject *parent)
    : QObject(parent)
    , _mavlink(mavlink)
{
    qCDebug(RTCMRouterLog) << this;
}

void RTCMRouter::setGpsRtk(GPSRtk *gpsRtk)
{
    _gpsDevices.clear();
    if (gpsRtk) {
        _gpsDevices.append(gpsRtk);
    }
}

void RTCMRouter::addGpsRtk(GPSRtk *gpsRtk)
{
    if (gpsRtk && !_gpsDevices.contains(gpsRtk)) {
        _gpsDevices.append(gpsRtk);
    }
}

void RTCMRouter::removeGpsRtk(GPSRtk *gpsRtk)
{
    _gpsDevices.removeAll(gpsRtk);
}

void RTCMRouter::routeToVehicles(const QByteArray &data)
{
    if (_mavlink) {
        _mavlink->RTCMDataUpdate(data);
    }
}

void RTCMRouter::routeToBaseStation(const QByteArray &data)
{
    for (auto &device : _gpsDevices) {
        if (device) {
            device->injectRTCMData(data);
        }
    }
}

void RTCMRouter::routeAll(const QByteArray &data)
{
    routeToVehicles(data);
    routeToBaseStation(data);
}
