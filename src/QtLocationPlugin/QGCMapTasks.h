#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <functional>

#include "QGCMapTaskBase.h"
#include "QGCTile.h"
#include "QGCTileCacheTypes.h"

struct QGCCacheTile;
class QGCCachedTileSet;
class QGCCacheWorker;

//-----------------------------------------------------------------------------

// Fire-and-forget command task: carries its TaskType plus the work captured as a
// callable, and emits a single payload-free completed() when the work succeeds.
// The callable returns false (after calling setError) to suppress completed().
class QGCCommandTask : public QGCMapTask
{
    Q_OBJECT

public:
    using Work = std::function<bool(QGCCacheWorker&, QGCMapTask&)>;

    QGCCommandTask(TaskType type, Work work, QObject* parent = nullptr)
        : QGCMapTask(type, parent), m_work(std::move(work))
    {}

    ~QGCCommandTask() override = default;

    void execute(QGCCacheWorker& worker) override
    {
        if (m_work && m_work(worker, *this)) {
            emit completed();
        }
    }

signals:
    void completed();

private:
    const Work m_work;
};

//-----------------------------------------------------------------------------

class QGCFetchTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCFetchTileSetTask(QObject* parent = nullptr) : QGCMapTask(TaskType::taskFetchTileSets, parent) {}

    ~QGCFetchTileSetTask() = default;

    void execute(QGCCacheWorker& worker) override;

    void setTileSetFetched(QGCCachedTileSet* tileSet) { emit tileSetFetched(tileSet); }

signals:
    void tileSetFetched(QGCCachedTileSet* tileSet);
};

//-----------------------------------------------------------------------------

class QGCCreateTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCCreateTileSetTask(QGCCachedTileSet* tileSet, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskCreateTileSet, parent), m_tileSet(tileSet), m_saved(false)
    {}

    ~QGCCreateTileSetTask();

    void execute(QGCCacheWorker& worker) override;

    QGCCachedTileSet* tileSet() { return m_tileSet; }

    void setTileSetSaved()
    {
        m_saved = true;
        emit tileSetSaved(m_tileSet);
    }

signals:
    void tileSetSaved(QGCCachedTileSet* tileSet);

private:
    QGCCachedTileSet* const m_tileSet = nullptr;
    bool m_saved = false;
};

//-----------------------------------------------------------------------------

class QGCFetchTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCFetchTileTask(const QString& hash, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskFetchTile, parent), m_hash(hash)
    {}

    ~QGCFetchTileTask() = default;

    void execute(QGCCacheWorker& worker) override;

    void setTileFetched(const QSharedPointer<QGCCacheTile>& tile) { emit tileFetched(tile); }

    QString hash() const { return m_hash; }

signals:
    void tileFetched(QSharedPointer<QGCCacheTile> tile);

private:
    const QString m_hash;
};

//-----------------------------------------------------------------------------

// R6 lower-zoom fallback: when a tile cannot be served from cache or network the
// worker scans cached ancestor tiles (z-1 .. z-maxLevelsUp). On a hit it crops and
// scales the ancestor sub-square covering this tile and returns it as a placeholder.
class QGCFetchFallbackTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCFetchFallbackTileTask(const QString& providerName, int x, int y, int z, int tileSize, int maxLevelsUp = 5,
                             QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskFetchFallbackTile, parent),
          m_providerName(providerName),
          m_x(x),
          m_y(y),
          m_z(z),
          m_tileSize(tileSize),
          m_maxLevelsUp(maxLevelsUp)
    {}

    ~QGCFetchFallbackTileTask() = default;

    void execute(QGCCacheWorker& worker) override;

    QString providerName() const { return m_providerName; }

    int x() const { return m_x; }

    int y() const { return m_y; }

    int z() const { return m_z; }

    int tileSize() const { return m_tileSize; }

    int maxLevelsUp() const { return m_maxLevelsUp; }

    void setFallbackFetched(const QByteArray& image, const QString& format, int levelDelta)
    {
        emit fallbackFetched(image, format, levelDelta);
    }

signals:
    void fallbackFetched(QByteArray image, QString format, int levelDelta);

private:
    const QString m_providerName;
    const int m_x;
    const int m_y;
    const int m_z;
    const int m_tileSize;
    const int m_maxLevelsUp;
};

//-----------------------------------------------------------------------------

class QGCSaveTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCSaveTileTask(QGCCacheTile* tile, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskCacheTile, parent), m_tile(tile)
    {}

    ~QGCSaveTileTask();

    void execute(QGCCacheWorker& worker) override;

    const QGCCacheTile* tile() const { return m_tile; }

    QGCCacheTile* tile() { return m_tile; }

private:
    QGCCacheTile* const m_tile = nullptr;
};

//-----------------------------------------------------------------------------

class QGCGetTileDownloadListTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCGetTileDownloadListTask(quint64 setID, int count, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskGetTileDownloadList, parent), m_setID(setID), m_count(count)
    {}

    ~QGCGetTileDownloadListTask() = default;

    void execute(QGCCacheWorker& worker) override;

    quint64 setID() const { return m_setID; }

    int count() const { return m_count; }

    void setTileListFetched(const QQueue<QGCTile*>& tiles) { emit tileListFetched(tiles); }

signals:
    void tileListFetched(QQueue<QGCTile*> tiles);

private:
    const quint64 m_setID = 0;
    const int m_count = 0;
};

//-----------------------------------------------------------------------------

class QGCExportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCExportTileTask(const QList<TileSetRecord>& sets, const QString& path, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskExport, parent), m_sets(sets), m_path(path)
    {}

    ~QGCExportTileTask() = default;

    void execute(QGCCacheWorker& worker) override;

    const QList<TileSetRecord>& sets() const { return m_sets; }

    QString path() const { return m_path; }

    void setExportCompleted() { emit actionCompleted(); }

    void setProgress(int percentage) { emit actionProgress(percentage); }

signals:
    void actionCompleted();
    void actionProgress(int percentage);

private:
    const QList<TileSetRecord> m_sets;
    const QString m_path;
};

//-----------------------------------------------------------------------------

class QGCImportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCImportTileTask(const QString& path, bool replace, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskImport, parent), m_path(path), m_replace(replace)
    {}

    ~QGCImportTileTask() = default;

    void execute(QGCCacheWorker& worker) override;

    QString path() const { return m_path; }

    bool replace() const { return m_replace; }

    int progress() const { return m_progress; }

    void setImportCompleted() { emit actionCompleted(); }

    void setProgress(int percentage)
    {
        m_progress = percentage;
        emit actionProgress(percentage);
    }

signals:
    void actionCompleted();
    void actionProgress(int percentage);

private:
    const QString m_path;
    const bool m_replace = false;
    int m_progress = 0;
};

//-----------------------------------------------------------------------------
