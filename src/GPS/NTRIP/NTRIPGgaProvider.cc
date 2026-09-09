#include "NTRIPGgaProvider.h"

#include <QtCore/QDateTime>

#include "Fact.h"
#include "FactGroup.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtkState.h"
#include "MultiVehicleManager.h"
#include "NMEAUtils.h"
#include "NTRIPSettings.h"
#include "NTRIPTransport.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(NTRIPGgaProviderLog, "GPS.NTRIP.NTRIPGgaProvider")

namespace {

/// Rejects "zero island" (0,0) as well as out-of-range and non-finite values.
/// QGeoCoordinate::isValid() alone accepts (0,0), which is how vehicles report
/// "no fix yet" — we must treat that as invalid for GGA upstream.
bool isSaneCoord(double lat, double lon)
{
    return qIsFinite(lat) && qIsFinite(lon) && !(lat == 0.0 && lon == 0.0) && qAbs(lat) <= 90.0 && qAbs(lon) <= 180.0;
}

GPSObservation::FixQuality vehicleFixQuality(int fix)
{
    switch (fix) {
        case 0:
        case 1:
            return GPSObservation::FixQuality::NoFix;
        case 2:
            return GPSObservation::FixQuality::Fix2D;
        case 3:
            return GPSObservation::FixQuality::Fix3D;
        case 4:
            return GPSObservation::FixQuality::Differential;
        case 5:
            return GPSObservation::FixQuality::RTKFloat;
        case 6:
            return GPSObservation::FixQuality::RTKFixed;
        default:
            return GPSObservation::FixQuality::Unknown;
    }
}

PositionResult getVehicleGPSPosition(Vehicle* veh, const mavlink_message_t& message)
{
    if (!veh || veh->vehicleLinkManager()->communicationLost())
        return {};

    FactGroup* gps = veh->gpsFactGroup();
    if (!gps)
        return {};

    Fact* latF = gps->getFact(QStringLiteral("lat"));
    Fact* lonF = gps->getFact(QStringLiteral("lon"));
    if (!latF || !lonF)
        return {};

    const double lat = latF->rawValue().toDouble();
    const double lon = lonF->rawValue().toDouble();

    if (isSaneCoord(lat, lon)) {
        GPSObservation observation;
        observation.position = QGeoPositionInfo(QGeoCoordinate(lat, lon), QDateTime::currentDateTimeUtc());
        observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
            mavlink_gps_raw_int_t fix{};
            mavlink_msg_gps_raw_int_decode(&message, &fix);
            observation.fixQuality = vehicleFixQuality(fix.fix_type);
            if (fix.fix_type >= GPS_FIX_TYPE_3D_FIX) {
                observation.position.setCoordinate(QGeoCoordinate(lat, lon, fix.alt / 1000.0));
                observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
            }
            if (fix.eph != UINT16_MAX && fix.eph > 0) {
                observation.horizontalDop = fix.eph / 100.0;
            }
        } else if (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY) {
            mavlink_high_latency_t fix{};
            mavlink_msg_high_latency_decode(&message, &fix);
            observation.fixQuality = vehicleFixQuality(fix.gps_fix_type);
        }
        // HIGH_LATENCY2 has position uncertainty in metres, not HDOP or a fix type.
        if (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY2 &&
            (mavlink_msg_high_latency2_get_failure_flags(&message) & HL_FAILURE_FLAG_GPS)) {
            observation.fixQuality = GPSObservation::FixQuality::NoFix;
        }
        return {observation, QStringLiteral("Vehicle GPS")};
    }
    return {};
}

PositionResult getVehicleEKFPosition(Vehicle* veh, quint64 receivedUs)
{
    if (!veh || receivedUs == 0 || veh->vehicleLinkManager()->communicationLost())
        return {};

    const QGeoCoordinate coord = veh->coordinate();
    if (coord.isValid() && isSaneCoord(coord.latitude(), coord.longitude())) {
        GPSObservation observation;
        observation.position = QGeoPositionInfo(coord, QDateTime::currentDateTimeUtc());
        observation.monotonicTimestampUs = receivedUs;
        observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
        observation.fixQuality = GPSObservation::FixQuality::Extrapolated;
        return {observation, QStringLiteral("Vehicle EKF")};
    }
    return {};
}

PositionResult getRTKBasePosition()
{
    GPSManager* gpsManager = GPSManager::instance();
    if (!gpsManager)
        return {};
    GPSRtkState* rtk = gpsManager->rtkState();
    if (!rtk)
        return {};

    GPSRTKFactGroup* rtkGroup = rtk->facts();
    if (!rtkGroup->valid()->rawValue().toBool())
        return {};

    const double lat = rtkGroup->currentLatitude()->rawValue().toDouble();
    const double lon = rtkGroup->currentLongitude()->rawValue().toDouble();
    const double alt = rtkGroup->currentAltitude()->rawValue().toDouble();

    if (isSaneCoord(lat, lon)) {
        GPSObservation observation;
        observation.position = QGeoPositionInfo(QGeoCoordinate(lat, lon, alt), QDateTime::currentDateTimeUtc());
        // Survey/fixed-base Facts do not identify their altitude datum.
        return {observation, QStringLiteral("RTK Base"), true};
    }
    return {};
}

PositionResult getGCSPosition()
{
    QGCPositionManager* posMgr = QGCPositionManager::instance();
    if (!posMgr)
        return {};

    const QGeoCoordinate coord = posMgr->gcsPosition();
    if (coord.isValid() && isSaneCoord(coord.latitude(), coord.longitude())) {
        GPSObservation observation;
        if (const auto* health = posMgr->sourceHealth()) {
            if (!health->usable()) {
                return {};
            }
            observation = health->observation();
            const int used = health->satellitesInUseCount();
            if (used >= 0) {
                observation.satellitesUsed = used;
            }
        } else {
            const QDateTime received = posMgr->gcsPositionTimestamp();
            const qint64 age = received.msecsTo(QDateTime::currentDateTimeUtc());
            if (!received.isValid() || age < 0 || age >= GPSSourceHealth::FRESHNESS_TIMEOUT_MS) {
                return {};
            }
            observation.position = posMgr->geoPositionInfo();
            if (!observation.usable() || observation.coordinate().latitude() != coord.latitude() ||
                observation.coordinate().longitude() != coord.longitude()) {
                return {};
            }
            observation.receivedAt = received;
            observation.monotonicTimestampUs = GPSObservation::monotonicNowUs() - age * 1000;
        }
        observation.position.setCoordinate(observation.coordinate());
        return {observation, QStringLiteral("GCS Position")};
    }
    return {};
}

}  // anonymous namespace

bool PositionResult::isValid() const
{
    if (!observation.position.isValid() || observation.fixQuality == GPSObservation::FixQuality::NoFix) {
        return false;
    }
    const qint64 age = observation.ageMilliseconds();
    return fixedReference ||
           (observation.monotonicTimestampUs != 0 && age >= 0 && age < GPSSourceHealth::FRESHNESS_TIMEOUT_MS);
}

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent) : QObject(parent)
{
    qCDebug(NTRIPGgaProviderLog) << this;
    _timer.setInterval(_normalInterval);
    connect(&_timer, &QChronoTimer::timeout, this, &NTRIPGgaProvider::_sendGGA);
}

NTRIPGgaProvider::~NTRIPGgaProvider()
{
    qCDebug(NTRIPGgaProviderLog) << this;
}

void NTRIPGgaProvider::init(NTRIPSettings* settings)
{
    // Cache the user-selected source and interval so the hot path (_sendGGA)
    // avoids a SettingsManager::instance()->ntripSettings()->...->rawValue()
    // chain per tick.
    if (!settings) {
        return;
    }

    auto* sourceFact = settings->ntripGgaPositionSource();
    auto refreshSource = [this, sourceFact]() {
        _cachedSource = static_cast<PositionSource>(sourceFact->rawValue().toUInt());
    };
    refreshSource();
    connect(sourceFact, &Fact::rawValueChanged, this, refreshSource);

    auto* intervalFact = settings->ntripGgaIntervalSec();
    auto refreshInterval = [this, intervalFact]() {
        const uint seconds = intervalFact->rawValue().toUInt();
        // Guard against 0 from a stale config — fall back to the default.
        _normalInterval =
            (seconds > 0) ? std::chrono::milliseconds{static_cast<qint64>(seconds) * 1000} : kDefaultInterval;
        if (_retryPhase == RetryPhase::Normal) {
            _timer.setInterval(_normalInterval);
        }
    };
    refreshInterval();
    connect(intervalFact, &Fact::rawValueChanged, this, refreshInterval);
}

void NTRIPGgaProvider::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaProvider::start(NTRIPTransport* transport)
{
    _transport = transport;
    _fastRetryCount = 0;
    _clearSource();
    _setRetryPhase(RetryPhase::Fast);
    _sendGGA();
    _timer.start();
}

void NTRIPGgaProvider::stop()
{
    _timer.stop();
    _transport = nullptr;
    _clearSource();
}

void NTRIPGgaProvider::_setRetryPhase(RetryPhase phase)
{
    _retryPhase = phase;
    _timer.setInterval(phase == RetryPhase::Fast ? kFastRetryInterval : _normalInterval);
}

void NTRIPGgaProvider::_clearSource()
{
    if (_source.isEmpty()) {
        return;
    }
    _source.clear();
    emit sourceChanged(_source);
}

void NTRIPGgaProvider::_sendGGA()
{
    if (!_transport) {
        return;
    }

    _trackVehicle();
    _ensureDefaultProviders();

    const auto position = _getBestPosition();

    if (!position.isValid()) {
        _clearSource();
        if (++_fastRetryCount >= 5 && _retryPhase == RetryPhase::Fast) {
            _setRetryPhase(RetryPhase::Normal);
        }
        return;
    }

    _fastRetryCount = 0;
    if (_retryPhase != RetryPhase::Normal) {
        _setRetryPhase(RetryPhase::Normal);
    }

    const QByteArray gga = NMEAUtils::makeGGA(position.observation);
    _transport->sendNMEA(gga);

    if (!position.source.isEmpty() && position.source != _source) {
        _source = position.source;
        emit sourceChanged(_source);
    }
}

void NTRIPGgaProvider::_trackVehicle()
{
    auto* manager = MultiVehicleManager::instance();
    Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
    if (_vehicle == vehicle) {
        return;
    }
    QObject::disconnect(_vehicleMessageConnection);
    _vehicle = vehicle;
    _vehicleGpsPosition = {};
    _vehicleEkfPosition = {};
    if (vehicle) {
        _vehicleMessageConnection =
            connect(vehicle, &Vehicle::mavlinkMessageReceived, this, [this](const mavlink_message_t& message) {
                const quint64 received = GPSObservation::monotonicNowUs();
                if (!_vehicle || message.sysid != _vehicle->id()) {
                    return;
                }
                if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
                    _vehicleGpsPosition = getVehicleGPSPosition(_vehicle, message);
                } else if (message.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
                    if (message.compid == _vehicle->defaultComponentId() &&
                        (mavlink_msg_global_position_int_get_lat(&message) != 0 ||
                         mavlink_msg_global_position_int_get_lon(&message) != 0)) {
                        _vehicleEkfPosition = getVehicleEKFPosition(_vehicle, received);
                    }
                } else if (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY ||
                           message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY2) {
                    _vehicleGpsPosition = getVehicleGPSPosition(_vehicle, message);
                    _vehicleEkfPosition = getVehicleEKFPosition(_vehicle, received);
                }
            });
    }
}

void NTRIPGgaProvider::_ensureDefaultProviders()
{
    // Lazily install the singleton-reaching default providers so construction
    // touches no singletons and tests can override any source via
    // setPositionProvider() before the first GGA tick. Only absent slots are
    // filled, so partial overrides are preserved.
    const std::pair<PositionSource, PositionProvider> kDefaults[] = {
        {PositionSource::VehicleGPS,
         [this]() {
             return _vehicle && !_vehicle->vehicleLinkManager()->communicationLost() ? _vehicleGpsPosition
                                                                                     : PositionResult{};
         }},
        {PositionSource::VehicleEKF,
         [this]() {
             return _vehicle && !_vehicle->vehicleLinkManager()->communicationLost() ? _vehicleEkfPosition
                                                                                     : PositionResult{};
         }},
        {PositionSource::RTKBase, &getRTKBasePosition},
        {PositionSource::GCSPosition, &getGCSPosition},
    };
    for (const auto& [source, provider] : kDefaults) {
        if (!_providers.contains(source)) {
            _providers.insert(source, provider);
        }
    }
}

PositionResult NTRIPGgaProvider::_getBestPosition() const
{
    const PositionSource source = _cachedSource;

    // If a specific source is requested, try only that one
    if (source != PositionSource::Auto) {
        auto it = _providers.find(source);
        if (it != _providers.end()) {
            return it.value()();
        }
        return {};
    }

    // Auto: try each provider in priority order
    static constexpr PositionSource kPriority[] = {
        PositionSource::VehicleGPS,
        PositionSource::VehicleEKF,
        PositionSource::RTKBase,
        PositionSource::GCSPosition,
    };

    for (PositionSource s : kPriority) {
        auto it = _providers.find(s);
        if (it != _providers.end()) {
            auto result = it.value()();
            if (result.isValid()) {
                return result;
            }
        }
    }

    return {};
}
