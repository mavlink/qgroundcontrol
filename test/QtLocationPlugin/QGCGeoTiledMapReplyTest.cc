#include "QGCGeoTiledMapReplyTest.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QStringList>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtTest/QSignalSpy>

#include "ManualScheduler.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCGeoFileTileCache.h"
#include "QGCGeoTiledMapReply.h"
#include "QtLocationMapId.h"
#include "TileCache.h"
#include "TileReplyNetworkMock.h"
#include "TileServerPauses.h"
#include "UnitTestTileGenerator.h"

namespace {

/// A tile of the plugin's first map type. Under unit tests a cache miss is served by UnitTestTileGenerator unless a
/// miss is forced.
QGeoTileSpec mapReplyTileSpec(int x)
{
    return QGeoTileSpec(QStringLiteral("QGroundControl"), 1, 13, x, 2867);
}

constexpr int kMapReplyZoom = 13;
constexpr int kMapReplyY = 2867;
constexpr int kMapReplyEdgePixels = 64;

/// A tile of OpenStreetMap, whose source is fixed: its server answers 404 where it has no tile. A tile that a test
/// fetches stays in the tile cache: the tiles of each test are too far apart to have a tile above in common.
QGeoTileSpec mapReplyStreetMapSpec(int x)
{
    const int mapId =
        QtLocationMapId::fromProviderId(MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Street Map")));
    return QGeoTileSpec(QStringLiteral("QGroundControl"), mapId, kMapReplyZoom, x, kMapReplyY);
}

/// The tile \a levels zoom levels above mapReplyStreetMapSpec(x), as MapReplyTileServer names it
QString mapReplyTileAbove(int x, int levels)
{
    return QStringLiteral("%1/%2/%3").arg(kMapReplyZoom - levels).arg(x >> levels).arg(kMapReplyY >> levels);
}

/// The server of OpenStreetMap: it has the tiles it was given, and answers 404 for any other. It also stands in for
/// the servers of the other providers.
class MapReplyTileServer : public QNetworkAccessManager
{
public:
    struct Answer
    {
        QNetworkReply::NetworkError error = QNetworkReply::NoError;
        int httpStatus = 0;
        QByteArray body;
        QByteArray retryAfter;  ///< The Retry-After header, when it is not empty
    };

    QHash<QString, QByteArray> tiles;       ///< By "zoom/x/y"
    QHash<QString, QList<Answer>> answers;  ///< By "zoom/x/y": the answers to its next requests, before its tile
    QString heldTile;                       ///< Its reply stays open
    QPointer<TileReplyCannedReply> heldReply;
    QByteArray otherProviderTile;           ///< The answer to every request for a tile of another provider
    QStringList requested;                  ///< "zoom/x/y", or the host of another provider's server

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        if (request.url().host() != QStringLiteral("tile.openstreetmap.org")) {
            requested.append(request.url().host());
            return new TileReplyCannedReply(request, otherProviderTile, QNetworkReply::NoError, 200, false, this);
        }

        static const QRegularExpression tilePath(QStringLiteral("^/(\\d+/\\d+/\\d+)\\.png$"));
        const QString tile = tilePath.match(request.url().path()).captured(1);
        requested.append(tile);

        const auto answer = answers.find(tile);
        if ((answer != answers.end()) && !answer->isEmpty()) {
            const Answer next = answer->takeFirst();
            auto* const reply = new TileReplyCannedReply(request, next.body, next.error, next.httpStatus, false, this);
            if (!next.retryAfter.isEmpty()) {
                reply->setHeader(QHttpHeaders::WellKnownHeader::RetryAfter, next.retryAfter);
            }
            return reply;
        }
        const auto found = tiles.constFind(tile);
        if (found == tiles.cend()) {
            return new TileReplyCannedReply(request, QByteArray(), QNetworkReply::ContentNotFoundError, 404, false,
                                            this);
        }
        const bool hold = tile == heldTile;
        auto* const reply = new TileReplyCannedReply(request, *found, QNetworkReply::NoError, 200, hold, this);
        if (hold) {
            heldReply = reply;
        }
        return reply;
    }
};

/// The answer of a server that was asked too often, which asks to wait \a retryAfter when that is given
MapReplyTileServer::Answer mapReplyTooManyRequests(const QByteArray& retryAfter = QByteArray())
{
    return {QNetworkReply::UnknownContentError, 429, QByteArray(), retryAfter};
}

/// The one line about a pause of the server of OpenStreetMap
QRegularExpression mapReplyPauseLogged(std::chrono::seconds pause)
{
    return QRegularExpression(QStringLiteral("OpenStreetMap.*asked too often.* for %1s(\\W|$)").arg(pause.count()));
}

/// The answer of a server that is unavailable, which asks to wait \a retryAfter when that is given
MapReplyTileServer::Answer mapReplyUnavailable(const QByteArray& retryAfter = QByteArray())
{
    return {QNetworkReply::ServiceUnavailableError, 503, QByteArray(), retryAfter};
}

/// The one line about a pause of the server of OpenStreetMap, which said it is unavailable
QRegularExpression mapReplyUnavailableLogged(std::chrono::seconds pause)
{
    return QRegularExpression(
        QStringLiteral("OpenStreetMap.* is unavailable: none of its tiles .* for %1s(\\W|$)").arg(pause.count()));
}

QByteArray mapReplyPng(const QImage& image)
{
    QByteArray data;
    QBuffer buffer(&data);
    return (buffer.open(QIODevice::WriteOnly) && image.save(&buffer, "PNG")) ? data : QByteArray();
}

/// Far from the colours of the cells beside it
QColor mapReplyCellColor(int column, int row, bool transparent)
{
    return QColor(32 + (column * 67) % 192, 32 + (row * 67) % 192, 128, transparent ? 128 : 255);
}

/// The edge of mapReplyCellsTile(): that of the other tiles, or a pixel for each of its cells
int mapReplyCellsTileEdgePixels(int cellsPerEdge)
{
    return qMax(kMapReplyEdgePixels, cellsPerEdge);
}

/// A tile of \a cellsPerEdge squared cells, each of its own colour
QByteArray mapReplyCellsTile(int cellsPerEdge, bool transparent)
{
    const int edgePixels = mapReplyCellsTileEdgePixels(cellsPerEdge);
    QImage image(edgePixels, edgePixels, transparent ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    const int cellPixels = edgePixels / cellsPerEdge;
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (int row = 0; row < cellsPerEdge; row++) {
        for (int column = 0; column < cellsPerEdge; column++) {
            painter.fillRect(column * cellPixels, row * cellPixels, cellPixels, cellPixels,
                             mapReplyCellColor(column, row, transparent));
        }
    }
    painter.end();
    return mapReplyPng(image);
}

/// A scaled tile of a transparent tile above keeps a colour only nearly: its pixels are premultiplied by their alpha
bool mapReplyColorsMatch(const QColor& actual, const QColor& expected)
{
    constexpr int kTolerance = 3;
    return (qAbs(actual.red() - expected.red()) <= kTolerance) &&
           (qAbs(actual.green() - expected.green()) <= kTolerance) &&
           (qAbs(actual.blue() - expected.blue()) <= kTolerance) &&
           (qAbs(actual.alpha() - expected.alpha()) <= kTolerance);
}

/// What tells that the centre of \a tile is not of the colour of a cell of mapReplyCellsTile(): empty when it is
QString mapReplyCellMismatch(const QImage& tile, int column, int row, bool transparent)
{
    const QColor center = tile.pixelColor(tile.width() / 2, tile.height() / 2);
    const QColor cell = mapReplyCellColor(column, row, transparent);
    return mapReplyColorsMatch(center, cell)
               ? QString()
               : QStringLiteral("%1 is not %2").arg(center.name(QColor::HexArgb), cell.name(QColor::HexArgb));
}

/// QtLocation's cache of the tiles of the maps, which the replies give the tiles above
class MapReplyTileCache : public QGCGeoFileTileCache
{
public:
    MapReplyTileCache() { init(); }

    /// The picture that a map is shown for \a spec: null for none
    QImage picture(const QGeoTileSpec& spec)
    {
        const QSharedPointer<QGeoTileTexture> texture = get(spec);
        return (texture && (texture->spec == spec)) ? texture->image : QImage();
    }
};

}  // namespace

void QGCGeoTiledMapReplyTest::_testTileDelivered()
{
    QGCGeoTiledMapReply mapReply(mapReplyTileSpec(4400));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!mapReply.mapImageData().isEmpty());
    QCOMPARE(mapReply.mapImageFormat(), QStringLiteral("png"));
    QVERIFY(mapReply.isCached());
}

void QGCGeoTiledMapReplyTest::_testFailureThatCanClearUpIsRetried_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<int>("delayMs");

    const int retryDelay = static_cast<int>(QGCGeoTiledMapReply::kRetryDelays.front().count());
    QTest::newRow("server busy") << 5425 << QNetworkReply::ServiceUnavailableError << 503 << retryDelay;
    QTest::newRow("server not reached") << 5553 << QNetworkReply::HostNotFoundError << 0 << retryDelay;
}

void QGCGeoTiledMapReplyTest::_testFailureThatCanClearUpIsRetried()
{
    QFETCH(int, x);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(int, delayMs);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // The server fails once, and has the tile afterwards
    const QString tile = mapReplyTileAbove(x, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(tile, {{networkError, httpStatus, QByteArray(), QByteArray()}});
    server.tiles.insert(tile, tileReplyPngBody());
    ManualScheduler scheduler;
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(x), nullptr, std::make_shared<GeoTileFailures>(&scheduler));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());

    // Not asked again before the delay, and without an end of the reply
    const std::chrono::milliseconds delay(delayMs);
    QVERIFY(scheduler.advanceBy(delay - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 1);
    QVERIFY(finishedSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!mapReply.mapImageData().isEmpty());
    QCOMPARE(server.requested, QStringList({tile, tile}));
}

void QGCGeoTiledMapReplyTest::_testFailureThatCannotClearUpIsRemembered_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("logged");

    // As for a key of the provider that is not valid
    QTest::newRow("refused by the server")
        << 5809 << QNetworkReply::ContentAccessDenied << 403 << QByteArray() << QStringLiteral("http 403");
    QTest::newRow("not a tile") << 5937 << QNetworkReply::NoError << 200
                                << QByteArrayLiteral("<html><body>Sign in to this network</body></html>")
                                << QStringLiteral("not a picture");
}

void QGCGeoTiledMapReplyTest::_testFailureThatCannotClearUpIsRemembered()
{
    QFETCH(int, x);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(QByteArray, body);
    QFETCH(QString, logged);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // The server fails once, and has the tile afterwards
    const QString tile = mapReplyTileAbove(x, 0);
    const QGeoTileSpec spec = mapReplyStreetMapSpec(x);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(tile, {{networkError, httpStatus, body, QByteArray()}});
    server.tiles.insert(tile, tileReplyPngBody());
    ManualScheduler scheduler;
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);

    QGCGeoTiledMapReply failed(spec, nullptr, failures);
    QSignalSpy failedSpy(&failed, &QGeoTiledMapReply::finished);
    expectLogMessage("QtLocationPlugin.QGCGeoTiledMapReply", QtWarningMsg, QRegularExpression(logged));
    failed.init();
    QVERIFY_SIGNAL_COUNT_WAIT(failedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    // No error, for which QtLocation asks five more times
    QCOMPARE(failed.error(), QGeoTiledMapReply::NoError);
    QVERIFY(failed.mapImageData().isEmpty());
    QVERIFY(!failed.isNoTile());
    QCOMPARE(server.requested.size(), 1);
    QCOMPARE(scheduler.pendingCount(), 0);

    // A map asks for the tile again whenever its view changes: the server is not asked again for a while
    QVERIFY(scheduler.advanceBy(GeoTileFailures::kRememberedFor - std::chrono::milliseconds(1)));
    QGCGeoTiledMapReply again(spec, nullptr, failures);
    again.init();
    QVERIFY(again.isFinished());
    QCOMPARE(again.error(), QGeoTiledMapReply::NoError);
    QVERIFY(again.mapImageData().isEmpty());
    QCOMPARE(server.requested.size(), 1);

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QGCGeoTiledMapReply later(spec, nullptr, failures);
    QSignalSpy laterSpy(&later, &QGeoTiledMapReply::finished);
    later.init();
    QVERIFY_SIGNAL_COUNT_WAIT(laterSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(later.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!later.mapImageData().isEmpty());
    QCOMPARE(server.requested.size(), 2);
}

void QGCGeoTiledMapReplyTest::_testAbortFinishesOnce()
{
    QGCGeoTiledMapReply aborted(mapReplyTileSpec(4402));
    QSignalSpy abortedSpy(&aborted, &QGeoTiledMapReply::finished);
    aborted.init();
    aborted.abort();
    QCOMPARE(abortedSpy.count(), 1);

    // The cache worker runs lookups in order: once this one is answered, the aborted reply's tile has arrived too
    QGCGeoTiledMapReply sentinel(mapReplyTileSpec(4403));
    QSignalSpy sentinelSpy(&sentinel, &QGeoTiledMapReply::finished);
    sentinel.init();
    QVERIFY_SIGNAL_COUNT_WAIT(sentinelSpy, 1, TestTimeout::mediumMs());

    QCOMPARE(abortedSpy.count(), 1);
    QVERIFY(aborted.mapImageData().isEmpty());
}

void QGCGeoTiledMapReplyTest::_testTileAboveShownWhereProviderHasNone_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("levels");
    QTest::addColumn<bool>("transparent");

    QTest::newRow("one zoom level above") << 4401 << 1 << false;
    QTest::newRow("two zoom levels above") << 4529 << 2 << false;
    // The zoom level that a map zooms in to, 23, over a provider's deepest tiles of the zoom level 15
    QTest::newRow("eight zoom levels above") << 4657 << 8 << false;
    QTest::newRow("transparent tile") << 4785 << 1 << true;
}

void QGCGeoTiledMapReplyTest::_testTileAboveShownWhereProviderHasNone()
{
    QFETCH(int, x);
    QFETCH(int, levels);
    QFETCH(bool, transparent);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // The server has no tile nearer than the one that shows the ground of 2^levels squared tiles of the zoom level
    const int cellsPerEdge = 1 << levels;
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.tiles.insert(mapReplyTileAbove(x, levels), mapReplyCellsTile(cellsPerEdge, transparent));
    MapReplyTileCache tileCache;
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(x), nullptr, nullptr, nullptr, &tileCache);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!mapReply.isNoTile());
    QStringList expectedRequests;
    for (int level = 0; level <= levels; level++) {
        expectedRequests.append(mapReplyTileAbove(x, level));
    }
    QCOMPARE(server.requested, expectedRequests);

    // The tile cache shows the cell of its ground, as large as a tile: the reply has no bytes to store
    QVERIFY(mapReply.mapImageData().isEmpty());
    const QImage tile = tileCache.picture(mapReplyStreetMapSpec(x));
    const int edgePixels = mapReplyCellsTileEdgePixels(cellsPerEdge);
    QCOMPARE(tile.size(), QSize(edgePixels, edgePixels));
    QCOMPARE(mapReplyCellMismatch(tile, x & (cellsPerEdge - 1), kMapReplyY & (cellsPerEdge - 1), transparent),
             QString());
}

void QGCGeoTiledMapReplyTest::_testTilesFromTheSameTileAboveMeet()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // Two tiles side by side, whose nearest tile above is black over the left one and white over the right one
    constexpr int kLeftX = 4913;
    constexpr int kLevels = 5;
    constexpr int kPartPixels = kMapReplyEdgePixels >> kLevels;
    QImage above(kMapReplyEdgePixels, kMapReplyEdgePixels, QImage::Format_RGB32);
    above.fill(Qt::black);
    const int rightPartLeft = ((kLeftX + 1) & ((1 << kLevels) - 1)) * kPartPixels;
    QPainter painter(&above);
    painter.fillRect(rightPartLeft, 0, kMapReplyEdgePixels - rightPartLeft, kMapReplyEdgePixels, Qt::white);
    painter.end();
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.tiles.insert(mapReplyTileAbove(kLeftX, kLevels), mapReplyPng(above));

    MapReplyTileCache tileCache;
    QGCGeoTiledMapReply left(mapReplyStreetMapSpec(kLeftX), nullptr, nullptr, nullptr, &tileCache);
    QGCGeoTiledMapReply right(mapReplyStreetMapSpec(kLeftX + 1), nullptr, nullptr, nullptr, &tileCache);
    QSignalSpy leftSpy(&left, &QGeoTiledMapReply::finished);
    QSignalSpy rightSpy(&right, &QGeoTiledMapReply::finished);
    left.init();
    right.init();
    QVERIFY_SIGNAL_COUNT_WAIT(leftSpy, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(rightSpy, 1, TestTimeout::mediumMs());
    const QImage leftTile = tileCache.picture(mapReplyStreetMapSpec(kLeftX));
    const QImage rightTile = tileCache.picture(mapReplyStreetMapSpec(kLeftX + 1));
    QCOMPARE(leftTile.size(), above.size());
    QCOMPARE(rightTile.size(), above.size());

    // Each tile is scaled with the pixels beside its part: black fades to white across the edge that they share
    constexpr int kRow = kMapReplyEdgePixels / 2;
    const int leftEdge = qGray(leftTile.pixel(kMapReplyEdgePixels - 1, kRow));
    const int rightEdge = qGray(rightTile.pixel(0, kRow));
    QCOMPARE_LT(qGray(leftTile.pixel(0, kRow)), 16);
    QCOMPARE_GT(qGray(rightTile.pixel(kMapReplyEdgePixels - 1, kRow)), 239);
    QCOMPARE_GT(leftEdge, 64);
    QCOMPARE_LT(rightEdge, 192);
    QCOMPARE_LT(qAbs(rightEdge - leftEdge), 32);
}

void QGCGeoTiledMapReplyTest::_testTileBelowDeepestZoomNotAskedFor()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // A tile two zoom levels below the deepest ones of OpenStreetMap, whose server answers a request for it with an
    // error
    const SharedMapProvider provider = MapProviderRegistry::findProvider(QStringLiteral("Street Map"));
    QVERIFY(provider);
    const int deepestZoom = provider->maxZoom();
    QCOMPARE_LT(deepestZoom, QGC_MAX_MAP_ZOOM);
    constexpr int kLevels = 2;
    constexpr int kCellsPerEdge = 1 << kLevels;
    constexpr int kX = 1000001;
    constexpr int kY = 700002;
    const QGeoTileSpec spec(QStringLiteral("QGroundControl"), QtLocationMapId::fromProviderId(provider->mapId()),
                            deepestZoom + kLevels, kX, kY);
    const QString deepest = QStringLiteral("%1/%2/%3").arg(deepestZoom).arg(kX >> kLevels).arg(kY >> kLevels);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.tiles.insert(deepest, mapReplyCellsTile(kCellsPerEdge, false));
    MapReplyTileCache tileCache;
    QGCGeoTiledMapReply mapReply(spec, nullptr, nullptr, nullptr, &tileCache);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    // The server is asked for the deepest tile above only, which is scaled up
    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QCOMPARE(server.requested, QStringList{deepest});
    const QImage tile = tileCache.picture(spec);
    QCOMPARE(tile.size(), QSize(kMapReplyEdgePixels, kMapReplyEdgePixels));
    QCOMPARE(mapReplyCellMismatch(tile, kX & (kCellsPerEdge - 1), kY & (kCellsPerEdge - 1), false), QString());
}

void QGCGeoTiledMapReplyTest::_testRepliesShareRequestForTileAbove()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // Two tiles that the provider lacks, under one tile above, which the server is slow to send
    constexpr int kX = 7216;
    const QString above = mapReplyTileAbove(kX, 1);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.heldTile = above;
    server.tiles.insert(above, tileReplyPngBody());
    const std::shared_ptr<GeoNoTiles> noTiles = std::make_shared<GeoNoTiles>();
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    MapReplyTileCache tileCache;
    QGCGeoTiledMapReply first(mapReplyStreetMapSpec(kX), noTiles, nullptr, requests, &tileCache);
    QGCGeoTiledMapReply second(mapReplyStreetMapSpec(kX + 1), noTiles, nullptr, requests, &tileCache);
    QSignalSpy firstSpy(&first, &QGeoTiledMapReply::finished);
    QSignalSpy secondSpy(&second, &QGeoTiledMapReply::finished);

    // The first reply asks for the tile above, and the second one then waits for it too
    first.init();
    QTRY_VERIFY_WITH_TIMEOUT(!server.heldReply.isNull(), TestTimeout::mediumMs());
    second.init();
    QTRY_VERIFY_WITH_TIMEOUT(noTiles->contains(mapReplyStreetMapSpec(kX + 1)), TestTimeout::mediumMs());
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.count(above), 1);

    // The request is not that of the first reply: it goes on without it, and is not made again
    first.abort();
    QCOMPARE(firstSpy.count(), 1);
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(!server.heldReply.isNull());
    QCOMPARE(server.requested.count(above), 1);
    server.heldReply->finishNow();
    QVERIFY_SIGNAL_COUNT_WAIT(secondSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(second.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!tileCache.picture(mapReplyStreetMapSpec(kX + 1)).isNull());
    QCOMPARE(server.requested.count(above), 1);
    QCOMPARE(firstSpy.count(), 1);
    QVERIFY(tileCache.picture(mapReplyStreetMapSpec(kX)).isNull());
}

void QGCGeoTiledMapReplyTest::_testTileAboveDecodedOnce()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // A tile that the provider lacks is scaled up from the tile above
    constexpr int kX = 7472;
    constexpr int kCellsPerEdge = 2;
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.tiles.insert(mapReplyTileAbove(kX, 1), mapReplyCellsTile(kCellsPerEdge, false));
    const std::shared_ptr<GeoNoTiles> noTiles = std::make_shared<GeoNoTiles>();
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    MapReplyTileCache tileCache;
    QGCGeoTiledMapReply first(mapReplyStreetMapSpec(kX), noTiles, nullptr, requests, &tileCache);
    QSignalSpy firstSpy(&first, &QGeoTiledMapReply::finished);
    first.init();
    QVERIFY_SIGNAL_COUNT_WAIT(firstSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!tileCache.picture(mapReplyStreetMapSpec(kX)).isNull());
    QCOMPARE(server.requested.size(), 2);

    // The tile beside it, which the provider is known to lack, is scaled up from the picture that the first reply
    // had decoded: the reply finishes at once, without asking the cache or the server for the tile above
    noTiles->insert(mapReplyStreetMapSpec(kX + 1));
    QGCGeoTiledMapReply beside(mapReplyStreetMapSpec(kX + 1), noTiles, nullptr, requests, &tileCache);
    beside.init();
    QVERIFY(beside.isFinished());
    QCOMPARE(beside.error(), QGeoTiledMapReply::NoError);
    const QImage tile = tileCache.picture(mapReplyStreetMapSpec(kX + 1));
    QCOMPARE(tile.size(), QSize(kMapReplyEdgePixels, kMapReplyEdgePixels));
    QCOMPARE(mapReplyCellMismatch(tile, (kX + 1) & (kCellsPerEdge - 1), kMapReplyY & (kCellsPerEdge - 1), false),
             QString());
    QCOMPARE(server.requested.size(), 2);
}

// A tile above that is no picture to read has nothing to show in the place of the tile: the reply ends without a
// tile, and the tile above is not asked for again for a while
void QGCGeoTiledMapReplyTest::_testTileAboveThatCannotBeReadIsGivenUp()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // The signature of a PNG, and nothing of a picture after it
    constexpr int kX = 7728;
    const QString above = mapReplyTileAbove(kX, 1);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.tiles.insert(above, QByteArray::fromHex("89504E470D0A1A0A") + QByteArray(64, 'x'));
    const std::shared_ptr<GeoNoTiles> noTiles = std::make_shared<GeoNoTiles>();
    ManualScheduler scheduler;
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    MapReplyTileCache tileCache;
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX), noTiles, failures, nullptr, &tileCache);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    expectLogMessage("QtLocationPlugin.QGCGeoTiledMapReply", QtWarningMsg,
                     QRegularExpression(QStringLiteral("not fetched.*cannot be scaled up")));
    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    // No error, for which QtLocation asks five more times
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(mapReply.mapImageData().isEmpty());
    QVERIFY(!mapReply.isNoTile());
    QVERIFY(tileCache.picture(mapReplyStreetMapSpec(kX)).isNull());
    QCOMPARE(server.requested, QStringList({mapReplyTileAbove(kX, 0), above}));

    // The tile beside it has the same tile above
    QGCGeoTiledMapReply beside(mapReplyStreetMapSpec(kX + 1), noTiles, failures, nullptr, &tileCache);
    QSignalSpy besideSpy(&beside, &QGeoTiledMapReply::finished);
    beside.init();
    QVERIFY_SIGNAL_COUNT_WAIT(besideSpy, 1, TestTimeout::mediumMs());
    QVERIFY(beside.mapImageData().isEmpty());
    QCOMPARE(server.requested, QStringList({mapReplyTileAbove(kX, 0), above, mapReplyTileAbove(kX + 1, 0)}));
}

void QGCGeoTiledMapReplyTest::_testNoTileAboveWithinLimit()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // The nearest tile above is one zoom level too far to be shown
    constexpr int kX = 5041;
    constexpr int kAskedLevels = 1 + QGCGeoTiledMapReply::kMaxAncestorLevels;
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.tiles.insert(mapReplyTileAbove(kX, kAskedLevels), tileReplyPngBody());
    const std::shared_ptr<GeoNoTiles> noTiles = std::make_shared<GeoNoTiles>();
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX), noTiles);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(mapReply.isNoTile());
    QVERIFY(mapReply.mapImageData().isEmpty());
    QCOMPARE(server.requested.size(), kAskedLevels);

    // A map asks for the tile again whenever its view changes: the server is asked for none of the tiles again
    QGCGeoTiledMapReply again(mapReplyStreetMapSpec(kX), noTiles);
    again.init();
    QVERIFY(again.isFinished());
    QVERIFY(again.isNoTile());
    QCOMPARE(server.requested.size(), kAskedLevels);

    // The tile beside it has the same tiles above
    QGCGeoTiledMapReply beside(mapReplyStreetMapSpec(kX - 1), noTiles);
    QSignalSpy besideSpy(&beside, &QGeoTiledMapReply::finished);
    beside.init();
    QVERIFY_SIGNAL_COUNT_WAIT(besideSpy, 1, TestTimeout::mediumMs());
    QVERIFY(beside.isNoTile());
    QCOMPARE(server.requested.size(), kAskedLevels + 1);
    QCOMPARE(server.requested.constLast(), mapReplyTileAbove(kX - 1, 0));
}

void QGCGeoTiledMapReplyTest::_testRetriesOfTileAboveAreBounded()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    // The provider has no tile here, and the server of the tile above does not answer until it is too late
    constexpr int kX = 5169;
    constexpr qsizetype kAboveRequests = 1 + static_cast<qsizetype>(QGCGeoTiledMapReply::kRetryDelays.size());
    const QString above = mapReplyTileAbove(kX, 1);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(
        above, QList<MapReplyTileServer::Answer>(kAboveRequests, {QNetworkReply::HostNotFoundError, 0, {}, {}}));
    server.tiles.insert(above, tileReplyPngBody());
    ManualScheduler scheduler;
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX), nullptr, std::make_shared<GeoTileFailures>(&scheduler));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    // One line for the tile, not one for each request
    expectLogMessage("QtLocationPlugin.QGCGeoTiledMapReply", QtWarningMsg,
                     QRegularExpression(QStringLiteral("not fetched.*network error")));
    mapReply.init();
    for (const std::chrono::milliseconds delay : QGCGeoTiledMapReply::kRetryDelays) {
        QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
        QVERIFY(finishedSpy.isEmpty());
        QVERIFY(scheduler.advanceBy(delay));
    }
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // No error, for which QtLocation asks five more times
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(mapReply.mapImageData().isEmpty());
    QVERIFY(!mapReply.isNoTile());
    QCOMPARE(server.requested.size(), 1 + kAboveRequests);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void QGCGeoTiledMapReplyTest::_testOfflineReplyWaitsForNetwork()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kX = 6065;
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.heldTile = mapReplyTileAbove(kX, 0);
    server.tiles.insert(server.heldTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    failures->setOnline(false);
    const QGeoTileSpec failedBefore = mapReplyStreetMapSpec(kX + 1);
    failures->insert(failedBefore);
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX), nullptr, failures);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QTRY_VERIFY_WITH_TIMEOUT(!server.heldReply.isNull(), TestTimeout::mediumMs());
    server.heldReply->failNow(QNetworkReply::HostNotFoundError);

    // Not asked again while the system is offline, up to the long delay
    QVERIFY(scheduler.advanceBy(QGCGeoTiledMapReply::kOfflineRetryDelays.front() - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 1);
    QVERIFY(finishedSpy.isEmpty());

    // Back online, the server is asked again, also for a tile that failed before
    failures->setOnline(true);
    QVERIFY(!failures->contains(failedBefore));
    QTRY_COMPARE_WITH_TIMEOUT(server.requested.size(), 2, TestTimeout::mediumMs());
    server.heldReply->finishNow();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!mapReply.mapImageData().isEmpty());
}

void QGCGeoTiledMapReplyTest::_testOfflineReplyAsksAgainOnce_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<bool>("onlineFirst");

    QTest::newRow("back online before the delay") << 6705 << true;
    // The system can wrongly report being offline, or reach the tile server on its own network: the server is asked
    // again after a long delay all the same
    QTest::newRow("delay before back online") << 6833 << false;
}

void QGCGeoTiledMapReplyTest::_testOfflineReplyAsksAgainOnce()
{
    QFETCH(int, x);
    QFETCH(bool, onlineFirst);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.heldTile = mapReplyTileAbove(x, 0);
    server.tiles.insert(server.heldTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    failures->setOnline(false);
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(x), nullptr, failures);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QTRY_VERIFY_WITH_TIMEOUT(!server.heldReply.isNull(), TestTimeout::mediumMs());
    server.heldReply->failNow(QNetworkReply::HostNotFoundError);

    const auto comeOnline = [&failures] { failures->setOnline(true); };
    const auto passDelay = [&scheduler] {
        return scheduler.advanceBy(QGCGeoTiledMapReply::kOfflineRetryDelays.front());
    };
    if (onlineFirst) {
        comeOnline();
    } else {
        QVERIFY(passDelay());
    }
    QTRY_COMPARE_WITH_TIMEOUT(server.requested.size(), 2, TestTimeout::mediumMs());

    // While that request is open, the other one asks nothing more. A second request for the tile would share the
    // fetch of the first, so the server would not see it, but the reply would end twice, the first time without a tile.
    if (onlineFirst) {
        QVERIFY(passDelay());
    } else {
        comeOnline();
    }
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 2);

    server.heldReply->finishNow();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!mapReply.mapImageData().isEmpty());
}

void QGCGeoTiledMapReplyTest::_testAbortWhileWaitingToRetry_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<bool>("online");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");

    QTest::newRow("waiting for the delay") << 6193 << true << QNetworkReply::ServiceUnavailableError << 503;
    QTest::newRow("waiting for the system to be online") << 6321 << false << QNetworkReply::HostNotFoundError << 0;
}

void QGCGeoTiledMapReplyTest::_testAbortWhileWaitingToRetry()
{
    QFETCH(int, x);
    QFETCH(bool, online);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.heldTile = mapReplyTileAbove(x, 0);
    server.tiles.insert(server.heldTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    failures->setOnline(online);
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(x), nullptr, failures);
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QTRY_VERIFY_WITH_TIMEOUT(!server.heldReply.isNull(), TestTimeout::mediumMs());
    server.heldReply->failNow(networkError, httpStatus);
    QVERIFY(finishedSpy.isEmpty());

    mapReply.abort();
    QCOMPARE(finishedSpy.count(), 1);

    // Neither the delay nor the system coming online makes the aborted reply ask again, or finish again
    failures->setOnline(true);
    QVERIFY(scheduler.advanceBy(std::chrono::hours(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(mapReply.mapImageData().isEmpty());
}

void QGCGeoTiledMapReplyTest::_testAbortStopsRequestForTileAbove()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kX = 5297;
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.heldTile = mapReplyTileAbove(kX, 1);
    server.tiles.insert(server.heldTile, tileReplyPngBody());
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);
    mapReply.init();
    QTRY_VERIFY_WITH_TIMEOUT(!server.heldReply.isNull(), TestTimeout::mediumMs());

    mapReply.abort();
    QCOMPARE(finishedSpy.count(), 1);

    // The request for the tile above is dropped, not left to finish the reply a second time
    QTRY_VERIFY_WITH_TIMEOUT(server.heldReply.isNull(), TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(mapReply.mapImageData().isEmpty());
}

// A map asks for dozens of tiles at once: a server that was asked too often is not asked for any of them
void QGCGeoTiledMapReplyTest::_testRateLimitPausesProvider()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 8192;
    constexpr int kLaterX = 8200;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    const QString laterTile = mapReplyTileAbove(kLaterX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(laterTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply later(mapReplyStreetMapSpec(kLaterX), nullptr, failures, requests);
    QSignalSpy refusedSpy(&refused, &QGeoTiledMapReply::finished);
    QSignalSpy laterSpy(&later, &QGeoTiledMapReply::finished);

    // One line for the server, not one for each tile
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // Also a reply that comes during the pause waits for its end
    later.init();
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested, QStringList({refusedTile}));
    QVERIFY(refusedSpy.isEmpty());
    QVERIFY(laterSpy.isEmpty());

    // They ask one after the other
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(refusedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QVERIFY_SIGNAL_COUNT_WAIT(laterSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!refused.mapImageData().isEmpty());
    QVERIFY(!later.mapImageData().isEmpty());
    QCOMPARE(server.requested.count(refusedTile), 2);
    QCOMPARE(server.requested.count(laterTile), 1);
}

void QGCGeoTiledMapReplyTest::_testRetryAfterSetsPause_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<QByteArray>("retryAfter");
    QTest::addColumn<int>("pauseSeconds");

    QTest::newRow("as long as asked") << 8448 << QByteArrayLiteral("90") << 90;
    QTest::newRow("longer than the longest")
        << 8704 << QByteArrayLiteral("86400") << static_cast<int>(TileServerPauses::kMaxPause.count());
    QTest::newRow("shorter than the shortest")
        << 8960 << QByteArrayLiteral("0") << static_cast<int>(TileServerPauses::kMinPause.count());
}

void QGCGeoTiledMapReplyTest::_testRetryAfterSetsPause()
{
    QFETCH(int, x);
    QFETCH(QByteArray, retryAfter);
    QFETCH(int, pauseSeconds);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    const QString tile = mapReplyTileAbove(x, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(tile, {mapReplyTooManyRequests(retryAfter)});
    server.tiles.insert(tile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(x), nullptr, std::make_shared<GeoTileFailures>(&scheduler));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    const std::chrono::seconds pause(pauseSeconds);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(pause));
    mapReply.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(scheduler.advanceBy(pause - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 1);
    QVERIFY(finishedSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!mapReply.mapImageData().isEmpty());
    QCOMPARE(server.requested.size(), 2);
}

// A server that answers 429 again after its pause gets a longer one, up to the longest, and a reply that it keeps
// refusing gives up as after the other failures that can clear up
void QGCGeoTiledMapReplyTest::_testRepeatedRateLimitIsBounded()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kX = 9216;
    const QString tile = mapReplyTileAbove(kX, 0);
    // Twice as long, the first pause is longer than the longest
    const std::chrono::seconds firstPause = TileServerPauses::kMaxPause * 2 / 3;
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(tile, {mapReplyTooManyRequests(QByteArray::number(firstPause.count())),
                                 mapReplyTooManyRequests(), mapReplyTooManyRequests(), mapReplyTooManyRequests()});
    server.tiles.insert(tile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX), nullptr, std::make_shared<GeoTileFailures>(&scheduler));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(firstPause));
    mapReply.init();
    const QList<std::chrono::seconds> pauses = {firstPause, TileServerPauses::kMaxPause, TileServerPauses::kMaxPause};
    for (qsizetype i = 0; i < pauses.size(); i++) {
        QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        QVERIFY(scheduler.advanceBy(pauses.at(i) - std::chrono::milliseconds(1)));
        QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
        QCOMPARE(server.requested.size(), i + 1);
        QVERIFY(finishedSpy.isEmpty());

        expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kMaxPause));
        if (i == pauses.size() - 1) {
            expectLogMessage("QtLocationPlugin.QGCGeoTiledMapReply", QtWarningMsg,
                             QRegularExpression(QStringLiteral("not fetched.*http 429")));
        }
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    }
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    // No error, for which QtLocation asks five more times
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(mapReply.mapImageData().isEmpty());
    QVERIFY(!mapReply.isNoTile());
    QCOMPARE(server.requested.size(), 1 + static_cast<qsizetype>(QGCGeoTiledMapReply::kRetryDelays.size()));
}

void QGCGeoTiledMapReplyTest::_testOtherProviderAskedDuringPause()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kX = 9472;
    const QString refusedTile = mapReplyTileAbove(kX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.otherProviderTile = tileReplyPngBody();
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kX), nullptr, failures, requests);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    const QGeoTileSpec otherSpec = mapReplyTileSpec(kX + 8);
    QVERIFY(otherSpec.mapId() != mapReplyStreetMapSpec(kX).mapId());
    QGCGeoTiledMapReply other(otherSpec, nullptr, failures, requests);
    QSignalSpy otherSpy(&other, &QGeoTiledMapReply::finished);
    other.init();
    QVERIFY_SIGNAL_COUNT_WAIT(otherSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(other.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!other.mapImageData().isEmpty());
    QCOMPARE(server.requested.size(), 2);
    QCOMPARE(server.requested.count(refusedTile), 1);
}

// Only asking the server waits
void QGCGeoTiledMapReplyTest::_testCachedTileServedDuringPause()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 9728;
    constexpr int kCachedX = 9736;
    const QString streetMap = QStringLiteral("Street Map");
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(streetMap);
    QVERIFY(provider);
    const QByteArray cachedTile = tileReplyPngBody(Qt::darkBlue);
    // The cache worker runs its tasks in order: the tile is stored before a reply asks for it
    TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(kCachedX, kMapReplyY, kMapReplyZoom),
                                     cachedTile, QStringLiteral("png"));

    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QGCGeoTiledMapReply cached(mapReplyStreetMapSpec(kCachedX), nullptr, failures, requests);
    QSignalSpy cachedSpy(&cached, &QGeoTiledMapReply::finished);
    cached.init();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(cached.mapImageData(), cachedTile);
    QVERIFY(cached.isCached());
    QCOMPARE(server.requested.size(), 1);
}

void QGCGeoTiledMapReplyTest::_testAbortDuringPause()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 9984;
    constexpr int kLaterX = 9992;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(mapReplyTileAbove(kLaterX, 0), tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply later(mapReplyStreetMapSpec(kLaterX), nullptr, failures, requests);
    QSignalSpy refusedSpy(&refused, &QGeoTiledMapReply::finished);
    QSignalSpy laterSpy(&later, &QGeoTiledMapReply::finished);

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    later.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    // The reply that the server refused, and one that came during the pause
    refused.abort();
    later.abort();
    QCOMPARE(refusedSpy.count(), 1);
    QCOMPARE(laterSpy.count(), 1);

    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 1);
    QCOMPARE(refusedSpy.count(), 1);
    QCOMPARE(laterSpy.count(), 1);
    QVERIFY(refused.mapImageData().isEmpty());
    QVERIFY(later.mapImageData().isEmpty());
}

// The server that asked for quiet gets no burst of the tiles of a map view when the pause ends
void QGCGeoTiledMapReplyTest::_testHeldRequestsAskOneAfterAnother()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 10240;
    constexpr int kHeldCount = 4;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // Held in this order, after the retry of the refused reply
    QStringList expected = {refusedTile, refusedTile};
    std::vector<std::unique_ptr<QGCGeoTiledMapReply>> held;
    for (int i = 1; i <= kHeldCount; i++) {
        const int x = kRefusedX + 8 * i;
        expected.append(mapReplyTileAbove(x, 0));
        server.tiles.insert(expected.constLast(), tileReplyPngBody());
        held.push_back(std::make_unique<QGCGeoTiledMapReply>(mapReplyStreetMapSpec(x), nullptr, failures, requests));
        held.back()->init();
        QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    }
    QCOMPARE(server.requested.size(), 1);

    // The first asks when the pause ends, and each next one an interval after the one before
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QCOMPARE(server.requested, expected.first(2));
    for (qsizetype asked = 3; asked <= expected.size(); asked++) {
        QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval - std::chrono::milliseconds(1)));
        QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
        QCOMPARE(server.requested.size(), asked - 1);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(server.requested, expected.first(asked));
    }
    QCOMPARE(scheduler.pendingCount(), 0);

    QTRY_VERIFY_WITH_TIMEOUT(refused.isFinished() && std::all_of(held.cbegin(), held.cend(),
                                                                 [](const auto& reply) { return reply->isFinished(); }),
                             TestTimeout::mediumMs());
    QVERIFY(!refused.mapImageData().isEmpty());
    for (const auto& reply : held) {
        QCOMPARE(reply->error(), QGeoTiledMapReply::NoError);
        QVERIFY(!reply->mapImageData().isEmpty());
    }
}

void QGCGeoTiledMapReplyTest::_testRateLimitDuringReleaseHoldsTheRest()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 10496;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    const QString firstTile = mapReplyTileAbove(kRefusedX + 8, 0);
    const QString secondTile = mapReplyTileAbove(kRefusedX + 16, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.heldTile = refusedTile;
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(firstTile, tileReplyPngBody());
    server.tiles.insert(secondTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply first(mapReplyStreetMapSpec(kRefusedX + 8), nullptr, failures, requests);
    QGCGeoTiledMapReply second(mapReplyStreetMapSpec(kRefusedX + 16), nullptr, failures, requests);

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_VERIFY_WITH_TIMEOUT(!server.heldReply.isNull(), TestTimeout::mediumMs());
    server.heldReply->failNow(QNetworkReply::UnknownContentError, 429);
    verifyExpectedLogMessage();
    first.init();
    second.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    // The request that asks first when the pause ends is refused again: the next pause holds the rest
    const std::chrono::seconds secondPause = 2 * TileServerPauses::kDefaultPause;
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QCOMPARE(server.requested, QStringList({refusedTile, refusedTile}));
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(secondPause));
    server.heldReply->failNow(QNetworkReply::UnknownContentError, 429);
    verifyExpectedLogMessage();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    QVERIFY(scheduler.advanceBy(secondPause - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 2);

    // In the order they were held: the retry of the refused reply after them
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(server.requested.constLast(), firstTile);
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QCOMPARE(server.requested.constLast(), secondTile);
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QCOMPARE(server.requested.constLast(), refusedTile);
    QCOMPARE(server.requested.size(), 5);
    server.heldReply->finishNow();

    QTRY_VERIFY_WITH_TIMEOUT(refused.isFinished() && first.isFinished() && second.isFinished(),
                             TestTimeout::mediumMs());
    QVERIFY(!refused.mapImageData().isEmpty());
    QVERIFY(!first.mapImageData().isEmpty());
    QVERIFY(!second.mapImageData().isEmpty());
}

void QGCGeoTiledMapReplyTest::_testAbortedHeldRequestTakesNoTurn()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 10752;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    const QString askingTile = mapReplyTileAbove(kRefusedX + 16, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    for (const int offset : {8, 16, 24}) {
        server.tiles.insert(mapReplyTileAbove(kRefusedX + offset, 0), tileReplyPngBody());
    }
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply abortedFirst(mapReplyStreetMapSpec(kRefusedX + 8), nullptr, failures, requests);
    QGCGeoTiledMapReply asking(mapReplyStreetMapSpec(kRefusedX + 16), nullptr, failures, requests);
    QGCGeoTiledMapReply abortedLast(mapReplyStreetMapSpec(kRefusedX + 24), nullptr, failures, requests);
    QSignalSpy abortedFirstSpy(&abortedFirst, &QGeoTiledMapReply::finished);
    QSignalSpy abortedLastSpy(&abortedLast, &QGeoTiledMapReply::finished);

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    abortedFirst.init();
    asking.init();
    abortedLast.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    // Aborted while it waits for its turn: the next one takes that turn
    abortedFirst.abort();
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QCOMPARE(server.requested, QStringList({refusedTile, refusedTile, askingTile}));

    // The last held one aborted, nothing is left to time
    abortedLast.abort();
    QCOMPARE(scheduler.pendingCount(), 0);
    QVERIFY(scheduler.advanceBy(std::chrono::hours(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 3);
    QCOMPARE(abortedFirstSpy.count(), 1);
    QCOMPARE(abortedLastSpy.count(), 1);
    QVERIFY(abortedFirst.mapImageData().isEmpty());
    QVERIFY(abortedLast.mapImageData().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(refused.isFinished() && asking.isFinished(), TestTimeout::mediumMs());
}

void QGCGeoTiledMapReplyTest::_testRequestDuringReleaseWaitsItsTurn()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 11008;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    const QString heldTile = mapReplyTileAbove(kRefusedX + 8, 0);
    const QString lateTile = mapReplyTileAbove(kRefusedX + 16, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(heldTile, tileReplyPngBody());
    server.tiles.insert(lateTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply held(mapReplyStreetMapSpec(kRefusedX + 8), nullptr, failures, requests);
    QGCGeoTiledMapReply late(mapReplyStreetMapSpec(kRefusedX + 16), nullptr, failures, requests);

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    held.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QCOMPARE(server.requested, QStringList({refusedTile, refusedTile}));

    // The pause is over, but the held reply has not had its turn: the late one waits after it
    late.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 2);
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QCOMPARE(server.requested.constLast(), heldTile);
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 3);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(server.requested.constLast(), lateTile);

    QTRY_VERIFY_WITH_TIMEOUT(refused.isFinished() && held.isFinished() && late.isFinished(), TestTimeout::mediumMs());
    QVERIFY(!late.mapImageData().isEmpty());
}

// Only asking the server waits for a turn
void QGCGeoTiledMapReplyTest::_testCachedTileServedDuringRelease()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 11264;
    constexpr int kCachedX = kRefusedX + 16;
    const QString streetMap = QStringLiteral("Street Map");
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(streetMap);
    QVERIFY(provider);
    const QByteArray cachedTile = tileReplyPngBody(Qt::darkCyan);
    // The cache worker runs its tasks in order: the tile is stored before a reply asks for it
    TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(kCachedX, kMapReplyY, kMapReplyZoom),
                                     cachedTile, QStringLiteral("png"));

    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyTooManyRequests()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(mapReplyTileAbove(kRefusedX + 8, 0), tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply held(mapReplyStreetMapSpec(kRefusedX + 8), nullptr, failures, requests);
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    held.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));

    // While the held reply waits for its turn
    QGCGeoTiledMapReply cached(mapReplyStreetMapSpec(kCachedX), nullptr, failures, requests);
    QSignalSpy cachedSpy(&cached, &QGeoTiledMapReply::finished);
    cached.init();
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(cached.mapImageData(), cachedTile);
    QVERIFY(cached.isCached());
    QCOMPARE(server.requested, QStringList({refusedTile, refusedTile}));

    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QTRY_VERIFY_WITH_TIMEOUT(refused.isFinished() && held.isFinished(), TestTimeout::mediumMs());
}

// The server says how long it will be unavailable: no tile of it is asked for before then
void QGCGeoTiledMapReplyTest::_testUnavailableWithRetryAfterPausesProvider()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 11520;
    const std::chrono::seconds pause(120);
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    const QString laterTile = mapReplyTileAbove(kRefusedX + 8, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyUnavailable(QByteArray::number(pause.count()))});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(laterTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply later(mapReplyStreetMapSpec(kRefusedX + 8), nullptr, failures, requests);

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyUnavailableLogged(pause));
    refused.init();
    // The pause, and the retry of the refused reply
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    later.init();
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    // The retry of the refused reply is made after its usual delay, and waits for the pause too
    const std::chrono::milliseconds retryDelay = QGCGeoTiledMapReply::kRetryDelays.front();
    QVERIFY(scheduler.advanceBy(retryDelay));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(scheduler.advanceBy(pause - retryDelay - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested, QStringList({refusedTile}));
    QVERIFY(!refused.isFinished());
    QVERIFY(!later.isFinished());

    // In the order they were held
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(server.requested, QStringList({refusedTile, laterTile}));
    QVERIFY(scheduler.advanceBy(TileServerPauses::kReleaseInterval));
    QCOMPARE(server.requested, QStringList({refusedTile, laterTile, refusedTile}));
    QTRY_VERIFY_WITH_TIMEOUT(refused.isFinished() && later.isFinished(), TestTimeout::mediumMs());
    QVERIFY(!refused.mapImageData().isEmpty());
    QVERIFY(!later.mapImageData().isEmpty());
}

// Without the time, a server that is unavailable is asked again as after other failures that can clear up
void QGCGeoTiledMapReplyTest::_testUnavailableWithoutRetryAfterPausesNothing()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kRefusedX = 11776;
    const QString refusedTile = mapReplyTileAbove(kRefusedX, 0);
    const QString laterTile = mapReplyTileAbove(kRefusedX + 8, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(refusedTile, {mapReplyUnavailable()});
    server.tiles.insert(refusedTile, tileReplyPngBody());
    server.tiles.insert(laterTile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    const std::shared_ptr<GeoTileFailures> failures = std::make_shared<GeoTileFailures>(&scheduler);
    const std::shared_ptr<GeoTileRequests> requests = std::make_shared<GeoTileRequests>();
    QGCGeoTiledMapReply refused(mapReplyStreetMapSpec(kRefusedX), nullptr, failures, requests);
    QGCGeoTiledMapReply later(mapReplyStreetMapSpec(kRefusedX + 8), nullptr, failures, requests);
    QSignalSpy refusedSpy(&refused, &QGeoTiledMapReply::finished);
    QSignalSpy laterSpy(&later, &QGeoTiledMapReply::finished);

    // Only the retry of the refused reply is timed
    refused.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    later.init();
    QVERIFY_SIGNAL_COUNT_WAIT(laterSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!later.mapImageData().isEmpty());
    QCOMPARE(server.requested, QStringList({refusedTile, laterTile}));

    const std::chrono::milliseconds retryDelay = QGCGeoTiledMapReply::kRetryDelays.front();
    QVERIFY(scheduler.advanceBy(retryDelay - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 2);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(refusedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!refused.mapImageData().isEmpty());
    QCOMPARE(server.requested, QStringList({refusedTile, laterTile, refusedTile}));
}

// A pause for a 503 lasts as long as the server asked, after a pause for a 429 too, and the next pause for a 429 is not
// longer for either
void QGCGeoTiledMapReplyTest::_testUnavailablePauseLastsAsAsked()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    constexpr int kX = 12032;
    const std::chrono::seconds unavailablePause = TileServerPauses::kDefaultPause * 3 / 2;
    const QString tile = mapReplyTileAbove(kX, 0);
    MapReplyTileServer server;
    const TileReplyNetworkScope networkScope(&server);
    server.answers.insert(tile,
                          {mapReplyTooManyRequests(), mapReplyUnavailable(QByteArray::number(unavailablePause.count())),
                           mapReplyTooManyRequests()});
    server.tiles.insert(tile, tileReplyPngBody());
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    QGCGeoTiledMapReply mapReply(mapReplyStreetMapSpec(kX), nullptr, std::make_shared<GeoTileFailures>(&scheduler));

    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    mapReply.init();
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // Not twice the pause before
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyUnavailableLogged(unavailablePause));
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    // The pause, and the retry of the reply
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    const std::chrono::milliseconds retryDelay = QGCGeoTiledMapReply::kRetryDelays.at(1);
    QVERIFY(scheduler.advanceBy(retryDelay));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QVERIFY(scheduler.advanceBy(unavailablePause - retryDelay - std::chrono::milliseconds(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(server.requested.size(), 2);

    // Not twice the pause for the 429 before, nor for the 503
    expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg, mapReplyPauseLogged(TileServerPauses::kDefaultPause));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(server.requested.size(), 3);
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);
    QVERIFY(scheduler.advanceBy(TileServerPauses::kDefaultPause));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!mapReply.mapImageData().isEmpty());
    QCOMPARE(server.requested.size(), 4);
}

UT_REGISTER_TEST(QGCGeoTiledMapReplyTest, TestLabel::Unit)
