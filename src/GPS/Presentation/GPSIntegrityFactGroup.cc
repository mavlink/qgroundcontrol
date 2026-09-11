#include "GPSIntegrityFactGroup.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <array>

#include "GPSObservation.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSIntegrityFactGroupLog, "GPS.Models.GPSIntegrityFactGroup")

GPSIntegrityFactGroup::GPSIntegrityFactGroup(QObject* parent, RuntimeScheduler* scheduler)
    : FactGroup(1000, QStringLiteral(":/json/GPS/Integrity/GPSFact.json"), parent)
    , _store(this, scheduler)
{
    qCDebug(GPSIntegrityFactGroupLog) << this;
    for (auto* fact :
         {&_systemErrors, &_spoofingState, &_jammingState, &_authenticationState, &_correctionsQuality, &_systemQuality,
          &_gnssSignalQuality, &_postProcessingQuality, &_correctionsProtocol, &_correctionsUsed, &_noisePerMillisecond,
          &_automaticGainControl, &_jammingIndicator, &_correctionsCrcFailed}) {
        _addFact(fact);
    }
    connect(&_store, &GPSIntegrityStore::observationChanged, this, &GPSIntegrityFactGroup::_refresh);
    _refresh();
}

GPSIntegrityFactGroup::~GPSIntegrityFactGroup()
{
    qCDebug(GPSIntegrityFactGroupLog) << this;
}

void GPSIntegrityFactGroup::update(const GPSIntegrityObservation& observation)
{
    _store.updateObservation(observation);
}

void GPSIntegrityFactGroup::_refresh()
{
    const QPointer<GPSIntegrityFactGroup> guard(this);
    const quint64 revision = ++_revision;
    const auto observation = _store.observation();
    const bool available = _store.available();
    const bool changed = _available != available || _systemErrorsKnown != observation.systemErrors.has_value();
    _available = available;
    _systemErrorsKnown = observation.systemErrors.has_value();
    const std::array<std::pair<Fact*, QVariant>, 14> values = {{
        {&_systemErrors, observation.systemErrors.value_or(0)},
        {&_spoofingState, observation.spoofingState.value_or(255)},
        {&_jammingState, observation.jammingState.value_or(255)},
        {&_authenticationState, observation.authenticationState.value_or(255)},
        {&_correctionsQuality, observation.correctionsQuality.value_or(255)},
        {&_systemQuality, observation.systemQuality.value_or(255)},
        {&_gnssSignalQuality, observation.gnssSignalQuality.value_or(255)},
        {&_postProcessingQuality, observation.postProcessingQuality.value_or(255)},
        {&_correctionsProtocol, observation.correctionsProtocol.value_or(255)},
        {&_correctionsUsed, observation.correctionsUsed.value_or(255)},
        {&_noisePerMillisecond, observation.noisePerMillisecond.value_or(-1)},
        {&_automaticGainControl, observation.automaticGainControl.value_or(-1)},
        {&_jammingIndicator, observation.jammingIndicator.value_or(-1)},
        {&_correctionsCrcFailed, observation.correctionsCrcFailed ? int(*observation.correctionsCrcFailed) : -1},

    }};
    for (const auto& [fact, value] : values) {
        if (!guard || revision != _revision) {
            return;
        }
        fact->setRawValue(value);
    }
    if (guard && revision == _revision) {
        _setTelemetryAvailable(_available);
    }
    if (guard && revision == _revision && changed) {
        emit availabilityChanged();
    }
}

void GPSIntegrityFactGroup::reset()
{
    _store.reset();
}
