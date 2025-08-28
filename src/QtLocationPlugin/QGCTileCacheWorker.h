/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <atomic>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheWorkerLog)

class QGCMapTask;
class QGCCachedTileSet;
class QGCTileCacheDatabase;

class QGCCacheWorker : public QThread
{
    Q_OBJECT

public:
    explicit QGCCacheWorker(QObject *parent = nullptr);
    ~QGCCacheWorker();

    void setDatabaseFile(const QString &path) { _databasePath = path; }
    bool isValid() const { return _valid.load(); }
    bool hasFailed() const { return _failed.load(); }
    int pendingTaskCount() const;

public slots:
    bool enqueueTask(QGCMapTask *task);
    void stop();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

protected:
    void run() final;

private:
    // Task execution
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

    // Helper methods
    bool _testTask(QGCMapTask *task) const;
    bool _init();
    void _updateSetTotals(QGCCachedTileSet *set);
    void _updateTotals();
    void _deleteBingNoTileTiles();

    // Member variables
    std::unique_ptr<QGCTileCacheDatabase> _database;
    mutable QMutex _taskQueueMutex;
    QQueue<QGCMapTask*> _taskQueue;
    QWaitCondition _waitc;
    QString _databasePath;

    // Statistics cache
    mutable QMutex _statsMutex;
    quint32 _defaultCount = 0;
    quint32 _totalCount = 0;
    quint64 _defaultSize = 0;
    quint64 _totalSize = 0;

    // Update timing
    QElapsedTimer _updateTimer;
    int _updateTimeout = kShortTimeout;

    // State flags
    std::atomic_bool _failed{false};
    std::atomic_bool _valid{false};
    std::atomic_bool _running{true};

    static constexpr int kShortTimeout = 2 * 1000;
    static constexpr int kLongTimeout = 5 * 1000;
    static constexpr const char *kConnectionName = "QGCTileCacheDB";
    static constexpr const char *kBingNoTileBytesPath = ":/res/BingNoTileBytes.dat";
};
