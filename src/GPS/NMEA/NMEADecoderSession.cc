#include "NMEADecoderSession.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include <algorithm>
#include <chrono>

#include "NMEAPositionSource.h"
#include "NMEASatelliteAdapter.h"
#include "NMEAStreamSplitter.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NMEADecoderSessionLog, "GPS.NMEA.NMEADecoderSession")

NMEADecoderSession::NMEADecoderSession(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent),
      _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)),
      _health(this, _scheduler),
      _satellites(this, _health.freshnessTimeoutMs(), _scheduler),
      _activityTask(_scheduler, this)
{
    qCDebug(NMEADecoderSessionLog) << this;
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, &NMEADecoderSession::satellitesChanged);
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
    return _positionSource.get();
}

bool NMEADecoderSession::start(QIODevice* device)
{
    const QPointer<NMEADecoderSession> guard(this);
    const QPointer<QIODevice> deviceGuard(device);
    stop();
    if (!guard || _active || !_scheduler || !deviceGuard || !deviceGuard->isReadable() ||
        deviceGuard->thread() != thread() || _scheduler->thread() != thread()) {
        return false;
    }
    const quint64 session = ++_sessionId;
    _active = true;
    _stream = std::make_unique<NMEAStreamSplitter>(device, nullptr, _scheduler);
    connect(_stream.get(), &NMEAStreamSplitter::dataReceived, this, &NMEADecoderSession::_receivedData);
    connect(
        _stream.get(), &NMEAStreamSplitter::closed, this,
        [this, session]() {
            if (_active && session == _sessionId) {
                stop();
            }
        },
        Qt::QueuedConnection);
    _satelliteAdapter = std::make_unique<NMEASatelliteAdapter>(nullptr, _scheduler);
    connect(_satelliteAdapter.get(), &NMEASatelliteAdapter::observationReceived, this,
            [this, session](const GPSSatelliteObservation& observation) {
                if (_active && session == _sessionId) {
                    _updateSatellites(observation);
                }
            });
    connect(_stream.get(), &NMEAStreamSplitter::sentenceReceived, _satelliteAdapter.get(),
            &NMEASatelliteAdapter::ingest);
    connect(_stream.get(), &NMEAStreamSplitter::closed, _satelliteAdapter.get(), &NMEASatelliteAdapter::close);
    _positionSource = std::make_unique<NMEAPositionSource>(_stream->positionDevice(), nullptr, _scheduler);
    connect(_positionSource.get(), &NMEAPositionSource::observationReceived, &_health,
            [this, session](GPSObservation observation) {
                if (!_active || session != _sessionId) {
                    return;
                }
                observation.sessionId = _sessionId;
                _health.updateObservation(observation);
            });
    connect(_positionSource.get(), &QGeoPositionInfoSource::errorOccurred, &_health,
            [this](QGeoPositionInfoSource::Error error) {
                if (error != QGeoPositionInfoSource::NoError) {
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
    _positionSource->startUpdates();
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
    const QPointer<NMEADecoderSession> guard(this);
    _positionSource.reset();
    if (!guard || _active || retiredSession != _sessionId) {
        return;
    }
    _satelliteAdapter.reset();
    if (!guard || _active || retiredSession != _sessionId) {
        return;
    }
    _stream.reset();
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

void NMEADecoderSession::_receivedData(quint64 receivedAtUs)
{
    if (!_active || !_scheduler || !receivedAtUs || receivedAtUs > _scheduler->nowUs()) {
        return;
    }
    const bool previouslyReceived = hasReceivedData();
    _lastDataTimestampUs = std::max(_lastDataTimestampUs, receivedAtUs);
    const auto ageUs = _scheduler->nowUs() - _lastDataTimestampUs;
    const auto lifetimeUs = static_cast<quint64>(
        std::chrono::microseconds(std::chrono::milliseconds(_health.freshnessTimeoutMs())).count());
    const bool receiving = ageUs < lifetimeUs;
    const bool changed = receiving != _receiving || !previouslyReceived;
    _receiving = receiving;
    _activityTask.cancel();
    if (receiving) {
        _activityTask.schedule(std::chrono::microseconds(lifetimeUs - ageUs), [this]() {
            _receiving = false;
            emit activityChanged();
        });
    }
    if (changed) {
        emit activityChanged();
    }
}
