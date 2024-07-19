/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 *  @file
 *  @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include "QGCTileSet.h"

#include <QtCore/QObject>
#include <QtCore/QByteArrayView>
#include <QtCore/QStringView>
#include <QtNetwork/QNetworkRequest>

#define MAX_MAP_ZOOM (23.0)

class MapProvider;

class UrlFactory
{
public:
    static QString getImageFormat(QStringView type, QByteArrayView image);
    static QString getImageFormat(int qtMapId, QByteArrayView image);

    static QNetworkRequest getTileURL(QStringView type, int x, int y, int zoom);
    static QNetworkRequest getTileURL(int qtMapId, int x, int y, int zoom);

    static quint32 averageSizeForType(QStringView type);

    static bool isElevation(int qtMapId);

    static int long2tileX(QStringView mapType, double lon, int z);
    static int lat2tileY(QStringView mapType, double lat, int z);

    static QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat,
                            double bottomRightLon, double bottomRightLat,
                            QStringView mapType);

    static const QList<std::shared_ptr<const MapProvider>>& getProviders() { return _providers; }
    static QStringList getProviderTypes();
    static int getQtMapIdFromProviderType(QStringView type);
    static QString getProviderTypeFromQtMapId(int qtMapId);
    static std::shared_ptr<const MapProvider> getMapProviderFromQtMapId(int qtMapId);
    static std::shared_ptr<const MapProvider> getMapProviderFromProviderType(QStringView type);
    static QString providerTypeFromHash(int hash);

    static int hashFromProviderType(QStringView type);
    static QString tileHashToType(QStringView tileHash);
    static QString getTileHash(QStringView type, int x, int y, int z);

    static constexpr const char* kCopernicusElevationProviderKey = "Copernicus Elevation";
    static constexpr const char* kCopernicusElevationProviderNotice = "© Airbus Defence and Space GmbH";

private:
    static const QList<std::shared_ptr<const MapProvider>> _providers;
};

typedef std::shared_ptr<const MapProvider> SharedMapProvider;
