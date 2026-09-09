#include "GPSReceiver.h"

#include <QtCore/QPointer>

#include "GPSReceiverFactGroup.h"
#include "GPSReceiverPositionSource.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverLog, "GPS.Receiver.GPSReceiver")

GPSReceiver::GPSReceiver(GPSReceiverSession& session, QObject* parent)
    : QObject(parent)
    , _session(session)
    , _health(this)
    , _satellites(this)
    , _positionSource(new GPSReceiverPositionSource(this))
    , _facts(new GPSReceiverFactGroup(this))
{
    qCDebug(GPSReceiverLog) << this;

    connect(&_satellites, &GPSSatelliteStore::observationChanged, this,
            [this](const GPSSatelliteObservation& observation) {
                const QPointer<GPSReceiver> guard(this);
                _health.applySatelliteObservation(observation);
                if (guard) {
                    emit satellitesReceived(observation);
                }
            });
    _satellites.beginSession(QStringLiteral("nativeReceiver"), _session.sessionId());
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, [this]() {
        const QPointer<GPSReceiver> guard(this);
        _facts->numSatellites()->setRawValue(_health.satellitesInViewCount());
        if (guard) {
            _facts->numSatellitesUsed()->setRawValue(_health.satellitesInUseCount());
        }
    });

    connect(&_health, &GPSSourceHealth::positionChanged, this, [this]() {
        const auto observation = _health.observation();
        if (_health.state() == GPSSourceHealth::NoData || _health.state() == GPSSourceHealth::Stale ||
            observation.ageMilliseconds() < 0) {
            _facts->resetPosition();
        } else {
            _facts->updatePosition(observation);
        }
    });

    connect(&_session, &GPSReceiverSession::receiverTypeChanged, this, &GPSReceiver::receiverTypeChanged);
    connect(&_session, &GPSReceiverSession::capabilitiesUpdated, this, &GPSReceiver::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::connectionErrorDetail, this, &GPSReceiver::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::stateChanged, this, &GPSReceiver::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::configurationStarted, this, &GPSReceiver::configurationStarted);
    connect(&_session, &GPSReceiverSession::attemptChanged, this, &GPSReceiver::_attemptChanged);
    connect(&_session, &GPSReceiverSession::connectionError, this, &GPSReceiver::_onGPSConnectionError);
    connect(&_session, &GPSReceiverSession::stateChanged, this, &GPSReceiver::receiverStateChanged);
    connect(&_session, &GPSReceiverSession::positionReceived, this, &GPSReceiver::_sensorGpsUpdate);
    connect(&_session, &GPSReceiverSession::satellitesReceived, this, &GPSReceiver::_satelliteInfoUpdate);
    connect(&_session, &GPSReceiverSession::relativePositionReceived, this, &GPSReceiver::relativePositionReceived);
    _attemptChanged(_session.attempt());
}

GPSReceiver::~GPSReceiver()
{
    qCDebug(GPSReceiverLog) << this;

    _session.disconnect(this);
}

void GPSReceiver::_attemptChanged(const GPSReceiverAttempt& attempt)
{
    const QPointer<GPSReceiver> guard(this);
    const quint64 generation = attempt.generation;
    if (attempt.ready()) {
        _onGPSConnect();
    } else if (attempt.terminal() || attempt.phase == GPSReceiverAttempt::Phase::Connecting) {
        _onGPSDisconnect();
    }
    if (!guard || _session.attempt().generation != generation) {
        return;
    }
    if (attempt.phase == GPSReceiverAttempt::Phase::Connecting) {
        _satellites.beginSession(QStringLiteral("nativeReceiver"), generation);
    }
    if (guard && _session.attempt().generation == generation) {
        _facts->lastError()->setRawValue(static_cast<int>(attempt.error));
    }
}

void GPSReceiver::_onGPSConnect()
{
    const QPointer<GPSReceiver> guard(this);
    const quint64 generation = _session.sessionId();
    const bool wasConnected = connected();
    _facts->connected()->setRawValue(true);
    if (guard && generation == _session.sessionId() && !wasConnected) {
        emit connectedChanged();
    }
}

void GPSReceiver::_onGPSDisconnect()
{
    const QPointer<GPSReceiver> guard(this);
    const quint64 generation = _session.sessionId();
    const auto current = [&]() { return guard && generation == _session.sessionId(); };
    const bool wasConnected = connected();
    _facts->connected()->setRawValue(false);
    if (!current()) {
        return;
    }
    if (wasConnected) {
        emit connectedChanged();
    }
    if (!current()) {
        return;
    }
    _positionSource->reset();
    if (!current()) {
        return;
    }
    _health.reset();
    if (current()) {
        _satellites.clear();
    }
}

void GPSReceiver::_onGPSConnectionError(GPSConnectionError error)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSReceiverLog) << "Failed to open GPS receiver transport";
            break;
        case GPSConnectionError::ConfigFailed:
            qCWarning(GPSReceiverLog) << "GPS receiver did not accept configuration";
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSReceiverLog) << "GPS device error, connection lost";
            break;
        case GPSConnectionError::None:
            break;
    }

    if (error != GPSConnectionError::None) {
        emit connectionFailed();
    }
}

bool GPSReceiver::connected() const
{
    return _facts->connected()->rawValue().toBool();
}

GPSReceiver::SatelliteCounts GPSReceiver::countSatellites(const GPSSatelliteObservation& msg)
{
    SatelliteCounts counts;
    counts.inView = static_cast<uint8_t>(msg.satellites.size());
    counts.used = msg.usedCount();
    return counts;
}

void GPSReceiver::_satelliteInfoUpdate(const GPSSatelliteObservation& msg)
{
    _satellites.updateObservation(msg);
}

void GPSReceiver::_sensorGpsUpdate(const GPSObservation& msg)
{
    if (connected()) {
        const QPointer<GPSReceiver> guard(this);
        const quint64 sessionId = _session.sessionId();
        _positionSource->updatePosition(msg);
        if (guard && connected() && sessionId == _session.sessionId()) {
            _health.updateObservation(msg);
        }
    }
}
