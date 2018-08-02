/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache
 *
 *   @author Gus Grubba <mavlink@grubba.com>
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
    UrlFactory::MapType type;
};

//-- IMPORTANT
//   Changes here must reflect those in QGeoTiledMappingManagerEngineQGC.cpp

stQGeoTileCacheQGCMapTypes kMapTypes[] = {
#ifndef QGC_LIMITED_MAPS
    {"Google Street Map",       UrlFactory::GoogleMap},
    {"Google Satellite Map",    UrlFactory::GoogleSatellite},
    {"Google Terrain Map",      UrlFactory::GoogleTerrain},
#endif
    {"Bing Street Map",         UrlFactory::BingMap},
    {"Bing Satellite Map",      UrlFactory::BingSatellite},
    {"Bing Hybrid Map",         UrlFactory::BingHybrid},
    {"Statkart Terrain Map",    UrlFactory::StatkartTopo},
    {"ENIRO Terrain Map",       UrlFactory::EniroTopo},

    {"VWorld Satellite Map",     UrlFactory::VWorldSatellite},
    {"VWorld Street Map",        UrlFactory::VWorldStreet}

    /*
    {"MapQuest Street Map",     UrlFactory::MapQuestMap},
    {"MapQuest Satellite Map",  UrlFactory::MapQuestSat}
    {"Open Street Map",         UrlFactory::OpenStreetMap}
     */
};

#define NUM_MAPS (sizeof(kMapTypes) / sizeof(stQGeoTileCacheQGCMapTypes))

stQGeoTileCacheQGCMapTypes kMapboxTypes[] = {
    {"Mapbox Street Map",       UrlFactory::MapboxStreets},
    {"Mapbox Satellite Map",    UrlFactory::MapboxSatellite},
    {"Mapbox High Contrast Map",UrlFactory::MapboxHighContrast},
    {"Mapbox Light Map",        UrlFactory::MapboxLight},
    {"Mapbox Dark Map",         UrlFactory::MapboxDark},
    {"Mapbox Hybrid Map",       UrlFactory::MapboxHybrid},
    {"Mapbox Wheat Paste Map",  UrlFactory::MapboxWheatPaste},
    {"Mapbox Streets Basic Map",UrlFactory::MapboxStreetsBasic},
    {"Mapbox Comic Map",        UrlFactory::MapboxComic},
    {"Mapbox Outdoors Map",     UrlFactory::MapboxOutdoors},
    {"Mapbox Run, Byke and Hike Map",   UrlFactory::MapboxRunBikeHike},
    {"Mapbox Pencil Map",       UrlFactory::MapboxPencil},
    {"Mapbox Pirates Map",      UrlFactory::MapboxPirates},
    {"Mapbox Emerald Map",      UrlFactory::MapboxEmerald}
};

#define NUM_MAPBOXMAPS (sizeof(kMapboxTypes) / sizeof(stQGeoTileCacheQGCMapTypes))

stQGeoTileCacheQGCMapTypes kEsriTypes[] = {
    {"Esri Street Map",       UrlFactory::EsriWorldStreet},
    {"Esri Satellite Map",    UrlFactory::EsriWorldSatellite},
    {"Esri Terrain Map",      UrlFactory::EsriTerrain}
};

#define NUM_ESRIMAPS (sizeof(kEsriTypes) / sizeof(stQGeoTileCacheQGCMapTypes))

stQGeoTileCacheQGCMapTypes kElevationTypes[] = {
    {"Airmap Elevation Data", UrlFactory::AirmapElevation}
};

#define NUM_ELEVMAPS (sizeof(kElevationTypes) / sizeof(stQGeoTileCacheQGCMapTypes))

static const char* kMaxDiskCacheKey = "MaxDiskCache";
static const char* kMaxMemCacheKey  = "MaxMemoryCache";

//-----------------------------------------------------------------------------
// Singleton
static QGCMapEngine* kMapEngine = NULL;
QGCMapEngine*
getQGCMapEngine()
{
    if(!kMapEngine)
        kMapEngine = new QGCMapEngine();
    return kMapEngine;
}

//-----------------------------------------------------------------------------
const double QGCMapEngine::srtm1TileSize = 0.01;

//-----------------------------------------------------------------------------
void
destroyMapEngine()
{
    if(kMapEngine) {
        delete kMapEngine;
        kMapEngine = NULL;
    }
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
    if(_urlFactory)
        delete _urlFactory;
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
QGCMapEngine::cacheTile(UrlFactory::MapType type, int x, int y, int z, const QByteArray& image, const QString &format, qulonglong set)
{
    QString hash = getTileHash(type, x, y, z);
    cacheTile(type, hash, image, format, set);
}

//-----------------------------------------------------------------------------
void
QGCMapEngine::cacheTile(UrlFactory::MapType type, const QString& hash, const QByteArray& image, const QString& format, qulonglong set)
{
    QGCSaveTileTask* task = new QGCSaveTileTask(new QGCCacheTile(hash, image, format, type, set));
    _worker.enqueueTask(task);
}

//-----------------------------------------------------------------------------
QString
QGCMapEngine::getTileHash(UrlFactory::MapType type, int x, int y, int z)
{
    return QString().sprintf("%04d%08d%08d%03d", (int)type, x, y, z);
}

//-----------------------------------------------------------------------------
UrlFactory::MapType
QGCMapEngine::hashToType(const QString& hash)
{
    QString type = hash.mid(0,4);
    return (UrlFactory::MapType)type.toInt();
}

//-----------------------------------------------------------------------------
QGCFetchTileTask*
QGCMapEngine::createFetchTileTask(UrlFactory::MapType type, int x, int y, int z)
{
    QString hash = getTileHash(type, x, y, z);
    QGCFetchTileTask* task = new QGCFetchTileTask(hash);
    return task;
}

//-----------------------------------------------------------------------------
QGCTileSet
QGCMapEngine::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, UrlFactory::MapType mapType)
{
    if(zoom <  1) zoom = 1;
    if(zoom > MAX_MAP_ZOOM) zoom = MAX_MAP_ZOOM;
    QGCTileSet set;
    if (mapType != UrlFactory::AirmapElevation) {
        set.tileX0 = long2tileX(topleftLon,     zoom);
        set.tileY0 = lat2tileY(topleftLat,      zoom);
        set.tileX1 = long2tileX(bottomRightLon, zoom);
        set.tileY1 = lat2tileY(bottomRightLat,  zoom);
    } else {
        set.tileX0 = long2elevationTileX(topleftLon,        zoom);
        set.tileY0 = lat2elevationTileY(bottomRightLat,     zoom);
        set.tileX1 = long2elevationTileX(bottomRightLon,    zoom);
        set.tileY1 = lat2elevationTileY(topleftLat,         zoom);
    }
    set.tileCount = (quint64)((quint64)set.tileX1 - (quint64)set.tileX0 + 1) * (quint64)((quint64)set.tileY1 - (quint64)set.tileY0 + 1);
    set.tileSize  = UrlFactory::averageSizeForType(mapType) * set.tileCount;
    return set;
}

//-----------------------------------------------------------------------------
int
QGCMapEngine::long2tileX(double lon, int z)
{
    return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

//-----------------------------------------------------------------------------
int
QGCMapEngine::lat2tileY(double lat, int z)
{
    return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

//-----------------------------------------------------------------------------
int
QGCMapEngine::long2elevationTileX(double lon, int z)
{
    Q_UNUSED(z);
    return (int)(floor((lon + 180.0) / srtm1TileSize));
}

//-----------------------------------------------------------------------------
int
QGCMapEngine::lat2elevationTileY(double lat, int z)
{
    Q_UNUSED(z);
    return (int)(floor((lat + 90.0) / srtm1TileSize));
}

//-----------------------------------------------------------------------------
UrlFactory::MapType
QGCMapEngine::getTypeFromName(const QString& name)
{
    size_t i;
    for(i = 0; i < NUM_MAPS; i++) {
        if(name.compare(kMapTypes[i].name, Qt::CaseInsensitive) == 0)
            return kMapTypes[i].type;
    }
    for(i = 0; i < NUM_MAPBOXMAPS; i++) {
        if(name.compare(kMapboxTypes[i].name, Qt::CaseInsensitive) == 0)
            return kMapboxTypes[i].type;
    }
    for(i = 0; i < NUM_ESRIMAPS; i++) {
        if(name.compare(kEsriTypes[i].name, Qt::CaseInsensitive) == 0)
            return kEsriTypes[i].type;
    }
    for(i = 0; i < NUM_ELEVMAPS; i++) {
        if(name.compare(kElevationTypes[i].name, Qt::CaseInsensitive) == 0)
            return kElevationTypes[i].type;
    }
    return UrlFactory::Invalid;
}

//-----------------------------------------------------------------------------
QStringList
QGCMapEngine::getMapNameList()
{
    QStringList mapList;
    for(size_t i = 0; i < NUM_MAPS; i++) {
        mapList << kMapTypes[i].name;
    }
    if(!qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxToken()->rawValue().toString().isEmpty()) {
        for(size_t i = 0; i < NUM_MAPBOXMAPS; i++) {
            mapList << kMapboxTypes[i].name;
        }
    }
    if(!qgcApp()->toolbox()->settingsManager()->appSettings()->esriToken()->rawValue().toString().isEmpty()) {
        for(size_t i = 0; i < NUM_ESRIMAPS; i++) {
            mapList << kEsriTypes[i].name;
        }
    }
    return mapList;
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
        return kLocale.toString((double)size / 1024.0, 'f', 1) + "kB";
    else if(size < 1024 * 1024 * 1024)
        return kLocale.toString((double)size / (1024.0 * 1024.0), 'f', 1) + "MB";
    else if(size < 1024.0 * 1024.0 * 1024.0 * 1024.0)
        return kLocale.toString((double)size / (1024.0 * 1024.0 * 1024.0), 'f', 1) + "GB";
    else
        return kLocale.toString((double)size / (1024.0 * 1024.0 * 1024.0 * 1024), 'f', 1) + "TB";
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
    quint64 maxSize = (quint64)getMaxDiskCache() * 1024L * 1024L;
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
QGCMapEngine::concurrentDownloads(UrlFactory::MapType type)
{
    switch(type) {
    case UrlFactory::GoogleMap:
    case UrlFactory::GoogleSatellite:
    case UrlFactory::GoogleTerrain:
    case UrlFactory::BingMap:
    case UrlFactory::BingSatellite:
    case UrlFactory::BingHybrid:
    case UrlFactory::StatkartTopo:
    case UrlFactory::EniroTopo:
    case UrlFactory::EsriWorldStreet:
    case UrlFactory::EsriWorldSatellite:
    case UrlFactory::EsriTerrain:
    case UrlFactory::AirmapElevation:
    case UrlFactory::VWorldMap:
    case UrlFactory::VWorldSatellite:
    case UrlFactory::VWorldStreet:
        return 12;
    /*
    case UrlFactory::MapQuestMap:
    case UrlFactory::MapQuestSat:
        return 8;
    */
    default:
        break;
    }
    return 6;
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
    getQGCMapEngine()->addTask(new QGCTestInternetTask());
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

