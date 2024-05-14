/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

Q_APPLICATION_STATIC(GPSManager, s_gpsManager);

GPSManager* GPSManager::instance()
{
    return s_gpsManager();
}

GPSManager::GPSManager(QObject* parent)
    : QObject(parent)
    , m_gpsRtk(new GPSRtk(this))
{
    qCDebug(GPSManagerLog) << Q_FUNC_INFO << this;
}

GPSManager::~GPSManager()
{
    qCDebug(GPSManagerLog) << Q_FUNC_INFO << this;
}
