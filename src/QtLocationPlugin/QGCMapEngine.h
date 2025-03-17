/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(QGCMapEngineLog)

class QGCCacheWorker;
class QGCMapTask;
class QThread;

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

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _pruned() { _prunning = false; }

private:
    QGCCacheWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
    bool _prunning = false;
};

extern QGCMapEngine *getQGCMapEngine();
