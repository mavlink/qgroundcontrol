#pragma once

#include <QtCore/QFuture>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>
#include <QtQmlIntegration/QtQmlIntegration>
#include <atomic>
#include <vector>

#include "LogEntry.h"

class QSqlQuery;
class QThread;

class LogStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged)
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY databasePathChanged)
    Q_PROPERTY(qint64 entryCount READ entryCount NOTIFY entryCountChanged)
    Q_PROPERTY(QString sessionId READ sessionId CONSTANT)

public:
    explicit LogStore(QObject* parent = nullptr);
    ~LogStore() override;

    void open(const QString& dbPath);
    void close();

    bool isOpen() const { return _isOpen.load(std::memory_order_relaxed); }

    QString databasePath() const;

    qint64 entryCount() const { return _entryCount.load(std::memory_order_relaxed); }

    QString sessionId() const { return _sessionId; }

    void append(LogEntry entry);
    void flush();

    struct QueryParams
    {
        QString sessionId;
        QDateTime fromTime;
        QDateTime toTime;
        int minLevel = LogEntry::Debug;
        QString category;
        QString textFilter;
        int limit = 10000;
        int offset = 0;
    };

    Q_INVOKABLE QList<LogEntry> query(const QueryParams& params) const;
    Q_INVOKABLE QStringList sessions() const;
    Q_INVOKABLE qint64 sessionEntryCount(const QString& sessionId) const;
    Q_INVOKABLE bool deleteSession(const QString& sessionId);
    Q_INVOKABLE void exportSession(const QString& sessionId, const QString& destFile, int format = 0);

signals:
    void isOpenChanged();
    void databasePathChanged();
    void entryCountChanged();
    void errorOccurred(const QString& message);
    void exportFinished(bool success);

private:
    class ScopedReadConnection;

    void _workerLoop();
    void _startWorker();
    void _stopWorker();
    static void _bindAndExec(QSqlQuery& q, const QString& sessionId, const LogEntry& entry);

    mutable QMutex _mutex;
    QWaitCondition _condition;
    std::vector<LogEntry> _pendingWrites;

    QString _dbPath;
    const QString _sessionId;
    QThread* _thread = nullptr;

    QString _writeConnName;

    QFuture<void> _exportFuture;
    std::atomic<bool> _isOpen{false};
    std::atomic<bool> _quit{false};
    std::atomic<qint64> _entryCount{0};
    static constexpr int kBatchSize = 500;
    static constexpr int kFlushIntervalMs = 2000;
};
