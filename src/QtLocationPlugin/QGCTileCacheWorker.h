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
 *   @brief Map Tile Cache Worker Thread
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtSql/QSqlDatabase>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheLog)

class QGCMapTask;
class QGCCachedTileSet;

class QGCCacheWorker : public QThread
{
    Q_OBJECT

public:
    explicit QGCCacheWorker(QObject* parent = nullptr);
    ~QGCCacheWorker();

    void stopRunning();
    bool enqueueTask(QGCMapTask* task);
    void setDatabaseFilePath(const QString& path) { _databasePath = path; }

private:
    void run() final;

    void        _runTask                (QGCMapTask* task);

    void        _saveTile               (QGCMapTask* mtask);
    void        _getTile                (QGCMapTask* mtask);
    void        _getTileSets            (QGCMapTask* mtask);
    void        _createTileSet          (QGCMapTask* mtask);
    void        _getTileDownloadList    (QGCMapTask* mtask);
    void        _updateTileDownloadState(QGCMapTask* mtask);
    void        _deleteTileSet          (QGCMapTask* mtask);
    void        _renameTileSet          (QGCMapTask* mtask);
    void        _resetCacheDatabase     (QGCMapTask* mtask);
    void        _pruneCache             (QGCMapTask* mtask);
    void        _exportSets             (QGCMapTask* mtask);
    void        _importSets             (QGCMapTask* mtask);
    bool        _testTask               (QGCMapTask* mtask);
    void        _deleteBingNoTileTiles  ();

    quint64     _findTile               (const QString &hash);
    bool        _findTileSetID          (const QString &name, quint64& setID);
    void        _updateSetTotals        (QGCCachedTileSet* set);
    bool        _init                   ();
    bool        _connectDB              ();
    bool        _createDB               (QSqlDatabase& db, bool createDefault = true);
    void        _disconnectDB           ();
    quint64     _getDefaultTileSet      ();
    void        _updateTotals           ();
    void        _deleteTileSet          (qulonglong id);

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private:
    QQueue<QGCMapTask*> _taskQueue;
    QMutex _taskQueueMutex;
    QWaitCondition _waitc;
    QString _databasePath;
    std::unique_ptr<QSqlDatabase> _db = nullptr;
    std::atomic_bool _valid = false;
    bool _failed = false;
    quint64 _defaultSet = UINT64_MAX;
    quint64 _totalSize = 0;
    quint32 _totalCount = 0;
    quint64 _defaultSize = 0;
    quint32 _defaultCount = 0;
    time_t _lastUpdate = 0;
    int _updateTimeout = 0;
};
