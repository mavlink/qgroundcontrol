#include "NMEASatelliteAdapter.h"

#include <algorithm>
#include <cmath>

#include "GPSQtRuntimeScheduler.h"
#include "GPSReadTimestamp.h"
#include "NMEAFields.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEASatelliteAdapterLog, "GPS.NMEA.NMEASatelliteAdapter")

NMEASatelliteAdapter::NMEASatelliteAdapter(QIODevice* source, QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _source(source)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
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
        _scheduler->cancel(*task);
        *task = 0;
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
    if (_open && _source && _source->canReadLine() && !_readTask) {
        _readTask = _scheduler->schedule(this, std::chrono::microseconds::zero(), [this]() {
            _readTask = 0;
            _readAvailable();
        });
    }
}

void NMEASatelliteAdapter::_parseSentence(const QByteArray& sentence, quint64 receivedAtUs)
{
    const auto parsed = NMEA::sentence({sentence.constData(), static_cast<size_t>(sentence.size())});
    if (!parsed)
        return;
    auto update = _assembler.ingest(*parsed, receivedAtUs);
    if (!update.completed.empty())
        _queue(std::move(update.completed));
    if (!update.accepted)
        return;
    _scheduler->cancel(_idleTask);
    _idleTask = _scheduler->schedule(this, std::chrono::milliseconds(150), [this]() {
        _idleTask = 0;
        _flush();
    });
    if (!_batchTask) {
        _batchTask = _scheduler->schedule(this, std::chrono::seconds(1), [this]() {
            _batchTask = 0;
            _flush();
        });
    }
}

void NMEASatelliteAdapter::_flush()
{
    _scheduler->cancel(_idleTask);
    _scheduler->cancel(_batchTask);
    _idleTask = _batchTask = 0;
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
        if (!_deliveryTask) {
            _deliveryTask = _scheduler->schedule(this, std::chrono::microseconds::zero(), [this]() {
                _deliveryTask = 0;
                _deliver();
            });
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
        _deliveryTask = _scheduler->schedule(this, std::chrono::microseconds::zero(), [this]() {
            _deliveryTask = 0;
            _deliver();
        });
    }
    emit observationReceived(observation);
}
