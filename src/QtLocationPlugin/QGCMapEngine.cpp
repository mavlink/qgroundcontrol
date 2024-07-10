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
#include "QGCMapEngine.h"
#include "QGCMapTileSet.h"
#include "QGCTileCacheWorker.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGCMapEngineData.h"
#include "QGCApplication.h"
#include <QGCLoggingCategory.h>

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>

#define CACHE_PATH_VERSION "300"

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "qgc.qtlocationplugin.qgcmapengine")

Q_DECLARE_METATYPE(QList<QGCTile*>)

Q_APPLICATION_STATIC(QGCMapEngine, s_mapEngine);

QGCMapEngine* getQGCMapEngine()
{
    return QGCMapEngine::instance();
}

QGCMapEngine* QGCMapEngine::instance()
{
    return s_mapEngine();
}

QGCMapEngine::QGCMapEngine(QObject *parent)
    : QObject(parent)
    , m_worker(new QGCCacheWorker(this))
{
    // qCDebug(QGCMapEngineLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QGCMapTask::TaskType>();
    (void) qRegisterMetaType<QGCTile>();
    (void) qRegisterMetaType<QList<QGCTile*>>();

    (void) connect(m_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);
}

QGCMapEngine::~QGCMapEngine()
{
    (void) disconnect(m_worker);
    m_worker->quit();
    m_worker->wait();

    // qCDebug(QGCMapEngineLog) << Q_FUNC_INFO << this;
}

void QGCMapEngine::init()
{
    _wipeOldCaches();

    // QString cacheDir = QAbstractGeoTileCache::baseCacheDirectory()
#ifdef __mobile__
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
#endif
    cacheDir += QStringLiteral("/QGCMapCache" CACHE_PATH_VERSION);
    if (!QDir::root().mkpath(cacheDir)) {
        qCWarning(QGCMapEngineLog) << "Could not create mapping disk cache directory:" << cacheDir;

        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if (!QDir::root().mkpath(cacheDir)) {
            qCWarning(QGCMapEngineLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir.clear();
        }
    }

    m_cachePath = cacheDir;
    if (!m_cachePath.isEmpty()) {
        const QString databaseFilePath(m_cachePath + "/" + QGeoFileTileCacheQGC::getCacheFilename());
        m_worker->setDatabaseFile(databaseFilePath);

        qCDebug(QGCMapEngineLog) << "Map Cache in:" << databaseFilePath;
    } else {
        qCCritical(QGCMapEngineLog) << "Could not find suitable map cache directory.";
    }

    QGCMapTask* const task = new QGCMapTask(QGCMapTask::taskInit);
    (void) addTask(task);

    if (m_cacheWasReset) {
        qgcApp()->showAppMessage(tr(
            "The Offline Map Cache database has been upgraded. "
            "Your old map cache sets have been reset."));
    }
}

bool QGCMapEngine::addTask(QGCMapTask *task)
{
    return m_worker->enqueueTask(task);
}

bool QGCMapEngine::_wipeDirectory(const QString &dirPath)
{
    bool result = true;

    const QDir dir(dirPath);
    if (dir.exists(dirPath)) {
        m_cacheWasReset = true;

        const QFileInfoList fileList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
        for (const QFileInfo &info : fileList) {
            if (info.isDir()) {
                result = _wipeDirectory(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirPath);
    }

    return result;
}

void QGCMapEngine::_wipeOldCaches()
{
    const QStringList oldCaches = {"/QGCMapCache55", "/QGCMapCache100"};
    for (const QString &cache : oldCaches) {
        QString oldCacheDir;
        #ifdef __mobile__
            oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        #else
            oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
        #endif
        oldCacheDir += cache;
        _wipeDirectory(oldCacheDir);
    }
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
