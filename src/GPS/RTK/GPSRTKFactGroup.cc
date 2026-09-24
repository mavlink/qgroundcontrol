#include "GPSRTKFactGroup.h"

#include <cmath>
#include <limits>

#include "GPSDriverReports.h"
#include "GPSReceiverConfig.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRTKFactGroupLog, "GPS.RTK.GPSRTKFactGroup")

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
    _addFact(&_fixTypeFact);
    _addFact(&_jammingStateFact);
    _addFact(&_spoofingStateFact);

    for (Fact* fact :
         {&_validFact, &_currentLatitudeFact, &_currentLongitudeFact, &_currentAltitudeFact, &_currentAccuracyFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSRTKFactGroup::currentBasePositionChanged);
    }
    for (Fact* fact : {&_jammingStateFact, &_spoofingStateFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSRTKFactGroup::interferenceWarningChanged);
    }
}

GPSRTKFactGroup::~GPSRTKFactGroup()
{
    // qCDebug(GPSRTKFactGroupLog) << Q_FUNC_INFO << this;
}

bool GPSRTKFactGroup::interferenceWarning() const
{
    using JammingState = GPSIntegrityReport::JammingState;
    using SpoofingState = GPSIntegrityReport::SpoofingState;
    const int jamming = _jammingStateFact.rawValue().toInt();
    const int spoofing = _spoofingStateFact.rawValue().toInt();
    return jamming == static_cast<int>(JammingState::Warning) || jamming == static_cast<int>(JammingState::Critical) ||
           spoofing == static_cast<int>(SpoofingState::Indicated) ||
           spoofing == static_cast<int>(SpoofingState::Multiple);
}

bool GPSRTKFactGroup::canSaveCurrentBasePosition() const
{
    const double accuracy = _currentAccuracyFact.rawValue().toDouble();
    if (!_validFact.rawValue().toBool() || !std::isfinite(accuracy) || accuracy < 0 ||
        accuracy > (std::numeric_limits<float>::max)()) {
        return false;
    }
    const GPSBaseStationConfig config{
        .mode =
            GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = _currentLatitudeFact.rawValue().toDouble(),
                                                     .longitudeDegrees = _currentLongitudeFact.rawValue().toDouble(),
                                                     .altitudeMeters = _currentAltitudeFact.rawValue().toFloat()},
                                        .accuracyMeters = static_cast<float>(accuracy)},
    };
    return gpsValidateBaseStationConfig(config) == GPSReceiverConfigError::None;
}
