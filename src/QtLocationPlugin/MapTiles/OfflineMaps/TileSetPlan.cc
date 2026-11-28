#include "TileSetPlan.h"

#include <algorithm>

#include "ElevationMapProvider.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapTilesSettings.h"

namespace {

/// The zoom in the keys of the tiles of an elevation provider that has no zoom levels: the terrain queries ask
/// the tile cache for it
constexpr int kTileSetPlanElevationZoom = 1;

}  // namespace

TileSetPlan TileSetPlan::forView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom,
                                 const QString& mapName, bool fetchElevation)
{
    TileSetPlan plan;
    plan.topleftLat = lat0;
    plan.topleftLon = lon0;
    plan.bottomRightLat = lat1;
    plan.bottomRightLon = lon1;
    const SharedMapProvider mapProvider = MapProviderRegistry::getMapProviderFromProviderType(mapName);
    const bool elevationMap = mapProvider && mapProvider->isElevationProvider();
    if (mapProvider && !elevationMap) {
        // The server of the provider has no tile below its deepest zoom level: a request for one only fails
        maxZoom = std::min(maxZoom, mapProvider->maxZoom());
        minZoom = std::min(minZoom, maxZoom);
    }
    plan.map.minZoom = minZoom;
    plan.map.maxZoom = maxZoom;
    plan.elevation.minZoom = kTileSetPlanElevationZoom;
    plan.elevation.maxZoom = kTileSetPlanElevationZoom;

    // GeoMap fetches the Terrarium tile of every imagery tile, at most as fine as Terrarium serves, and the anchor
    // zoom tiles around every tile finer than those
    plan.terrain.maxZoom = std::min(maxZoom, TerrariumElevationProvider::kMaxZoom);
    plan.terrain.minZoom = std::min(minZoom, std::min(plan.terrain.maxZoom, TerrariumElevationProvider::kAnchorZoom));
    const QString terrainProviderName = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);

    // An elevation provider does not have the zooms of the view: a set of Terrarium tiles has those that GeoMap
    // fetches, and a set of tiles without zoom levels the one that their keys have
    if (mapName == terrainProviderName) {
        plan.map.minZoom = plan.terrain.minZoom;
        plan.map.maxZoom = plan.terrain.maxZoom;
    } else if (elevationMap) {
        plan.map.minZoom = kTileSetPlanElevationZoom;
        plan.map.maxZoom = kTileSetPlanElevationZoom;
    }

    const auto addTiles = [lon0, lat0, lon1, lat1](Set& set, const SharedMapProvider& provider) {
        if (!provider) {
            return;
        }
        for (int z = set.minZoom; z <= set.maxZoom; z++) {
            set.tiles += provider->getTileCount(z, lon0, lat0, lon1, lat1);
        }
    };

    addTiles(plan.map, mapProvider);

    if (fetchElevation && !elevationMap) {
        addTiles(plan.elevation,
                 MapProviderRegistry::getMapProviderFromProviderType(MapTilesSettings::elevationMapProvider()));

        if (MapTilesSettings::usesGeoMapEngine()) {
            addTiles(plan.terrain, MapProviderRegistry::getMapProviderFromProviderType(terrainProviderName));
        }
    }

    return plan;
}
