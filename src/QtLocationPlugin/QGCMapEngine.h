/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCMapEngineLog)

class QGCMapTask;
class QGCCacheWorker;

class QGCMapEngine : public QObject
{
    Q_OBJECT

public:
    QGCMapEngine(QObject *parent = nullptr);
    ~QGCMapEngine();

    void init(const QString &databasePath);
    bool addTask(QGCMapTask *task);

    static QGCMapEngine *instance();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _pruned() { m_prunning = false; }

private:
    QGCCacheWorker *m_worker = nullptr;
    bool m_prunning = false;
};

extern QGCMapEngine *getQGCMapEngine();
