#include "QGeoFileTileCacheQGC.h"

#include <QtCore/QDir>
#include <QtCore/QtMinMax>

#include "MapsSettings.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog, "QtLocationPlugin.QGeoFileTileCacheQGC")

QGeoFileTileCacheQGC::QGeoFileTileCacheQGC(QObject* parent)
    : QGeoFileTileCache(_directory(), parent)
{
    qCDebug(QGeoFileTileCacheQGCLog) << this;

    setMaxDiskUsage(0);
    setMaxMemoryUsage(_memoryLimit());
}

QGeoFileTileCacheQGC::~QGeoFileTileCacheQGC()
{
    if (QGeoFileTileCacheQGCLog().isDebugEnabled()) {
        printStats();
    }

    qCDebug(QGeoFileTileCacheQGCLog) << this;
}

QString QGeoFileTileCacheQGC::_directory()
{
    // QGeoFileTileCache::init() creates its directory and lists it, and uses one that other applications share when
    // it is given none. The directory stays empty, as no tile fits a disk usage of 0.
    const QString cachePath = QGCMapEngine::cachePath();
    return (cachePath.isEmpty() ? QDir::tempPath() + QLatin1String("/QGCMapCache") : cachePath) +
           QLatin1String("/providers");
}

int QGeoFileTileCacheQGC::_memoryLimit()
{
    constexpr quint32 kBytesPerMegabyte = 1024 * 1024;
    constexpr quint32 kDefaultMegabytes = 3;  // QtLocation's own limit
    constexpr quint32 kMaxMegabytes = 1024;   // QtLocation takes the limit as an int

    const quint32 megabytes = SettingsManager::instance()->mapsSettings()->maxCacheMemorySize()->rawValue().toUInt();
    return static_cast<int>(qMin((megabytes == 0) ? kDefaultMegabytes : megabytes, kMaxMegabytes) * kBytesPerMegabyte);
}
