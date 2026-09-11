#include "GPSIntegrityStore.h"

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSIntegrityStoreLog, "GPS.Core.GPSIntegrityStore")

GPSIntegrityStore::GPSIntegrityStore(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _expiryTask(_scheduler, this)
{
    qCDebug(GPSIntegrityStoreLog) << this;
    connect(_scheduler, &QObject::destroyed, this, [this]() {
        _scheduler = nullptr;
        _observation = {};
        _refresh();
    });
}

GPSIntegrityStore::~GPSIntegrityStore()
{
    qCDebug(GPSIntegrityStoreLog) << this;
    if (_scheduler) {
        _scheduler->disconnect(this);
    }
}

void GPSIntegrityStore::beginSession(quint64 sessionId)
{
    if (_sessionId == sessionId) {
        return;
    }
    _sessionId = sessionId;
    _observation = {};
    _observation.sessionId = sessionId;
    _refresh();
}

void GPSIntegrityStore::updateObservation(const GPSIntegrityObservation& observation)
{
    if (!_scheduler || observation.monotonicTimestampUs > _scheduler->nowUs() ||
        (_sessionId && observation.sessionId != *_sessionId)) {
        return;
    }
    auto accepted = observation;
    const auto stamp = accepted.monotonicTimestampUs;
    auto provenance = accepted.provenance.value_or(GPSIntegrityProvenance{stamp, stamp, stamp, stamp, stamp});
    const quint64 nowUs = _scheduler->nowUs();
    const GPSIntegrityObservation previous =
        accepted.sessionId == _observation.sessionId ? _observation : GPSIntegrityObservation{};
    const auto previousStamp = previous.monotonicTimestampUs;
    const auto previousProvenance = previous.provenance.value_or(
        GPSIntegrityProvenance{previousStamp, previousStamp, previousStamp, previousStamp, previousStamp});
    // A navigation snapshot can carry older diagnostics alongside a newly received diagnostic group.
    const auto retainNewest = [&](quint64& receipt, quint64 previousReceipt, auto... fields) {
        if (receipt < previousReceipt || receipt > nowUs) {
            receipt = previousReceipt;
            ((accepted.*fields = previous.*fields), ...);
        }
    };
    retainNewest(provenance.jammingTimestampUs, previousProvenance.jammingTimestampUs,
                 &GPSIntegrityObservation::jammingState);
    retainNewest(provenance.spoofingTimestampUs, previousProvenance.spoofingTimestampUs,
                 &GPSIntegrityObservation::spoofingState);
    retainNewest(provenance.authenticationTimestampUs, previousProvenance.authenticationTimestampUs,
                 &GPSIntegrityObservation::authenticationState);
    retainNewest(provenance.correctionsTimestampUs, previousProvenance.correctionsTimestampUs,
                 &GPSIntegrityObservation::correctionsProtocol, &GPSIntegrityObservation::correctionsUsed,
                 &GPSIntegrityObservation::correctionsCrcFailed);
    retainNewest(provenance.rfTimestampUs, previousProvenance.rfTimestampUs,
                 &GPSIntegrityObservation::noisePerMillisecond, &GPSIntegrityObservation::automaticGainControl,
                 &GPSIntegrityObservation::jammingIndicator);
    retainNewest(accepted.monotonicTimestampUs, previousStamp, &GPSIntegrityObservation::systemErrors,
                 &GPSIntegrityObservation::correctionsQuality, &GPSIntegrityObservation::systemQuality,
                 &GPSIntegrityObservation::gnssSignalQuality, &GPSIntegrityObservation::postProcessingQuality);
    accepted.provenance = provenance;
    _observation = std::move(accepted);
    _refresh();
}

void GPSIntegrityStore::reset()
{
    _sessionId.reset();
    _observation = {};
    _refresh();
}

void GPSIntegrityStore::_refresh()
{
    _expiryTask.cancel();
    const quint64 nowUs = _scheduler ? _scheduler->nowUs() : 0;
    quint64 remainingUs = FRESHNESS_TIMEOUT_US;
    const auto retainFresh = [&](auto& value, quint64 timestamp) {
        if (!value) {
            return;
        }
        if (!_scheduler || !timestamp || timestamp > nowUs || nowUs - timestamp >= FRESHNESS_TIMEOUT_US) {
            value.reset();
        } else {
            remainingUs = (std::min) (remainingUs, FRESHNESS_TIMEOUT_US - (nowUs - timestamp));
        }
    };
    const auto stamp = _observation.monotonicTimestampUs;
    const auto provenance = _observation.provenance.value_or(GPSIntegrityProvenance{stamp, stamp, stamp, stamp, stamp});
    retainFresh(_observation.jammingState, provenance.jammingTimestampUs);
    retainFresh(_observation.spoofingState, provenance.spoofingTimestampUs);
    retainFresh(_observation.authenticationState, provenance.authenticationTimestampUs);
    retainFresh(_observation.correctionsProtocol, provenance.correctionsTimestampUs);
    retainFresh(_observation.correctionsUsed, provenance.correctionsTimestampUs);
    retainFresh(_observation.noisePerMillisecond, provenance.rfTimestampUs);
    retainFresh(_observation.automaticGainControl, provenance.rfTimestampUs);
    retainFresh(_observation.jammingIndicator, provenance.rfTimestampUs);
    retainFresh(_observation.correctionsCrcFailed, provenance.correctionsTimestampUs);
    retainFresh(_observation.systemErrors, stamp);
    retainFresh(_observation.correctionsQuality, stamp);
    retainFresh(_observation.systemQuality, stamp);
    retainFresh(_observation.gnssSignalQuality, stamp);
    retainFresh(_observation.postProcessingQuality, stamp);
    _available = _observation.systemErrors.has_value() || _observation.spoofingState.has_value() ||
                 _observation.jammingState.has_value() || _observation.authenticationState.has_value() ||
                 _observation.correctionsQuality.has_value() || _observation.systemQuality.has_value() ||
                 _observation.gnssSignalQuality.has_value() || _observation.postProcessingQuality.has_value() ||
                 _observation.correctionsProtocol.has_value() || _observation.correctionsUsed.has_value() ||
                 _observation.noisePerMillisecond.has_value() || _observation.automaticGainControl.has_value() ||
                 _observation.jammingIndicator.has_value() || _observation.correctionsCrcFailed.has_value();
    if (_available) {
        _expiryTask.schedule(std::chrono::microseconds(remainingUs), [this]() { _refresh(); });
    }
    const auto snapshot = _observation;
    emit observationChanged(snapshot);
}
