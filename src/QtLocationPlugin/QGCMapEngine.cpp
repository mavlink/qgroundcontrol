#include "QGCMapEngine.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QSharedPointer>

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCTile.h"
#include "QGCTileCache.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileSet.h"

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "QtLocationPlugin.QGCMapEngine")

Q_APPLICATION_STATIC(QGCMapEngine, _mapEngine);

QGCMapEngine* getQGCMapEngine()
{
    return QGCMapEngine::instance();
}

QGCMapEngine::QGCMapEngine(QObject* parent) : QObject(parent)
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
    (void) qRegisterMetaType<QSharedPointer<QGCCacheTile>>("QSharedPointer<QGCCacheTile>");
}

QGCMapEngine::~QGCMapEngine()
{
    if (m_worker) {
        (void) disconnect(m_worker);
        // ~QGCCacheWorker() runs stop() + _thread.wait().
        delete m_worker;
        m_worker = nullptr;
    }

    qCDebug(QGCMapEngineLog) << this;
}

QGCMapEngine* QGCMapEngine::instance()
{
    return _mapEngine();
}

void QGCMapEngine::init(const QString& databasePath)
{
    if (m_initialized) {
        return;
    }

    m_initialized = true;

    m_worker = new QGCCacheWorker();
    m_worker->setDatabaseFile(databasePath);
    (void) connect(m_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);

    QGCMapTask* task = new QGCMapTask(QGCMapTask::TaskType::taskInit);
    if (!addTask(task)) {
        task->deleteLater();
        m_initialized = false;
    }
}

bool QGCMapEngine::addTask(QGCMapTask* task)
{
    // DirectConnection: enqueueTask must run synchronously to return its result and
    // start the worker; its queue is mutex-protected, so main-thread calls are safe.
    bool result = false;
    (void) QMetaObject::invokeMethod(m_worker, &QGCCacheWorker::enqueueTask, Qt::DirectConnection, qReturnArg(result),
                                     task);
    return result;
}

void QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint64 maxSize = static_cast<quint64>(QGCTileCache::getMaxDiskCacheSetting()) * qPow(1024, 2);
    if (!m_pruning && (defaultsize > maxSize)) {
        m_pruning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        QGCCommandTask* task = new QGCCommandTask(QGCMapTask::TaskType::taskPruneCache,
                                                  [amountToPrune](QGCCacheWorker& worker, QGCMapTask& self) {
                                                      if (!worker.validateDatabase(&self)) {
                                                          return false;
                                                      }
                                                      if (!worker.database()->pruneCache(amountToPrune)) {
                                                          self.setError("Error pruning cache");
                                                          return false;
                                                      }
                                                      return true;
                                                  });
        (void) connect(task, &QGCCommandTask::completed, this, &QGCMapEngine::_pruned);
        if (!addTask(task)) {
            task->deleteLater();
            m_pruning = false;
        }
    }
}
