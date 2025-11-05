/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMapEngine.h"

#include <QtCore/QApplicationStatic>

#include "QGCCachedTileSet.h"
#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCTile.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileSet.h"
#include "QGeoFileTileCacheQGC.h"

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "QtLocationPlugin.QGCMapEngine")

Q_APPLICATION_STATIC(QGCMapEngine, _mapEngine);

QGCMapEngine *getQGCMapEngine()
{
    return QGCMapEngine::instance();
}

QGCMapEngine::QGCMapEngine(QObject *parent)
    : QObject(parent)
{
    qCDebug(QGCMapEngineLog) << this;

    (void) qRegisterMetaType<QGCMapTask::TaskType>("QGCMapTask::TaskType");
    (void) qRegisterMetaType<QGCTile>("QGCTile");
    (void) qRegisterMetaType<QGCTile*>("QGCTile*");
    (void) qRegisterMetaType<QList<QGCTile*>>("QList<QGCTile*>");
    (void) qRegisterMetaType<QGCTileSet>("QGCTileSet");
    (void) qRegisterMetaType<QGCTileSet*>("QGCTileSet*");
    (void) qRegisterMetaType<QGCCacheTile>("QGCCacheTile");
    (void) qRegisterMetaType<QGCCacheTile*>("QGCCacheTile*");
}

QGCMapEngine::~QGCMapEngine()
{
    if (m_initialized && m_worker) {
        (void) disconnect(m_worker);
        m_worker->stop();
        (void) m_worker->wait();
    }

    qCDebug(QGCMapEngineLog) << this;
}

QGCMapEngine *QGCMapEngine::instance()
{
    return _mapEngine();
}

void QGCMapEngine::init(const QString &databasePath)
{
    if (m_initialized) {
        return;
    }

    m_initialized = true;

    m_worker = new QGCCacheWorker(this);
    m_worker->setDatabaseFile(databasePath);
    (void) connect(m_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);

    QGCMapTask *task = new QGCMapTask(QGCMapTask::TaskType::taskInit);
    if (!addTask(task)) {
        task->deleteLater();
        m_initialized = false;
    }
}

bool QGCMapEngine::addTask(QGCMapTask *task)
{
    bool result = false;
    (void) QMetaObject::invokeMethod(m_worker, &QGCCacheWorker::enqueueTask, Qt::DirectConnection, qReturnArg(result), task);
    return result;
}

void QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint64 maxSize = static_cast<quint64>(QGeoFileTileCacheQGC::getMaxDiskCacheSetting()) * qPow(1024, 2);
    if (!m_pruning && (defaultsize > maxSize)) {
        m_pruning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        QGCPruneCacheTask *task = new QGCPruneCacheTask(amountToPrune);
        (void) connect(task, &QGCPruneCacheTask::pruned, this, &QGCMapEngine::_pruned);
        if (!addTask(task)) {
            task->deleteLater();
            m_pruning = false;
        }
    }
}
