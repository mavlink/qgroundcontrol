#include "NMEADecoderSession.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <utility>

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include "MonotonicClock.h"
#include "NMEAPositionSource.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NMEADecoderSessionLog, "GPS.NMEA.NMEADecoderSession")

NMEADecoderSession::NMEADecoderSession(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _satelliteFlushTask(_scheduler, this)
    , _satelliteDeliveryTask(_scheduler, this)
    , _health(this, _scheduler)
    , _satellites(this, _health.freshnessTimeoutMs(), _scheduler)
    , _activityTask(_scheduler, this)
{
    qCDebug(NMEADecoderSessionLog) << this;
    connect(&_satellites, &GPSSatelliteStore::observationChanged, &_health,
            &GPSSourceHealth::applySatelliteObservation);
    connect(&_satellites, &GPSSatelliteStore::observationChanged, this,
            [this](const GPSSatelliteObservation& observation) {
                if (observation.sessionId == _sessionId) {
                    emit satellitesReceived(observation);
                }
            });
}

NMEADecoderSession::~NMEADecoderSession()
{
    qCDebug(NMEADecoderSessionLog) << this;
    blockSignals(true);
    _health.blockSignals(true);
    _satellites.blockSignals(true);
    stop();
}

QGeoPositionInfoSource* NMEADecoderSession::positionSource() const
{
    return _decoders.position.get();
}

bool NMEADecoderSession::start(QIODevice* device)
{
    const QPointer<NMEADecoderSession> guard(this);
    const QPointer<QIODevice> deviceGuard(device);
    stop();
    if (!guard || _active || !deviceGuard || !deviceGuard->isReadable() || deviceGuard->thread() != thread()) {
        return false;
    }
    const quint64 session = ++_sessionId;
    _active = true;
    _decoders.position = std::make_unique<NMEAPositionSource>(device, nullptr, _scheduler);
    connect(_decoders.position.get(), &NMEAPositionSource::dataReceived, this, [this, session](quint64 receivedAtUs) {
        if (_active && session == _sessionId) {
            _receivedData(receivedAtUs);
        }
    });
    connect(
        _decoders.position.get(), &NMEAPositionSource::closed, this,
        [this, session]() {
            if (_active && session == _sessionId) {
                stop();
            }
        },
        Qt::QueuedConnection);
    _satellitesOpen = true;
    connect(_decoders.position.get(), &NMEAPositionSource::sentenceReceived, this,
            [this, session](const NMEASentenceEnvelope& sentence) {
                if (_active && session == _sessionId) {
                    _ingestSatellites(sentence);
                }
            });
    connect(_decoders.position.get(), &NMEAPositionSource::closed, this, [this, session]() {
        if (session == _sessionId) {
            _closeSatellites();
        }
    });
    connect(_decoders.position.get(), &NMEAPositionSource::observationReceived, &_health,
            [this, session](GPSObservation observation) {
                if (!_active || session != _sessionId) {
                    return;
                }
                observation.sessionId = _sessionId;
                _health.updateObservation(observation);
            });
    connect(_decoders.position.get(), &QGeoPositionInfoSource::errorOccurred, &_health,
            [this, session](QGeoPositionInfoSource::Error error) {
                if (_active && session == _sessionId && error != QGeoPositionInfoSource::NoError) {
                    _health.invalidatePosition();
                }
            });
    _health.reset();
    if (!guard || !_active || session != _sessionId) {
        return false;
    }
    _satellites.beginSession(QStringLiteral("nmeaReceiver"), session);
    if (!guard || !_active || session != _sessionId) {
        return false;
    }
    _decoders.position->startUpdates();
    return guard && _active && session == _sessionId;
}

void NMEADecoderSession::_updateSatellites(const GPSSatelliteObservation& report)
{
    if (!_active) {
        return;
    }
    auto observation = report;
    observation.sessionId = _sessionId;
    observation.sourceId = QStringLiteral("nmeaReceiver");
    _satellites.updateObservation(observation);
}

void NMEADecoderSession::setFreshnessTimeoutMs(int timeoutMs)
{
    const QPointer<NMEADecoderSession> guard(this);
    const quint64 session = _sessionId;
    _health.setFreshnessTimeoutMs(timeoutMs);
    if (guard && session == _sessionId) {
        _satellites.setFreshnessTimeoutMs(timeoutMs);
        if (guard && session == _sessionId && _lastDataTimestampUs) {
            _receivedData(_lastDataTimestampUs);
        }
    }
}

void NMEADecoderSession::stop()
{
    if (!_active) {
        return;
    }
    _active = false;
    const quint64 retiredSession = ++_sessionId;
    _activityTask.cancel();
    _lastDataTimestampUs = 0;
    _receiving = false;
    _closeSatellites();
    const QPointer<NMEADecoderSession> guard(this);
    {
        // The retiring decoder may install a replacement from its destruction notification.
        auto retired = std::exchange(_decoders, {});
    }
    if (!guard || _active || retiredSession != _sessionId) {
        return;
    }
    _health.reset();
    if (guard && !_active && retiredSession == _sessionId) {
        _satellites.beginSession(QStringLiteral("nmeaReceiver"), retiredSession);
        if (guard && retiredSession == _sessionId) {
            emit activityChanged();
        }
    }
}

void NMEADecoderSession::_closeSatellites()
{
    _satellitesOpen = false;
    _satelliteFlushTask.cancel();
    _satelliteDeliveryTask.cancel();
    _satelliteAssembler.clear();
    _pendingSatellites.clear();
}

void NMEADecoderSession::_ingestSatellites(const NMEASentenceEnvelope& sentence)
{
    if (!_satellitesOpen) {
        return;
    }
    auto update = _satelliteAssembler.ingest(sentence.sentence(), sentence.receivedAtUs(), _scheduler->nowUs());
    _queueSatellites(std::move(update.completed));
    _scheduleSatelliteFlush();
}

void NMEADecoderSession::_flushSatellites()
{
    if (!_satellitesOpen) {
        return;
    }
    _queueSatellites(_satelliteAssembler.flushDue(_scheduler->nowUs()));
    _scheduleSatelliteFlush();
}

void NMEADecoderSession::_scheduleSatelliteFlush()
{
    _satelliteFlushTask.cancel();
    if (const auto deadline = _satelliteAssembler.deadlineUs()) {
        const auto now = _scheduler->nowUs();
        _satelliteFlushTask.schedule(std::chrono::microseconds(*deadline > now ? *deadline - now : 0),
                                     [this]() { _flushSatellites(); });
    }
}

void NMEADecoderSession::_queueSatellites(NMEA::SatelliteEpoch epoch)
{
    GPSSatelliteObservation observation;
    observation.updateMode = GPSSatelliteObservation::UpdateMode::ConstellationDelta;
    for (const auto& system : epoch) {
        GPSSatelliteConstellation constellation;
        constellation.constellation = system.constellation;
        constellation.view.receivedAtUs = system.inViewTimestampUs;
        constellation.view.count = system.inView;
        constellation.usage.receivedAtUs = system.inUseTimestampUs;
        if (system.usedIds) {
            constellation.usage.count = static_cast<int>(system.usedIds->size());
        }
        observation.constellations.append(constellation);
        observation.monotonicTimestampUs =
            std::max<quint64>({observation.monotonicTimestampUs, system.inViewTimestampUs, system.inUseTimestampUs});
    }
    if (observation.constellations.isEmpty()) {
        return;
    }
    constexpr qsizetype MAX_PENDING_EPOCHS = 64;
    if (_pendingSatellites.size() >= MAX_PENDING_EPOCHS) {
        _pendingSatellites.removeFirst();
    }
    _pendingSatellites.append(observation);
    if (!_satelliteDeliveryTask.active()) {
        _satelliteDeliveryTask.schedule(std::chrono::microseconds::zero(), [this]() { _deliverSatellites(); });
    }
}

void NMEADecoderSession::_deliverSatellites()
{
    if (!_satellitesOpen || _pendingSatellites.isEmpty()) {
        return;
    }
    const auto observation = _pendingSatellites.takeFirst();
    if (!_pendingSatellites.isEmpty()) {
        _satelliteDeliveryTask.schedule(std::chrono::microseconds::zero(), [this]() { _deliverSatellites(); });
    }
    _updateSatellites(observation);
}

void NMEADecoderSession::_receivedData(quint64 receivedAtUs)
{
    if (!_active || !receivedAtUs || receivedAtUs > _scheduler->nowUs()) {
        return;
    }
    const bool previouslyReceived = hasReceivedData();
    _lastDataTimestampUs = std::max(_lastDataTimestampUs, receivedAtUs);
    const auto remaining = MonotonicClock::remaining(_lastDataTimestampUs, _scheduler->nowUs(),
                                                     std::chrono::milliseconds(_health.freshnessTimeoutMs()));
    const bool receiving = remaining > std::chrono::microseconds::zero();
    const bool changed = receiving != _receiving || !previouslyReceived;
    _receiving = receiving;
    _activityTask.cancel();
    if (receiving) {
        _activityTask.schedule(remaining, [this]() {
            _receiving = false;
            emit activityChanged();
        });
    }
    if (changed) {
        emit activityChanged();
    }
}
