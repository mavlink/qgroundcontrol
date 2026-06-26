#include "QGeoFileTileCacheQGC.h"

#include <QtLocation/private/qgeotilespec_p.h>

#include "QGCLoggingCategory.h"
#include "QGCTileCache.h"

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "QtLocationPlugin.QGeoFileTileCacheQGC")

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(const QVariantMap& parameters, QObject* parent)
    : QAbstractGeoTileCache(parent)
{
    Q_UNUSED(parameters);
    qCDebug(QGeoFileTileCacheQGCLog) << this;

    // Init cache paths once before the cache worker thread starts.
    QGCTileCache::ensureInitialized();
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
    qCDebug(QGeoFileTileCacheQGCLog) << this;
}

QSharedPointer<QGeoTileTexture> QGeoFileTileCacheQGC::get(const QGeoTileSpec& spec)
{
    Q_UNUSED(spec);
    return QSharedPointer<QGeoTileTexture>();
}

void QGeoFileTileCacheQGC::insert(const QGeoTileSpec& spec, const QByteArray& bytes, const QString& format,
                                  QAbstractGeoTileCache::CacheAreas areas)
{
    Q_UNUSED(spec);
    Q_UNUSED(bytes);
    Q_UNUSED(format);
    Q_UNUSED(areas);
}
