#include "GPSReceiverFactGroup.h"

#include <cmath>
#include <limits>
#include <utility>

#include "GPSReceiverConfig.h"

GPSReceiverFactGroup::GPSReceiverFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/GPS/GPSReceiverFact.json"), parent)
{
    _addFact(&_currentDurationFact);
    _addFact(&_currentAccuracyFact);
    _addFact(&_validFact);
    _addFact(&_activeFact);
    _addFact(&_numSatellitesFact);
    _addFact(&_numSatellitesUsedFact);
    _addFact(&_fixTypeFact);
    _addFact(&_jammingStateFact);
    _addFact(&_spoofingStateFact);
    _addFact(&_antennaStateFact);

    for (Fact* fact : {&_jammingStateFact, &_spoofingStateFact, &_antennaStateFact}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSReceiverFactGroup::_updateReceiverWarning);
    }
    _reset();
}

void GPSReceiverFactGroup::_update(Fact& fact, const QVariant& value)
{
    // Unavailable values stay NaN; rewriting NaN would report a change, as NaN never equals itself.
    if (!std::isnan(value.toDouble()) || !std::isnan(fact.rawValue().toDouble())) {
        fact.setRawValue(value);
    }
}

void GPSReceiverFactGroup::_restoreDefaults(std::initializer_list<Fact*> facts)
{
    for (Fact* fact : facts) {
        _update(*fact, fact->rawDefaultValue());
    }
}

void GPSReceiverFactGroup::_setSurvey(const GPSSurveyReport& report)
{
    const double accuracy = report.meanAccuracyMeters.value_or(qQNaN());
    _update(_currentDurationFact, static_cast<qint64>(report.duration.count()));
    _update(_currentAccuracyFact, accuracy);
    _update(_validFact, report.valid);
    _update(_activeFact, report.active);
    if (report.active) {
        _setCurrentBasePositionSaved(false);
    }
    std::optional<GPSBaseStationConfig::Fixed> base;
    if (report.valid && std::isfinite(accuracy) && accuracy >= 0 && accuracy <= (std::numeric_limits<float>::max)()) {
        const GPSBaseStationConfig::Fixed fixed{.position = report.position,
                                                .accuracyMeters = static_cast<float>(accuracy)};
        if (gpsValidateBaseStationConfig({.mode = fixed}) == GPSReceiverConfigError::None) {
            base = fixed;
        }
    }
    _setCurrentBasePosition(base);
}

void GPSReceiverFactGroup::_setSatellites(int inView, int used)
{
    _update(_numSatellitesFact, inView);
    _update(_numSatellitesUsedFact, used);
}

void GPSReceiverFactGroup::_setSolution(GPSFixQuality fixType, const GPSIntegrityReport& integrity)
{
    _update(_fixTypeFact, static_cast<int>(fixType));
    _update(_jammingStateFact, static_cast<int>(integrity.jamming.state));
    _update(_spoofingStateFact, static_cast<int>(integrity.spoofing.state));
    _update(_antennaStateFact, static_cast<int>(integrity.antenna.state));
}

void GPSReceiverFactGroup::_clearSolution()
{
    _restoreDefaults({&_numSatellitesFact, &_numSatellitesUsedFact, &_fixTypeFact, &_jammingStateFact,
                      &_spoofingStateFact, &_antennaStateFact});
}

void GPSReceiverFactGroup::_reset()
{
    _restoreDefaults({&_currentDurationFact, &_currentAccuracyFact, &_validFact, &_activeFact, &_numSatellitesFact,
                      &_numSatellitesUsedFact, &_fixTypeFact, &_jammingStateFact, &_spoofingStateFact,
                      &_antennaStateFact});
    _setCurrentBasePosition(std::nullopt);
    _setTelemetryAvailable(false);
}

void GPSReceiverFactGroup::_updateReceiverWarning()
{
    using JammingState = GPSIntegrityReport::JammingState;
    using SpoofingState = GPSIntegrityReport::SpoofingState;
    using AntennaState = GPSIntegrityReport::AntennaState;
    const int jamming = _jammingStateFact.rawValue().toInt();
    const int spoofing = _spoofingStateFact.rawValue().toInt();
    const int antenna = _antennaStateFact.rawValue().toInt();
    const bool warning =
        jamming == static_cast<int>(JammingState::Warning) || jamming == static_cast<int>(JammingState::Critical) ||
        spoofing == static_cast<int>(SpoofingState::Indicated) ||
        spoofing == static_cast<int>(SpoofingState::Multiple) || antenna == static_cast<int>(AntennaState::Open) ||
        antenna == static_cast<int>(AntennaState::Short);
    if (std::exchange(_receiverWarning, warning) != warning) {
        emit receiverWarningChanged();
    }
}

void GPSReceiverFactGroup::_setCurrentBasePosition(const std::optional<GPSBaseStationConfig::Fixed>& base)
{
    const bool couldSave = canSaveCurrentBasePosition();
    _currentBasePosition = base;
    if (!base) {
        _setCurrentBasePositionSaved(false);
    }
    if (canSaveCurrentBasePosition() != couldSave) {
        emit currentBasePositionChanged();
    }
}

void GPSReceiverFactGroup::_setCurrentBasePositionSaved(bool saved)
{
    if (std::exchange(_currentBasePositionSaved, saved) != saved) {
        emit currentBasePositionChanged();
    }
}
