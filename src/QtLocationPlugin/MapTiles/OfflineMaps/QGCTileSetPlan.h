#pragma once

#include <QtCore/QString>

#include "QGCTileSet.h"

/// The offline tile sets that saving the view of a map creates, and the tiles that each of them gets
struct QGCTileSetPlan
{
    /// The tiles of one set, which are of the zooms minZoom to maxZoom. A set without tiles is not created.
    struct Set
    {
        QGCTileSet tiles;
        int minZoom = 0;
        int maxZoom = 0;
    };

    /// Plans the sets of the view between its top left corner, at \a lon0 and \a lat0, and its bottom right one, at
    /// \a lon1 and \a lat1. Imagery \a mapName gets a set of the zooms \a minZoom to \a maxZoom and, when
    /// \a fetchElevation, the sets of elevation tiles beside it. An elevation provider \a mapName gets one set, of
    /// the zooms that the provider has for those of the view.
    static QGCTileSetPlan forView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom,
                                  const QString& mapName, bool fetchElevation);

    quint64 tileCount() const { return map.tiles.tileCount + elevation.tiles.tileCount + terrain.tiles.tileCount; }

    quint64 tileSize() const { return map.tiles.tileSize + elevation.tiles.tileSize + terrain.tiles.tileSize; }

    double topleftLat = 0.;
    double topleftLon = 0.;
    double bottomRightLat = 0.;
    double bottomRightLon = 0.;
    /// The tiles of the map of the view
    Set map;
    /// The tiles of the elevation provider of the settings
    Set elevation;
    /// Terrarium tiles GeoMap renders its terrain from: empty unless elevation is fetched and GeoMap is enabled
    Set terrain;
};
