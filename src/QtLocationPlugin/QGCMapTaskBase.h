#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

class QGCCacheWorker;

// Thread affinity: tasks are created/connected on the main thread and never moved
// to the worker; execute() runs on the worker thread, so result signals cross
// threads via queued AutoConnection. Raw-pointer result signals require a
// matching qRegisterMetaType before any Qt::QueuedConnection delivery.
class QGCMapTask : public QObject
{
    Q_OBJECT

public:
    enum class TaskType
    {
        taskInit,
        taskCacheTile,
        taskRefreshTileValidators,
        taskFetchTile,
        taskFetchFallbackTile,
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
    Q_ENUM(TaskType);

    explicit QGCMapTask(TaskType type, QObject* parent = nullptr) : QObject(parent), m_type(type) {}

    ~QGCMapTask() override = default;

    TaskType type() const { return m_type; }

    // Runs on the worker thread.
    virtual void execute(QGCCacheWorker& worker) { Q_UNUSED(worker); }

    void setError(const QString& errorString = QString()) { emit error(m_type, errorString); }

signals:
    void error(QGCMapTask::TaskType type, const QString& errorString);

private:
    const TaskType m_type = TaskType::taskInit;
};
