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

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QObject>

class MapProvider;

class UrlFactory
{
public:
    static QString getTileURL(QStringView type, int x, int y, int zoom);
    static QString getTileURL(int qtMapId, int x, int y, int zoom);

    static QString getImageFormat(QStringView type, QByteArrayView image);
    static QString getImageFormat(int qtMapId, QByteArrayView image);

    static quint32 averageSizeForType(QStringView type);

    static int long2tileX(QStringView mapType, double lon, int z);
    static int lat2tileY(QStringView mapType, double lat, int z);

    static const QList<std::shared_ptr<MapProvider>> getProviders() { return m_providers; }
    static QStringList getProviderTypes();
    static int getQtMapIdFromProviderType(QStringView type);
    static QString getProviderTypeFromQtMapId(int qtMapId);
    static std::shared_ptr<MapProvider> getProviderFromQtMapId(int qtMapId);
    static std::shared_ptr<MapProvider> getProviderFromProviderType(QStringView type);
    static QString getProviderTypeFromHash(int tileHash);
    static QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat,
                            double bottomRightLon, double bottomRightLat,
                            QStringView mapType);
    static bool isElevation(int qtMapId);
    static const std::shared_ptr<MapProvider> getElevationProvider();

    static int hashFromProviderType(QStringView type);
    static QString tileHashToType(QStringView tileHash);
    static QString getTileHash(QStringView type, int x, int y, int z);

private:
    static const QList<std::shared_ptr<MapProvider>> m_providers;
};

typedef std::shared_ptr<MapProvider> SharedMapProvider;
