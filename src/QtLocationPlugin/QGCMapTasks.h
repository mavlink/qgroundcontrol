/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache Data
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QString>

#include "QGCTile.h"
#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"

class QGCMapTask : public QObject
{
    Q_OBJECT

public:
    enum TaskType {
        taskInit,
        taskCacheTile,
        taskFetchTile,
        taskFetchTileSets,
        taskCreateTileSet,
        taskGetTileDownloadList,
        taskUpdateTileDownloadState,
        taskDeleteTileSet,
        taskRenameTileSet,
        taskPruneCache,
        taskReset,
        taskExport,
        taskImport
    };
    Q_ENUM(TaskType);

    explicit QGCMapTask(TaskType type, QObject *parent = nullptr)
        : QObject(parent)
        , m_type(type)
    {}
    virtual ~QGCMapTask() = default;

    TaskType type() const { return m_type; }

    void setError(const QString &errorString = QString())
    {
        emit error(m_type, errorString);
    }

signals:
    void error(QGCMapTask::TaskType type, const QString &errorString);

private:
    const TaskType m_type = TaskType::taskInit;
};

//-----------------------------------------------------------------------------

class QGCFetchTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCFetchTileSetTask(QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskFetchTileSets, parent)
    {}
    ~QGCFetchTileSetTask() = default;

    void setTileSetFetched(QGCCachedTileSet *tileSet)
    {
        emit tileSetFetched(tileSet);
    }

signals:
    void tileSetFetched(QGCCachedTileSet *tileSet);
};

//-----------------------------------------------------------------------------

class QGCCreateTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCCreateTileSetTask(QGCCachedTileSet *tileSet, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskCreateTileSet, parent)
        , m_tileSet(tileSet)
        , m_saved(false)
    {}
    ~QGCCreateTileSetTask()
    {
        if (!m_saved) {
            delete m_tileSet;
        }
    }

    QGCCachedTileSet *tileSet() { return m_tileSet; }

    void setTileSetSaved()
    {
        m_saved = true;
        emit tileSetSaved(m_tileSet);
    }

signals:
    void tileSetSaved(QGCCachedTileSet *tileSet);

private:
    QGCCachedTileSet* const m_tileSet = nullptr;
    bool m_saved = false;
};

//-----------------------------------------------------------------------------

class QGCFetchTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCFetchTileTask(const QString &hash, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskFetchTile, parent)
        , m_hash(hash)
    {}
    ~QGCFetchTileTask() = default;

    void setTileFetched(QGCCacheTile *tile)
    {
        emit tileFetched(tile);
    }

    QString hash() const { return m_hash; }

signals:
    void tileFetched(QGCCacheTile *tile);

private:
    const QString m_hash;
};

//-----------------------------------------------------------------------------

class QGCSaveTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCSaveTileTask(QGCCacheTile *tile, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskCacheTile, parent)
        , m_tile(tile)
    {}
    ~QGCSaveTileTask()
    {
        delete m_tile;
    }

    QGCCacheTile *tile() { return m_tile; }

private:
    QGCCacheTile* const m_tile = nullptr;
};

//-----------------------------------------------------------------------------

class QGCGetTileDownloadListTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCGetTileDownloadListTask(quint64 setID, int count, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskGetTileDownloadList, parent)
        , m_setID(setID)
        , m_count(count)
    {}
    ~QGCGetTileDownloadListTask() = default;

    quint64 setID() const { return m_setID; }
    int count() const { return m_count; }

    void setTileListFetched(const QQueue<QGCTile*> &tiles)
    {
        emit tileListFetched(tiles);
    }

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
    QGCUpdateTileDownloadStateTask(quint64 setID, QGCTile::TileState state, const QString &hash, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskUpdateTileDownloadState, parent)
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
    explicit QGCDeleteTileSetTask(quint64 setID, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskDeleteTileSet, parent)
        , m_setID(setID)
    {}
    ~QGCDeleteTileSetTask() = default;

    quint64 setID() const { return m_setID; }

    void setTileSetDeleted()
    {
        emit tileSetDeleted(m_setID);
    }

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
    QGCRenameTileSetTask(quint64 setID, const QString &newName, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskRenameTileSet, parent)
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
    explicit QGCPruneCacheTask(quint64 amount, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskPruneCache, parent)
        , m_amount(amount)
    {}
    ~QGCPruneCacheTask() = default;

    quint64 amount() const { return m_amount; }

    void setPruned()
    {
        emit pruned();
    }

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
    QGCResetTask(QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskReset, parent)
    {}
    ~QGCResetTask() = default;

    void setResetCompleted()
    {
        emit resetCompleted();
    }

signals:
    void resetCompleted();
};

//-----------------------------------------------------------------------------

class QGCExportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    explicit QGCExportTileTask(const QVector<QGCCachedTileSet*> &sets, const QString &path, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskExport, parent)
        , m_sets(sets)
        , m_path(path)
    {}
    ~QGCExportTileTask() = default;

    QVector<QGCCachedTileSet*> sets() const { return m_sets; }
    QString path() const { return m_path; }

    void setExportCompleted()
    {
        emit actionCompleted();
    }

    void setProgress(int percentage)
    {
        emit actionProgress(percentage);
    }

signals:
    void actionCompleted();
    void actionProgress(int percentage);

private:
    const QVector<QGCCachedTileSet*> m_sets;
    const QString m_path;
};

//-----------------------------------------------------------------------------

class QGCImportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCImportTileTask(const QString &path, bool replace, QObject *parent = nullptr)
        : QGCMapTask(QGCMapTask::taskImport, parent)
        , m_path(path)
        , m_replace(replace)
    {}
    ~QGCImportTileTask() = default;

    QString path() const { return m_path; }
    bool replace() const { return m_replace; }

    void setImportCompleted()
    {
        emit actionCompleted();
    }

    void setProgress(int percentage)
    {
        emit actionProgress(percentage);
    }

signals:
    void actionCompleted();
    void actionProgress(int percentage);

private:
    const QString m_path;
    const bool m_replace = false;
};

//-----------------------------------------------------------------------------
