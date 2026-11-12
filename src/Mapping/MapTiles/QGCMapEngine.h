#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class QGCMapTask;
class QGCCacheWorker;
class QNetworkAccessManager;

class QGCMapEngine : public QObject
{
    Q_OBJECT

public:
    explicit QGCMapEngine(QObject* parent = nullptr);
    ~QGCMapEngine();

    void init(const QString& databasePath);

    /// Initializes the engine with the application's tile cache database unless that already happened. Maps other
    /// than QtLocation's (GeoMap) call this, since only a QtLocation map initializes the engine on its own.
    void ensureInitialized();
    bool addTask(QGCMapTask* task);

    /// Stores a tile in the cache database, in \a tileSet or else in the default set. Does nothing while persistence
    /// is disabled.
    void cacheTile(const QString& type, const QString& hash, const QByteArray& image, const QString& format,
                   quint64 tileSet = UINT64_MAX);

    /// Directory of the tile cache, created on first use. Empty when no directory could be created.
    static QString cachePath();
    static QString databaseFilePath();

    /// The network manager for the requests of the maps: tiles, and lists such as the credits of a provider. The
    /// requests share it, and so its connections to a server. Created on first use, never null, and alive as long
    /// as the engine: shutdown() and close() leave it. For the main thread, where the engine lives.
    QNetworkAccessManager* networkManager();

    /// For tests: networkManager() returns \a networkManager, which is not owned, until that is destroyed or null
    /// is set.
    void setNetworkManagerForTest(QNetworkAccessManager* networkManager);

    /// Stops the cache worker thread and closes the database. Call before
    /// QCoreApplication teardown when the engine was initialized outside the
    /// normal QML map lifecycle (e.g. unit test runs).
    void shutdown();

    /// Final shutdown: shutdown(), then ignore later init() calls, so a map created during teardown
    /// cannot reopen the database once the application object is gone.
    void close();

    /// close() on the application's engine, if one was created.
    static void closeInstance();

    static QGCMapEngine* instance();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

    /// The user configured another source for the provider \a mapId (MapProvider::sourceSettings()). The tiles of
    /// that provider which a map holds, and those it gets for a request it made before, are of the previous source.
    void tileSourceChanged(int mapId);

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

    void _pruned() { m_pruning = false; }

private:
    QGCCacheWorker* m_worker = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    QPointer<QNetworkAccessManager> m_testNetworkManager;
    bool m_pruning = false;
    std::atomic<bool> m_initialized = false;
    std::atomic<bool> m_closed = false;
};
