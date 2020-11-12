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
#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <math.h>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <stdio.h>

#include "QGCMapEngine.h"
#include "QGCMapTileSet.h"

Q_DECLARE_METATYPE(QGCMapTask::TaskType)
Q_DECLARE_METATYPE(QGCTile)
Q_DECLARE_METATYPE(QList<QGCTile*>)

static const char* kDbFileName = "qgcMapCache.db";
static QLocale kLocale;

#define CACHE_PATH_VERSION  "300"

struct stQGeoTileCacheQGCMapTypes {
    const char* name;
    QString type;
};

static const char* kMaxDiskCacheKey = "MaxDiskCache";
static const char* kMaxMemCacheKey  = "MaxMemoryCache";

//-----------------------------------------------------------------------------
// Singleton
static QGCMapEngine* kMapEngine = nullptr;
QGCMapEngine*
getQGCMapEngine()
{
    if(!kMapEngine)
        kMapEngine = new QGCMapEngine();
    return kMapEngine;
}

//-----------------------------------------------------------------------------
void
destroyMapEngine()
{
    delete kMapEngine;
    kMapEngine = nullptr;
}

//-----------------------------------------------------------------------------
QGCMapEngine::QGCMapEngine()
    : _urlFactory(new UrlFactory())
#ifdef WE_ARE_KOSHER
    //-- TODO: Get proper version
    #if defined Q_OS_MAC
        , _userAgent("QGroundControl (Macintosh; Intel Mac OS X 10_9_3) AppleWebKit/537.75.14 (KHTML, like Gecko) Version/2.9.0")
    #elif defined Q_OS_WIN32
        , _userAgent("QGroundControl (Windows; Windows NT 6.0) (KHTML, like Gecko) Version/2.9.0")
    #else
        , _userAgent("QGroundControl (X11; Ubuntu; Linux x86_64) (KHTML, like Gecko) Version/2.9.0")
    #endif
#else
    #if defined Q_OS_MAC
        , _userAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.6; rv:25.0) Gecko/20100101 Firefox/25.0")
    #elif defined Q_OS_WIN32
        , _userAgent("Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.0) Gecko/20130401 Firefox/31.0")
    #else
        , _userAgent("Mozilla/5.0 (X11; Linux i586; rv:31.0) Gecko/20100101 Firefox/31.0")
    #endif
#endif
    , _maxDiskCache(0)
    , _maxMemCache(0)
    , _prunning(false)
    , _cacheWasReset(false)
    , _isInternetActive(false)
{
    qRegisterMetaType<QGCMapTask::TaskType>();
    qRegisterMetaType<QGCTile>();
    qRegisterMetaType<QList<QGCTile*>>();
    connect(&_worker, &QGCCacheWorker::updateTotals,   this, &QGCMapEngine::_updateTotals);
    connect(&_worker, &QGCCacheWorker::internetStatus, this, &QGCMapEngine::_internetStatus);
}

//-----------------------------------------------------------------------------
QGCMapEngine::~QGCMapEngine()
{
    _worker.quit();
    _worker.wait();
    delete _urlFactory;
    _urlFactory = nullptr;
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::_checkWipeDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    if (dir.exists(dirPath)) {
        _cacheWasReset = true;
        _wipeDirectory(dirPath);
    }
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::_wipeOldCaches()
{
    QString oldCacheDir;
#ifdef __mobile__
    oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)      + QLatin1String("/QGCMapCache55");
#else
    oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/QGCMapCache55");
#endif
    _checkWipeDirectory(oldCacheDir);
#ifdef __mobile__
    oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)      + QLatin1String("/QGCMapCache100");
#else
    oldCacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/QGCMapCache100");
#endif
    _checkWipeDirectory(oldCacheDir);
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::init()
{
    //-- Delete old style caches (if present)
    _wipeOldCaches();
    //-- Figure out cache path
#ifdef __mobile__
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)      + QLatin1String("/QGCMapCache" CACHE_PATH_VERSION);
#else
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/QGCMapCache" CACHE_PATH_VERSION);
#endif
    if(!QDir::root().mkpath(cacheDir)) {
        qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
        cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
        if(!QDir::root().mkpath(cacheDir)) {
            qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
            cacheDir.clear();
        }
    }
    _cachePath = cacheDir;
    if(!_cachePath.isEmpty()) {
        _cacheFile = kDbFileName;
        _worker.setDatabaseFile(_cachePath + "/" + _cacheFile);
        qDebug() << "Map Cache in:" << _cachePath << "/" << _cacheFile;
    } else {
        qCritical() << "Could not find suitable map cache directory.";
    }
    QGCMapTask* task = new QGCMapTask(QGCMapTask::taskInit);
    _worker.enqueueTask(task);
}

//-----------------------------------------------------------------------------
bool
QGCMapEngine::_wipeDirectory(const QString& dirPath)
{
    bool result = true;
    QDir dir(dirPath);
    if (dir.exists(dirPath)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
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

//-----------------------------------------------------------------------------
void
QGCMapEngine::addTask(QGCMapTask* task)
{
    _worker.enqueueTask(task);
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::cacheTile(QString type, int x, int y, int z, const QByteArray& image, const QString &format, qulonglong set)
{
    QString hash = getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set);
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::cacheTile(QString type, const QString& hash, const QByteArray& image, const QString& format, qulonglong set)
{
    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    //-- If we are allowed to persist data, save tile to cache
    if(!appSettings->disableAllPersistence()->rawValue().toBool()) {
        QGCSaveTileTask* task = new QGCSaveTileTask(new QGCCacheTile(hash, image, format, type, set));
        _worker.enqueueTask(task);
    }
}

//-----------------------------------------------------------------------------
QString
QGCMapEngine::getTileHash(QString type, int x, int y, int z)
{
    return QString::asprintf("%010d%08d%08d%03d", getQGCMapEngine()->urlFactory()->getIdFromType(type), x, y, z);
}

//-----------------------------------------------------------------------------
QString
QGCMapEngine::hashToType(const QString& hash)
{
    QString type = hash.mid(0,10);
    return urlFactory()->getTypeFromId(type.toInt());
}

//-----------------------------------------------------------------------------
	QGCFetchTileTask*
QGCMapEngine::createFetchTileTask(QString type, int x, int y, int z)
{
	QString hash = getTileHash(type, x, y, z);
	QGCFetchTileTask* task = new QGCFetchTileTask(hash);
	return task;
}

//-----------------------------------------------------------------------------
	QGCTileSet
QGCMapEngine::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, QString mapType)
{
	if(zoom <  1) zoom = 1;
	if(zoom > MAX_MAP_ZOOM) zoom = MAX_MAP_ZOOM;

    return getQGCMapEngine()->urlFactory()->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat, mapType);
}


//-----------------------------------------------------------------------------
QStringList
QGCMapEngine::getMapNameList()
{
    return QStringList(getQGCMapEngine()->urlFactory()->getProviderTable().keys());
}

//-----------------------------------------------------------------------------
quint32
QGCMapEngine::getMaxDiskCache()
{
    if(!_maxDiskCache) {
        QSettings settings;
        _maxDiskCache = settings.value(kMaxDiskCacheKey, 1024).toUInt();
    }
    return _maxDiskCache;
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::setMaxDiskCache(quint32 size)
{
    QSettings settings;
    settings.setValue(kMaxDiskCacheKey, size);
    _maxDiskCache = size;
}

//-----------------------------------------------------------------------------
quint32
QGCMapEngine::getMaxMemCache()
{
    if(!_maxMemCache) {
        QSettings settings;
#ifdef __mobile__
        _maxMemCache = settings.value(kMaxMemCacheKey, 16).toUInt();
#else
        _maxMemCache = settings.value(kMaxMemCacheKey, 128).toUInt();
#endif
    }
    //-- Size in MB
    if(_maxMemCache > 1024)
        _maxMemCache = 1024;
    return _maxMemCache;
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::setMaxMemCache(quint32 size)
{
    //-- Size in MB
    if(size > 1024)
        size = 1024;
    QSettings settings;
    settings.setValue(kMaxMemCacheKey, size);
    _maxMemCache = size;
}

//-----------------------------------------------------------------------------
QString
QGCMapEngine::bigSizeToString(quint64 size)
{
    if(size < 1024)
        return kLocale.toString(size);
    else if(size < 1024 * 1024)
        return kLocale.toString(static_cast<double>(size) / 1024.0, 'f', 1) + "kB";
    else if(size < 1024 * 1024 * 1024)
        return kLocale.toString(static_cast<double>(size) / (1024.0 * 1024.0), 'f', 1) + "MB";
    else if(size < 1024.0 * 1024.0 * 1024.0 * 1024.0)
        return kLocale.toString(static_cast<double>(size) / (1024.0 * 1024.0 * 1024.0), 'f', 1) + "GB";
    else
        return kLocale.toString(static_cast<double>(size) / (1024.0 * 1024.0 * 1024.0 * 1024), 'f', 1) + "TB";
}

//-----------------------------------------------------------------------------
QString
QGCMapEngine::storageFreeSizeToString(quint64 size_MB)
{
    if(size_MB < 1024)
        return kLocale.toString(static_cast<double>(size_MB) , 'f', 0) + " MB";
    else if(size_MB < 1024.0 * 1024.0)
        return kLocale.toString(static_cast<double>(size_MB) / (1024.0), 'f', 2) + " GB";
    else
        return kLocale.toString(static_cast<double>(size_MB) / (1024.0 * 1024), 'f', 2) + " TB";
}

//-----------------------------------------------------------------------------
QString
QGCMapEngine::numberToString(quint64 number)
{
    return kLocale.toString(number);
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::_updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize)
{
    emit updateTotals(totaltiles, totalsize, defaulttiles, defaultsize);
    quint64 maxSize = static_cast<quint64>(getMaxDiskCache()) * 1024L * 1024L;
    if(!_prunning && defaultsize > maxSize) {
        //-- Prune Disk Cache
        _prunning = true;
        QGCPruneCacheTask* task = new QGCPruneCacheTask(defaultsize - maxSize);
        connect(task, &QGCPruneCacheTask::pruned, this, &QGCMapEngine::_pruned);
        getQGCMapEngine()->addTask(task);
    }
}
//-----------------------------------------------------------------------------
void
QGCMapEngine::_pruned()
{
    _prunning = false;
}

//-----------------------------------------------------------------------------
int
QGCMapEngine::concurrentDownloads(QString type)
{
    Q_UNUSED(type);
    // TODO : We may want different values depending on
    // the provider here, let it like this as all provider are set to 12
    // at the moment
    return 12;
}

//-----------------------------------------------------------------------------
QGCCreateTileSetTask::~QGCCreateTileSetTask()
{
    //-- If not sent out, delete it
    if(!_saved && _tileSet)
        delete _tileSet;
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::testInternet()
{
    if(qgcApp()->toolbox()->settingsManager()->appSettings()->checkInternet()->rawValue().toBool())
        getQGCMapEngine()->addTask(new QGCTestInternetTask());
    else
        _internetStatus(true);
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::_internetStatus(bool active)
{
    if(_isInternetActive != active) {
        _isInternetActive = active;
        emit internetUpdated();
    }
}

// Resolution math: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Resolution_and_Scale
