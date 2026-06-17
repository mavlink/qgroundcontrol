#include "NTRIPGgaProvider.h"

#include <QtCore/QDateTime>

#include "Fact.h"
#include "FactGroup.h"
#ifndef QGC_NO_SERIAL_LINK
#include "GPSManager.h"
#include "GPSRtk.h"
#endif
#include "MultiVehicleManager.h"
#include "NMEAUtils.h"
#include "NTRIPSettings.h"
#include "NTRIPTransport.h"
#include "PositionManager.h"
#include "Vehicle.h"

namespace {

/// Rejects "zero island" (0,0) as well as out-of-range and non-finite values.
/// QGeoCoordinate::isValid() alone accepts (0,0), which is how vehicles report
/// "no fix yet" — we must treat that as invalid for GGA upstream.
bool isSaneCoord(double lat, double lon)
{
    return qIsFinite(lat) && qIsFinite(lon) && !(lat == 0.0 && lon == 0.0) && qAbs(lat) <= 90.0 && qAbs(lon) <= 180.0;
}

PositionResult getVehicleGPSPosition()
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (!mvm)
        return {};
    Vehicle* veh = mvm->activeVehicle();
    if (!veh)
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
        return {QGeoCoordinate(lat, lon, veh->coordinate().altitude()), QStringLiteral("Vehicle GPS")};
    }
    return {};
}

PositionResult getVehicleEKFPosition()
{
    MultiVehicleManager* mvm = MultiVehicleManager::instance();
    if (!mvm)
        return {};
    Vehicle* veh = mvm->activeVehicle();
    if (!veh)
        return {};

    const QGeoCoordinate coord = veh->coordinate();
    if (coord.isValid() && isSaneCoord(coord.latitude(), coord.longitude())) {
        return {coord, QStringLiteral("Vehicle EKF")};
    }
    return {};
}

#ifndef QGC_NO_SERIAL_LINK
PositionResult getRTKBasePosition()
{
    GPSManager* gpsManager = GPSManager::instance();
    if (!gpsManager)
        return {};
    GPSRtk* rtk = gpsManager->gpsRtk();
    if (!rtk)
        return {};

    FactGroup* rtkGroup = rtk->gpsRtkFactGroup();
    if (!rtkGroup)
        return {};

    Fact* validF = rtkGroup->getFact(QStringLiteral("valid"));
    if (!validF || !validF->rawValue().toBool())
        return {};

    Fact* latF = rtkGroup->getFact(QStringLiteral("currentLatitude"));
    Fact* lonF = rtkGroup->getFact(QStringLiteral("currentLongitude"));
    if (!latF || !lonF)
        return {};

    Fact* altF = rtkGroup->getFact(QStringLiteral("currentAltitude"));
    const double lat = latF->rawValue().toDouble();
    const double lon = lonF->rawValue().toDouble();
    const double alt = altF ? altF->rawValue().toDouble() : 0.0;

    if (isSaneCoord(lat, lon)) {
        return {QGeoCoordinate(lat, lon, alt), QStringLiteral("RTK Base")};
    }
    return {};
}
#endif // QGC_NO_SERIAL_LINK

PositionResult getGCSPosition()
{
    QGCPositionManager* posMgr = QGCPositionManager::instance();
    if (!posMgr)
        return {};

    const QGeoCoordinate coord = posMgr->gcsPosition();
    if (coord.isValid() && isSaneCoord(coord.latitude(), coord.longitude())) {
        return {coord, QStringLiteral("GCS Position")};
    }
    return {};
}

}  // anonymous namespace

NTRIPGgaProvider::NTRIPGgaProvider(QObject* parent) : QObject(parent)
{
    _timer.setInterval(_normalInterval);
    connect(&_timer, &QChronoTimer::timeout, this, &NTRIPGgaProvider::_sendGGA);
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

    _ensureDefaultProviders();

    const auto position = _getBestPosition();

    if (!position.isValid()) {
        if (++_fastRetryCount >= 5 && _retryPhase == RetryPhase::Fast) {
            _setRetryPhase(RetryPhase::Normal);
        }
        return;
    }

    _fastRetryCount = 0;
    if (_retryPhase != RetryPhase::Normal) {
        _setRetryPhase(RetryPhase::Normal);
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

void NTRIPGgaProvider::_ensureDefaultProviders()
{
    // Lazily install the singleton-reaching default providers so construction
    // touches no singletons and tests can override any source via
    // setPositionProvider() before the first GGA tick. Only absent slots are
    // filled, so partial overrides are preserved.
    static const std::pair<PositionSource, PositionProvider> kDefaults[] = {
        {PositionSource::VehicleGPS, &getVehicleGPSPosition},
        {PositionSource::VehicleEKF, &getVehicleEKFPosition},
#ifndef QGC_NO_SERIAL_LINK
        {PositionSource::RTKBase, &getRTKBasePosition},
#endif
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
