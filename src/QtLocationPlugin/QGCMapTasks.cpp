#include "QGCMapTasks.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QtGui/QImage>

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileFallback.h"

QGCCreateTileSetTask::~QGCCreateTileSetTask()
{
    if (!m_saved) {
        delete m_tileSet;
    }
}

QGCSaveTileTask::~QGCSaveTileTask()
{
    delete m_tile;
}

void QGCSaveTileTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    const QGCCacheTile* t = tile();
    if (!worker.database()->saveTile(t->hash, t->format, t->img, t->type, t->tileSet, t->etag, t->lastModified,
                                     t->expiresAt, t->mustRevalidate)) {
        setError("Error saving tile to cache");
    }
}

void QGCFetchTileTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    auto tile = worker.database()->getTile(hash());
    if (tile) {
        (void) worker.database()->bumpTileAccessed(hash());
        setTileFetched(QSharedPointer<QGCCacheTile>(tile.release()));
    } else {
        setError("Tile not in cache database");
    }
}

void QGCFetchFallbackTileTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    const QSize tileSizePx(tileSize(), tileSize());
    for (int d = 1; d <= maxLevelsUp(); ++d) {
        const int az = z() - d;
        if (az < 0) {
            break;
        }
        const int ax = x() >> d;
        const int ay = y() >> d;
        const QString hash = UrlFactory::getTileHash(providerName(), ax, ay, az);
        auto tile = worker.database()->getTile(hash);
        if (!tile) {
            continue;
        }

        QImage ancestor;
        if (!ancestor.loadFromData(tile->img)) {
            continue;
        }
        const QImage scaled = scaleAncestorToChild(ancestor, x(), y(), d, tileSizePx);
        if (scaled.isNull()) {
            continue;
        }
        const QByteArray bytes = encodeFallbackTile(scaled);
        if (bytes.isEmpty()) {
            continue;
        }
        (void) worker.database()->bumpTileAccessed(hash);
        setFallbackFetched(bytes, QStringLiteral("png"), d);
        return;
    }

    setError("No ancestor tile available for fallback");
}

void QGCFetchTileSetTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    const QList<TileSetRecord> records = worker.database()->getTileSets();
    for (const auto& rec : records) {
        QGCCachedTileSet* set = new QGCCachedTileSet(rec.name);
        set->blockSignals(true);

        set->setId(rec.setID);
        set->setMapTypeStr(rec.mapTypeStr);
        set->setTopleftLat(rec.topleftLat);
        set->setTopleftLon(rec.topleftLon);
        set->setBottomRightLat(rec.bottomRightLat);
        set->setBottomRightLon(rec.bottomRightLon);
        set->setMinZoom(rec.minZoom);
        set->setMaxZoom(rec.maxZoom);
        set->setType(rec.mapTypeStr);
        set->setTotalTileCount(rec.numTiles);
        set->setDefaultSet(rec.defaultSet);
        set->setCreationDate(QDateTime::fromSecsSinceEpoch(rec.date));

        const SetTotalsResult totals =
            worker.database()->computeSetTotals(rec.setID, rec.defaultSet, rec.numTiles, set->type());
        set->setSavedTileCount(totals.savedTileCount);
        set->setSavedTileSize(totals.savedTileSize);
        set->setTotalTileSize(totals.totalTileSize);
        set->setUniqueTileCount(totals.uniqueTileCount);
        set->setUniqueTileSize(totals.uniqueTileSize);

        set->blockSignals(false);
        (void) set->moveToThread(QCoreApplication::instance()->thread());
        setTileSetFetched(set);
    }
}

void QGCCreateTileSetTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    const auto setID = worker.database()->createTileSet(
        tileSet()->name(), tileSet()->mapTypeStr(), tileSet()->topleftLat(), tileSet()->topleftLon(),
        tileSet()->bottomRightLat(), tileSet()->bottomRightLon(), tileSet()->minZoom(), tileSet()->maxZoom(),
        tileSet()->type(), tileSet()->totalTileCount());

    if (!setID.has_value()) {
        setError("Error saving tile set");
        return;
    }

    tileSet()->blockSignals(true);
    tileSet()->setId(setID.value());

    const SetTotalsResult totals = worker.database()->computeSetTotals(setID.value(), tileSet()->defaultSet(),
                                                                       tileSet()->totalTileCount(), tileSet()->type());
    tileSet()->setSavedTileCount(totals.savedTileCount);
    tileSet()->setSavedTileSize(totals.savedTileSize);
    tileSet()->setTotalTileSize(totals.totalTileSize);
    tileSet()->setUniqueTileCount(totals.uniqueTileCount);
    tileSet()->setUniqueTileSize(totals.uniqueTileSize);
    tileSet()->blockSignals(false);

    setTileSetSaved();
}

void QGCGetTileDownloadListTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    const QList<QGCTile> tileValues = worker.database()->getTileDownloadList(setID(), count());
    QQueue<QGCTile*> tiles;
    for (const auto& t : tileValues) {
        tiles.enqueue(new QGCTile(t));
    }
    setTileListFetched(tiles);
}

void QGCImportTileTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    auto progressCb = [this](int pct) { setProgress(pct); };

    DatabaseResult result;
    if (replace()) {
        result = worker.database()->importSetsReplace(path(), progressCb);
    } else {
        result = worker.database()->importSetsMerge(path(), progressCb);
    }

    worker.setDatabaseValid(worker.database()->isValid());

    if (!result.success) {
        setError(result.errorString);
        return;
    }

    if (replace() && worker.database()->isValid()) {
        QSettings settings;
        settings.remove(QLatin1String(QGCTileCacheDatabase::kBingNoTileDoneKey));
        worker.database()->deleteBingNoTileTiles();
    }

    setImportCompleted();
}

void QGCExportTileTask::execute(QGCCacheWorker& worker)
{
    if (!worker.validateDatabase(this)) {
        return;
    }

    auto progressCb = [this](int pct) { setProgress(pct); };
    DatabaseResult result = worker.database()->exportSets(sets(), path(), progressCb);

    if (!result.success) {
        setError(result.errorString);
        return;
    }

    setExportCompleted();
}
