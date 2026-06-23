#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

class QGCMapTask : public QObject
{
    Q_OBJECT

public:
    enum class TaskType {
        taskInit,
        taskCacheTile,
        taskFetchTile,
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

    explicit QGCMapTask(TaskType type, QObject *parent = nullptr)
        : QObject(parent)
        , m_type(type)
    {}
    ~QGCMapTask() override = default;

    TaskType type() const { return m_type; }

    void setError(const QString &errorString = QString())
    {
        emit error(m_type, errorString);
    }

signals:
    void error(QGCMapTask::TaskType type, const QString &errorString);

private:
    const TaskType m_type = TaskType::taskInit;
};
