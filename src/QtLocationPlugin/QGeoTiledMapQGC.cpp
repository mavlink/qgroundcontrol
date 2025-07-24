/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGeoTiledMapQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include <QGCLoggingCategory.h>

QGC_LOGGING_CATEGORY(QGeoTiledMapQGCLog, "qgc.qtlocationplugin.qgeotiledmapqgc")

QGeoTiledMapQGC::QGeoTiledMapQGC(QGeoTiledMappingManagerEngineQGC *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{
	// qCDebug(QGeoTiledMapQGCLog) << Q_FUNC_INFO << this;
}

QGeoTiledMapQGC::~QGeoTiledMapQGC()
{
    // qCDebug(QGeoTiledMapQGCLog) << Q_FUNC_INFO << this;
}

QGeoMap::Capabilities QGeoTiledMapQGC::capabilities() const
{
    return Capabilities(SupportsVisibleRegion
                        | SupportsAnchoringCoordinate
                        | SupportsVisibleArea);
}

/*void QGeoTiledMapQGC::evaluateCopyrights(const QSet<QGeoTileSpec> &visibleTiles)
{
    if (visibleTiles.isEmpty()) {
        return;
    }

    const QGeoTileSpec tile = *(visibleTiles.constBegin());

    const MapProvider* const provider = getProviderFromQtMapId(tile.mapId());

    if (provider) {
        emit copyrightsChanged(provider->copyright());
    }
}*/
