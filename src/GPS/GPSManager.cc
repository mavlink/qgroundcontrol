#include "GPSManager.h"

#include "AppMessages.h"
#include "Fact.h"
#include "FactGroup.h"
#include "GPSCorrectionManager.h"
#include "GPSMavlinkOutput.h"
#include "GPSPositionPolicy.h"
#include "GPSRtk.h"
#include "GPSSourceHealth.h"
#include "LinkManager.h"
#include "MonotonicClock.h"
#include "MultiVehicleManager.h"
#include "NMEASourceManager.h"
#include "NTRIPConfiguration.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#ifndef QGC_NO_SERIAL_LINK
#include "RTKAutoConnect.h"
#include "SerialPortManager.h"
#endif
#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(GPSManagerLog, "GPS.GPSManager")

Q_APPLICATION_STATIC(GPSManager, _gpsManager);

GPSManager::GPSManager(QObject* parent)
    : QObject(parent)
    , _corrections(new GPSCorrectionManager(this))
    , _gpsRtk(new GPSRtk(this))
    , _ntripManager(NTRIPManager::instance())
{
    qCDebug(GPSManagerLog) << this;
    auto* output = new GPSMavlinkOutput(this);
    _corrections->rtcmMavlink()->setOutputProvider([output]() { return output->outputs(); });
    _gpsRtk->setCorrectionManager(_corrections);
    _ntripManager->setCorrectionManager(_corrections);
}

GPSManager::~GPSManager()
{
    shutdown();
    qCDebug(GPSManagerLog) << this;
}

GPSManager* GPSManager::instance()
{
    return _gpsManager();
}

NTRIPConfiguration GPSManager::ntripConfigFromSettings(NTRIPSettings& settings)
{
    const auto read = [](Fact* fact, const QVariant& fallback) { return fact ? fact->rawValue() : fallback; };
    NTRIPConfiguration config;
    auto& connection = config.connection;
    connection.host = read(settings.ntripServerHostAddress(), connection.host).toString();
    connection.port = read(settings.ntripServerPort(), connection.port).toInt();
    connection.username = read(settings.ntripUsername(), connection.username).toString();
    connection.password = read(settings.ntripPassword(), connection.password).toString();
    connection.mountpoint = read(settings.ntripMountpoint(), connection.mountpoint).toString();
    connection.useTls = read(settings.ntripUseTls(), connection.useTls).toBool();
    connection.allowSelfSignedCerts =
        read(settings.ntripAllowSelfSignedCerts(), connection.allowSelfSignedCerts).toBool();
    config.filter.whitelist = read(settings.ntripWhitelist(), config.filter.whitelist).toString();
    auto& udpForward = config.udpForward;
    udpForward.enabled = read(settings.ntripUdpForwardEnabled(), udpForward.enabled).toBool();
    udpForward.address = read(settings.ntripUdpTargetAddress(), udpForward.address).toString();
    udpForward.port = static_cast<quint16>(read(settings.ntripUdpTargetPort(), udpForward.port).toUInt());
    return config;
}

PositionResult GPSManager::vehicleGgaPosition(Vehicle* vehicle, NTRIPGgaProvider::PositionSource source, quint64 nowUs)
{
    using Source = NTRIPGgaProvider::PositionSource;
    if (!vehicle || vehicle->isOfflineEditingVehicle() || !vehicle->vehicleLinkManager() ||
        vehicle->vehicleLinkManager()->communicationLost() ||
        (source != Source::VehicleGPS && source != Source::VehicleEKF)) {
        return {};
    }
    const auto& observation =
        source == Source::VehicleGPS ? vehicle->gpsObservation() : vehicle->fusedPositionObservation();
    if (MonotonicClock::remaining(observation.monotonicTimestampUs, nowUs,
                                  std::chrono::milliseconds(GPSSourceHealth::FRESHNESS_TIMEOUT_MS))
            .count() == 0) {
        return {};
    }
    const auto accepted = GPSPositionPolicy::project(observation, GPSObservation::PositionUse::Gga);
    // The coordinate-only GGA encoder cannot represent unknown MSL altitude.
    if (!accepted || accepted->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel ||
        !qIsFinite(accepted->position.coordinate().altitude())) {
        return {};
    }
    const auto coordinate = accepted->position.coordinate();
    // Vehicles report zero island before acquiring a fix.
    if (coordinate.latitude() == 0 && coordinate.longitude() == 0) {
        return {};
    }
    return {coordinate, source == Source::VehicleGPS ? QStringLiteral("Vehicle GPS") : QStringLiteral("Vehicle EKF")};
}

void GPSManager::configureGgaProvider(NTRIPGgaProvider& provider, NTRIPSettings* settings)
{
    using Source = NTRIPGgaProvider::PositionSource;
    const auto position = [](const QGeoCoordinate& coordinate, const QString& label) {
        return coordinate.isValid() && (coordinate.latitude() != 0.0 || coordinate.longitude() != 0.0)
                   ? PositionResult{coordinate, label}
                   : PositionResult{};
    };
    provider.setPositionProvider(Source::VehicleGPS, []() -> PositionResult {
        auto* manager = MultiVehicleManager::instance();
        Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
        return vehicleGgaPosition(vehicle, Source::VehicleGPS, MonotonicClock::nowUs());
    });
    provider.setPositionProvider(Source::VehicleEKF, []() -> PositionResult {
        auto* manager = MultiVehicleManager::instance();
        Vehicle* vehicle = manager ? manager->activeVehicle() : nullptr;
        return vehicleGgaPosition(vehicle, Source::VehicleEKF, MonotonicClock::nowUs());
    });
    provider.setPositionProvider(Source::RTKBase, [position, rtk = QPointer<GPSRtk>(_gpsRtk)]() -> PositionResult {
        FactGroup* facts = rtk ? rtk->gpsRtkFactGroup() : nullptr;
        Fact* valid = facts ? facts->getFact(QStringLiteral("valid")) : nullptr;
        Fact* latitude = facts ? facts->getFact(QStringLiteral("currentLatitude")) : nullptr;
        Fact* longitude = facts ? facts->getFact(QStringLiteral("currentLongitude")) : nullptr;
        Fact* altitude = facts ? facts->getFact(QStringLiteral("currentAltitude")) : nullptr;
        if (!valid || !valid->rawValue().toBool() || !latitude || !longitude) {
            return {};
        }
        return position(QGeoCoordinate(latitude->rawValue().toDouble(), longitude->rawValue().toDouble(),
                                       altitude ? altitude->rawValue().toDouble() : 0.0),
                        QStringLiteral("RTK Base"));
    });
    provider.setPositionProvider(Source::GCSPosition, [position]() -> PositionResult {
        auto* manager = QGCPositionManager::instance();
        return manager ? position(manager->gcsPosition(), QStringLiteral("GCS Position")) : PositionResult{};
    });
    if (settings) {
        const auto refresh = [&provider, settings]() {
            provider.configure(
                {static_cast<Source>(settings->ntripGgaPositionSource()->rawValue().toUInt()),
                 std::chrono::milliseconds(settings->ntripGgaIntervalSec()->rawValue().toUInt() * qint64(1000))});
        };
        refresh();
        connect(settings->ntripGgaPositionSource(), &Fact::rawValueChanged, &provider, refresh);
        connect(settings->ntripGgaIntervalSec(), &Fact::rawValueChanged, &provider, refresh);
    }
}

void GPSManager::init()
{
    if (_connectionTimer || _shutdown) {
        return;
    }
    _corrections->init(SettingsManager::instance()->gpsCorrectionSettings());
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    _nmeaSources = new NMEASourceManager(settings, QGCPositionManager::instance(), this);
#ifndef QGC_NO_SERIAL_LINK
    _rtkAutoConnect = new RTKAutoConnect(settings, _gpsRtk, SerialPortManager::instance(), this);
    connect(_rtkAutoConnect, &RTKAutoConnect::connectRequested, this,
            [this](const QString& device, const QString& name) { _gpsRtk->connectGPS(device, name); });
    connect(_rtkAutoConnect, &RTKAutoConnect::disconnectRequested, _gpsRtk, &GPSRtk::disconnectGPS);
#endif
    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(1000);
    connect(_connectionTimer, &QTimer::timeout, this, &GPSManager::_updateConnections);
    if (!QGC::runningUnitTests()) {
        _connectionTimer->start();
    }
}

void GPSManager::_updateConnections()
{
    if (LinkManager::instance()->connectionsSuspended()) {
        return;
    }
    _nmeaSources->update();
#ifndef QGC_NO_SERIAL_LINK
    _rtkAutoConnect->update();
#endif
}

void GPSManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    if (_connectionTimer) {
        _connectionTimer->stop();
    }
    if (_nmeaSources) {
        _nmeaSources->stop();
    }
#ifndef QGC_NO_SERIAL_LINK
    if (_rtkAutoConnect) {
        _rtkAutoConnect->stop();
    }
#endif
    _gpsRtk->disconnectGPS();
    if (_ntripManager) {
        _ntripManager->stopNTRIP();
    }
    _corrections->shutdown();
}
