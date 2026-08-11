#include "UnitTestTileGenerator.h"

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
#include <limits>

#include "BaseClasses/TerrainTest.h"
#include "ElevationMapProvider.h"
#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCacheWorker.h"
#include "TerrainTileCopernicus.h"
#include "TileMath.h"

QGC_LOGGING_CATEGORY(UnitTestTileGeneratorLog, "Test.UnitTestTileGenerator")

namespace {

// Set from the test thread, consumed on the cache worker thread
QAtomicInt s_forcedMissCount;

/// Resolves the provider type from a tile hash without tripping UrlFactory's
/// "not found" warnings on garbage hashes.
QString _providerTypeFromHash(const QString& hash)
{
    // Hash format: "%010d%08d%08d%03d" (provider map id, x, y, zoom)
    if (hash.size() != 29) {
        return QString();
    }

    bool ok = false;
    const int providerId = hash.left(10).toInt(&ok);
    if (!ok) {
        return QString();
    }

    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (UrlFactory::getQtMapIdFromProviderType(type) == providerId) {
            return type;
        }
    }

    return QString();
}

/// Builds a serialized terrain tile for the given Copernicus tile x/y using the
/// production serializer, sampling UnitTestTerrainData heights on the SRTM1 grid.
QByteArray _syntheticTerrainTileData(int x, int y)
{
    constexpr double tileSizeDeg = TerrainTileCopernicus::kTileSizeDegrees;
    constexpr double spacingDeg = TerrainTileCopernicus::kTileValueSpacingDegrees;

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

    return TerrainTileCopernicus::serializeFromData(QJsonDocument(root).toJson(QJsonDocument::Compact));
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

QGCCacheTile* generateTile(const QString& hash)
{
    if (s_forcedMissCount.loadAcquire() > 0) {
        s_forcedMissCount.deref();  // simulated cache miss: the fetch task takes its error path
        return nullptr;
    }

    const QString type = _providerTypeFromHash(hash);
    if (type.isEmpty()) {
        return nullptr;
    }

    if (UrlFactory::getElevationProviderTypes().contains(type)) {
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
        if (type == QLatin1String(TerrariumElevationProvider::kProviderKey)) {
            return new QGCCacheTile(hash, _syntheticTerrariumTileData(x, y, zoom), QStringLiteral("png"), type);
        }
        return new QGCCacheTile(hash, _syntheticTerrainTileData(x, y), QStringLiteral("bin"), type);
    }

    static const QByteArray placeholderImage = []() {
        QFile file(QStringLiteral(":/res/notile.png"));
        if (!file.open(QFile::ReadOnly)) {
            qFatal("UnitTestTileGenerator: failed to open placeholder tile %s: %s", qPrintable(file.fileName()),
                   qPrintable(file.errorString()));
        }
        return file.readAll();
    }();
    return new QGCCacheTile(hash, placeholderImage, QStringLiteral("png"), type);
}

void setForcedMissCount(int count)
{
    s_forcedMissCount.storeRelease(count);
}

QByteArray syntheticTerrariumTileData(int x, int y, int zoom)
{
    return _syntheticTerrariumTileData(x, y, zoom);
}

void install()
{
    QGCCacheWorker::setUnitTestTileGenerator(&generateTile);
}

void initMapEngine()
{
    // The map engine (tile cache worker) is normally initialized when the first QML
    // map is created. Tests query terrain without a map, so initialize it here with
    // a fresh, per-process temp database: every fetch misses and is served by the
    // generator. QTemporaryDir gives each parallel ctest process its own database
    // and cleans up at exit.
    static QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        qFatal("UnitTestTileGenerator: failed to create temp dir for tile cache db: %s",
               qPrintable(tempDir.errorString()));
    }
    getQGCMapEngine()->init(tempDir.filePath(QStringLiteral("qgc_unit_test_tile_cache.db")));
}

void shutdownMapEngine()
{
    getQGCMapEngine()->shutdown();
}

}  // namespace UnitTestTileGenerator
