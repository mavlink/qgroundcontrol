#include "QGCTileSetImporter.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QtEndian>
#include <QtCore/QtNumeric>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <cstring>
#include <functional>
#include <optional>
#include <set>
#include <vector>

#include "QGCCacheTile.h"
#include "QGCCompression.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCSqlHelper.h"
#include "QGCTileCacheDatabase.h"

QGC_LOGGING_CATEGORY(QGCTileSetImporterLog, "QtLocationPlugin.QGCTileSetImporter")

namespace QGCTileSetImporter {

namespace {

// Flush imported tiles to the cache in chunks; each chunk is one transaction with
// reused prepared queries (QGCTileCacheDatabase::saveTileBatch) instead of a
// per-tile transaction + redundant SELECT.
constexpr int kImportBatchSize = 4096;

// Decompressed-size caps for untrusted PMTiles blobs. Tile payloads are bounded
// generously (256 MiB) while directory blobs are tiny in practice (16 MiB cap).
constexpr qint64 kMaxTileDecompressedBytes = 256LL * 1024 * 1024;
constexpr qint64 kMaxDirDecompressedBytes = 16LL * 1024 * 1024;

// TMS (MBTiles) -> XYZ row flip. zoom in [0, 30]; 1 << zoom stays within int range.
int tmsRowToXyz(int zoom, int tmsRow)
{
    return ((1 << zoom) - 1) - tmsRow;
}

} // namespace

Result importMBTiles(QGCTileCacheDatabase &db, const QString &archivePath, const QString &setName,
                     const QString &providerType)
{
    Result result;

    if (!db.isValid()) {
        result.errorString = QStringLiteral("Tile cache database is not valid");
        return result;
    }

    // Read-only connection to the source archive; never mutated by the importer.
    QGCSqlHelper::ScopedConnection conn(archivePath, /*readOnly*/ true, QStringLiteral("QGCMBTiles"));
    if (!conn.isValid()) {
        result.errorString = QStringLiteral("Failed to open MBTiles archive: %1").arg(archivePath);
        return result;
    }

    QSqlDatabase srcDB = conn.database();

    // mb-util/tippecanoe emit a deduplicated schema (map + images) and may omit the
    // classic `tiles` view. Prefer `tiles` when it has rows, else fall back to the
    // map/images join. Both expose zoom_level/tile_column/tile_row/tile_data.
    bool useDedupSchema = false;
    {
        QSqlQuery probe(srcDB);
        const bool tilesHasRows = probe.exec(QStringLiteral("SELECT 1 FROM tiles LIMIT 1")) && probe.next();
        probe.finish();
        if (!tilesHasRows) {
            QSqlQuery mapProbe(srcDB);
            const bool mapHasRows = mapProbe.exec(QStringLiteral("SELECT 1 FROM map LIMIT 1")) && mapProbe.next();
            mapProbe.finish();
            if (!mapHasRows) {
                result.errorString = QStringLiteral("MBTiles archive has no tiles to import");
                return result;
            }
            useDedupSchema = true;
        }
    }

    const QString zoomSql = useDedupSchema
        ? QStringLiteral("SELECT MIN(zoom_level), MAX(zoom_level) FROM map")
        : QStringLiteral("SELECT MIN(zoom_level), MAX(zoom_level) FROM tiles");
    const QString tileSql = useDedupSchema
        ? QStringLiteral("SELECT m.zoom_level, m.tile_column, m.tile_row, i.tile_data "
                         "FROM map m JOIN images i ON m.tile_id = i.tile_id")
        : QStringLiteral("SELECT zoom_level, tile_column, tile_row, tile_data FROM tiles");

    int minZoom = 0;
    int maxZoom = 0;
    {
        QSqlQuery zoomQuery(srcDB);
        if (!zoomQuery.exec(zoomSql) || !zoomQuery.next()) {
            result.errorString = QStringLiteral("Failed to read MBTiles zoom range: %1").arg(zoomQuery.lastError().text());
            return result;
        }
        minZoom = zoomQuery.value(0).toInt();
        maxZoom = zoomQuery.value(1).toInt();
    }

    const auto setID = db.createImportedTileSet(setName, providerType, minZoom, maxZoom);
    if (!setID.has_value()) {
        result.errorString = QStringLiteral("Failed to create imported tile set");
        return result;
    }

    QSqlQuery tileQuery(srcDB);
    tileQuery.setForwardOnly(true);
    if (!tileQuery.exec(tileSql)) {
        result.errorString = QStringLiteral("Failed to read MBTiles tiles: %1").arg(tileQuery.lastError().text());
        (void) db.deleteTileSet(setID.value());
        return result;
    }

    quint32 imported = 0;
    // Accumulate tiles and flush via saveTileBatch (one transaction, reused prepared
    // queries) every kImportBatchSize, instead of a per-tile transaction + SELECT.
    std::vector<QGCCacheTile> chunk;
    chunk.reserve(kImportBatchSize);
    const auto flush = [&]() -> bool {
        if (chunk.empty()) {
            return true;
        }
        QList<const QGCCacheTile *> ptrs;
        ptrs.reserve(static_cast<qsizetype>(chunk.size()));
        for (const QGCCacheTile &t : chunk) {
            ptrs.append(&t);
        }
        const bool ok = db.saveTileBatch(ptrs);
        chunk.clear();
        return ok;
    };

    while (tileQuery.next()) {
        const int zoom = tileQuery.value(0).toInt();
        const int column = tileQuery.value(1).toInt();
        const int tmsRow = tileQuery.value(2).toInt();
        const QByteArray tileData = tileQuery.value(3).toByteArray();
        if (tileData.isEmpty()) {
            continue;
        }

        const int xyzRow = tmsRowToXyz(zoom, tmsRow);
        // Reuse the cache's canonical hashing so imported tiles dedupe with cached ones.
        const QString hash = UrlFactory::getTileHash(providerType, column, xyzRow, zoom);
        const QString format = UrlFactory::getImageFormat(providerType, tileData);

        chunk.emplace_back(hash, tileData, format, providerType, setID.value());
        ++imported;
        if ((chunk.size() >= static_cast<size_t>(kImportBatchSize)) && !flush()) {
            qCWarning(QGCTileSetImporterLog) << "Failed to import MBTiles tile batch";
            result.errorString = QStringLiteral("Failed to import MBTiles tile batch");
            (void) db.deleteTileSet(setID.value());
            return result;
        }
    }

    if (!flush()) {
        qCWarning(QGCTileSetImporterLog) << "Failed to import MBTiles tile batch";
        result.errorString = QStringLiteral("Failed to import MBTiles tile batch");
        (void) db.deleteTileSet(setID.value());
        return result;
    }

    // Reflect the imported count in the set row (createImportedTileSet seeds numTiles=0).
    {
        QSqlQuery update(db.database());
        if (update.prepare(QStringLiteral("UPDATE TileSets SET numTiles = ? WHERE setID = ?"))) {
            update.addBindValue(imported);
            update.addBindValue(setID.value());
            (void) update.exec();
        }
    }

    // Bulk import inflates the WAL; fold it back into the main DB and shrink the -wal.
    QSqlDatabase cacheDB = db.database();
    QGCSqlHelper::walCheckpointTruncate(cacheDB);

    qCDebug(QGCTileSetImporterLog) << "Imported" << imported << "tiles from" << archivePath << "into set" << setID.value();

    result.success = true;
    result.setID = setID.value();
    result.tileCount = imported;
    return result;
}

namespace {

// PMTiles v3 fixed header is exactly 127 bytes; magic is "PMTiles" + version byte.
constexpr int kPMTilesHeaderSize = 127;
constexpr char kPMTilesMagic[] = {'P', 'M', 'T', 'i', 'l', 'e', 's'};

// internal_compression / tile_compression codes (spec section: compression).
enum class PMCompression : quint8 { Unknown = 0, None = 1, Gzip = 2, Brotli = 3, Zstd = 4 };

// tile_type codes.
enum class PMTileType : quint8 { Unknown = 0, Mvt = 1, Png = 2, Jpeg = 3, Webp = 4, Avif = 5 };

struct PMHeader {
    quint64 rootDirOffset = 0;
    quint64 rootDirBytes = 0;
    quint64 jsonMetadataOffset = 0;
    quint64 jsonMetadataBytes = 0;
    quint64 leafDirsOffset = 0;
    quint64 leafDirsBytes = 0;
    quint64 tileDataOffset = 0;
    quint64 tileDataBytes = 0;
    quint64 addressedTilesCount = 0;
    PMCompression internalCompression = PMCompression::Unknown;
    PMCompression tileCompression = PMCompression::Unknown;
    PMTileType tileType = PMTileType::Unknown;
    quint8 minZoom = 0;
    quint8 maxZoom = 0;
};

struct PMEntry {
    quint64 tileId = 0;
    quint64 offset = 0;
    quint32 length = 0;
    quint32 runLength = 0;
};

QString compressionName(PMCompression c)
{
    switch (c) {
    case PMCompression::None: return QStringLiteral("None");
    case PMCompression::Gzip: return QStringLiteral("Gzip");
    case PMCompression::Brotli: return QStringLiteral("Brotli");
    case PMCompression::Zstd: return QStringLiteral("Zstd");
    default: return QStringLiteral("Unknown");
    }
}

// Decompress a PMTiles blob per its declared compression. None -> passthrough,
// Gzip -> libarchive inflate. Brotli/Zstd are unsupported and return nullopt
// (caller surfaces a codec-named error).
std::optional<QByteArray> pmDecompress(const QByteArray &data, PMCompression compression, qint64 maxBytes)
{
    switch (compression) {
    case PMCompression::None:
        // Even uncompressed sections must respect the cap on untrusted input.
        if (maxBytes > 0 && data.size() > maxBytes) {
            return std::nullopt;
        }
        return data;
    case PMCompression::Gzip: {
        const QByteArray out = QGCCompression::decompressData(data, QGCCompression::Format::GZIP, maxBytes);
        if (out.isEmpty() && !data.isEmpty()) {
            return std::nullopt;
        }
        return out;
    }
    default:
        return std::nullopt;
    }
}

bool parsePMHeader(const QByteArray &bytes, PMHeader &out, QString &error)
{
    if (bytes.size() < kPMTilesHeaderSize) {
        error = QStringLiteral("PMTiles header is truncated (%1 bytes)").arg(bytes.size());
        return false;
    }
    if (std::memcmp(bytes.constData(), kPMTilesMagic, sizeof(kPMTilesMagic)) != 0) {
        error = QStringLiteral("Not a PMTiles archive (bad magic)");
        return false;
    }
    const quint8 version = static_cast<quint8>(bytes.at(7));
    if (version != 3) {
        error = QStringLiteral("Unsupported PMTiles version %1 (only v3 is supported)").arg(version);
        return false;
    }

    const auto *p = reinterpret_cast<const uchar *>(bytes.constData());
    auto u64 = [p](int off) { return qFromLittleEndian<quint64>(p + off); };

    out.rootDirOffset = u64(8);
    out.rootDirBytes = u64(16);
    out.jsonMetadataOffset = u64(24);
    out.jsonMetadataBytes = u64(32);
    out.leafDirsOffset = u64(40);
    out.leafDirsBytes = u64(48);
    out.tileDataOffset = u64(56);
    out.tileDataBytes = u64(64);
    out.addressedTilesCount = u64(72);
    // u64 tile_entries_count(80), tile_contents_count(88), bool clustered(96).
    out.internalCompression = static_cast<PMCompression>(static_cast<quint8>(bytes.at(97)));
    out.tileCompression = static_cast<PMCompression>(static_cast<quint8>(bytes.at(98)));
    out.tileType = static_cast<PMTileType>(static_cast<quint8>(bytes.at(99)));
    out.minZoom = static_cast<quint8>(bytes.at(100));
    out.maxZoom = static_cast<quint8>(bytes.at(101));
    return true;
}

// Reads a base-128 varint. Returns false if the buffer is exhausted.
bool readVarint(const uchar *data, qint64 size, qint64 &pos, quint64 &value)
{
    value = 0;
    int shift = 0;
    while (pos < size && shift < 64) {
        const quint8 b = data[pos++];
        value |= static_cast<quint64>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            return true;
        }
        shift += 7;
    }
    return false;
}

// Deserialize a (decompressed) directory blob: delta-encoded tile_ids, then
// offsets (0 == consecutive after previous entry), lengths, run_lengths.
bool deserializeDirectory(const QByteArray &blob, std::vector<PMEntry> &entries, QString &error)
{
    const auto *data = reinterpret_cast<const uchar *>(blob.constData());
    const qint64 size = blob.size();
    qint64 pos = 0;

    quint64 numEntries = 0;
    if (!readVarint(data, size, pos, numEntries)) {
        error = QStringLiteral("Corrupt PMTiles directory (entry count)");
        return false;
    }
    // Each entry contributes at least one byte across its four varint streams
    // (tile_id, run_length, length, offset). A numEntries larger than the bytes
    // left in the blob is impossible — reject before allocating to bound memory.
    const quint64 remainingBytes = static_cast<quint64>(size - pos);
    if (numEntries > remainingBytes) {
        error = QStringLiteral("Corrupt PMTiles directory (entry count %1 exceeds %2 remaining bytes)")
                    .arg(numEntries)
                    .arg(remainingBytes);
        return false;
    }
    entries.assign(static_cast<size_t>(numEntries), PMEntry{});

    quint64 lastId = 0;
    for (quint64 i = 0; i < numEntries; ++i) {
        quint64 delta = 0;
        if (!readVarint(data, size, pos, delta)) {
            error = QStringLiteral("Corrupt PMTiles directory (tile id)");
            return false;
        }
        lastId += delta;
        entries[i].tileId = lastId;
    }
    for (quint64 i = 0; i < numEntries; ++i) {
        quint64 rl = 0;
        if (!readVarint(data, size, pos, rl)) {
            error = QStringLiteral("Corrupt PMTiles directory (run length)");
            return false;
        }
        entries[i].runLength = static_cast<quint32>(rl);
    }
    for (quint64 i = 0; i < numEntries; ++i) {
        quint64 len = 0;
        if (!readVarint(data, size, pos, len)) {
            error = QStringLiteral("Corrupt PMTiles directory (length)");
            return false;
        }
        entries[i].length = static_cast<quint32>(len);
    }
    for (quint64 i = 0; i < numEntries; ++i) {
        quint64 ofs = 0;
        if (!readVarint(data, size, pos, ofs)) {
            error = QStringLiteral("Corrupt PMTiles directory (offset)");
            return false;
        }
        if (ofs == 0 && i > 0) {
            entries[i].offset = entries[i - 1].offset + entries[i - 1].length;
        } else if (ofs == 0) {
            // First entry with delta 0 has no predecessor; offset is 0 (guard ofs-1 underflow).
            entries[i].offset = 0;
        } else {
            entries[i].offset = ofs - 1;
        }
    }
    return true;
}

std::optional<QByteArray> readSection(QFile &file, quint64 offset, quint64 length)
{
    if (length == 0) {
        return QByteArray();
    }
    if (!file.seek(static_cast<qint64>(offset))) {
        return std::nullopt;
    }
    QByteArray out = file.read(static_cast<qint64>(length));
    if (static_cast<quint64>(out.size()) != length) {
        return std::nullopt;
    }
    return out;
}

// Derive the cache image format string from tile_type, falling back to magic-byte
// sniffing for the generic/unknown case.
QString formatForTileType(PMTileType type, const QString &providerType, const QByteArray &tileData)
{
    switch (type) {
    case PMTileType::Png: return QStringLiteral("png");
    case PMTileType::Jpeg: return QStringLiteral("jpg");
    case PMTileType::Webp: return QStringLiteral("webp");
    case PMTileType::Avif: return QStringLiteral("avif");
    default: return UrlFactory::getImageFormat(providerType, tileData);
    }
}

} // namespace

quint64 zxyToTileId(quint8 z, quint32 x, quint32 y)
{
    // Number of tiles in all levels below z: sum(4^i) for i in [0, z) == (4^z - 1) / 3.
    quint64 acc = 0;
    for (quint8 i = 0; i < z; ++i) {
        acc += (1ULL << (2 * i));
    }

    // Hilbert d2xy inverse (xy2d) over an n = 2^z grid.
    quint64 n = 1ULL << z;
    quint64 rx = 0;
    quint64 ry = 0;
    quint64 d = 0;
    quint64 tx = x;
    quint64 ty = y;
    for (quint64 s = n / 2; s > 0; s /= 2) {
        rx = (tx & s) > 0 ? 1 : 0;
        ry = (ty & s) > 0 ? 1 : 0;
        d += s * s * ((3 * rx) ^ ry);
        // rotate
        if (ry == 0) {
            if (rx == 1) {
                tx = s - 1 - tx;
                ty = s - 1 - ty;
            }
            std::swap(tx, ty);
        }
    }
    return acc + d;
}

Result importPMTiles(QGCTileCacheDatabase &db, const QString &archivePath, const QString &setName,
                     const QString &providerType)
{
    Result result;

    if (!db.isValid()) {
        result.errorString = QStringLiteral("Tile cache database is not valid");
        return result;
    }

    QFile file(archivePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorString = QStringLiteral("Failed to open PMTiles archive: %1").arg(archivePath);
        return result;
    }

    const QByteArray headerBytes = file.read(kPMTilesHeaderSize);
    PMHeader header;
    if (!parsePMHeader(headerBytes, header, result.errorString)) {
        return result;
    }

    if (header.tileType == PMTileType::Mvt) {
        result.errorString = QStringLiteral("PMTiles vector (MVT) archives are not supported; raster tiles only");
        return result;
    }
    if (header.tileCompression == PMCompression::Brotli || header.tileCompression == PMCompression::Zstd) {
        result.errorString = QStringLiteral("Unsupported PMTiles tile compression: %1")
                                 .arg(compressionName(header.tileCompression));
        return result;
    }
    if (header.internalCompression == PMCompression::Brotli || header.internalCompression == PMCompression::Zstd) {
        result.errorString = QStringLiteral("Unsupported PMTiles directory compression: %1")
                                 .arg(compressionName(header.internalCompression));
        return result;
    }

    const auto rootRaw = readSection(file, header.rootDirOffset, header.rootDirBytes);
    if (!rootRaw) {
        result.errorString = QStringLiteral("Failed to read PMTiles root directory");
        return result;
    }
    const auto rootBlob = pmDecompress(*rootRaw, header.internalCompression, kMaxDirDecompressedBytes);
    if (!rootBlob) {
        result.errorString = QStringLiteral("Failed to decompress PMTiles root directory (%1)")
                                 .arg(compressionName(header.internalCompression));
        return result;
    }

    std::vector<PMEntry> rootEntries;
    if (!deserializeDirectory(*rootBlob, rootEntries, result.errorString)) {
        return result;
    }

    const auto setID = db.createImportedTileSet(setName, providerType, header.minZoom, header.maxZoom);
    if (!setID.has_value()) {
        result.errorString = QStringLiteral("Failed to create imported tile set");
        return result;
    }

    quint32 imported = 0;
    bool failed = false;

    // Tile reads must stay inside the declared tile-data region; leaf reads inside the
    // leaf-dirs region. Both checks reject wraps and out-of-region [offset, offset+len).
    const auto inRegion = [](quint64 base, quint64 regionLen, quint64 relOffset, quint32 length) -> bool {
        if (relOffset > regionLen) {
            return false;
        }
        const quint64 end = relOffset + length;
        if (end < relOffset || end > regionLen) {  // wrap or past region end
            return false;
        }
        quint64 absEnd = 0;
        if (qAddOverflow(base, relOffset, &absEnd)) {  // absolute start wraps
            return false;
        }
        quint64 dummy = 0;
        return !qAddOverflow(absEnd, static_cast<quint64>(length), &dummy);  // absolute end wraps
    };

    // Accumulated import buffer flushed in transactions of kImportBatchSize tiles via
    // saveTileBatch (one transaction, reused prepared queries) instead of per-tile.
    std::vector<QGCCacheTile> chunk;
    chunk.reserve(kImportBatchSize);
    const auto flush = [&]() -> bool {
        if (chunk.empty()) {
            return true;
        }
        QList<const QGCCacheTile *> ptrs;
        ptrs.reserve(static_cast<qsizetype>(chunk.size()));
        for (const QGCCacheTile &t : chunk) {
            ptrs.append(&t);
        }
        const bool ok = db.saveTileBatch(ptrs);
        chunk.clear();
        if (!ok) {
            result.errorString = QStringLiteral("Failed to import PMTiles tile batch");
        }
        return ok;
    };

    // Read + decompress + queue one tile entry. PMTiles uses XYZ addressing (no TMS flip).
    const auto saveEntry = [&](quint8 z, quint32 x, quint32 y, const PMEntry &entry) -> bool {
        if (!inRegion(header.tileDataOffset, header.tileDataBytes, entry.offset, entry.length)) {
            result.errorString = QStringLiteral("PMTiles tile read outside tile-data region");
            return false;
        }
        const auto tileRaw = readSection(file, header.tileDataOffset + entry.offset, entry.length);
        if (!tileRaw) {
            result.errorString = QStringLiteral("Failed to read PMTiles tile data");
            return false;
        }
        const auto tileBlob = pmDecompress(*tileRaw, header.tileCompression, kMaxTileDecompressedBytes);
        if (!tileBlob) {
            result.errorString = QStringLiteral("Unsupported PMTiles tile compression: %1")
                                     .arg(compressionName(header.tileCompression));
            return false;
        }
        if (tileBlob->isEmpty()) {
            return true;
        }
        const QString hash = UrlFactory::getTileHash(providerType, static_cast<int>(x), static_cast<int>(y),
                                                     static_cast<int>(z));
        const QString format = formatForTileType(header.tileType, providerType, *tileBlob);
        chunk.emplace_back(hash, *tileBlob, format, providerType, setID.value());
        ++imported;
        if (chunk.size() >= static_cast<size_t>(kImportBatchSize)) {
            return flush();
        }
        return true;
    };

    // Expand a tile id into z/x/y. Inverse of zxyToTileId over the run. Returns false on
    // a zoom that would overflow the 1<<(2*zoom) shift (UB) — i.e. a corrupt tile id.
    const auto tileIdToZxy = [](quint64 tileId, quint8 &z, quint32 &x, quint32 &y) -> bool {
        quint64 acc = 0;
        quint8 zoom = 0;
        while (true) {
            if (zoom > 31) {  // 1ULL << (2*32) is UB; PMTiles zoom never reaches here
                return false;
            }
            const quint64 levelTiles = 1ULL << (2 * zoom);
            if (acc + levelTiles > tileId) {
                break;
            }
            acc += levelTiles;
            ++zoom;
        }
        quint64 pos = tileId - acc;
        quint64 n = 1ULL << zoom;
        quint64 tx = 0;
        quint64 ty = 0;
        for (quint64 s = 1; s < n; s *= 2) {
            quint64 rx = 1 & (pos / 2);
            quint64 ry = 1 & (pos ^ rx);
            if (ry == 0) {
                if (rx == 1) {
                    tx = s - 1 - tx;
                    ty = s - 1 - ty;
                }
                std::swap(tx, ty);
            }
            tx += s * rx;
            ty += s * ry;
            pos /= 4;
        }
        z = zoom;
        x = static_cast<quint32>(tx);
        y = static_cast<quint32>(ty);
        return true;
    };

    // Bound total run-length expansion against the header's addressed-tile count so a
    // crafted run_length can't fan out into billions of saveEntry calls (0 == unknown).
    const quint64 maxExpanded = header.addressedTilesCount > 0 ? header.addressedTilesCount : UINT64_MAX;
    quint64 expanded = 0;

    // Walk a directory; recurse into leaf dirs (run_length == 0). Depth is capped and a
    // visited-offset set breaks cyclic leaf pointers (PMTiles trees are shallow).
    constexpr int kMaxLeafDepth = 4;
    std::set<quint64> visitedLeaves;
    std::function<bool(const std::vector<PMEntry> &, int)> walk =
        [&](const std::vector<PMEntry> &entries, int depth) -> bool {
        for (const PMEntry &entry : entries) {
            if (entry.runLength == 0) {
                if (depth >= kMaxLeafDepth) {
                    result.errorString = QStringLiteral("PMTiles leaf directory nesting too deep");
                    return false;
                }
                if (!visitedLeaves.insert(entry.offset).second) {
                    result.errorString = QStringLiteral("PMTiles leaf directory cycle detected");
                    return false;
                }
                if (!inRegion(header.leafDirsOffset, header.leafDirsBytes, entry.offset, entry.length)) {
                    result.errorString = QStringLiteral("PMTiles leaf read outside leaf-dirs region");
                    return false;
                }
                const auto leafRaw = readSection(file, header.leafDirsOffset + entry.offset, entry.length);
                if (!leafRaw) {
                    result.errorString = QStringLiteral("Failed to read PMTiles leaf directory");
                    return false;
                }
                const auto leafBlob = pmDecompress(*leafRaw, header.internalCompression, kMaxDirDecompressedBytes);
                if (!leafBlob) {
                    result.errorString = QStringLiteral("Failed to decompress PMTiles leaf directory (%1)")
                                             .arg(compressionName(header.internalCompression));
                    return false;
                }
                std::vector<PMEntry> leafEntries;
                if (!deserializeDirectory(*leafBlob, leafEntries, result.errorString)) {
                    return false;
                }
                if (!walk(leafEntries, depth + 1)) {
                    return false;
                }
                continue;
            }
            for (quint32 run = 0; run < entry.runLength; ++run) {
                if (++expanded > maxExpanded) {
                    result.errorString = QStringLiteral("PMTiles run-length expansion exceeds addressed-tile count");
                    return false;
                }
                quint8 z = 0;
                quint32 x = 0;
                quint32 y = 0;
                if (!tileIdToZxy(entry.tileId + run, z, x, y)) {
                    result.errorString = QStringLiteral("Corrupt PMTiles tile id");
                    return false;
                }
                if (!saveEntry(z, x, y, entry)) {
                    return false;
                }
            }
        }
        return true;
    };

    failed = !walk(rootEntries, 0) || !flush();
    if (failed) {
        qCWarning(QGCTileSetImporterLog) << "PMTiles import failed:" << result.errorString;
        (void) db.deleteTileSet(setID.value());
        return result;
    }

    {
        QSqlQuery update(db.database());
        if (update.prepare(QStringLiteral("UPDATE TileSets SET numTiles = ? WHERE setID = ?"))) {
            update.addBindValue(imported);
            update.addBindValue(setID.value());
            (void) update.exec();
        }
    }

    // Bulk import inflates the WAL; fold it back into the main DB and shrink the -wal.
    QSqlDatabase cacheDB = db.database();
    QGCSqlHelper::walCheckpointTruncate(cacheDB);

    qCDebug(QGCTileSetImporterLog) << "Imported" << imported << "tiles from" << archivePath << "into set"
                                   << setID.value();

    result.success = true;
    result.setID = setID.value();
    result.tileCount = imported;
    return result;
}

Result import(QGCTileCacheDatabase &db, const QString &archivePath, const QString &setName, const QString &providerType)
{
    const QString suffix = QFileInfo(archivePath).suffix().toLower();
    if (suffix == QStringLiteral("mbtiles")) {
        return importMBTiles(db, archivePath, setName, providerType);
    }
    if (suffix == QStringLiteral("pmtiles")) {
        return importPMTiles(db, archivePath, setName, providerType);
    }

    Result result;
    result.errorString = QStringLiteral("Unsupported tile archive format: .%1").arg(suffix);
    return result;
}

} // namespace QGCTileSetImporter
