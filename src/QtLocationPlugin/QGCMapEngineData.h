/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    QGCMapTask(TaskType type)
        : m_type(type)
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
    TaskType m_type;
};

Q_DECLARE_METATYPE(QGCMapTask::TaskType)

class QGCFetchTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCFetchTileSetTask()
        : QGCMapTask(QGCMapTask::taskFetchTileSets)
    {}

    void setTileSetFetched(QGCCachedTileSet* tileSet)
    {
        emit tileSetFetched(tileSet);
    }

signals:
    void tileSetFetched(QGCCachedTileSet* tileSet);
};

class QGCCreateTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCCreateTileSetTask(QGCCachedTileSet* tileSet)
        : QGCMapTask(QGCMapTask::taskCreateTileSet)
        , m_tileSet(tileSet)
        , m_saved(false)
    {}

    ~QGCCreateTileSetTask()
    {
        if(!m_saved && m_tileSet) {
            delete m_tileSet;
        }
    }

    QGCCachedTileSet* tileSet() { return m_tileSet; }

    void setTileSetSaved()
    {
        m_saved = true;
        emit tileSetSaved(m_tileSet);
    }

signals:
    void tileSetSaved(QGCCachedTileSet* tileSet);

private:
    QGCCachedTileSet* m_tileSet;
    bool m_saved;
};

class QGCFetchTileTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCFetchTileTask(const QString &hash)
        : QGCMapTask(QGCMapTask::taskFetchTile)
        , m_hash(hash)
    {}

    void setTileFetched(QGCCacheTile* tile)
    {
        emit tileFetched(tile);
    }

    QString hash() const { return m_hash; }

signals:
    void tileFetched(QGCCacheTile* tile);

private:
    QString m_hash;
};

class QGCSaveTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCSaveTileTask(QGCCacheTile* tile)
        : QGCMapTask(QGCMapTask::taskCacheTile)
        , m_tile(tile)
    {}

    ~QGCSaveTileTask()
    {
        delete m_tile;
        m_tile = nullptr;
    }

    QGCCacheTile* tile() { return m_tile; }

private:
    QGCCacheTile* m_tile;
};

class QGCGetTileDownloadListTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCGetTileDownloadListTask(qulonglong setID, int count)
        : QGCMapTask(QGCMapTask::taskGetTileDownloadList)
        , m_setID(setID)
        , m_count(count)
    {}

    qulonglong setID() const { return m_setID; }
    int count() const { return m_count; }

    void setTileListFetched(QList<QGCTile*> tiles)
    {
        emit tileListFetched(tiles);
    }

signals:
    void tileListFetched(QList<QGCTile*> tiles);

private:
    qulonglong m_setID;
    int m_count;
};

class QGCUpdateTileDownloadStateTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCUpdateTileDownloadStateTask(qulonglong setID, QGCTile::TileState state, const QString& hash)
        : QGCMapTask(QGCMapTask::taskUpdateTileDownloadState)
        , m_setID(setID)
        , m_state(state)
        , m_hash(hash)
    {}

    QString hash() const { return m_hash; }
    qulonglong setID() const { return m_setID; }
    QGCTile::TileState state() const { return m_state; }

private:
    qulonglong m_setID;
    QGCTile::TileState m_state;
    QString m_hash;
};

class QGCDeleteTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCDeleteTileSetTask(qulonglong setID)
        : QGCMapTask(QGCMapTask::taskDeleteTileSet)
        , m_setID(setID)
    {}

    qulonglong setID() const { return m_setID; }

    void setTileSetDeleted()
    {
        emit tileSetDeleted(m_setID);
    }

signals:
    void tileSetDeleted(qulonglong setID);

private:
    qulonglong m_setID;
};

class QGCRenameTileSetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCRenameTileSetTask(qulonglong setID, const QString &newName)
        : QGCMapTask(QGCMapTask::taskRenameTileSet)
        , m_setID(setID)
        , m_newName(newName)
    {}

    qulonglong setID() const { return m_setID; }
    QString newName() const { return m_newName; }

private:
    qulonglong m_setID;
    QString m_newName;
};

class QGCPruneCacheTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCPruneCacheTask(quint64 amount)
        : QGCMapTask(QGCMapTask::taskPruneCache)
        , m_amount(amount)
    {}

    quint64 amount() const { return m_amount; }

    void setPruned()
    {
        emit pruned();
    }

signals:
    void pruned();

private:
    quint64 m_amount;
};

class QGCResetTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCResetTask()
        : QGCMapTask(QGCMapTask::taskReset)
    {}

    void setResetCompleted()
    {
        emit resetCompleted();
    }

signals:
    void resetCompleted();
};

class QGCExportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCExportTileTask(QVector<QGCCachedTileSet*> sets, const QString &path)
        : QGCMapTask(QGCMapTask::taskExport)
        , m_sets(sets)
        , m_path(path)
    {}

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
    QVector<QGCCachedTileSet*> m_sets;
    QString m_path;
};

class QGCImportTileTask : public QGCMapTask
{
    Q_OBJECT

public:
    QGCImportTileTask(const QString &path, bool replace)
        : QGCMapTask(QGCMapTask::taskImport)
        , m_path(path)
        , m_replace(replace)
    {}

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
    QString m_path;
    bool m_replace;
};
