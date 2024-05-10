/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

class QGCCacheWorker;
class QGCMapTask;

class QGCMapEngine : public QObject
{
    Q_OBJECT

public:
    QGCMapEngine(QObject *parent = nullptr);
    ~QGCMapEngine();

    static QGCMapEngine* instance();

    void init();
    bool addTask(QGCMapTask *task);

    QString getCachePath() const { return m_cachePath; }
    bool wasCacheReset() const { return m_cacheWasReset; }
signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private slots:
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _pruned() { m_prunning = false; }

private:
    void _wipeOldCaches();
    bool _wipeDirectory(const QString& dirPath);

    QGCCacheWorker* m_worker = nullptr;
    bool m_prunning = false;
    bool m_cacheWasReset = false;
    QString m_cachePath;
};

QGCMapEngine* getQGCMapEngine();
