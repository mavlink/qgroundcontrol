#include "GPSManager.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject *parent)
    : QObject(parent)
    , _gpsRtk(new GPSRtk(this))
{
    // qCDebug(GPSManagerLog) << Q_FUNC_INFO << this;
}

GPSManager::~GPSManager()
{
    // qCDebug(GPSManagerLog) << Q_FUNC_INFO << this;
}

GPSManager *GPSManager::instance()
{
    return _gpsManager();
}
