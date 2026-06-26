#pragma once

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <atomic>
#include <memory>

class QGCMapTask;
class QGCTileCacheDatabase;

// QObject worker on an owned QThread (never subclass QThread). A Qt event loop drains the
// mutex-protected task queue on the worker thread, coalescing consecutive cache-tile saves.
class QGCCacheWorker : public QObject
{
    Q_OBJECT

public:
    QGCCacheWorker();
    ~QGCCacheWorker() override;

    void setDatabaseFile(const QString& path)
    {
        if (isRunning()) {
            return;
        }
        _databasePath = path;
    }

    bool isRunning() const { return _thread.isRunning(); }

    // Bounded wait only: callers must pass an explicit timeout. The destructor waits
    // for the thread itself, so an unbounded ULONG_MAX default is never the intent.
    bool wait(unsigned long timeMs) { return _thread.wait(timeMs); }

    // Accessors used by QGCMapTask::execute() implementations running on this thread.
    QGCTileCacheDatabase* database() const { return _database.get(); }

    bool validateDatabase(QGCMapTask* task);
    void emitTotals();

    void setDatabaseValid(bool valid) { _dbValid = valid; }

public slots:
    bool enqueueTask(QGCMapTask* task);
    void stop();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private slots:
    void _init();
    void _drainQueue();
    void _shutdown();

private:
    void _saveTileBatch(const QList<QGCMapTask*>& tasks);

    QThread _thread;
    std::unique_ptr<QGCTileCacheDatabase> _database;
    QTimer* _updateTimer = nullptr;
    QMutex _taskQueueMutex;
    QQueue<QGCMapTask*> _taskQueue;
    QString _databasePath;
    std::atomic_bool _dbValid = false;
    std::atomic_bool _stopRequested = false;
    std::atomic_bool _drainScheduled = false;

    static constexpr int kUpdateTimerIntervalMs = 3000;
    static constexpr int kMaxSaveBatch = 64;
};
