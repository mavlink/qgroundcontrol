#pragma once

#include <memory>

#include <QtCore/QByteArrayView>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringView>
#include <QtCore/QUrl>

#include "QGCTileSet.h"

class MapProvider;
class ElevationProvider;

class UrlFactory
{
public:
    static QString getImageFormat(QStringView type, QByteArrayView image);
    static QString getImageFormat(int mapId, QByteArrayView image);

    static QUrl getTileURL(QStringView type, int x, int y, int zoom);
    static QUrl getTileURL(int mapId, int x, int y, int zoom);

    static quint32 averageSizeForType(QStringView type);

    static bool isElevation(int mapId);

    static int long2tileX(QStringView mapType, double lon, int z);
    static int lat2tileY(QStringView mapType, double lat, int z);

    static QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                   double bottomRightLat, QStringView mapType);

    static const QList<std::shared_ptr<const MapProvider>>& getProviders() { return _providers; }

    static QStringList getElevationProviderTypes();
    static QStringList getProviderTypes();
    static int getMapIdFromProviderType(QStringView type);
    static QString getProviderTypeFromMapId(int mapId);
    static std::shared_ptr<const MapProvider> getMapProviderFromMapId(int mapId);
    static std::shared_ptr<const MapProvider> getMapProviderFromProviderType(QStringView type);

    /// The ids the providers had in schema version 1 of the tile cache on this build, each with the provider's id.
    /// Version 1 numbered the providers by their position in the provider list of the build.
    static QMap<int, int> legacyMapIds();

    /// Key of a tile in the tile cache: provider id, x, y and zoom as "%010d%08d%08d%03d". For a provider with a
    /// cache variant (MapProvider::cacheVariant()), '-' and the first 8 hex digits of the SHA-1 of the variant
    /// follow, so that the tiles of every source the user configures have keys of their own.
    /// A \a type that names no provider is a warning and gives a key with the id -1.
    /// Takes the variant from the provider, which reads settings: call it on the main thread only.
    static QString getTileHash(QStringView type, int x, int y, int z);
    /// The key for \a cacheVariant, which the caller got on the main thread. Callable on any thread.
    static QString getTileHash(QStringView type, int x, int y, int z, QStringView cacheVariant);

    /// What the key of a tile ends with for \a cacheVariant. Empty for an empty variant.
    static QString cacheVariantSuffix(QStringView cacheVariant);

    /// The cache variant of the provider \a type. Main thread only.
    static QString cacheVariant(QStringView type);

    /// The cache variants by provider id, of the providers that have one now. Main thread only.
    static QHash<int, QString> cacheVariants();

private:
    template <typename Provider>
    static std::shared_ptr<const MapProvider> _provider(int mapId);

    /// The provider named \a type, or null. Does not log: the callers differ in what a name without a provider is
    /// to them.
    static std::shared_ptr<const MapProvider> _findProvider(QStringView type);

    /// The key of a tile of \a provider, which the caller looked up by \a type
    static QString _tileHash(const std::shared_ptr<const MapProvider>& provider, QStringView type, int x, int y, int z,
                             QStringView cacheVariant);

    static const QList<std::shared_ptr<const MapProvider>> _providers;
};

typedef std::shared_ptr<const MapProvider> SharedMapProvider;
typedef std::shared_ptr<const ElevationProvider> SharedElevationProvider;
