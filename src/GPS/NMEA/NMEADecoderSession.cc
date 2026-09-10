#include "NMEADecoderSession.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include "GPSQtRuntimeScheduler.h"
#include "NMEAPositionSource.h"
#include "NMEASatelliteAdapter.h"
#include "NMEAStreamSplitter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEADecoderSessionLog, "GPS.NMEA.NMEADecoderSession")

NMEADecoderSession::NMEADecoderSession(QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _health(this, _scheduler)
    , _satellites(this, 5000, _scheduler)
{
    qCDebug(NMEADecoderSessionLog) << this;
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, &NMEADecoderSession::satellitesChanged);
    connect(&_satellites, &GPSSatelliteStore::observationChanged, this,
            [this](const GPSSatelliteObservation& observation) {
                const QPointer<NMEADecoderSession> guard(this);
                _health.applySatelliteObservation(observation);
                if (guard && observation.sessionId == _sessionId) {
                    emit satellitesReceived(observation);
                }
            });
}

NMEADecoderSession::~NMEADecoderSession()
{
    qCDebug(NMEADecoderSessionLog) << this;
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
    if (!guard || _active || !_scheduler || !deviceGuard || !deviceGuard->isReadable()) {
        return false;
    }
    const quint64 session = ++_sessionId;
    _active = true;
    _stream = std::make_unique<NMEAStreamSplitter>(device);
    _satelliteAdapter = std::make_unique<NMEASatelliteAdapter>(nullptr, nullptr, _scheduler);
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
    }
}

void NMEADecoderSession::stop()
{
    if (!_active) {
        return;
    }
    _active = false;
    const quint64 retiredSession = ++_sessionId;
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
    }
}
