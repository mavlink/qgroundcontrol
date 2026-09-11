#include "GPSReceiver.h"

#include <QtCore/QPointer>

#include "GPSReceiverFactGroup.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverLog, "GPS.Receiver.GPSReceiver")

GPSReceiver::GPSReceiver(GPSReceiverState& state, QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _session(state.session())
    , _health(*state.health())
    , _facts(new GPSReceiverFactGroup(this, scheduler, state.integrity()))
{
    qCDebug(GPSReceiverLog) << this;

    connect(state.satellites(), &GPSSatelliteStore::observationChanged, this, &GPSReceiver::satellitesReceived);
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
        const auto observation = _health.acceptedObservation(GPSObservation::PositionUse::Diagnostics);
        if (!guard || _projectionRevision != revision || _session.sessionId() != sessionId) {
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
    _attemptChanged(_session.attempt());
    _facts->numSatellites()->setRawValue(_health.satellitesInViewCount());
    _facts->numSatellitesUsed()->setRawValue(_health.satellitesInUseCount());
    if (const auto observation = _health.acceptedObservation(GPSObservation::PositionUse::Diagnostics)) {
        _facts->updatePosition(*observation);
    }
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
