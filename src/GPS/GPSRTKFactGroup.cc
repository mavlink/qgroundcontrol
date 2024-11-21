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
    : FactGroup(1000, ":/json/Vehicle/GPSRTKFact.json", parent)
    , _connected(new Fact(0, _connectedFactName, FactMetaData::valueTypeBool, this))
    , _currentDuration(new Fact(0, _currentDurationFactName, FactMetaData::valueTypeDouble, this))
    , _currentAccuracy(new Fact(0, _currentAccuracyFactName, FactMetaData::valueTypeDouble, this))
    , _currentLatitude(new Fact(0, _currentLatitudeFactName, FactMetaData::valueTypeDouble, this))
    , _currentLongitude(new Fact(0, _currentLongitudeFactName, FactMetaData::valueTypeDouble, this))
    , _currentAltitude(new Fact(0, _currentAltitudeFactName, FactMetaData::valueTypeFloat, this))
    , _valid(new Fact(0, _validFactName, FactMetaData::valueTypeBool, this))
    , _active(new Fact(0, _activeFactName, FactMetaData::valueTypeBool, this))
    , _numSatellites(new Fact(0, _numSatellitesFactName, FactMetaData::valueTypeInt32, this))
{
    // qCDebug(GPSRTKFactGroupLog) << Q_FUNC_INFO << this;

    _addFact(_connected, _connectedFactName);
    _addFact(_currentDuration, _currentDurationFactName);
    _addFact(_currentAccuracy, _currentAccuracyFactName);
    _addFact(_currentLatitude, _currentLatitudeFactName);
    _addFact(_currentLongitude, _currentLongitudeFactName);
    _addFact(_currentAltitude, _currentAltitudeFactName);
    _addFact(_valid, _validFactName);
    _addFact(_active, _activeFactName);
    _addFact(_numSatellites, _numSatellitesFactName);
}

GPSRTKFactGroup::~GPSRTKFactGroup()
{
    // qCDebug(GPSRTKFactGroupLog) << Q_FUNC_INFO << this;
}
