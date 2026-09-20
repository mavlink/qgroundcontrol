#include "NMEASatelliteAdapter.h"

#include <algorithm>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

namespace {
constexpr qsizetype MAX_PENDING_EPOCHS = 64;
}  // namespace

QGC_LOGGING_CATEGORY(NMEASatelliteAdapterLog, "GPS.NMEA.NMEASatelliteAdapter")

NMEASatelliteAdapter::NMEASatelliteAdapter(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _flushTask(_scheduler, this)
    , _deliveryTask(_scheduler, this)
{
    qCDebug(NMEASatelliteAdapterLog) << this;
}

NMEASatelliteAdapter::~NMEASatelliteAdapter()
{
    qCDebug(NMEASatelliteAdapterLog) << this;
    close();
}

void NMEASatelliteAdapter::close()
{
    _open = false;
    for (auto* task : {&_flushTask, &_deliveryTask}) {
        task->cancel();
    }
    _assembler.clear();
    _pending.clear();
}

void NMEASatelliteAdapter::ingest(const NMEASentenceEnvelope& envelope)
{
    if (!_open || !_scheduler) {
        return;
    }
    auto update = _assembler.ingest(envelope.sentence(), envelope.receivedAtUs(), _scheduler->nowUs());
    if (!update.completed.empty())
        _queue(std::move(update.completed));
    _scheduleFlush();
}

void NMEASatelliteAdapter::_flush()
{
    if (!_open || !_scheduler) {
        return;
    }
    _queue(_assembler.flushDue(_scheduler->nowUs()));
    _scheduleFlush();
}

void NMEASatelliteAdapter::_scheduleFlush()
{
    _flushTask.cancel();
    if (const auto deadline = _assembler.deadlineUs(); deadline && _scheduler) {
        const auto now = _scheduler->nowUs();
        _flushTask.schedule(std::chrono::microseconds(*deadline > now ? *deadline - now : 0), [this]() { _flush(); });
    }
}

void NMEASatelliteAdapter::_queue(NMEA::SatelliteEpoch epoch)
{
    GPSSatelliteObservation observation;
    observation.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    for (const auto& system : epoch) {
        GPSSatelliteProvenance provenance;
        provenance.constellation = system.constellation;
        provenance.inViewTimestampUs = system.inViewTimestampUs;
        provenance.inUseTimestampUs = system.inUseTimestampUs;
        if (system.usedIds) {
            provenance.usedSatelliteIds = QList<int>(system.usedIds->begin(), system.usedIds->end());
            provenance.satellitesUsed = static_cast<int>(system.usedIds->size());
        }
        for (const auto& value : system.satellites) {
            GPSSatellite satellite;
            satellite.id = value.id;
            satellite.prn = value.prn;
            satellite.constellation = value.constellation;
            satellite.elevationDegrees = value.elevation;
            satellite.normalizedAzimuthDegrees = value.azimuth;
            satellite.signalStrength = value.signal;
            observation.satellites.append(satellite);
        }
        observation.provenance.append(provenance);
        observation.monotonicTimestampUs =
            std::max<quint64>({observation.monotonicTimestampUs, system.inViewTimestampUs, system.inUseTimestampUs});
    }
    if (!observation.provenance.isEmpty()) {
        // Bound queued epochs if a receiver outpaces the application event loop.
        if (_pending.size() >= MAX_PENDING_EPOCHS) {
            _pending.removeFirst();
        }
        _pending.append(observation);
        if (!_deliveryTask.active()) {
            _deliveryTask.schedule(std::chrono::microseconds::zero(), [this]() { _deliver(); });
        }
    }
}

void NMEASatelliteAdapter::_deliver()
{
    if (!_open || _pending.isEmpty()) {
        return;
    }
    const auto observation = _pending.takeFirst();
    // Schedule before notification; callbacks may close or destroy this assembler.
    if (!_pending.isEmpty()) {
        _deliveryTask.schedule(std::chrono::microseconds::zero(), [this]() { _deliver(); });
    }
    emit observationReceived(observation);
}
