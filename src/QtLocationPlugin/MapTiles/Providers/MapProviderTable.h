#pragma once

#include <memory>

#include <QtCore/QList>

class MapProvider;

namespace MapProviderTable {

/// The providers of the maps that are shown: every provider but the elevation providers (ElevationMapProvider.h)
QList<std::shared_ptr<const MapProvider>> providers();

}  // namespace MapProviderTable
