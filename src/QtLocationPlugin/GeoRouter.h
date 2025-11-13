/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(GeoRouterLog)

class MapProvider;

struct ProviderEndpoint {
    QString baseUrl;
    QGeoCoordinate location;  // Approximate CDN location
    int priority = 0;  // Lower is better
};

class GeoRouter : public QObject
{
    Q_OBJECT

public:
    explicit GeoRouter(QObject *parent = nullptr);

    // Register provider endpoints with geographic locations
    void registerEndpoint(int mapId, const QString &baseUrl, const QGeoCoordinate &location, int priority = 0);
    void clearEndpoints(int mapId);

    // Get best endpoint based on user location
    QString selectEndpoint(int mapId, const QGeoCoordinate &userLocation) const;
    QString selectEndpoint(int mapId) const;  // Use last known location

    // Update user location for routing decisions
    void setUserLocation(const QGeoCoordinate &location);
    QGeoCoordinate userLocation() const { return _userLocation; }

    // Get all endpoints for a provider
    QList<ProviderEndpoint> endpoints(int mapId) const;

private:
    double calculateDistance(const QGeoCoordinate &from, const QGeoCoordinate &to) const;
    QString selectBestEndpoint(const QList<ProviderEndpoint> &endpoints, const QGeoCoordinate &userLocation) const;

    QMap<int, QList<ProviderEndpoint>> _providerEndpoints;
    QGeoCoordinate _userLocation;
    mutable QMutex _mutex;
};
