#include "QGCMapEngine.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkAccessManager>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "MapProvider.h"
#include "MapsSettings.h"
#include "QGCCacheTile.h"
#include "QGCCacheWorker.h"
#include "QGCCachedTileSet.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCNetworkHelper.h"
#include "QGCTile.h"
#include "QGCTileSet.h"
#include "SettingsManager.h"
#include "UrlFactory.h"

QGC_LOGGING_CATEGORY(QGCMapEngineLog, "MapTiles.QGCMapEngine")

Q_APPLICATION_STATIC(QGCMapEngine, _mapEngine);

namespace {

bool wipeCacheDirectory(const QString& dirPath, bool& cacheWasReset)
{
    bool result = true;

    const QDir dir(dirPath);
    if (dir.exists(dirPath)) {
        cacheWasReset = true;

        const QFileInfoList fileList = dir.entryInfoList(
            QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
        for (const QFileInfo& info : fileList) {
            if (info.isDir()) {
                result = wipeCacheDirectory(info.absoluteFilePath(), cacheWasReset);
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

/// Removes the caches of earlier database versions. Returns true when one existed.
bool wipeOldCaches()
{
    bool cacheWasReset = false;
    const QStringList oldCaches = {"/QGCMapCache55", "/QGCMapCache100", "/QGCMapCache300"};
    for (const QString& cache : oldCaches) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        QString oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
        QString oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
#endif
        oldCacheDir += cache;
        (void) wipeCacheDirectory(oldCacheDir, cacheWasReset);
    }
    return cacheWasReset;
}

QString createCacheDirectory()
{
    const bool cacheWasReset = wipeOldCaches();

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
    cacheDir += QStringLiteral("/QGCMapCache");
    if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
        qCWarning(QGCMapEngineLog) << "Could not create mapping disk cache directory:" << cacheDir;

        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if (!QGCFileHelper::ensureDirectoryExists(cacheDir)) {
            qCWarning(QGCMapEngineLog) << "Could not create mapping disk cache directory:" << cacheDir;
            cacheDir.clear();
        }
    }

    if (cacheDir.isEmpty()) {
        qCCritical(QGCMapEngineLog) << "Could not find suitable map cache directory.";
    } else {
        qCDebug(QGCMapEngineLog) << "Map Cache in:" << cacheDir;
    }

    if (cacheWasReset) {
        // The context is the class this message moved from, so its existing translations still apply
        QGC::showAppMessage(QCoreApplication::translate("QGeoFileTileCacheQGC",
                                                        "The Offline Map Cache database has been upgraded. "
                                                        "Your old map cache sets have been reset."));
    }

    return cacheDir;
}

}  // namespace

QGCMapEngine::QGCMapEngine(QObject* parent)
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

    for (const SharedMapProvider& provider : UrlFactory::getProviders()) {
        const int mapId = provider->getMapId();
        for (const Fact* const setting : provider->sourceSettings()) {
            (void) connect(setting, &Fact::rawValueChanged, this, [this, mapId] { emit tileSourceChanged(mapId); });
        }
    }
}

QGCMapEngine::~QGCMapEngine()
{
    shutdown();

    qCDebug(QGCMapEngineLog) << this;
}

void QGCMapEngine::shutdown()
{
    // Tear down whenever a worker exists, even if init() failed partway and
    // cleared m_initialized - otherwise the worker thread would leak.
    if (m_worker) {
        (void) disconnect(m_worker);
        m_worker->stop();
        (void) m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }
    m_initialized = false;
}

void QGCMapEngine::close()
{
    m_closed = true;
    shutdown();
}

void QGCMapEngine::closeInstance()
{
    // The application static is destroyed only after QCoreApplication, too late for the worker's
    // database teardown; don't create the engine just to close it.
    if (_mapEngine.exists()) {
        _mapEngine()->close();
    }
}

QGCMapEngine* QGCMapEngine::instance()
{
    return _mapEngine();
}

QNetworkAccessManager* QGCMapEngine::networkManager()
{
    if (m_testNetworkManager) {
        return m_testNetworkManager;
    }

    if (!m_networkManager) {
        m_networkManager = QGCNetworkHelper::createNetworkManager(this);
    }
    return m_networkManager;
}

void QGCMapEngine::setNetworkManagerForTest(QNetworkAccessManager* networkManager)
{
    m_testNetworkManager = networkManager;
}

void QGCMapEngine::init(const QString& databasePath)
{
    if (m_initialized || m_closed) {
        return;
    }

    m_initialized = true;

    m_worker = new QGCCacheWorker(this);
    m_worker->setDatabaseFile(databasePath);
    m_worker->setCacheVariants(UrlFactory::cacheVariants());
    (void) connect(m_worker, &QGCCacheWorker::updateTotals, this, &QGCMapEngine::_updateTotals);

    QGCMapTask* task = new QGCMapTask(QGCMapTask::TaskType::taskInit);
    if (!addTask(task)) {
        task->deleteLater();
        m_initialized = false;
    }
}

void QGCMapEngine::ensureInitialized()
{
    if (m_initialized || m_closed) {
        return;
    }

    init(databaseFilePath());
}

QString QGCMapEngine::cachePath()
{
    static const QString path = createCacheDirectory();
    return path;
}

QString QGCMapEngine::databaseFilePath()
{
    const QString path = cachePath();
    return path.isEmpty() ? QString() : (path + QStringLiteral("/qgcMapCache.db"));
}

void QGCMapEngine::cacheTile(const QString& type, const QString& hash, const QByteArray& image, const QString& format,
                             quint64 tileSet)
{
    if (SettingsManager::instance()->appSettings()->disableAllPersistence()->rawValue().toBool()) {
        return;
    }

    QGCSaveTileTask* const task = new QGCSaveTileTask(new QGCCacheTile(hash, image, format, type, tileSet));
    if (!addTask(task)) {
        task->deleteLater();
    }
}

bool QGCMapEngine::addTask(QGCMapTask* task)
{
    if (!m_worker) {
        return false;
    }

    // DirectConnection is intentional: the worker thread uses a custom loop (not
    // an event loop), so queued connections would never be delivered. The queue
    // is mutex-protected in enqueueTask, so calling from the main thread is safe.
    bool result = false;
    (void) QMetaObject::invokeMethod(m_worker, &QGCCacheWorker::enqueueTask, Qt::DirectConnection, qReturnArg(result),
                                     task);
    return result;
}

void QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);

    const quint32 maxDiskCacheMB = SettingsManager::instance()->mapsSettings()->maxCacheDiskSize()->rawValue().toUInt();
    const quint64 maxSize = static_cast<quint64>(maxDiskCacheMB) * qPow(1024, 2);
    if (!m_pruning && (defaultsize > maxSize)) {
        m_pruning = true;

        const quint64 amountToPrune = defaultsize - maxSize;
        QGCPruneCacheTask* task = new QGCPruneCacheTask(amountToPrune);
        (void) connect(task, &QGCPruneCacheTask::pruned, this, &QGCMapEngine::_pruned);
        if (!addTask(task)) {
            task->deleteLater();
            m_pruning = false;
        }
    }
}
