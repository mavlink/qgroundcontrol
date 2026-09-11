#include "GPSReceiver.h"

#include <QtCore/QPointer>

#include "GPSReceiverFactGroup.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverLog, "GPS.Receiver.GPSReceiver")

GPSReceiver::GPSReceiver(GPSReceiverSession& session, QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _session(session)
    , _health(this, scheduler)
    , _satellites(this, GPSSourceHealth::FRESHNESS_TIMEOUT_MS, scheduler)
    , _facts(new GPSReceiverFactGroup(this, scheduler))
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
        const QPointer<GPSReceiver> guard(this);
        const quint64 revision = ++_projectionRevision;
        const quint64 sessionId = _session.sessionId();
        const auto current = [&]() {
            return guard && _projectionRevision == revision && _session.sessionId() == sessionId;
        };
        const auto observation = _health.acceptedObservation(GPSObservation::PositionUse::Diagnostics);
        if (_health.state() == GPSSourceHealth::NoData) {
            _facts->integrity()->reset();
        } else {
            _facts->integrity()->update(GPSIntegrityObservation::fromPosition(_health.observation()));
        }
        if (!current()) {
            return;
        }
        if (observation) {
            _facts->updatePosition(*observation);
        } else {
            _facts->resetPosition();
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

void GPSReceiver::_satelliteInfoUpdate(const GPSSatelliteObservation& msg)
{
    _satellites.updateObservation(msg);
}

void GPSReceiver::_sensorGpsUpdate(const GPSObservation& msg)
{
    if (connected()) {
        _health.updateObservation(msg);
    }
}
