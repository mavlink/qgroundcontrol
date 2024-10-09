/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GPSManager.h"
#include "GPSRtk.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>

QGC_LOGGING_CATEGORY(GPSManagerLog, "qgc.gps.gpsmanager")

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
