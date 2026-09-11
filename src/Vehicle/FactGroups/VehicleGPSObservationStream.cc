#include "VehicleGPSObservationStream.h"

#include <QtCore/QPointer>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(VehicleGPSObservationStreamLog, "GPS.VehicleGPSObservationStream")

VehicleGPSObservationStream::VehicleGPSObservationStream(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _integrity1(this, _scheduler)
    , _integrity2(this, _scheduler)
{
    qCDebug(VehicleGPSObservationStreamLog) << this;
}

VehicleGPSObservationStream::~VehicleGPSObservationStream()
{
    qCDebug(VehicleGPSObservationStreamLog) << this;
}

VehicleGPSObservation VehicleGPSObservationStream::gps(int receiver) const
{
    return receiver >= 0 && receiver < 2 ? _gps[receiver] : VehicleGPSObservation();
}

GPSIntegrityStore* VehicleGPSObservationStream::integrity(int receiver)
{
    return receiver == 0 ? &_integrity1 : receiver == 1 ? &_integrity2 : nullptr;
}

void VehicleGPSObservationStream::_publish(int receiver, const VehicleGPSObservation& observation)
{
    const auto revision = ++_revision;
    auto accepted = observation;
    accepted.position.monotonicTimestampUs = _scheduler ? _scheduler->nowUs() : 0;
    accepted.fusedPosition.monotonicTimestampUs = accepted.position.monotonicTimestampUs;
    _gps[receiver] = accepted;
    const QPointer<VehicleGPSObservationStream> guard(this);
    if (accepted.fusedPosition.position.isValid()) {
        _fused = accepted.fusedPosition;
        emit fusedPositionReceived(_fused);
        if (!guard || revision != _revision) {
            return;
        }
    }
    emit gpsReceived(receiver, accepted);
}

void VehicleGPSObservationStream::handleMessage(const mavlink_message_t& message, int systemId, int defaultComponentId)
{
    if (systemId && message.sysid != systemId) {
        return;
    }
    switch (message.msgid) {
        case MAVLINK_MSG_ID_GPS_RAW_INT: {
            mavlink_gps_raw_int_t value = {};
            mavlink_msg_gps_raw_int_decode(&message, &value);
            _publish(0, VehicleGPSObservation::fromMessage(value));
            break;
        }
        case MAVLINK_MSG_ID_GPS2_RAW: {
            mavlink_gps2_raw_t value = {};
            mavlink_msg_gps2_raw_decode(&message, &value);
            _publish(1, VehicleGPSObservation::fromMessage(value));
            break;
        }
        case MAVLINK_MSG_ID_HIGH_LATENCY: {
            mavlink_high_latency_t value = {};
            mavlink_msg_high_latency_decode(&message, &value);
            _publish(0, VehicleGPSObservation::fromMessage(value));
            break;
        }
        case MAVLINK_MSG_ID_HIGH_LATENCY2: {
            mavlink_high_latency2_t value = {};
            mavlink_msg_high_latency2_decode(&message, &value);
            _publish(0, VehicleGPSObservation::fromMessage(value));
            break;
        }
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
            if (defaultComponentId && message.compid != defaultComponentId) {
                break;
            }
            mavlink_global_position_int_t value = {};
            mavlink_msg_global_position_int_decode(&message, &value);
            ++_revision;
            _fused = {};
            if (value.lat != 0 || value.lon != 0) {
                _fused.receivedAt = QDateTime::currentDateTimeUtc();
                _fused.monotonicTimestampUs = _scheduler ? _scheduler->nowUs() : 0;
                _fused.sourceId = QStringLiteral("VehicleEKF");
                _fused.position = QGeoPositionInfo(
                    QGeoCoordinate(value.lat * 1e-7, value.lon * 1e-7, value.alt / 1000.0), _fused.receivedAt);
                _fused.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
                _fused.fixQuality = GPSObservation::FixQuality::Extrapolated;
            }
            emit fusedPositionReceived(_fused);
            break;
        }
        case MAVLINK_MSG_ID_GNSS_INTEGRITY: {
            mavlink_gnss_integrity_t value = {};
            mavlink_msg_gnss_integrity_decode(&message, &value);
            _integrityReceived(value);
            break;
        }
        default:
            break;
    }
}

void VehicleGPSObservationStream::_integrityReceived(const mavlink_gnss_integrity_t& message)
{
    auto* store = integrity(message.id);
    if (!store) {
        return;
    }
    GPSIntegrityObservation observation;
    observation.monotonicTimestampUs = _scheduler ? _scheduler->nowUs() : 0;
    observation.systemErrors = message.system_errors;
    if (message.spoofing_state != UINT8_MAX) {
        observation.spoofingState = message.spoofing_state;
    }
    if (message.jamming_state != UINT8_MAX) {
        observation.jammingState = message.jamming_state;
    }
    if (message.authentication_state != UINT8_MAX) {
        observation.authenticationState = message.authentication_state;
    }
    if (message.corrections_quality != UINT8_MAX) {
        observation.correctionsQuality = message.corrections_quality;
    }
    if (message.system_status_summary != UINT8_MAX) {
        observation.systemQuality = message.system_status_summary;
    }
    if (message.gnss_signal_quality != UINT8_MAX) {
        observation.gnssSignalQuality = message.gnss_signal_quality;
    }
    if (message.post_processing_quality != UINT8_MAX) {
        observation.postProcessingQuality = message.post_processing_quality;
    }
    const QPointer<VehicleGPSObservationStream> guard(this);
    store->updateObservation(observation);
    if (guard) {
        emit integrityReceived(message.id);
    }
}

void VehicleGPSObservationStream::reset()
{
    const QPointer<VehicleGPSObservationStream> guard(this);
    const auto revision = ++_revision;
    _gps = {};
    _fused = {};
    _integrity1.reset();
    if (!guard || revision != _revision) {
        return;
    }
    _integrity2.reset();
    if (!guard || revision != _revision) {
        return;
    }
    emit gpsReceived(0, {});
    if (!guard || revision != _revision) {
        return;
    }
    emit gpsReceived(1, {});
    if (guard && revision == _revision) {
        emit fusedPositionReceived({});
    }
}
