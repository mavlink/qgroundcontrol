#include "GPSRTKFactGroup.h"

#include <cmath>
#include <limits>

#include "GPSReceiverConfig.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRTKFactGroupLog, "GPS.GPSRTKFactGroup")

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
    _addFact(&_numSatellitesUsedFact);
    _addFact(&_lastErrorFact);

    for (Fact* fact :
         {&_validFact, &_currentLatitudeFact, &_currentLongitudeFact, &_currentAltitudeFact, &_currentAccuracyFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSRTKFactGroup::currentBasePositionChanged);
    }
}

GPSRTKFactGroup::~GPSRTKFactGroup()
{
    // qCDebug(GPSRTKFactGroupLog) << Q_FUNC_INFO << this;
}

bool GPSRTKFactGroup::canSaveCurrentBasePosition() const
{
    const double accuracy = _currentAccuracyFact.rawValue().toDouble();
    if (!_validFact.rawValue().toBool() || !std::isfinite(accuracy) || accuracy < 0 ||
        accuracy > (std::numeric_limits<float>::max)()) {
        return false;
    }
    const GPSBaseStationConfig config{
        .useFixedBase = true,
        .fixedPosition = {.latitudeDegrees = _currentLatitudeFact.rawValue().toDouble(),
                          .longitudeDegrees = _currentLongitudeFact.rawValue().toDouble(),
                          .altitudeMeters = _currentAltitudeFact.rawValue().toFloat()},
        .fixedBaseAccuracyMeters = static_cast<float>(accuracy),
    };
    return gpsValidateBaseStationConfig(config) == GPSReceiverConfigError::None;
}
