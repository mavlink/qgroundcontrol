#pragma once

#include <memory>

#include <QtCore/QList>

class MapProvider;

namespace MapProviderTable {

/// The id of the map of the tiles that the user imported from MBTiles files, which has no server
constexpr int kImportedMapId = 42;

/// The providers of the maps that are shown: every provider but the elevation providers (ElevationMapProvider.h)
QList<std::shared_ptr<const MapProvider>> providers();

}  // namespace MapProviderTable
