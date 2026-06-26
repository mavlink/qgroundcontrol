#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

class QGCFetchTileTask;

// Static facade over the SQLite tile store (QGCTileCacheDatabase via the cache
// worker): cache paths, tile-save tasks and validator refresh. Independent of
// the QtLocation QAbstractGeoTileCache vtable (see QGeoFileTileCacheQGC).
namespace QGCTileCache {
// Initialize cache paths once. Must run before the cache worker thread starts.
void ensureInitialized();

quint32 getMaxDiskCacheSetting();

void cacheTile(const QString& type, int x, int y, int z, const QByteArray& image, const QString& format,
               qulonglong set = UINT64_MAX, const QByteArray& etag = QByteArray(),
               const QByteArray& lastModified = QByteArray(), qint64 expiresAt = 0, bool mustRevalidate = false);
void cacheTile(const QString& type, const QString& hash, const QByteArray& image, const QString& format,
               qulonglong set = UINT64_MAX, const QByteArray& etag = QByteArray(),
               const QByteArray& lastModified = QByteArray(), qint64 expiresAt = 0, bool mustRevalidate = false);

QGCFetchTileTask* createFetchTileTask(const QString& type, int x, int y, int z);

// Refresh cache-validation metadata for an already-cached tile (e.g. after a
// 304 Not Modified) without rewriting the blob.
void refreshTileValidators(const QString& type, const QString& hash, const QByteArray& etag,
                           const QByteArray& lastModified, qint64 expiresAt);

QString getDatabaseFilePath();
QString getCachePath();
}  // namespace QGCTileCache
