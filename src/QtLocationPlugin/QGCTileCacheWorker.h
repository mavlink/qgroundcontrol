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
 *   @brief Map Tile Cache Worker Thread
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheWorkerLog)

class QGCMapTask;
class QGCCachedTileSet;
class QSqlDatabase;

class QGCCacheWorker : public QThread
{
    Q_OBJECT

public:
    explicit QGCCacheWorker(QObject *parent = nullptr);
    ~QGCCacheWorker();

    void setDatabaseFile(const QString &path) { _databasePath = path; }

public slots:
    bool enqueueTask(QGCMapTask *task);
    void stop();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

protected:
    void run() final;

private:
    void _runTask(QGCMapTask *task);

    void _saveTile(QGCMapTask *task);
    void _getTile(QGCMapTask *task);
    void _getTileSets(QGCMapTask *task);
    void _createTileSet(QGCMapTask *task);
    void _getTileDownloadList(QGCMapTask *task);
    void _updateTileDownloadState(QGCMapTask *task);
    void _pruneCache(QGCMapTask *task);
    void _deleteTileSet(QGCMapTask *task);
    void _renameTileSet(QGCMapTask *task);
    void _resetCacheDatabase(QGCMapTask *task);
    void _importSets(QGCMapTask *task);
    void _exportSets(QGCMapTask *task);
    bool _testTask(QGCMapTask *task);

    bool _connectDB();
    void _disconnectDB();
    bool _createDB(QSqlDatabase &db, bool createDefault = true);
    bool _findTileSetID(const QString &name, quint64 &setID);
    bool _init();
    quint64 _findTile(const QString &hash);
    quint64 _getDefaultTileSet();
    void _deleteBingNoTileTiles();
    void _deleteTileSet(quint64 id);
    void _updateSetTotals(QGCCachedTileSet *set);
    void _updateTotals();

    std::shared_ptr<QSqlDatabase> _db = nullptr;
    QMutex _taskQueueMutex;
    QQueue<QGCMapTask*> _taskQueue;
    QWaitCondition _waitc;
    QString _databasePath;
    quint32 _defaultCount = 0;
    quint32 _totalCount = 0;
    quint64 _defaultSet = UINT64_MAX;
    quint64 _defaultSize = 0;
    quint64 _totalSize = 0;
    QElapsedTimer _updateTimer;
    int _updateTimeout = kShortTimeout;
    std::atomic_bool _failed = false;
    std::atomic_bool _valid = false;

    static QByteArray _bingNoTileImage;
    static constexpr const char *kSession = "QGeoTileWorkerSession";
    static constexpr const char *kExportSession = "QGeoTileExportSession";
    static constexpr int kShortTimeout = 2;
    static constexpr int kLongTimeout = 5;
};
