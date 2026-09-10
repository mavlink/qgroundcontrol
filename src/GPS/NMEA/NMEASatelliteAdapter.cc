#include "NMEASatelliteAdapter.h"

#include <algorithm>

#include "GPSQtRuntimeScheduler.h"
#include "GPSReadTimestamp.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEASatelliteAdapterLog, "GPS.NMEA.NMEASatelliteAdapter")

NMEASatelliteAdapter::NMEASatelliteAdapter(QIODevice* source, QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _source(source)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _idleTask(_scheduler, this)
    , _batchTask(_scheduler, this)
    , _deliveryTask(_scheduler, this)
    , _readTask(_scheduler, this)
{
    qCDebug(NMEASatelliteAdapterLog) << this;
    if (source) {
        connect(source, &QIODevice::readyRead, this, &NMEASatelliteAdapter::_readAvailable);
        connect(source, &QIODevice::aboutToClose, this, &NMEASatelliteAdapter::close);
        connect(source, &QObject::destroyed, this, &NMEASatelliteAdapter::close);
    }
}

NMEASatelliteAdapter::~NMEASatelliteAdapter()
{
    qCDebug(NMEASatelliteAdapterLog) << this;
    close();
}

void NMEASatelliteAdapter::close()
{
    _open = false;
    for (auto* task : {&_idleTask, &_batchTask, &_deliveryTask, &_readTask}) {
        task->cancel();
    }
    _assembler.clear();
    _pending.clear();
}

void NMEASatelliteAdapter::_readAvailable()
{
    qsizetype remaining = 64 * 1024;
    while (_open && _source && _source->canReadLine() && remaining > 0) {
        const QByteArray sentence = _source->readLine(4096);
        if (sentence.isEmpty()) {
            break;
        }
        remaining -= sentence.size();
        _parseSentence(sentence, GPSReadTimestamp::from(_source));
    }
    if (_open && _source && _source->canReadLine() && !_readTask.active()) {
        _readTask.schedule(std::chrono::microseconds::zero(), [this]() { _readAvailable(); });
    }
}

void NMEASatelliteAdapter::_parseSentence(const QByteArray& sentence, quint64 receivedAtUs)
{
    if (const auto envelope = NMEASentenceEnvelope::parse(sentence, receivedAtUs)) {
        ingest(*envelope);
    }
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
    _idleTask.schedule(std::chrono::milliseconds(150), [this]() { _flush(); });
    if (!_batchTask.active()) {
        _batchTask.schedule(std::chrono::seconds(1), [this]() { _flush(); });
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
            provenance.satellitesUsed = system.usedIds->size();
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
        if (_pending.size() >= 64) {
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
