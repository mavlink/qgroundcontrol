#include "GPSRtk.h"

#include "GPSRTKFactGroup.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverPositionSource.h"
#include "GPSType.h"
#include "QGCLoggingCategory.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#include "SerialPortManager.h"
#endif

#include <QtCore/QPointer>

#include <utility>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.RTK.GPSRtk")

GPSRtk::GPSRtk(QObject* parent)
    : QObject(parent)
    , _session(this)
    , _health(this)
    , _positionSource(new GPSReceiverPositionSource(this))
    , _gpsRtkFactGroup(new GPSRTKFactGroup(this))
{
    qCDebug(GPSRtkLog) << this;

    connect(&_health, &GPSSourceHealth::satellitesChanged, this, [this]() {
        _gpsRtkFactGroup->numSatellites()->setRawValue(qMax(0, _health.satellitesInViewCount()));
        _gpsRtkFactGroup->numSatellitesUsed()->setRawValue(qMax(0, _health.satellitesInUseCount()));
    });

    connect(&_session, &GPSReceiverSession::receiverTypeChanged, this, &GPSRtk::receiverTypeChanged);
    connect(&_session, &GPSReceiverSession::capabilitiesUpdated, this, &GPSRtk::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::connectionErrorDetail, this, &GPSRtk::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::stateChanged, this, &GPSRtk::diagnosticsChanged);
    connect(&_session, &GPSReceiverSession::configurationStarted, this, &GPSRtk::configurationStarted);
    connect(&_session, &GPSReceiverSession::receiverReady, this, &GPSRtk::_onGPSConnect);
    connect(&_session, &GPSReceiverSession::disconnected, this, &GPSRtk::_onGPSDisconnect);
    connect(&_session, &GPSReceiverSession::connectionError, this, &GPSRtk::_onGPSConnectionError);
    connect(&_session, &GPSReceiverSession::stateChanged, this, &GPSRtk::receiverStateChanged);
    connect(&_session, &GPSReceiverSession::positionReceived, this, &GPSRtk::_sensorGpsUpdate);
    connect(&_session, &GPSReceiverSession::satellitesReceived, this, &GPSRtk::_satelliteInfoUpdate);
    connect(&_session, &GPSReceiverSession::relativePositionReceived, this, &GPSRtk::relativePositionReceived);
    connect(&_session, &GPSReceiverSession::rtcmReceived, this, &GPSRtk::rtcmDataReceived);
    connect(&_session, &GPSReceiverSession::rtcmFrameReceived, this, &GPSRtk::rtcmFrameReceived);
    connect(&_session, &GPSReceiverSession::surveyInReceived, this, &GPSRtk::_onGPSSurveyInStatus);
}

GPSRtk::~GPSRtk()
{
    qCDebug(GPSRtkLog) << this;

    disconnectGPS();
    _session.disconnect(this);
}

void GPSRtk::_onGPSConnect()
{
    _gpsRtkFactGroup->lastError()->setRawValue(static_cast<int>(GPSConnectionError::None));
    const bool wasConnected = connected();
    _gpsRtkFactGroup->connected()->setRawValue(true);
    if (!wasConnected) {
        emit connectedChanged();
    }
}

void GPSRtk::_onGPSDisconnect()
{
    const bool wasConnected = connected();
    _gpsRtkFactGroup->connected()->setRawValue(false);
    if (wasConnected) {
        emit connectedChanged();
    }
    _positionSource->reset();
    _health.reset();
    _gpsRtkFactGroup->valid()->setRawValue(false);
    _gpsRtkFactGroup->active()->setRawValue(false);
    _gpsRtkFactGroup->currentDuration()->setRawValue(0);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(qQNaN());
    _gpsRtkFactGroup->currentLatitude()->setRawValue(qQNaN());
    _gpsRtkFactGroup->currentLongitude()->setRawValue(qQNaN());
    _gpsRtkFactGroup->currentAltitude()->setRawValue(qQNaN());
    _gpsRtkFactGroup->numSatellites()->setRawValue(0);
    _gpsRtkFactGroup->numSatellitesUsed()->setRawValue(0);
}

void GPSRtk::_onGPSConnectionError(GPSConnectionError error)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSRtkLog) << "Failed to open GPS receiver transport";
            break;
        case GPSConnectionError::ConfigFailed:
            qCWarning(GPSRtkLog) << "GPS receiver did not accept configuration";
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSRtkLog) << "GPS device error, connection lost";
            break;
        case GPSConnectionError::None:
            break;
    }

    _gpsRtkFactGroup->lastError()->setRawValue(static_cast<int>(error));
    if (error != GPSConnectionError::None) {
        emit connectionFailed();
    }
}

void GPSRtk::_onGPSSurveyInStatus(const GPSSurveyInStatus& status)
{
    _gpsRtkFactGroup->currentDuration()->setRawValue(status.durationSecs);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(status.meanAccuracyMM) / 1000.0);
    _gpsRtkFactGroup->currentLatitude()->setRawValue(status.latitude);
    _gpsRtkFactGroup->currentLongitude()->setRawValue(status.longitude);
    _gpsRtkFactGroup->currentAltitude()->setRawValue(status.altitude);
    _gpsRtkFactGroup->valid()->setRawValue(status.valid);
    _gpsRtkFactGroup->active()->setRawValue(status.active);
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtk::connectGPS(const QString& device, QStringView gps_type, GPSReceiverConfig config)
{
    auto reservation = SerialPortManager::instance()->reservePort(device);
    if (!reservation) {
        qCDebug(GPSRtkLog) << "Serial port is already reserved:" << device;
        return;
    }
    const GPSType type = GPSReceiverCapabilities::typeForName(gps_type).value_or(GPSType::u_blox);
    connectReceiver(
        type,
        [device, reservation](const std::atomic_bool& requestStop) {
            return std::make_unique<SerialGPSTransport>(device, requestStop);
        },
        config);
}
#endif

void GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory, GPSReceiverConfig config)
{
    _gpsRtkFactGroup->lastError()->setRawValue(static_cast<int>(GPSConnectionError::None));
    _session.start(type, std::move(transportFactory), config);
}

void GPSRtk::disconnectGPS()
{
    _session.stop();
}

void GPSRtk::shutdown()
{
    _session.shutdown();
}

bool GPSRtk::connected() const
{
    return _gpsRtkFactGroup->connected()->rawValue().toBool();
}

FactGroup* GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup;
}

GPSRtk::SatelliteCounts GPSRtk::countSatellites(const GPSSatelliteObservation& msg)
{
    SatelliteCounts counts;
    counts.inView = static_cast<uint8_t>(msg.satellites.size());
    counts.used = msg.usedCount();
    return counts;
}

void GPSRtk::_satelliteInfoUpdate(const GPSSatelliteObservation& msg)
{
    const SatelliteCounts counts = countSatellites(msg);
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << QStringLiteral("%1 in view, %2 used").arg(counts.inView).arg(counts.used);
    const qint64 age = GPSSourceHealth::ageMilliseconds(msg.monotonicTimestampUs);
    _health.updateSatelliteCounts(counts.inView, counts.used, age);
}

void GPSRtk::_sensorGpsUpdate(const GPSObservation& msg)
{
    if (connected()) {
        const quint64 sessionId = _session.sessionId();
        _positionSource->updatePosition(msg);
        if (connected() && sessionId == _session.sessionId()) {
            _health.updateObservation(msg);
        }
    }
}
