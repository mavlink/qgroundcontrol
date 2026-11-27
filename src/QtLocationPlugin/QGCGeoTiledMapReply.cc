#include "QGCGeoTiledMapReply.h"

#include <QtGui/QImage>

#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCGeoFileTileCache.h"
#include "QGCLoggingCategory.h"
#include "QtLocationMapId.h"
#include "RuntimeScheduler.h"
#include "ScheduledTask.h"
#include "TileReply.h"

QGC_LOGGING_CATEGORY(QGCGeoTiledMapReplyLog, "QtLocationPlugin.QGCGeoTiledMapReply")

namespace {

/// The tile \a levels zoom levels above \a spec that shows the ground of \a spec
QGeoTileSpec mapReplyAncestorSpec(const QGeoTileSpec& spec, int levels)
{
    return QGeoTileSpec(spec.plugin(), spec.mapId(), spec.zoom() - levels, spec.x() >> levels, spec.y() >> levels,
                        spec.version());
}

}  // namespace

void GeoNoTiles::insert(const QGeoTileSpec& spec)
{
    // Bounds the memory of a long flight over places without tiles
    constexpr qsizetype kMaxSpecs = 16384;
    if (_specs.size() >= kMaxSpecs) {
        _specs.clear();
    }
    (void) _specs.insert(spec);
}

GeoTileRequests::GeoTileRequests()
{
    // A few screens of the tiles above, which are 256 KB each at 256 pixels along the edge
    constexpr qsizetype kMaxImageBytes = 8 * 1024 * 1024;
    _images.setMaxCost(kMaxImageBytes);
}

std::shared_ptr<TileReply> GeoTileRequests::request(const QGeoTileSpec& spec)
{
    if (const std::shared_ptr<TileReply> underWay = _requests.value(spec).lock(); underWay && !underWay->isFinished()) {
        return underWay;
    }

    // Bounds the memory of a long flight: the requests that ended are forgotten
    constexpr qsizetype kMaxRequests = 256;
    if (_requests.size() >= kMaxRequests) {
        (void) _requests.removeIf(
            [](const decltype(_requests)::iterator& request) { return request.value().expired(); });
    }

    // The replies that hold the request own it. The last of them deletes it, from within its finished() too, which a
    // request allows: one that was deleted later would need an event loop to end with.
    const std::shared_ptr<TileReply> request(
        new TileReply(QtLocationMapId::toProviderId(spec.mapId()), spec.x(), spec.y(), spec.zoom()));
    _requests.insert(spec, request);
    request->start();
    return request;
}

QImage GeoTileRequests::image(const QGeoTileSpec& spec) const
{
    const QImage* const image = _images.object(spec);
    return image ? *image : QImage();
}

QImage GeoTileRequests::decode(const QGeoTileSpec& spec, const QByteArray& data)
{
    const QImage image = QImage::fromData(data);
    if (!image.isNull()) {
        // The cache owns the picture it is given, and deletes one that is too large for it
        (void) _images.insert(spec, new QImage(image), image.sizeInBytes());
    }
    return image;
}

void GeoTileRequests::clear()
{
    _requests.clear();
    _images.clear();
}

GeoTileFailures::GeoTileFailures(RuntimeScheduler* scheduler)
    : _scheduler(RuntimeScheduler::orDefault(scheduler, this))
{}

bool GeoTileFailures::contains(const QGeoTileSpec& spec)
{
    const auto failure = _until.constFind(spec);
    if (failure == _until.cend()) {
        return false;
    }
    if (_scheduler->nowUs() < failure.value()) {
        return true;
    }
    (void) _until.erase(failure);
    return false;
}

void GeoTileFailures::insert(const QGeoTileSpec& spec)
{
    // Bounds the memory of a long flight without a working tile server
    constexpr qsizetype kMaxSpecs = 16384;
    if (_until.size() >= kMaxSpecs) {
        _until.clear();
    }
    const auto rememberedFor = std::chrono::duration_cast<std::chrono::microseconds>(kRememberedFor);
    (void) _until.insert(spec, _scheduler->nowUs() + static_cast<quint64>(rememberedFor.count()));
}

void GeoTileFailures::setOnline(bool online)
{
    if (online == _online) {
        return;
    }
    _online = online;
    if (online) {
        // Tiles that failed for want of a connection can be fetched now
        clear();
        emit backOnline();
    }
}

QGCGeoTiledMapReply::QGCGeoTiledMapReply(const QGeoTileSpec& spec, std::shared_ptr<GeoNoTiles> noTiles,
                                         std::shared_ptr<GeoTileFailures> failures,
                                         std::shared_ptr<GeoTileRequests> requests, QGCGeoFileTileCache* tileCache,
                                         QObject* parent)
    : QGeoTiledMapReply(spec, parent)
    , _noTiles(noTiles ? std::move(noTiles) : std::make_shared<GeoNoTiles>())
    , _failures(failures ? std::move(failures) : std::make_shared<GeoTileFailures>())
    , _requests(requests ? std::move(requests) : std::make_shared<GeoTileRequests>())
    , _tileCache(tileCache)
    , _retryTask(std::make_unique<ScheduledTask>(_failures->scheduler(), this))
{
    qCDebug(QGCGeoTiledMapReplyLog) << this;
}

QGCGeoTiledMapReply::~QGCGeoTiledMapReply()
{
    qCDebug(QGCGeoTiledMapReplyLog) << this;
}

void QGCGeoTiledMapReply::init()
{
    if (_tileReply || isFinished()) {
        return;
    }

    // A tile below the deepest ones of its provider is not asked for: the server answers such a request with an error,
    // not with the news that it has no tile
    const QGeoTileSpec spec = tileSpec();
    if (const SharedMapProvider provider =
            MapProviderRegistry::getMapProviderFromMapId(QtLocationMapId::toProviderId(spec.mapId()))) {
        _ancestorLevels = qMax(0, spec.zoom() - provider->maxZoom());
    }
    _requestTile();
}

void QGCGeoTiledMapReply::abort()
{
    _retryTask->cancel();
    (void) disconnect(_backOnline);
    _dropTileReply();
    QGeoTiledMapReply::abort();
}

void QGCGeoTiledMapReply::_requestTile()
{
    const QGeoTileSpec spec = tileSpec();
    for (; (_ancestorLevels <= kMaxAncestorLevels) && (_ancestorLevels <= spec.zoom()); _ancestorLevels++) {
        const QGeoTileSpec ancestor = mapReplyAncestorSpec(spec, _ancestorLevels);
        if (_noTiles->contains(ancestor)) {
            continue;
        }
        if (_failures->contains(ancestor)) {
            qCDebug(QGCGeoTiledMapReplyLog) << "not asked for again yet, as it failed lately:" << ancestor;
            setFinished(true);
            return;
        }

        if (_ancestorLevels > 0) {
            // Another reply had the tile above decoded: neither the cache nor the server is asked for it
            if (const QImage above = _requests->image(ancestor); !above.isNull()) {
                _finishFromAbove(above, QByteArray(), QString(), true);
                return;
            }
        }

        _tileReply = _requests->request(ancestor);
        (void) connect(_tileReply.get(), &TileReply::finished, this, &QGCGeoTiledMapReply::_tileReplyFinished);
        return;
    }

    // No error of the reply: QtLocation asks five more times for a tile whose reply has one
    _noTile = true;
    setFinished(true);
}

void QGCGeoTiledMapReply::_tileReplyFinished()
{
    // This reply lets go of the request, which is deleted at the end of the function unless another reply holds it
    const std::shared_ptr<TileReply> tileReply = std::move(_tileReply);
    if (!tileReply) {
        return;
    }

    const QGeoTileSpec ancestor = mapReplyAncestorSpec(tileSpec(), _ancestorLevels);
    const TileReply::Error error = tileReply->error();
    if (error == TileReply::Error::NoError) {
        if (_ancestorLevels > 0) {
            _finishFromAbove(_requests->decode(ancestor, tileReply->data()), tileReply->data(), tileReply->format(),
                             tileReply->isCached());
            return;
        }
        setMapImageData(tileReply->data());
        setMapImageFormat(tileReply->format());
        setCached(tileReply->isCached());
        setFinished(true);
        return;
    }

    if (error == TileReply::Error::NoTileError) {
        qCDebug(QGCGeoTiledMapReplyLog) << tileReply->errorString() << "-" << _ancestorLevels << "zoom levels above";
        _noTiles->insert(ancestor);
        _ancestorLevels++;
        _requestTile();
        return;
    }

    if (!_retryLater(*tileReply)) {
        _giveUp(tileReply->errorString());
    }
}

void QGCGeoTiledMapReply::_finishFromAbove(const QImage& above, const QByteArray& aboveData, const QString& aboveFormat,
                                           bool cached)
{
    if (above.isNull()) {
        // The tile above is not asked for again: the cache serves the same bytes
        _giveUp(QStringLiteral("the tile %1 zoom levels above cannot be scaled up").arg(_ancestorLevels));
        return;
    }
    if (_tileCache) {
        _tileCache->insertFromAbove(tileSpec(), mapReplyAncestorSpec(tileSpec(), _ancestorLevels), above, aboveData,
                                    aboveFormat);
    }
    setCached(cached);
    setFinished(true);
}

void QGCGeoTiledMapReply::_dropTileReply()
{
    if (_tileReply) {
        (void) disconnect(_tileReply.get(), nullptr, this, nullptr);
        _tileReply.reset();
    }
}

bool QGCGeoTiledMapReply::_retryLater(const TileReply& tileReply)
{
    const TileReply::NetworkFailure failure = tileReply.networkFailure();
    const bool canClearUp = (failure == TileReply::NetworkFailure::Unreachable) ||
                            (failure == TileReply::NetworkFailure::Unavailable) ||
                            (failure == TileReply::NetworkFailure::RateLimited);
    // Waits for the system to come back online count too: a system that goes on and off line does not make a reply
    // ask for ever
    if (!canClearUp || (_retries >= static_cast<int>(kRetryDelays.size()))) {
        return false;
    }
    const auto retry = static_cast<size_t>(_retries++);

    if (failure == TileReply::NetworkFailure::RateLimited) {
        // The answer paused the server: the request waits for the pause to end, after its cache lookup
        qCDebug(QGCGeoTiledMapReplyLog) << tileReply.errorString()
                                        << "- asking again once the pause of the server ends";
        _requestTile();
        return true;
    }

    // A system that does not report being back online can be wrong, or reach the tile server on its own network: the
    // server is also asked again after the long delay
    const bool waitsForNetwork = (failure == TileReply::NetworkFailure::Unreachable) && !_failures->isOnline();
    const std::chrono::milliseconds delay = waitsForNetwork ? kOfflineRetryDelays[retry] : kRetryDelays[retry];
    if (!_retryTask->schedule(delay, [this] { _retry(); })) {
        return false;
    }
    if (waitsForNetwork) {
        qCDebug(QGCGeoTiledMapReplyLog) << tileReply.errorString() << "- asking again once the system is online, or in"
                                        << delay;
        _backOnline = connect(_failures.get(), &GeoTileFailures::backOnline, this, &QGCGeoTiledMapReply::_retry,
                              Qt::SingleShotConnection);
    } else {
        qCDebug(QGCGeoTiledMapReplyLog) << tileReply.errorString() << "- asking again in" << delay;
    }
    return true;
}

void QGCGeoTiledMapReply::_retry()
{
    // The first of the delay and the system coming back online asks again: not the other one too
    _retryTask->cancel();
    (void) disconnect(_backOnline);
    // Also for an aborted reply, which is finished
    if (isFinished()) {
        return;
    }
    _requestTile();
}

void QGCGeoTiledMapReply::_giveUp(const QString& errorString)
{
    const QGeoTileSpec failed = mapReplyAncestorSpec(tileSpec(), _ancestorLevels);
    // The replies that waited for the same fetch fail with it: one of them tells
    if (_failures->contains(failed)) {
        qCDebug(QGCGeoTiledMapReplyLog) << failed << "not fetched:" << errorString;
    } else {
        qCWarning(QGCGeoTiledMapReplyLog)
            << "tile" << failed.x() << failed.y() << "of zoom level" << failed.zoom() << "of map" << failed.mapId()
            << "not fetched, not asked for again for" << GeoTileFailures::kRememberedFor << ':' << errorString;
        _failures->insert(failed);
    }
    // No error of the reply: QtLocation asks five more times for a tile whose reply has one
    setFinished(true);
}
