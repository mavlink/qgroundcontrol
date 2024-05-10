#include "QGeoTiledMapQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include <QGCLoggingCategory.h>

QGC_LOGGING_CATEGORY(QGeoTiledMapQGCLog, "qgc.qtlocationplugin.qgeotiledmapqgc")

QGeoTiledMapQGC::QGeoTiledMapQGC(QGeoTiledMappingManagerEngineQGC *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{
	qCDebug(QGeoTiledMapQGCLog) << Q_FUNC_INFO << this;
}

QGeoTiledMapQGC::~QGeoTiledMapQGC()
{
    qCDebug(QGeoTiledMapQGCLog) << Q_FUNC_INFO << this;
}

QGeoMap::Capabilities QGeoTiledMapQGC::capabilities() const
{
    return Capabilities(SupportsVisibleRegion
                        | SupportsSetBearing
                        | SupportsAnchoringCoordinate
                        | SupportsVisibleArea);
}
