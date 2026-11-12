#include "GPSReceiverFactGroup.h"

#include <cmath>
#include <limits>
#include <utility>

#include "GPSReceiverConfig.h"
#include "GPSReceiverMessages.h"

QString GPSConstellationSatellites::name() const
{
    return GPSReceiverMessages::constellationName(constellation);
}

GPSReceiverFactGroup::GPSReceiverFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/GPS/GPSReceiverFact.json"), parent)
{
    _addFact(&_currentDurationFact);
    _addFact(&_currentAccuracyFact);
    _addFact(&_targetDurationFact);
    _addFact(&_targetAccuracyFact);
    _addFact(&_baseStateFact);
    _addFact(&_baseLatitudeFact);
    _addFact(&_baseLongitudeFact);
    _addFact(&_baseHeightFact);
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

void GPSReceiverFactGroup::_write(quint64 status, Fact& fact, const QVariant& value)
{
    if (status == _resets) {
        _update(fact, value);
    }
}

void GPSReceiverFactGroup::_restoreDefaults(std::initializer_list<Fact*> facts)
{
    for (Fact* fact : facts) {
        _update(*fact, fact->rawDefaultValue());
    }
}

void GPSReceiverFactGroup::_setSurveyTarget(const std::optional<GPSBaseStationConfig::SurveyIn>& survey)
{
    if (!survey) {
        _restoreDefaults({&_targetDurationFact, &_targetAccuracyFact});
        return;
    }
    _update(_targetDurationFact, static_cast<qint64>(survey->duration.count()));
    _update(_targetAccuracyFact, survey->accuracyMeters);
}

void GPSReceiverFactGroup::_setBaseStatus(const GPSBaseStatus& report)
{
    const quint64 status = _resets;
    const double accuracy = report.meanAccuracyMeters.value_or(qQNaN());
    _write(status, _currentDurationFact, static_cast<qint64>(report.duration.count()));
    _write(status, _currentAccuracyFact, accuracy);
    _write(status, _baseLatitudeFact, report.position.latitudeDegrees);
    _write(status, _baseLongitudeFact, report.position.longitudeDegrees);
    _write(status, _baseHeightFact, report.position.altitudeMeters);
    const bool valid = report.state == GPSBaseState::Valid;
    const bool active = report.state == GPSBaseState::Active;
    if (active) {
        _setCurrentBasePositionSaved(false);
    }
    std::optional<GPSBaseStationConfig::Fixed> base;
    if (valid && std::isfinite(accuracy) && accuracy >= 0 && accuracy <= (std::numeric_limits<float>::max)()) {
        const GPSBaseStationConfig::Fixed fixed{.position = report.position,
                                                .accuracyMeters = static_cast<float>(accuracy)};
        if (gpsValidateBaseStationConfig({.mode = fixed}) == GPSReceiverConfigError::None) {
            base = fixed;
        }
    }
    if (status == _resets) {
        _setCurrentBasePosition(base);
    }
    // Last: its observers find the rest of the report with it.
    _write(status, _baseStateFact, static_cast<int>(report.state));
}

void GPSReceiverFactGroup::_setSatellites(const GPSSatelliteReport& report)
{
    QList<GPSConstellationSatellites> constellations;
    bool identified = false;
    for (const GPSSatelliteReport::Constellation& reported : report.constellationsInView()) {
        constellations.append(
            {.constellation = reported.constellation, .used = reported.used.value_or(-1), .inView = reported.inView});
        identified = identified || reported.constellation != GPSConstellation::Unknown;
    }
    // Satellites the receiver assigns to no constellation are listed beside the others, never alone.
    if (!identified) {
        constellations.clear();
    }
    // Before the Facts, which notify as they are written.
    _constellations = std::move(constellations);
    const quint64 status = _resets;
    _write(status, _numSatellitesFact, report.inView.value_or(-1));
    _write(status, _numSatellitesUsedFact, report.used.value_or(-1));
}

void GPSReceiverFactGroup::_setSolution(GPSFixQuality fixType, const GPSIntegrityReport& integrity)
{
    const quint64 status = _resets;
    _write(status, _jammingStateFact, static_cast<int>(integrity.jamming.state));
    _write(status, _spoofingStateFact, static_cast<int>(integrity.spoofing.state));
    _write(status, _antennaStateFact, static_cast<int>(integrity.antenna.state));
    // Last, as the base state is, for an observer of the fix.
    _write(status, _fixTypeFact, static_cast<int>(fixType));
}

void GPSReceiverFactGroup::_clearSolution()
{
    _restoreDefaults({&_numSatellitesFact, &_numSatellitesUsedFact, &_fixTypeFact, &_jammingStateFact,
                      &_spoofingStateFact, &_antennaStateFact});
    _constellations.clear();
}

void GPSReceiverFactGroup::_reset()
{
    ++_resets;
    _restoreDefaults({&_currentDurationFact, &_currentAccuracyFact, &_targetDurationFact, &_targetAccuracyFact,
                      &_baseStateFact, &_baseLatitudeFact, &_baseLongitudeFact, &_baseHeightFact, &_numSatellitesFact,
                      &_numSatellitesUsedFact, &_fixTypeFact, &_jammingStateFact, &_spoofingStateFact,
                      &_antennaStateFact});
    _constellations.clear();
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
