#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

class QGCMapTask;
class QGCCacheWorker;

class QGCMapEngine : public QObject
{
    Q_OBJECT

public:
    explicit QGCMapEngine(QObject *parent = nullptr);
    ~QGCMapEngine();

    void init(const QString &databasePath);
    bool addTask(QGCMapTask *task);

    /// Stops the cache worker thread and closes the database. Call before
    /// QCoreApplication teardown when the engine was initialized outside the
    /// normal QML map lifecycle (e.g. unit test runs).
    void shutdown();

    /// Final shutdown: shutdown(), then ignore later init() calls, so a map created during teardown
    /// cannot reopen the database once the application object is gone.
    void close();

    /// close() on the application's engine, if one was created.
    static void closeInstance();

    static QGCMapEngine *instance();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _pruned() { m_pruning = false; }

private:
    QGCCacheWorker *m_worker = nullptr;
    bool m_pruning = false;
    std::atomic<bool> m_initialized = false;
    std::atomic<bool> m_closed = false;
};

extern QGCMapEngine *getQGCMapEngine();
