#include "GPSBaseStationFactGroup.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSBaseStationFactGroupLog, "GPS.BaseStation.GPSBaseStationFactGroup")

GPSBaseStationFactGroup::GPSBaseStationFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSBaseStationFact.json"), parent)
{
    qCDebug(GPSBaseStationFactGroupLog) << this;

    _addFact(&_currentDurationFact);
    _addFact(&_currentAccuracyFact);
    _addFact(&_currentLatitudeFact);
    _addFact(&_currentLongitudeFact);
    _addFact(&_currentAltitudeFact);
    _addFact(&_validFact);
    _addFact(&_activeFact);
}

GPSBaseStationFactGroup::~GPSBaseStationFactGroup()
{
    qCDebug(GPSBaseStationFactGroupLog) << this;
}
