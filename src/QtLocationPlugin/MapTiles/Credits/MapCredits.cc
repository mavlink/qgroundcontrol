#include "MapCredits.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QApplicationStatic>
#include <QtCore/QUrl>
#include <QtCore/QtMath>

#include "ElevationMapProvider.h"
#include "EsriAttribution.h"
#include "MapEngine.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapTilesApp.h"
#include "WebMercator.h"

Q_APPLICATION_STATIC(MapCredits, _mapCredits);

namespace {

/// The edge of a tile in pixels: the Web Mercator map of the world is one tile at zoom level 0, and its edge doubles
/// with every level
constexpr double kMapCreditsTilePixels = 256.0;

/// A zoom level that round-off left this far below a whole number is that number: GeoMap computes its zoom level
/// from the distance of its camera
constexpr double kMapCreditsZoomLevelTolerance = 1e-6;

}  // namespace

MapCredits* MapCredits::instance()
{
    return _mapCredits();
}

MapCredits::MapCredits(QObject* parent)
    : QObject(parent)
{}

QString MapCredits::mapAttribution(const QString& mapType)
{
    const SharedMapProvider provider = MapProviderRegistry::findProvider(mapType);
    return provider ? provider->attribution() : QString();
}

QString MapCredits::terrainAttribution()
{
    return mapAttribution(QString::fromLatin1(TerrariumElevationProvider::kProviderKey));
}

QString MapCredits::areaAttribution(const QString& mapType, double latitude, double longitude, double zoomLevel,
                                    double width, double height)
{
    const SharedMapProvider provider = MapProviderRegistry::findProvider(mapType);
    const QUrl listUrl = provider ? provider->areaAttributionUrl() : QUrl();
    if (listUrl.isEmpty()) {
        return QString();
    }
    // No area: a view without a size, or without a centre
    if (!qIsFinite(latitude) || !qIsFinite(longitude) || !qIsFinite(zoomLevel) || !(width > 0.0) || !(height > 0.0)) {
        return QString();
    }
    EsriAttribution* const lists = _areaAttributionLists();
    if (!lists) {
        return QString();
    }

    const double worldPixels = kMapCreditsTilePixels * std::exp2(zoomLevel);
    const double halfWidthDegrees = (width / worldPixels) * 180.0;
    const double halfHeight = (height / worldPixels) / 2.0;
    const double centerY = std::clamp(WebMercator::yOfLatitude(latitude), 0.0, 1.0);
    const double north = WebMercator::latitudeOfY(std::max(centerY - halfHeight, 0.0));
    const double south = WebMercator::latitudeOfY(std::min(centerY + halfHeight, 1.0));

    // A map draws the tiles of the level at or below its zoom level, magnified
    const double tileZoom = std::floor(zoomLevel + kMapCreditsZoomLevelTolerance);
    const int zoom = static_cast<int>(std::clamp(tileZoom, 0.0, static_cast<double>(QGC_MAX_MAP_ZOOM)));

    return lists->credits(listUrl, south, longitude - halfWidthDegrees, north, longitude + halfWidthDegrees, zoom);
}

EsriAttribution* MapCredits::_areaAttributionLists()
{
    if (MapTilesApp::runningUnitTests() && !MapEngine::instance()->hasNetworkManagerForTest()) {
        return nullptr;
    }
    if (!_esriAttribution) {
        _esriAttribution = new EsriAttribution(this);
        (void) connect(_esriAttribution, &EsriAttribution::listReceived, this,
                       &MapCredits::_notifyAreaAttributionChanged);
    }

    return _esriAttribution;
}

void MapCredits::_notifyAreaAttributionChanged()
{
    _areaAttributionRevision++;
    emit areaAttributionChanged();
}
