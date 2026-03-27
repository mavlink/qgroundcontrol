#include "NTRIPGgaProvider.h"
#include "NTRIPTransport.h"
#include "NTRIPSettings.h"
#include "NMEAUtils.h"
#include "Fact.h"
#include "FactGroup.h"
#include "GPSManager.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QDateTime>

namespace {

/// Rejects "zero island" (0,0) as well as out-of-range and non-finite values.
/// QGeoCoordinate::isValid() alone accepts (0,0), which is how vehicles report
/// "no fix yet" — we must treat that as invalid for GGA upstream.
bool isSaneCoord(double lat, double lon)
{
    return qIsFinite(lat) && qIsFinite(lon)
        && !(lat == 0.0 && lon == 0.0)
        && qAbs(lat) <= 90.0 && qAbs(lon) <= 180.0;
}

PositionResult getVehicleGPSPosition()
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (!mvm) return {};
    Vehicle* veh = mvm->activeVehicle();
    if (!veh) return {};

    FactGroup* gps = veh->gpsFactGroup();
    if (!gps) return {};

    Fact* latF = gps->getFact(QStringLiteral("lat"));
    Fact* lonF = gps->getFact(QStringLiteral("lon"));
    if (!latF || !lonF) return {};

    const double lat = latF->rawValue().toDouble();
    const double lon = lonF->rawValue().toDouble();

    if (isSaneCoord(lat, lon)) {
        return {QGeoCoordinate(lat, lon, veh->coordinate().altitude()), QStringLiteral("Vehicle GPS")};
    }
    return {};
}

PositionResult getVehicleEKFPosition()
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (!mvm) return {};
    Vehicle* veh = mvm->activeVehicle();
    if (!veh) return {};

    const QGeoCoordinate coord = veh->coordinate();
    if (coord.isValid() && isSaneCoord(coord.latitude(), coord.longitude())) {
        return {coord, QStringLiteral("Vehicle EKF")};
    }
    return {};
}

PositionResult getRTKBasePosition()
{
    if (!GPSManager::instance()->connected()) {
        return {};
    }

    FactGroup* rtkFg = GPSManager::instance()->gpsRtkFactGroup();
    if (!rtkFg) return {};

    Fact* latF = rtkFg->getFact(QStringLiteral("baseLatitude"));
    Fact* lonF = rtkFg->getFact(QStringLiteral("baseLongitude"));
    Fact* altF = rtkFg->getFact(QStringLiteral("baseAltitude"));
    Fact* fixF = rtkFg->getFact(QStringLiteral("baseFixType"));
    if (!latF || !lonF || !fixF) return {};

    const double lat = latF->rawValue().toDouble();
    const double lon = lonF->rawValue().toDouble();
    const double alt = altF ? altF->rawValue().toDouble() : 0.0;
    const int fixType = fixF->rawValue().toInt();

    if (fixType > 0 && isSaneCoord(lat, lon)) {
        return {QGeoCoordinate(lat, lon, alt), QStringLiteral("RTK Base")};
    }
    return {};
}

PositionResult getGCSPosition()
{
    QGCPositionManager* posMgr = QGCPositionManager::instance();
    if (!posMgr) return {};

    const QGeoCoordinate coord = posMgr->gcsPosition();
    if (coord.isValid() && isSaneCoord(coord.latitude(), coord.longitude())) {
        return {coord, QStringLiteral("GCS Position")};
    }
    return {};
}

} // anonymous namespace

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent)
    : QObject(parent)
{
    _timer.setInterval(kNormalIntervalMs);
    connect(&_timer, &QTimer::timeout, this, &NTRIPGgaProvider::_sendGGA);

    // Register default providers (can be overridden via setPositionProvider)
    _providers[PositionSource::VehicleGPS]  = &getVehicleGPSPosition;
    _providers[PositionSource::VehicleEKF]  = &getVehicleEKFPosition;
    _providers[PositionSource::RTKBase]     = &getRTKBasePosition;
    _providers[PositionSource::GCSPosition] = &getGCSPosition;

    // Cache the user-selected source so the hot path (_sendGGA) avoids a
    // SettingsManager::instance()->ntripSettings()->...->rawValue() chain per tick.
    if (auto* settings = SettingsManager::instance()->ntripSettings()) {
        auto* fact = settings->ntripGgaPositionSource();
        auto refresh = [this, fact]() {
            _cachedSource = static_cast<PositionSource>(fact->rawValue().toUInt());
        };
        refresh();
        connect(fact, &Fact::rawValueChanged, this, refresh);
    }
}

void NTRIPGgaProvider::setPositionProvider(PositionSource source, PositionProvider provider)
{
    _providers[source] = std::move(provider);
}

void NTRIPGgaProvider::start(NTRIPTransport* transport)
{
    _transport = transport;
    _fastRetryCount = 0;
    _timer.setInterval(kFastRetryIntervalMs);
    _sendGGA();
    _timer.start();
}

void NTRIPGgaProvider::stop()
{
    _timer.stop();
    _transport = nullptr;
}

void NTRIPGgaProvider::_sendGGA()
{
    if (!_transport) {
        return;
    }

    const auto position = _getBestPosition();

    if (!position.isValid()) {
        if (++_fastRetryCount >= 5 && _timer.interval() != kNormalIntervalMs) {
            _timer.setInterval(kNormalIntervalMs);
        }
        return;
    }

    _fastRetryCount = 0;
    if (_timer.interval() != kNormalIntervalMs) {
        _timer.setInterval(kNormalIntervalMs);
    }

    double alt_msl = position.coordinate.altitude();
    if (!qIsFinite(alt_msl)) {
        alt_msl = 0.0;
    }

    const QByteArray gga = NMEAUtils::makeGGA(position.coordinate, alt_msl);
    _transport->sendNMEA(gga);

    if (!position.source.isEmpty() && position.source != _source) {
        _source = position.source;
        emit sourceChanged(_source);
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
