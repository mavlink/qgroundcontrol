#include "UnitTestTileGenerator.h"

#include <limits>

#include <QtCore/QAtomicInt>
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtPositioning/QGeoCoordinate>

#include "BaseClasses/TerrainTest.h"
#include "CacheTasks.h"
#include "CacheTile.h"
#include "CacheWorker.h"
#include "CopernicusTileFormat.h"
#include "ElevationMapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCLoggingCategory.h"
#include "TileCache.h"
#include "TileMath.h"

QGC_LOGGING_CATEGORY(UnitTestTileGeneratorLog, "Test.UnitTestTileGenerator")

namespace {

// Set from the test thread, consumed on the cache worker thread
QAtomicInt s_forcedMissCount;

/// Resolves the provider from a tile hash without tripping MapProviderRegistry's
/// "not found" warnings on garbage hashes.
SharedMapProvider _providerFromHash(const QString& hash)
{
    // Hash format: "%010d%08d%08d%03d" (provider map id, x, y, zoom), followed by '-' and 8 hex digits for a
    // provider with a cache variant
    const bool hasCacheVariant = (hash.size() == 38) && (hash.at(29) == QLatin1Char('-'));
    if ((hash.size() != 29) && !hasCacheVariant) {
        return nullptr;
    }

    bool ok = false;
    const int providerId = hash.left(10).toInt(&ok);
    if (!ok) {
        return nullptr;
    }

    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        if (provider->mapId() == providerId) {
            return provider;
        }
    }

    return nullptr;
}

/// Builds a serialized terrain tile for the given Copernicus tile x/y using the
/// production serializer, sampling UnitTestTerrainData heights on the SRTM1 grid.
QByteArray _syntheticTerrainTileData(int x, int y)
{
    constexpr double tileSizeDeg = CopernicusTileFormat::kTileSizeDegrees;
    constexpr double spacingDeg = CopernicusTileFormat::kValueSpacingDegrees;

    // Declared bounds are padded by one arc-second on every side. Coordinate-to-tile
    // hashing (floor) and TerrainTile's cell indexing round independently, so an exact
    // tile-edge coordinate can otherwise index one cell outside the grid (seen with
    // (0,0) queries). With the padding, cell size stays exactly one arc-second:
    // (tileSize + 2*spacing) / gridSize == spacing.
    const int gridSize = qRound(tileSizeDeg / spacingDeg) + 2;  // 36 + 2 padding cells
    const double swLat = (y * tileSizeDeg) - 90.0 - spacingDeg;
    const double swLon = (x * tileSizeDeg) - 180.0 - spacingDeg;

    double minHeight = std::numeric_limits<double>::max();
    double maxHeight = std::numeric_limits<double>::lowest();
    double sumHeights = 0.;

    QJsonArray carpet;
    for (int latIdx = 0; latIdx < gridSize; latIdx++) {
        QJsonArray row;
        // Sample at cell centers: cell values are representative of the cell and
        // never land knife-edge on region boundaries (which lie on whole arc-seconds).
        const double lat = swLat + ((latIdx + 0.5) * spacingDeg);
        for (int lonIdx = 0; lonIdx < gridSize; lonIdx++) {
            const double lon = swLon + ((lonIdx + 0.5) * spacingDeg);
            const double height = UnitTestTerrainData::heightAt(QGeoCoordinate(lat, lon));
            minHeight = qMin(minHeight, height);
            maxHeight = qMax(maxHeight, height);
            sumHeights += height;
            (void) row.append(height);
        }
        (void) carpet.append(row);
    }

    const QJsonObject root{
        {"status", "success"},
        {"data",
         QJsonObject{
             {"bounds",
              QJsonObject{{"sw", QJsonArray{swLat, swLon}},
                          {"ne", QJsonArray{swLat + (gridSize * spacingDeg), swLon + (gridSize * spacingDeg)}}}},
             {"stats",
              QJsonObject{{"min", minHeight}, {"max", maxHeight}, {"avg", sumHeights / (gridSize * gridSize)}}},
             {"carpet", carpet},
         }},
    };

    return CopernicusTileFormat::serialize(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

/// Builds a terrarium-encoded PNG for the given slippy tile, sampling
/// UnitTestTerrainData heights at pixel centers (256x256, zooms as requested).
QByteArray _syntheticTerrariumTileData(int x, int y, int zoom)
{
    constexpr int kSize = 256;
    const QPointF minCorner = TileMath::tileMinCorner({x, y, zoom});
    const double span = TileMath::tileSpanAtZoom(zoom);

    QImage image(kSize, kSize, QImage::Format_RGB888);
    for (int row = 0; row < kSize; row++) {
        // Slippy row 0 is the tile's north edge; world y grows north
        const double worldY = minCorner.y() + span * (1.0 - ((row + 0.5) / kSize));
        uchar* line = image.scanLine(row);
        for (int col = 0; col < kSize; col++) {
            const double worldX = minCorner.x() + span * ((col + 0.5) / kSize);
            const double height = UnitTestTerrainData::heightAt(TileMath::worldToGeo(QPointF(worldX, worldY)));
            // Terrarium: height = (R*256 + G + B/256) - 32768, i.e. 1/256 m quanta,
            // clamped to the 24-bit representable range
            const qint64 q = qBound(Q_INT64_C(0), qRound64((height + 32768.0) * 256.0), Q_INT64_C(0xFFFFFF));
            line[col * 3] = static_cast<uchar>((q >> 16) & 0xFF);
            line[(col * 3) + 1] = static_cast<uchar>((q >> 8) & 0xFF);
            line[(col * 3) + 2] = static_cast<uchar>(q & 0xFF);
        }
    }

    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        qFatal("UnitTestTileGenerator: failed to encode synthetic terrarium tile PNG");
    }
    return data;
}

}  // namespace

namespace UnitTestTileGenerator {

CacheTile* generateTile(const QString& hash)
{
    if (s_forcedMissCount.loadAcquire() > 0) {
        s_forcedMissCount.deref();  // simulated cache miss: the fetch task takes its error path
        return nullptr;
    }

    const SharedMapProvider provider = _providerFromHash(hash);
    if (!provider) {
        return nullptr;
    }
    const int mapId = provider->mapId();

    if (provider->isElevationProvider()) {
        // The provider prefix already validated as numeric, so unparsable x/y can only
        // mean corrupt hash construction upstream — warn so strict-mode tests fail.
        bool okX = false;
        bool okY = false;
        bool okZoom = false;
        const int x = hash.mid(10, 8).toInt(&okX);
        const int y = hash.mid(18, 8).toInt(&okY);
        const int zoom = hash.mid(26, 3).toInt(&okZoom);
        if (!okX || !okY || !okZoom) {
            qCWarning(UnitTestTileGeneratorLog) << "Internal error: unparsable x/y/zoom in tile hash" << hash;
            return nullptr;
        }
        if (mapId == TerrariumElevationProvider::kMapId) {
            return new CacheTile(hash, _syntheticTerrariumTileData(x, y, zoom), QStringLiteral("png"), mapId);
        }
        return new CacheTile(hash, _syntheticTerrainTileData(x, y), QStringLiteral("bin"), mapId);
    }

    static const QByteArray placeholderImage = []() {
        QFile file(QStringLiteral(":/res/notile.png"));
        if (!file.open(QFile::ReadOnly)) {
            qFatal("UnitTestTileGenerator: failed to open placeholder tile %s: %s", qPrintable(file.fileName()),
                   qPrintable(file.errorString()));
        }
        return file.readAll();
    }();
    return new CacheTile(hash, placeholderImage, QStringLiteral("png"), mapId);
}

void setForcedMissCount(int count)
{
    s_forcedMissCount.storeRelease(count);
}

QByteArray syntheticTerrariumTileData(int x, int y, int zoom)
{
    return _syntheticTerrariumTileData(x, y, zoom);
}

QString mapProviderType(int index)
{
    const QStringList elevationTypes = MapProviderRegistry::elevationProviderTypes();
    for (const QString& type : MapProviderRegistry::providerTypes()) {
        if (elevationTypes.contains(type)) {
            continue;
        }
        if (index == 0) {
            return type;
        }
        index--;
    }
    return QString();
}

bool resetTileCache()
{
    std::optional<bool> reset;
    // Declared after the flag: a signal still queued when this function returns is dropped with its context
    const QObject context;
    CacheTask* const task = CacheTasks::resetCache();
    (void) QObject::connect(task, &CacheTask::finished, &context,
                            [&reset](const QString& error) { reset = error.isEmpty(); });
    if (!TileCache::instance()->addTask(task)) {
        return false;
    }
    return QTest::qWaitFor([&reset] { return reset.has_value(); }, TestTimeout::mediumMs()) && *reset;
}

bool waitForCacheTasks()
{
    bool done = false;
    // Declared after the flag: a signal still queued when this function returns is dropped with its context
    const QObject context;
    // A task that does nothing: the worker ends it once it has run the tasks queued before it
    CacheTask* const task = new CacheTask(QStringLiteral("Wait"), [](CacheWorker&, CacheTask&) { return QString(); });
    (void) QObject::connect(task, &CacheTask::finished, &context, [&done] { done = true; });
    if (!TileCache::instance()->addTask(task)) {
        return false;
    }
    return QTest::qWaitFor([&done] { return done; }, TestTimeout::mediumMs());
}

void install()
{
    CacheWorker::setUnitTestTileGenerator(&generateTile);
}

void initTileCache()
{
    // The tile cache (and its worker) is normally initialized when the first QML
    // map is created. Tests query terrain without a map, so initialize it here with
    // a fresh, per-process temp database: every fetch misses and is served by the
    // generator. QTemporaryDir gives each parallel ctest process its own database
    // and cleans up at exit.
    static QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        qFatal("UnitTestTileGenerator: failed to create temp dir for tile cache db: %s",
               qPrintable(tempDir.errorString()));
    }
    TileCache::instance()->init(tempDir.filePath(QStringLiteral("qgc_unit_test_tile_cache.db")));
}

void shutdownTileCache()
{
    TileCache::instance()->shutdown();
}

}  // namespace UnitTestTileGenerator
