#include "QGeoTiledMapQGC.h"

#include "QGCLoggingCategory.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

QGC_LOGGING_CATEGORY(QGeoTiledMapQGCLog, "QtLocationPlugin.QGeoTiledMapQGC")

QGeoTiledMapQGC::QGeoTiledMapQGC(QGeoTiledMappingManagerEngineQGC* engine, QObject* parent)
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
    return Capabilities(SupportsVisibleRegion | SupportsAnchoringCoordinate | SupportsVisibleArea);
}
