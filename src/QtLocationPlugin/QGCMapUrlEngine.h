#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringView>
#include <QtCore/QUrl>

#include <memory>
#include <mutex>

#include "QGCTileSet.h"

class MapProvider;
class ElevationProvider;

class UrlFactory
{
public:
    static QString getImageFormat(QStringView type, QByteArrayView image);
    static QString getImageFormat(int qtMapId, QByteArrayView image);

    static QUrl getTileURL(QStringView type, int x, int y, int zoom);
    static QUrl getTileURL(int qtMapId, int x, int y, int zoom);

    static quint32 averageSizeForType(QStringView type);

    static bool isElevation(int qtMapId);

    static int long2tileX(QStringView mapType, double lon, int z);
    static int lat2tileY(QStringView mapType, double lat, int z);

    static QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat,
                            double bottomRightLon, double bottomRightLat,
                            QStringView mapType);

    static const QList<std::shared_ptr<const MapProvider>>& getProviders() { return _providers; }
    static QStringList getElevationProviderTypes();
    static QStringList getProviderTypes();
    static constexpr int defaultSetMapId() { return -1; }

    static int getQtMapIdFromProviderType(QStringView type);
    static QString getProviderTypeFromQtMapId(int qtMapId);
    static std::shared_ptr<const MapProvider> getMapProviderFromQtMapId(int qtMapId);
    static std::shared_ptr<const MapProvider> getMapProviderFromProviderType(QStringView type);
    static QString providerTypeFromHash(int hash);

    static int hashFromProviderType(QStringView type);
    static QString tileHashToType(QStringView tileHash);
    static QString getTileHash(QStringView type, int x, int y, int z);

    // Retina/HiDPI (R4): when the display reports a device-pixel-ratio > 1 the
    // engine requests 512px (@2x) tiles. Computed once from QGuiApplication and
    // cached; returns false when no QGuiApplication exists (e.g. unit tests).
    static bool useRetinaTiles();
    // Pixel scale for the active display: 2 when useRetinaTiles(), else 1. Used by
    // providers that build @2x / scale=2 URLs.
    static int tilePixelScale();

private:
    static void _ensureInitialized();

    static const QList<std::shared_ptr<const MapProvider>> _providers;
    static QHash<int, std::shared_ptr<const MapProvider>> _providersByMapId;
    static QHash<QString, std::shared_ptr<const MapProvider>> _providersByName;
    static std::once_flag _initFlag;
};

typedef std::shared_ptr<const MapProvider> SharedMapProvider;
typedef std::shared_ptr<const ElevationProvider> SharedElevationProvider;
