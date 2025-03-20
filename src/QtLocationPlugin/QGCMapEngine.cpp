/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMapEngine.h"
#include "QGCCachedTileSet.h"
#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCTile.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileSet.h"
#include "QGeoFileTileCacheQGC.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QThread>
#include <QtCore/QtMath>

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "qgc.qtlocationplugin.qgcmapengine")

Q_DECLARE_METATYPE(QList<QGCTile*>)

Q_APPLICATION_STATIC(QGCMapEngine, _mapEngine);

QGCMapEngine *getQGCMapEngine()
{
    return QGCMapEngine::instance();
}

QGCMapEngine::QGCMapEngine(QObject *parent)
    : QObject(parent)
    , _worker(new QGCCacheWorker(nullptr))
    , _workerThread(new QThread(this))
{
    // qCDebug(QGCMapEngineLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QGCMapTask::TaskType>("TaskType");
    (void) qRegisterMetaType<QGCTile>("QGCTile");
    (void) qRegisterMetaType<QList<QGCTile*>>("QList<QGCTile*>");
    (void) qRegisterMetaType<QGCTileSet>("QGCTileSet");
    (void) qRegisterMetaType<QGCCacheTile>("QGCCacheTile");

    _workerThread->setObjectName(QStringLiteral("QGCTileCacheWorker"));

    (void) _worker->moveToThread(_workerThread);

    (void) connect(_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);

    _workerThread->start();
}

QGCMapEngine::~QGCMapEngine()
{
    (void) disconnect(_worker);
    _worker->stop();

    _workerThread->quit();
    (void) _workerThread->wait();

    delete _worker;

    // qCDebug(QGCMapEngineLog) << Q_FUNC_INFO << this;
}

QGCMapEngine *QGCMapEngine::instance()
{
    return _mapEngine();
}

void QGCMapEngine::init(const QString &databasePath)
{
    _worker->setDatabaseFile(databasePath);

    QGCMapTask *const task = new QGCMapTask(QGCMapTask::taskInit);
    (void) addTask(task);
}

bool QGCMapEngine::addTask(QGCMapTask *task)
{
    _worker->enqueueTask(task);
    return true;
}

void QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint64 maxSize = static_cast<quint64>(QGeoFileTileCacheQGC::getMaxDiskCacheSetting()) * qPow(1024, 2);
    if (!_prunning && (defaultsize > maxSize)) {
        _prunning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        QGCPruneCacheTask *const task = new QGCPruneCacheTask(amountToPrune);
        (void) connect(task, &QGCPruneCacheTask::pruned, this, &QGCMapEngine::_pruned);
        (void) addTask(task);
    }
}
