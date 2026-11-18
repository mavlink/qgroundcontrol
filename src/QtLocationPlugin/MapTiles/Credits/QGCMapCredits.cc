#include "QGCMapCredits.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QApplicationStatic>
#include <QtCore/QUrl>
#include <QtCore/QtMath>
#include <QtNetwork/QNetworkAccessManager>

#include "AppMessages.h"
#include "ElevationMapProvider.h"
#include "EsriAttribution.h"
#include "MapProvider.h"
#include "UrlFactory.h"

Q_APPLICATION_STATIC(QGCMapCredits, _mapCredits);

namespace {

/// The edge of a tile in pixels: the Web Mercator map of the world is one tile at zoom level 0, and its edge doubles
/// with every level
constexpr double kMapCreditsTilePixels = 256.0;

/// A zoom level that round-off left this far below a whole number is that number: GeoMap computes its zoom level
/// from the distance of its camera
constexpr double kMapCreditsZoomLevelTolerance = 1e-6;

/// Where \a latitude, in degrees, lies between the north edge (0) and the south edge (1) of the Web Mercator map of
/// the world. Beyond those edges for a latitude that the map does not have.
double mapCreditsMercatorY(double latitude)
{
    return 0.5 - (std::asinh(std::tan(qDegreesToRadians(latitude))) / (2.0 * M_PI));
}

/// The latitude, in degrees, that mapCreditsMercatorY() gives \a mercatorY for
double mapCreditsLatitude(double mercatorY)
{
    return qRadiansToDegrees(std::atan(std::sinh(M_PI * (1.0 - (2.0 * mercatorY)))));
}

}  // namespace

QGCMapCredits* QGCMapCredits::instance()
{
    return _mapCredits();
}

QGCMapCredits::QGCMapCredits(QObject* parent)
    : QObject(parent)
{}

QString QGCMapCredits::mapAttribution(const QString& mapType)
{
    const SharedMapProvider provider = UrlFactory::findProvider(mapType);
    return provider ? provider->attribution() : QString();
}

QString QGCMapCredits::terrainAttribution()
{
    return mapAttribution(QString::fromLatin1(TerrariumElevationProvider::kProviderKey));
}

QString QGCMapCredits::areaAttribution(const QString& mapType, double latitude, double longitude, double zoomLevel,
                                       double width, double height)
{
    const SharedMapProvider provider = UrlFactory::findProvider(mapType);
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
    const double centerY = std::clamp(mapCreditsMercatorY(latitude), 0.0, 1.0);
    const double north = mapCreditsLatitude(std::max(centerY - halfHeight, 0.0));
    const double south = mapCreditsLatitude(std::min(centerY + halfHeight, 1.0));

    // A map draws the tiles of the level at or below its zoom level, magnified
    const double tileZoom = std::floor(zoomLevel + kMapCreditsZoomLevelTolerance);
    const int zoom = static_cast<int>(std::clamp(tileZoom, 0.0, static_cast<double>(QGC_MAX_MAP_ZOOM)));

    return lists->credits(listUrl, south, longitude - halfWidthDegrees, north, longitude + halfWidthDegrees, zoom);
}

void QGCMapCredits::setAreaAttributionNetworkManager(QNetworkAccessManager* networkManager)
{
    delete _esriAttribution;
    _esriAttribution = nullptr;
    _areaAttributionNetworkManager = networkManager;
    _notifyAreaAttributionChanged();
}

EsriAttribution* QGCMapCredits::_areaAttributionLists()
{
    // No unit test reaches a server: under them, the lists are fetched through the network manager of a test only
    if (!_esriAttribution && (_areaAttributionNetworkManager || !QGC::runningUnitTests())) {
        _esriAttribution = new EsriAttribution(_areaAttributionNetworkManager, this);
        (void) connect(_esriAttribution, &EsriAttribution::listReceived, this,
                       &QGCMapCredits::_notifyAreaAttributionChanged);
    }

    return _esriAttribution;
}

void QGCMapCredits::_notifyAreaAttributionChanged()
{
    _areaAttributionRevision++;
    emit areaAttributionChanged();
}
