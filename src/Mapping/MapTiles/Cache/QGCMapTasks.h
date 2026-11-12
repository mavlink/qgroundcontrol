#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QString>

#include "QGCCacheTile.h"
#include "QGCMapTask.h"
#include "QGCTile.h"
#include "QGCTileCacheTypes.h"

class QGCCachedTileSet;

// A signal that passes the type needs it complete, for its meta-type
Q_MOC_INCLUDE("QGCCachedTileSet.h")

//-----------------------------------------------------------------------------

class QGCFetchTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCFetchTileSetTask(QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskFetchTileSets, parent)
    {}

    ~QGCFetchTileSetTask() = default;

    void setTileSetFetched(QGCCachedTileSet* tileSet) { emit tileSetFetched(tileSet); }

signals:
    void tileSetFetched(QGCCachedTileSet* tileSet);
};

//-----------------------------------------------------------------------------

class QGCCreateTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    /// Create it on the main thread: it takes the cache variant of the set's provider there, for the worker thread
    /// that computes the keys of the set's tiles.
    explicit QGCCreateTileSetTask(QGCCachedTileSet* tileSet, QObject* parent = nullptr);

    ~QGCCreateTileSetTask();

    QGCCachedTileSet* tileSet() { return m_tileSet; }

    QString cacheVariant() const { return m_cacheVariant; }

    void setTileSetSaved()
    {
        m_saved = true;
        emit tileSetSaved(m_tileSet);
    }

signals:
    void tileSetSaved(QGCCachedTileSet* tileSet);

private:
    QGCCachedTileSet* const m_tileSet = nullptr;
    const QString m_cacheVariant;
    bool m_saved = false;
};

//-----------------------------------------------------------------------------

class QGCFetchTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCFetchTileTask(const QString& hash, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskFetchTile, parent)
        , m_hash(hash)
    {}

    ~QGCFetchTileTask() = default;

    void setTileFetched(const QGCCacheTile& tile) { emit tileFetched(tile); }

    QString hash() const { return m_hash; }

signals:
    /// By value, so that a tile queued for a receiver that is deleted meanwhile is not leaked
    void tileFetched(const QGCCacheTile& tile);

private:
    const QString m_hash;
};

//-----------------------------------------------------------------------------

class QGCSaveTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCSaveTileTask(QGCCacheTile* tile, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskCacheTile, parent)
        , m_tile(tile)
    {}

    ~QGCSaveTileTask();

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
        : QGCMapTask(TaskType::taskGetTileDownloadList, parent)
        , m_setID(setID)
        , m_count(count)
    {}

    ~QGCGetTileDownloadListTask() = default;

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

class QGCUpdateTileDownloadStateTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCUpdateTileDownloadStateTask(quint64 setID, QGCTile::TileState state, const QString& hash,
                                   QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskUpdateTileDownloadState, parent)
        , m_setID(setID)
        , m_state(state)
        , m_hash(hash)
    {}

    ~QGCUpdateTileDownloadStateTask() = default;

    QString hash() const { return m_hash; }

    quint64 setID() const { return m_setID; }

    QGCTile::TileState state() const { return m_state; }

private:
    const quint64 m_setID = 0;
    const QGCTile::TileState m_state = QGCTile::StatePending;
    const QString m_hash;
};

//-----------------------------------------------------------------------------

class QGCDeleteTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCDeleteTileSetTask(quint64 setID, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskDeleteTileSet, parent)
        , m_setID(setID)
    {}

    ~QGCDeleteTileSetTask() = default;

    quint64 setID() const { return m_setID; }

    void setTileSetDeleted() { emit tileSetDeleted(m_setID); }

signals:
    void tileSetDeleted(quint64 setID);

private:
    const quint64 m_setID = 0;
};

//-----------------------------------------------------------------------------

class QGCRenameTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCRenameTileSetTask(quint64 setID, const QString& newName, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskRenameTileSet, parent)
        , m_setID(setID)
        , m_newName(newName)
    {}

    ~QGCRenameTileSetTask() = default;

    quint64 setID() const { return m_setID; }

    QString newName() const { return m_newName; }

private:
    const quint64 m_setID = 0;
    const QString m_newName;
};

//-----------------------------------------------------------------------------

class QGCPruneCacheTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCPruneCacheTask(quint64 amount, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskPruneCache, parent)
        , m_amount(amount)
    {}

    ~QGCPruneCacheTask() = default;

    quint64 amount() const { return m_amount; }

    void setPruned() { emit pruned(); }

signals:
    void pruned();

private:
    const quint64 m_amount = 0;
};

//-----------------------------------------------------------------------------

class QGCResetTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCResetTask(QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskReset, parent)
    {}

    ~QGCResetTask() = default;

    void setResetCompleted() { emit resetCompleted(); }

signals:
    void resetCompleted();
};

//-----------------------------------------------------------------------------

class QGCExportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCExportTileTask(const QList<TileSetRecord>& sets, const QString& path, QObject* parent = nullptr)
        : QGCMapTask(TaskType::taskExport, parent)
        , m_sets(sets)
        , m_path(path)
    {}

    ~QGCExportTileTask() = default;

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
    /// Create it on the main thread: it takes the cache variants there, which the tiles of a file from before
    /// schema version 3 get.
    QGCImportTileTask(const QString& path, bool replace, QObject* parent = nullptr);

    ~QGCImportTileTask() = default;

    QString path() const { return m_path; }

    bool replace() const { return m_replace; }

    QHash<int, QString> cacheVariants() const { return m_cacheVariants; }

    void setImportCompleted() { emit actionCompleted(); }

    void setProgress(int percentage) { emit actionProgress(percentage); }

signals:
    void actionCompleted();
    void actionProgress(int percentage);

private:
    const QString m_path;
    const bool m_replace = false;
    const QHash<int, QString> m_cacheVariants;
};

//-----------------------------------------------------------------------------
