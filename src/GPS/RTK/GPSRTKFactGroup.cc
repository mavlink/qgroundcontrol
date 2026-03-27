#include "GPSRTKFactGroup.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRTKFactGroupLog, "GPS.GPSRTKFactGroup")

GPSRTKFactGroup::GPSRTKFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSRTKFact.json"), parent)
{
    qCDebug(GPSRTKFactGroupLog) << this;

    _addFact(&_connectedFact);
    _addFact(&_currentDurationFact);
    _addFact(&_currentAccuracyFact);
    _addFact(&_currentLatitudeFact);
    _addFact(&_currentLongitudeFact);
    _addFact(&_currentAltitudeFact);
    _addFact(&_validFact);
    _addFact(&_activeFact);
    _addFact(&_numSatellitesFact);
    _addFact(&_baseLatitudeFact);
    _addFact(&_baseLongitudeFact);
    _addFact(&_baseAltitudeFact);
    _addFact(&_baseFixTypeFact);
    _addFact(&_headingFact);
    _addFact(&_headingValidFact);
    _addFact(&_baselineLengthFact);
    _addFact(&_carrierFixedFact);
}

namespace {
QGeoCoordinate assembleCoord(double lat, double lon, double alt)
{
    if (!qIsFinite(lat) || !qIsFinite(lon)) {
        return {};
    }
    QGeoCoordinate c(lat, lon, qIsFinite(alt) ? alt : 0.0);
    return c.isValid() ? c : QGeoCoordinate{};
}
} // namespace

QGeoCoordinate GPSRTKFactGroup::currentPosition() const
{
    return assembleCoord(_currentLatitudeFact.rawValue().toDouble(),
                         _currentLongitudeFact.rawValue().toDouble(),
                         _currentAltitudeFact.rawValue().toDouble());
}

QGeoCoordinate GPSRTKFactGroup::basePosition() const
{
    return assembleCoord(_baseLatitudeFact.rawValue().toDouble(),
                         _baseLongitudeFact.rawValue().toDouble(),
                         _baseAltitudeFact.rawValue().toDouble());
}

void GPSRTKFactGroup::resetToDefaults()
{
    _connectedFact.setRawValue(false);
    _activeFact.setRawValue(false);
    _validFact.setRawValue(false);
    _numSatellitesFact.setRawValue(0);
    _currentDurationFact.setRawValue(0.0);
    _currentAccuracyFact.setRawValue(qQNaN());
    _currentLatitudeFact.setRawValue(qQNaN());
    _currentLongitudeFact.setRawValue(qQNaN());
    _currentAltitudeFact.setRawValue(qQNaN());
    _baseLatitudeFact.setRawValue(qQNaN());
    _baseLongitudeFact.setRawValue(qQNaN());
    _baseAltitudeFact.setRawValue(qQNaN());
    _baseFixTypeFact.setRawValue(0);
    _headingFact.setRawValue(qQNaN());
    _headingValidFact.setRawValue(false);
    _baselineLengthFact.setRawValue(qQNaN());
    _carrierFixedFact.setRawValue(false);
}
