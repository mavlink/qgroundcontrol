#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheWorkerLog)

class QGCMapTask;

class QGCTileCacheWorker : public QThread
{
    Q_OBJECT

public:
    explicit QGCTileCacheWorker(QObject *parent = nullptr);
    ~QGCTileCacheWorker();

    void setDatabaseFile(const QString &path) { _databasePath = path; }
    bool enqueueTask(QGCMapTask *task);
    void stop();

signals:
    void updateTotals(quint32 totalTiles, quint64 totalSize, quint32 defaultTiles, quint64 defaultSize);

private:
    void run() final;

    void _processTask(QGCMapTask *task) const;
    void _handleSaveTileTask(QGCMapTask *task) const;
    void _handleFetchTileTask(QGCMapTask *task) const;
    void _handleFetchTileSetsTask(QGCMapTask *task) const;
    void _handleCreateTileSetTask(QGCMapTask *task) const;
    void _handleGetTileDownloadListTask(QGCMapTask *task) const;
    void _handleUpdateTileDownloadStateTask(QGCMapTask *task) const;
    void _handlePruneCacheTask(QGCMapTask *task) const;
    void _handleDeleteTileSetTask(QGCMapTask *task) const;
    void _handleRenameTileSetTask(QGCMapTask *task) const;
    void _handleResetCacheDatabaseTask(QGCMapTask *task) const;
    void _handleImportSetsTask(QGCMapTask *task) const;
    void _handleExportSetsTask(QGCMapTask *task) const;

    QMutex _taskQueueMutex;
    QQueue<QGCMapTask*> _taskQueue;
    QWaitCondition _waitc;
    QString _databasePath;
    class QGCTileCacheDatabase *_dbManager = nullptr;
};
