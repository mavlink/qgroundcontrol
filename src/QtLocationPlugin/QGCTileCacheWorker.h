#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtSql/QSqlError>
#include <functional>

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheWorkerLog)

class QGCMapTask;
class QGCCachedTileSet;
class QGCImportTileTask;
class QSqlDatabase;
class QSqlQuery;

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
    void downloadStatusUpdated(quint64 setID, quint32 pending, quint32 downloading, quint32 errors);

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
    void _setCacheEnabled(QGCMapTask *task);
    void _setDefaultCacheEnabled(QGCMapTask *task);
    bool _testTask(QGCMapTask *task);
    void _notifyImportFailure(QGCImportTileTask *task, const QString &message);
    bool _importReplace(QGCImportTileTask *task);
    bool _importAppend(QGCImportTileTask *task);

    bool _connectDB();
    void _disconnectDB();
    bool _createDB(QSqlDatabase &db, bool createDefault = true);
    bool _findTileSetID(const QString &name, quint64 &setID);
    bool _init();
    bool _verifyDatabaseVersion() const;
    quint64 _findTile(const QString &hash);
    quint64 _getDefaultTileSet();
    void _deleteBingNoTileTiles();
    void _deleteTileSet(quint64 id);
    void _updateSetTotals(QGCCachedTileSet *set);
    void _updateTotals();
    void _emitDownloadStatus(quint64 setID);
    void _setTaskError(QGCMapTask *task, const QString &baseMessage, const QSqlError &error = QSqlError()) const;

    bool _batchDeleteTiles(QSqlQuery& query, const QList<quint64>& tileIds, const QString& errorContext);
    bool _generateUniqueTileSetName(QString& name);
    QSqlDatabase _getDB() const;
    bool _canAccessDatabase() const;
    enum class VersionReadStatus {
        Success,
        NoAccess,
        OpenFailed,
        QueryFailed,
        VersionMismatch
    };
    VersionReadStatus _checkDatabaseVersion(const QString &path, QString *errorMessage = nullptr) const;
    VersionReadStatus _readDatabaseVersion(const QString &path, const QString &connectionName, int &version) const;

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
    std::atomic_bool _stopping = false;
    std::atomic_bool _cacheEnabled = true;
    std::atomic_bool _defaultCachingEnabled = true;

    static constexpr const char *kSession = "QGeoTileWorkerSession";
    static constexpr const char *kExportSession = "QGeoTileExportSession";
    static constexpr int kShortTimeout = 400;
    static constexpr int kLongTimeout = 1000;
    static constexpr int kCurrentSchemaVersion = 2;
    static constexpr int kSqlTrue = 1;
    static constexpr int kPruneBatchSize = 128;
    static constexpr int kTaskQueueThreshold = 100;
    static constexpr int kWorkerWaitTimeoutMs = 5000;
    static constexpr int kMaxNameGenerationAttempts = 999;
    static constexpr int kSqliteDefaultVariableLimit = 999;
};
