#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(QGCMapEngineLog)

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

    static QGCMapEngine *instance();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void downloadStatusUpdated(quint64 setID, quint32 pending, quint32 downloading, quint32 errors);

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _pruned() { m_pruning = false; }
    void _downloadStatus(quint64 setID, quint32 pending, quint32 downloading, quint32 errors);

private:
    QGCCacheWorker *m_worker = nullptr;
    bool m_pruning = false;
    std::atomic<bool> m_initialized = false;
};

extern QGCMapEngine *getQGCMapEngine();
