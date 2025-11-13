/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoRouter.h"

QGC_LOGGING_CATEGORY(GeoRouterLog, "qgc.qtlocationplugin.georouter")

GeoRouter::GeoRouter(QObject *parent)
    : QObject(parent)
{
}

void GeoRouter::registerEndpoint(int mapId, const QString &baseUrl, const QGeoCoordinate &location, int priority)
{
    QMutexLocker locker(&_mutex);

    ProviderEndpoint endpoint;
    endpoint.baseUrl = baseUrl;
    endpoint.location = location;
    endpoint.priority = priority;

    _providerEndpoints[mapId].append(endpoint);

    qCDebug(GeoRouterLog) << "Registered endpoint for mapId" << mapId
                           << "URL:" << baseUrl
                           << "Location:" << location
                           << "Priority:" << priority;
}

void GeoRouter::clearEndpoints(int mapId)
{
    QMutexLocker locker(&_mutex);
    _providerEndpoints.remove(mapId);
}

QString GeoRouter::selectEndpoint(int mapId, const QGeoCoordinate &userLocation) const
{
    QMutexLocker locker(&_mutex);

    const auto endpoints = _providerEndpoints.value(mapId);
    if (endpoints.isEmpty()) {
        return QString();
    }

    return selectBestEndpoint(endpoints, userLocation);
}

QString GeoRouter::selectEndpoint(int mapId) const
{
    return selectEndpoint(mapId, _userLocation);
}

QString GeoRouter::selectBestEndpoint(const QList<ProviderEndpoint> &endpoints, const QGeoCoordinate &userLocation) const
{
    if (endpoints.isEmpty()) {
        return QString();
    }

    // If no user location, use priority
    if (!userLocation.isValid()) {
        const auto minIt = std::min_element(endpoints.begin(), endpoints.end(),
                                             [](const ProviderEndpoint &a, const ProviderEndpoint &b) {
                                                 return a.priority < b.priority;
                                             });
        return minIt->baseUrl;
    }

    // Find closest endpoint with geographic distance + priority
    const ProviderEndpoint *best = nullptr;
    double bestScore = std::numeric_limits<double>::max();

    for (const auto &endpoint : endpoints) {
        double distance = calculateDistance(userLocation, endpoint.location);
        // Normalize priority impact (priority 0-10 range)
        double score = distance + (endpoint.priority * 1000.0);  // 1000 km per priority point

        if (score < bestScore) {
            bestScore = score;
            best = &endpoint;
        }
    }

    return best ? best->baseUrl : endpoints.first().baseUrl;
}

double GeoRouter::calculateDistance(const QGeoCoordinate &from, const QGeoCoordinate &to) const
{
    if (!from.isValid() || !to.isValid()) {
        return std::numeric_limits<double>::max();
    }

    return from.distanceTo(to);  // Returns meters
}

void GeoRouter::setUserLocation(const QGeoCoordinate &location)
{
    QMutexLocker locker(&_mutex);
    _userLocation = location;
    qCDebug(GeoRouterLog) << "User location updated:" << location;
}

QList<ProviderEndpoint> GeoRouter::endpoints(int mapId) const
{
    QMutexLocker locker(&_mutex);
    return _providerEndpoints.value(mapId);
}
