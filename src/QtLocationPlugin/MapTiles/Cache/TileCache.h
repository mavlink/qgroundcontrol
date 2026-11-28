#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>

class CacheWorker;
class CacheTask;

/// The tile cache of the maps: the database of tiles on disk, and the worker thread that reads and writes it. For
/// the main thread only: the worker gets its work as tasks.
class TileCache : public QObject
{
    Q_OBJECT

public:
    explicit TileCache(QObject* parent = nullptr);
    ~TileCache();

    void init(const QString& databasePath);

    /// Initializes the cache with the application's tile cache database unless that already happened. Maps other
    /// than QtLocation's (GeoMap) call this, since only a QtLocation map initializes the cache on its own.
    void ensureInitialized();

    /// Queues \a task (CacheTasks, TileSetTasks) for the cache worker, and takes it: it is deleted once it has
    /// finished. False for a task that is not queued: it is dropped while the cache has no worker, and finishes with an
    /// error when the worker cannot run it.
    bool addTask(CacheTask* task);

    /// Stores a tile of the provider \a mapId in the cache database, in \a tileSet or else in the default set. Does
    /// nothing while persistence is disabled.
    void cacheTile(int mapId, const QString& hash, const QByteArray& image, const QString& format,
                   quint64 tileSet = UINT64_MAX);

    /// Directory of the tile cache, created on first use. Empty when no directory could be created.
    static QString cachePath();
    static QString databaseFilePath();

    /// Stops the cache worker thread and closes the database. Call before
    /// QCoreApplication teardown when the cache was initialized outside the
    /// normal QML map lifecycle (e.g. unit test runs).
    void shutdown();

    /// Final shutdown: shutdown(), then ignore later init() calls, so a map created during teardown
    /// cannot reopen the database once the application object is gone.
    void close();

    /// close() on the application's cache, if one was created.
    static void closeInstance();

    static TileCache* instance();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private:
    CacheWorker* _worker = nullptr;
    bool _pruning = false;
    /// The size of the default set that a prune failed to reduce, or 0
    quint64 _unprunedSize = 0;
    bool _initialized = false;
    bool _closed = false;
};
