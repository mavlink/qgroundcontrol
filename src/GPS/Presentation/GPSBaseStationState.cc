#include "GPSBaseStationState.h"

#include <QtCore/QPointer>

#include <array>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSBaseStationStateLog, "GPS.BaseStation.GPSBaseStationState")

GPSBaseStationState::GPSBaseStationState(GPSReceiverState& state, GPSBaseStationFactGroup& facts, QObject* parent)
    : QObject(parent)
    , _state(state)
    , _facts(facts)
{
    qCDebug(GPSBaseStationStateLog) << this;
    connect(&_state, &GPSReceiverState::referenceChanged, this, &GPSBaseStationState::_project);
    _project();
}

GPSBaseStationState::~GPSBaseStationState()
{
    qCDebug(GPSBaseStationStateLog) << this;
}

void GPSBaseStationState::_project()
{
    const QPointer<GPSBaseStationState> guard(this);
    const quint64 revision = ++_revision;
    const auto status = _state.surveyStatus();
    const auto reference = _state.reference();
    emit referenceChanged();
    if (!guard || _revision != revision) {
        return;
    }
    const auto coordinate = reference.observation.position.coordinate();
    const std::array<std::pair<Fact*, QVariant>, 7> values = {{
        {_facts.currentDuration(), status.durationSecs},
        {_facts.currentAccuracy(), reference.accuracyMeters.value_or(qQNaN())},
        {_facts.currentLatitude(), coordinate.latitude()},
        {_facts.currentLongitude(), coordinate.longitude()},
        {_facts.currentAltitude(), coordinate.altitude()},
        {_facts.valid(), status.valid},
        {_facts.active(), status.active},
    }};
    for (const auto& [fact, value] : values) {
        if (!guard || _revision != revision) {
            return;
        }
        fact->setRawValue(value);
    }
}
