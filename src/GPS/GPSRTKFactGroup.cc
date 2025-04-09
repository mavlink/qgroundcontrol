/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GPSRTKFactGroup.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRTKFactGroupLog, "qgc.gps.gpsrtkfactgroup")

GPSRTKFactGroup::GPSRTKFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSRTKFact.json"), parent)
{
    // qCDebug(GPSRTKFactGroupLog) << Q_FUNC_INFO << this;

    _addFact(&_connectedFact);
    _addFact(&_currentDurationFact);
    _addFact(&_currentAccuracyFact);
    _addFact(&_currentLatitudeFact);
    _addFact(&_currentLongitudeFact);
    _addFact(&_currentAltitudeFact);
    _addFact(&_validFact);
    _addFact(&_activeFact);
    _addFact(&_numSatellitesFact);
}

GPSRTKFactGroup::~GPSRTKFactGroup()
{
    // qCDebug(GPSRTKFactGroupLog) << Q_FUNC_INFO << this;
}
