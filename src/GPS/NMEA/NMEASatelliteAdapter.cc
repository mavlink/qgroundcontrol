#include "NMEASatelliteAdapter.h"

#include <algorithm>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

namespace {
constexpr auto SATELLITE_IDLE_TIMEOUT = std::chrono::milliseconds(150);
constexpr auto SATELLITE_BATCH_TIMEOUT = std::chrono::seconds(1);
constexpr qsizetype MAX_PENDING_EPOCHS = 64;
}  // namespace

QGC_LOGGING_CATEGORY(NMEASatelliteAdapterLog, "GPS.NMEA.NMEASatelliteAdapter")

NMEASatelliteAdapter::NMEASatelliteAdapter(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent),
      _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)),
      _idleTask(_scheduler, this),
      _batchTask(_scheduler, this),
      _deliveryTask(_scheduler, this)
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
    for (auto* task : {&_idleTask, &_batchTask, &_deliveryTask}) {
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
    auto update = _assembler.ingest(envelope.sentence(), envelope.receivedAtUs());
    if (!update.completed.empty())
        _queue(std::move(update.completed));
    if (!update.accepted)
        return;
    _idleTask.cancel();
    _idleTask.schedule(SATELLITE_IDLE_TIMEOUT, [this]() { _flush(); });
    if (!_batchTask.active()) {
        _batchTask.schedule(SATELLITE_BATCH_TIMEOUT, [this]() { _flush(); });
    }
}

void NMEASatelliteAdapter::_flush()
{
    _idleTask.cancel();
    _batchTask.cancel();
    _queue(_assembler.flush());
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
