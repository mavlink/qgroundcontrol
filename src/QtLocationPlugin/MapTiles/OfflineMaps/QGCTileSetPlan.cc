#include "QGCTileSetPlan.h"

#include "ElevationMapProvider.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "MapProvider.h"
#include "PlanViewSettings.h"
#include "SettingsManager.h"
#include "UrlFactory.h"

namespace {

/// The zoom in the keys of the tiles of an elevation provider that has no zoom levels: the terrain queries ask
/// the tile cache for it
constexpr int kTileSetPlanElevationZoom = 1;

}  // namespace

QGCTileSetPlan QGCTileSetPlan::forView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom,
                                       const QString& mapName, bool fetchElevation)
{
    QGCTileSetPlan plan;
    plan.topleftLat = lat0;
    plan.topleftLon = lon0;
    plan.bottomRightLat = lat1;
    plan.bottomRightLon = lon1;
    plan.map.minZoom = minZoom;
    plan.map.maxZoom = maxZoom;
    plan.elevation.minZoom = kTileSetPlanElevationZoom;
    plan.elevation.maxZoom = kTileSetPlanElevationZoom;

    // GeoMap fetches the Terrarium tile of every imagery tile, at most as fine as Terrarium serves, and the anchor
    // zoom tiles around every tile finer than those
    plan.terrain.maxZoom = qMin(maxZoom, TerrariumElevationProvider::kMaxZoom);
    plan.terrain.minZoom = qMin(minZoom, qMin(plan.terrain.maxZoom, TerrariumElevationProvider::kAnchorZoom));
    const QString terrainProviderName = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);

    // An elevation provider does not have the zooms of the view: a set of Terrarium tiles has those that GeoMap
    // fetches, and a set of tiles without zoom levels the one that their keys have
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromProviderType(mapName);
    const bool elevationMap = mapProvider && mapProvider->isElevationProvider();
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
        const SettingsManager* const settings = SettingsManager::instance();
        const QString elevationProviderName =
            settings->flightMapSettings()->elevationMapProvider()->rawValue().toString();
        addTiles(plan.elevation, UrlFactory::getMapProviderFromProviderType(elevationProviderName));

        if (settings->flyViewSettings()->useGeoMapEngine()->rawValue().toBool() ||
            settings->planViewSettings()->useGeoMapEngine()->rawValue().toBool()) {
            addTiles(plan.terrain, UrlFactory::getMapProviderFromProviderType(terrainProviderName));
        }
    }

    return plan;
}
