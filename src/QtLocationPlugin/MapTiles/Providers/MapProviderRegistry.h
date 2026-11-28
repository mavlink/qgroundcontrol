#pragma once

#include <memory>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringView>

class MapProvider;

class MapProviderRegistry
{
public:
    static const QList<std::shared_ptr<const MapProvider>>& providers();

    static QStringList elevationProviderTypes();
    static QStringList providerTypes();

    /// The provider with the permanent id \a mapId. Null for an id that no provider has, which is a warning unless
    /// the id is -1.
    static std::shared_ptr<const MapProvider> getMapProviderFromMapId(int mapId);
    /// The provider named \a type. Null for a name that no provider has, which is a warning unless the name is empty.
    static std::shared_ptr<const MapProvider> getMapProviderFromProviderType(QStringView type);
    /// As getMapProviderFromProviderType(), without the warning: for a name that can be the name of no provider
    static std::shared_ptr<const MapProvider> findProvider(QStringView type);

    /// -1 for a name that no provider has
    static int getMapIdFromProviderType(QStringView type);
    /// Empty for an id that no provider has
    static QString getProviderTypeFromMapId(int mapId);

    /// The ids the providers had in schema version 1 of the tile cache on this build, each with the provider's id.
    /// Version 1 numbered the providers by their position in the provider list of the build.
    static QMap<int, int> legacyMapIds();

    /// The cache variants (MapProvider::cacheVariant()) by provider id, of the providers that have one now. Main
    /// thread only.
    static QHash<int, QString> cacheVariants();
};

typedef std::shared_ptr<const MapProvider> SharedMapProvider;
