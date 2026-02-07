#include "QGeoTiledMapQGC.h"

#include <QtCore/QLoggingCategory>
#include "QGeoTiledMappingManagerEngineQGC.h"

Q_STATIC_LOGGING_CATEGORY(QGeoTiledMapQGCLog, "QtLocationPlugin.QGeoTiledMapQGC")

QGeoTiledMapQGC::QGeoTiledMapQGC(QGeoTiledMappingManagerEngineQGC *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{
    qCDebug(QGeoTiledMapQGCLog) << this;
}

QGeoTiledMapQGC::~QGeoTiledMapQGC()
{
    qCDebug(QGeoTiledMapQGCLog) << this;
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
