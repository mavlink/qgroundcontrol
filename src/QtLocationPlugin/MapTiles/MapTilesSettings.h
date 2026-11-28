#pragma once

#include <QtCore/QString>

class Fact;

/// The settings of the application that the tile library and the QtLocation plugin above it read. Only
/// MapTilesSettings.cc knows where the application keeps them: the build gives no other file of the two the headers
/// of the settings.
namespace MapTilesSettings {

/// The provider and the type of the map the user chose. Null before the application has made its settings.
Fact* mapProvider();
Fact* mapType();

/// The name of the elevation provider the user chose
QString elevationMapProvider();

/// A view draws its terrain with GeoMap, which fetches the tiles of the Terrarium provider
bool usesGeoMapEngine();

/// The application saves nothing, so no tile either
bool persistenceDisabled();

/// The megabytes that the tiles of the default set may take
quint32 maxCacheDiskSizeMB();

/// The megabytes of tiles that a map of the QtLocation plugin may keep in memory
quint32 maxCacheMemorySizeMB();

/// What the user set for the services of the maps
QString esriToken();
QString mapboxToken();
QString tiandituToken();
QString vworldToken();
QString openaipToken();
const Fact* mapboxAccount();
const Fact* mapboxStyle();
const Fact* customUrl();

/// The language of QGC
const Fact* language();

}  // namespace MapTilesSettings
