#include "QGCGeoTiledMap.h"

#include <cmath>

#include <QtCore/QtMath>
#include <QtLocation/private/qgeocameradata_p.h>
#include <QtPositioning/QGeoCoordinate>

#include "MapCredits.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCGeoTiledMappingManagerEngine.h"
#include "QGCLoggingCategory.h"
#include "QtLocationMapId.h"

QGC_LOGGING_CATEGORY(QGCGeoTiledMapLog, "QtLocationPlugin.QGCGeoTiledMap")

QGCGeoTiledMap::QGCGeoTiledMap(QGCGeoTiledMappingManagerEngine* engine, QObject* parent)
    : QGeoTiledMap(engine, parent)
{
    qCDebug(QGCGeoTiledMapLog) << this;

    // The list of the sources of a map arrives after the view first asked for them
    (void) connect(MapCredits::instance(), &MapCredits::areaAttributionChanged, this,
                   &QGCGeoTiledMap::_updateCopyrights);
}

QGCGeoTiledMap::~QGCGeoTiledMap()
{
    qCDebug(QGCGeoTiledMapLog) << this;
}

void QGCGeoTiledMap::setCopyrightVisible(bool visible)
{
    if (visible) {
        // Not what the notice that was visible before was told
        _copyrights.clear();
    }
    QGeoTiledMap::setCopyrightVisible(visible);
}

void QGCGeoTiledMap::evaluateCopyrights(const QSet<QGeoTileSpec>& visibleTiles)
{
    Q_UNUSED(visibleTiles);
    _updateCopyrights();
}

void QGCGeoTiledMap::_updateCopyrights()
{
    QString credits;
    const SharedMapProvider provider =
        MapProviderRegistry::getMapProviderFromMapId(QtLocationMapId::toProviderId(activeMapType().mapId()));
    if (provider) {
        credits = provider->attribution();
        const QGeoCameraData& camera = cameraData();
        // The credits are those of an area with north up: of the box around the view, which a turned map shows at
        // an angle
        const double bearing = qDegreesToRadians(camera.bearing());
        const double width =
            std::abs(viewportWidth() * std::cos(bearing)) + std::abs(viewportHeight() * std::sin(bearing));
        const double height =
            std::abs(viewportWidth() * std::sin(bearing)) + std::abs(viewportHeight() * std::cos(bearing));
        const QString areaCredits =
            MapCredits::instance()->areaAttribution(provider->mapName(), camera.center().latitude(),
                                                    camera.center().longitude(), camera.zoomLevel(), width, height);
        if (!areaCredits.isEmpty()) {
            credits += (credits.isEmpty() ? QString() : QStringLiteral(", ")) + areaCredits;
        }
    }

    // The notice takes HTML, and hides for an empty one
    const QString html = credits.toHtmlEscaped();
    if (html != _copyrights) {
        _copyrights = html;
        emit copyrightsChanged(html);
    }
}
