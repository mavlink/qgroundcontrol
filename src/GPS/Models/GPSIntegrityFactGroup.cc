#include "GPSIntegrityFactGroup.h"

#include <QtCore/QPointer>

#include <array>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSIntegrityFactGroupLog, "GPS.Models.GPSIntegrityFactGroup")

GPSIntegrityFactGroup::GPSIntegrityFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/GPSFact.json"), parent)
{
    qCDebug(GPSIntegrityFactGroupLog) << this;
    for (auto* fact :
         {&_systemErrors, &_spoofingState, &_jammingState, &_authenticationState, &_correctionsQuality, &_systemQuality,
          &_gnssSignalQuality, &_postProcessingQuality, &_correctionsProtocol, &_correctionsUsed}) {
        _addFact(fact);
    }
    _expiryTimer.setSingleShot(true);
    connect(&_expiryTimer, &QChronoTimer::timeout, this, &GPSIntegrityFactGroup::reset);
    reset();
}

GPSIntegrityFactGroup::~GPSIntegrityFactGroup()
{
    qCDebug(GPSIntegrityFactGroupLog) << this;
}

void GPSIntegrityFactGroup::update(const GPSIntegrityObservation& observation)
{
    const QPointer<GPSIntegrityFactGroup> guard(this);
    const quint64 revision = ++_revision;
    const auto age = GPSObservation::ageMilliseconds(observation.monotonicTimestampUs);
    if (observation.monotonicTimestampUs == 0 || age < 0 || age >= 5000) {
        reset();
        return;
    }
    const bool available = observation.systemErrors.has_value() || observation.spoofingState.has_value() ||
                           observation.jammingState.has_value() || observation.authenticationState.has_value() ||
                           observation.correctionsQuality.has_value() || observation.systemQuality.has_value() ||
                           observation.gnssSignalQuality.has_value() || observation.postProcessingQuality.has_value() ||
                           observation.correctionsProtocol.has_value() || observation.correctionsUsed.has_value();
    const bool changed = _available != available || _systemErrorsKnown != observation.systemErrors.has_value();
    _available = available;
    _systemErrorsKnown = observation.systemErrors.has_value();
    _expiryTimer.setInterval(std::chrono::milliseconds(5000 - age));
    _expiryTimer.start();
    const std::array<std::pair<Fact*, QVariant>, 10> values = {{
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
    const bool changed = _available || _systemErrorsKnown;
    _available = false;
    _systemErrorsKnown = false;
    const std::array<std::pair<Fact*, QVariant>, 10> values = {{
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
