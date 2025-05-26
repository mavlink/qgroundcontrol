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

#include "QGCMapEngine.h"
#include "QGCCachedTileSet.h"
#include "QGCTileCacheWorker.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGCMapTasks.h"
#include "QGCTileSet.h"
#include "QGCTile.h"
#include "QGCCacheTile.h"
#include <QGCLoggingCategory.h>

#include <QtCore/qapplicationstatic.h>

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "qgc.qtlocationplugin.qgcmapengine")

Q_DECLARE_METATYPE(QList<QGCTile*>)

Q_APPLICATION_STATIC(QGCMapEngine, _mapEngine);

QGCMapEngine *getQGCMapEngine()
{
    return QGCMapEngine::instance();
}

QGCMapEngine::QGCMapEngine(QObject *parent)
    : QObject(parent)
    , m_worker(new QGCCacheWorker(this))
{
    // qCDebug(QGCMapEngineLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QGCMapTask::TaskType>("TaskType");
    (void) qRegisterMetaType<QGCTile>("QGCTile");
    (void) qRegisterMetaType<QList<QGCTile*>>("QList<QGCTile*>");
    (void) qRegisterMetaType<QGCTileSet>("QGCTileSet");
    (void) qRegisterMetaType<QGCCacheTile>("QGCCacheTile");

    (void) connect(m_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);
}

QGCMapEngine::~QGCMapEngine()
{
    (void) disconnect(m_worker);
    m_worker->stop();
    m_worker->wait();

    // qCDebug(QGCMapEngineLog) << Q_FUNC_INFO << this;
}

QGCMapEngine *QGCMapEngine::instance()
{
    return _mapEngine();
}

void QGCMapEngine::init(const QString &databasePath)
{
    m_worker->setDatabaseFile(databasePath);

    QGCMapTask* const task = new QGCMapTask(QGCMapTask::taskInit);
    (void) addTask(task);
}

bool QGCMapEngine::addTask(QGCMapTask *task)
{
    return m_worker->enqueueTask(task);
}

void QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint64 maxSize = static_cast<quint64>(QGeoFileTileCacheQGC::getMaxDiskCacheSetting()) * pow(1024, 2);
    if (!m_prunning && (defaultsize > maxSize)) {
        m_prunning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        QGCPruneCacheTask* const task = new QGCPruneCacheTask(amountToPrune);
        (void) connect(task, &QGCPruneCacheTask::pruned, this, &QGCMapEngine::_pruned);
        (void) addTask(task);
    }
}
