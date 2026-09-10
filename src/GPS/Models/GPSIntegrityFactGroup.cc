#include "GPSIntegrityFactGroup.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <array>

#include "GPSObservation.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSIntegrityFactGroupLog, "GPS.Models.GPSIntegrityFactGroup")

GPSIntegrityFactGroup::GPSIntegrityFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSFact.json"), parent)
{
    qCDebug(GPSIntegrityFactGroupLog) << this;
    for (auto* fact :
         {&_systemErrors, &_spoofingState, &_jammingState, &_authenticationState, &_correctionsQuality, &_systemQuality,
          &_gnssSignalQuality, &_postProcessingQuality, &_correctionsProtocol, &_correctionsUsed, &_noisePerMillisecond,
          &_automaticGainControl, &_jammingIndicator, &_correctionsCrcFailed}) {
        _addFact(fact);
    }
    _expiryTimer.setSingleShot(true);
    connect(&_expiryTimer, &QChronoTimer::timeout, this, &GPSIntegrityFactGroup::_refresh);
    reset();
}

GPSIntegrityFactGroup::~GPSIntegrityFactGroup()
{
    qCDebug(GPSIntegrityFactGroupLog) << this;
}

void GPSIntegrityFactGroup::update(const GPSIntegrityObservation& observation)
{
    _observation = observation;
    _refresh();
}

void GPSIntegrityFactGroup::_refresh()
{
    const QPointer<GPSIntegrityFactGroup> guard(this);
    const quint64 revision = ++_revision;
    auto observation = _observation;
    const quint64 now = GPSObservation::monotonicNowUs();
    quint64 remainingUs = 5000000;
    const auto retainFresh = [&](auto& value, quint64 timestamp) {
        if (!value) {
            return;
        }
        if (!timestamp || timestamp > now || now - timestamp >= 5000000) {
            value.reset();
        } else {
            remainingUs = (std::min) (remainingUs, 5000000 - (now - timestamp));
        }
    };
    const auto stamp = observation.monotonicTimestampUs;
    const auto provenance = observation.provenance.value_or(GPSIntegrityProvenance{stamp, stamp, stamp, stamp, stamp});
    retainFresh(observation.jammingState, provenance.jammingTimestampUs);
    retainFresh(observation.spoofingState, provenance.spoofingTimestampUs);
    retainFresh(observation.authenticationState, provenance.authenticationTimestampUs);
    retainFresh(observation.correctionsProtocol, provenance.correctionsTimestampUs);
    retainFresh(observation.correctionsUsed, provenance.correctionsTimestampUs);
    retainFresh(observation.noisePerMillisecond, provenance.rfTimestampUs);
    retainFresh(observation.automaticGainControl, provenance.rfTimestampUs);
    retainFresh(observation.jammingIndicator, provenance.rfTimestampUs);
    retainFresh(observation.correctionsCrcFailed, provenance.correctionsTimestampUs);
    retainFresh(observation.systemErrors, stamp);
    retainFresh(observation.correctionsQuality, stamp);
    retainFresh(observation.systemQuality, stamp);
    retainFresh(observation.gnssSignalQuality, stamp);
    retainFresh(observation.postProcessingQuality, stamp);
    const bool available = observation.systemErrors.has_value() || observation.spoofingState.has_value() ||
                           observation.jammingState.has_value() || observation.authenticationState.has_value() ||
                           observation.correctionsQuality.has_value() || observation.systemQuality.has_value() ||
                           observation.gnssSignalQuality.has_value() || observation.postProcessingQuality.has_value() ||
                           observation.correctionsProtocol.has_value() || observation.correctionsUsed.has_value() ||
                           observation.noisePerMillisecond.has_value() ||
                           observation.automaticGainControl.has_value() || observation.jammingIndicator.has_value() ||
                           observation.correctionsCrcFailed.has_value();
    const bool changed = _available != available || _systemErrorsKnown != observation.systemErrors.has_value();
    _available = available;
    _systemErrorsKnown = observation.systemErrors.has_value();
    _expiryTimer.stop();
    if (available) {
        _expiryTimer.setInterval(std::chrono::microseconds(remainingUs));
        _expiryTimer.start();
    }
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
    const QPointer<GPSIntegrityFactGroup> guard(this);
    const quint64 revision = ++_revision;
    _expiryTimer.stop();
    _observation = {};
    const bool changed = _available || _systemErrorsKnown;
    _available = false;
    _systemErrorsKnown = false;
    const std::array<std::pair<Fact*, QVariant>, 14> values = {{
        {&_systemErrors, 0},
        {&_spoofingState, 255},
        {&_jammingState, 255},
        {&_authenticationState, 255},
        {&_correctionsQuality, 255},
        {&_systemQuality, 255},
        {&_gnssSignalQuality, 255},
        {&_postProcessingQuality, 255},
        {&_correctionsProtocol, 255},
        {&_correctionsUsed, 255},
        {&_noisePerMillisecond, -1},
        {&_automaticGainControl, -1},
        {&_jammingIndicator, -1},
        {&_correctionsCrcFailed, -1},

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
