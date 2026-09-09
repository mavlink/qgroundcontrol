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
    , _positionSource(new GPSReceiverPositionSource(this))
    , _facts(new GPSReceiverFactGroup(this))
{
    qCDebug(GPSReceiverLog) << this;

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

    connect(&_session, &GPSReceiverSession::receiverTypeChanged, this, [this](GPSType type) {
        const QPointer<GPSReceiver> guard(this);
        _facts->lastError()->setRawValue(static_cast<int>(GPSConnectionError::None));
        if (guard) {
            emit receiverTypeChanged(type);
        }
    });
    connect(&_session, &GPSReceiverSession::capabilitiesUpdated, this, &GPSReceiver::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::connectionErrorDetail, this, &GPSReceiver::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::stateChanged, this, &GPSReceiver::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::configurationStarted, this, &GPSReceiver::configurationStarted);
    connect(&_session, &GPSReceiverSession::receiverReady, this, &GPSReceiver::_onGPSConnect);
    connect(&_session, &GPSReceiverSession::disconnected, this, &GPSReceiver::_onGPSDisconnect);
    connect(&_session, &GPSReceiverSession::connectionError, this, &GPSReceiver::_onGPSConnectionError);
    connect(&_session, &GPSReceiverSession::stateChanged, this, &GPSReceiver::receiverStateChanged);
    connect(&_session, &GPSReceiverSession::positionReceived, this, &GPSReceiver::_sensorGpsUpdate);
    connect(&_session, &GPSReceiverSession::satellitesReceived, this, &GPSReceiver::_satelliteInfoUpdate);
    connect(&_session, &GPSReceiverSession::relativePositionReceived, this, &GPSReceiver::relativePositionReceived);
    if (_session.ready()) {
        _onGPSConnect();
    }
}

GPSReceiver::~GPSReceiver()
{
    qCDebug(GPSReceiverLog) << this;

    _session.disconnect(this);
}

void GPSReceiver::_onGPSConnect()
{
    _facts->lastError()->setRawValue(static_cast<int>(GPSConnectionError::None));
    const bool wasConnected = connected();
    _facts->connected()->setRawValue(true);
    if (!wasConnected) {
        emit connectedChanged();
    }
}

void GPSReceiver::_onGPSDisconnect()
{
    const bool wasConnected = connected();
    _facts->connected()->setRawValue(false);
    if (wasConnected) {
        emit connectedChanged();
    }
    _positionSource->reset();
    _health.reset();
    _facts->numSatellites()->setRawValue(-1);
    _facts->numSatellitesUsed()->setRawValue(-1);
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

    _facts->lastError()->setRawValue(static_cast<int>(error));
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
    const SatelliteCounts counts = countSatellites(msg);
    qCDebug(GPSReceiverLog) << Q_FUNC_INFO << QStringLiteral("%1 in view, %2 used").arg(counts.inView).arg(counts.used);
    const qint64 age = GPSSourceHealth::ageMilliseconds(msg.monotonicTimestampUs);
    _health.updateSatelliteCounts(counts.inView, counts.used, age);
}

void GPSReceiver::_sensorGpsUpdate(const GPSObservation& msg)
{
    if (connected()) {
        const quint64 sessionId = _session.sessionId();
        _positionSource->updatePosition(msg);
        if (connected() && sessionId == _session.sessionId()) {
            _health.updateObservation(msg);
        }
    }
}
