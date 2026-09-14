#include "GPSRtk.h"

#include "GPSCorrectionManager.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverTypes.h"
#include "QGCLoggingCategory.h"
#include "RTCMFrame.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#include "SerialPortManager.h"
#endif

#include <QtCore/QPointer>

#include <utility>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.GPSRtk")

namespace {
struct GPSReceiverTypeEntry
{
    QLatin1StringView key;
    GPSReceiverType type;
    int manufacturerId;  // RTKSettings::baseReceiverManufacturers enum value
};

constexpr GPSReceiverTypeEntry kGPSReceiverTypeTable[] = {
    {QLatin1StringView("trimble"), GPSReceiverType::trimble, 1},
    {QLatin1StringView("septentrio"), GPSReceiverType::septentrio, 2},
    {QLatin1StringView("femtomes"), GPSReceiverType::femto, 3},
    {QLatin1StringView("blox"), GPSReceiverType::ublox, 4},
};
}  // namespace

GPSRtk::GPSRtk(QObject* parent) : QObject(parent), _gpsRtkFactGroup(new GPSRTKFactGroup(this))
{
    qCDebug(GPSRtkLog) << this;

    (void) qRegisterMetaType<satellite_info_s>("satellite_info_s");
    (void) qRegisterMetaType<sensor_gps_s>("sensor_gps_s");
    (void) qRegisterMetaType<GPSConnectionError>("GPSConnectionError");
    (void) qRegisterMetaType<GPSSurveyInStatus>("GPSSurveyInStatus");
}

GPSRtk::~GPSRtk()
{
    disconnectGPS();

    qCDebug(GPSRtkLog) << this;
}

void GPSRtk::_onGPSConnect()
{
    _gpsRtkFactGroup->lastError()->setRawValue(static_cast<int>(GPSConnectionError::None));
    _gpsRtkFactGroup->connected()->setRawValue(true);
}

void GPSRtk::_onGPSDisconnect()
{
    _correctionRegistration.reset();
    _gpsRtkFactGroup->connected()->setRawValue(false);
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
}

void GPSRtk::_onGPSSurveyInStatus(const GPSSurveyInStatus& status)
{
    _gpsRtkFactGroup->currentDuration()->setRawValue(static_cast<qint64>(status.duration.count()));
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(status.meanAccuracyMeters.value_or(qQNaN()));
    _gpsRtkFactGroup->currentLatitude()->setRawValue(status.coordinate.latitude());
    _gpsRtkFactGroup->currentLongitude()->setRawValue(status.coordinate.longitude());
    _gpsRtkFactGroup->currentAltitude()->setRawValue(status.altitudeEllipsoidMeters);
    _gpsRtkFactGroup->valid()->setRawValue(status.valid);
    _gpsRtkFactGroup->active()->setRawValue(status.active);
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtk::connectGPS(const QString& device, QStringView gps_type)
{
    auto reservation = SerialPortManager::instance()->reservePort(device);
    if (!reservation) {
        qCDebug(GPSRtkLog) << "Serial port is already reserved:" << device;
        return;
    }
    GPSReceiverType type = GPSReceiverType::ublox;
    for (const GPSReceiverTypeEntry& entry : kGPSReceiverTypeTable) {
        if (gps_type.contains(entry.key, Qt::CaseInsensitive)) {
            type = entry.type;
            break;
        }
    }
    connectReceiver(
        type,
        [device, reservation](const std::atomic_bool& requestStop) {
            return std::make_unique<SerialGPSTransport>(device, requestStop);
        },
        QStringLiteral("serial:%1").arg(device));
}
#endif

void GPSRtk::setCorrectionManager(GPSCorrectionManager* manager)
{
    if (_correctionManager == manager) {
        return;
    }
    if (_gpsProvider) {
        qCWarning(GPSRtkLog) << "Inject the correction manager before connecting a receiver";
        return;
    }
    _correctionRegistration.reset();
    _correctionManager = manager;
}

void GPSRtk::connectReceiver(GPSReceiverType type, GPSProvider::TransportFactory transportFactory,
                             const QString& sourceInstance)
{
    RTKSettings* const rtkSettings = SettingsManager::instance()->rtkSettings();
    for (const GPSReceiverTypeEntry& entry : kGPSReceiverTypeTable) {
        if (entry.type == type) {
            rtkSettings->baseReceiverManufacturers()->setRawValue(entry.manufacturerId);
            break;
        }
    }

    disconnectGPS();

    _gpsRtkFactGroup->lastError()->setRawValue(static_cast<int>(GPSConnectionError::None));
    const bool useFixedBase =
        static_cast<BaseModeDefinition::Mode>(rtkSettings->useFixedBasePosition()->rawValue().toInt()) ==
        BaseModeDefinition::Mode::BaseFixed;
    GPSReceiverConfig rtkConfig;
    if (useFixedBase) {
        rtkConfig.base = GPSFixedBaseConfig{
            .coordinate = QGeoCoordinate(rtkSettings->fixedBasePositionLatitude()->rawValue().toDouble(),
                                         rtkSettings->fixedBasePositionLongitude()->rawValue().toDouble()),
            .altitudeEllipsoidMeters = rtkSettings->fixedBasePositionAltitude()->rawValue().toFloat(),
            .accuracyMeters = rtkSettings->fixedBasePositionAccuracy()->rawValue().toFloat(),
        };
    } else {
        rtkConfig.base = GPSSurveyInConfig{
            .accuracyMeters = rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(),
            .minimumDuration =
                std::chrono::seconds(rtkSettings->surveyInMinObservationDuration()->rawValue().toLongLong()),
        };
    }
    _gpsProvider = new GPSProvider(std::move(transportFactory), type, rtkConfig, this);
    const QPointer<GPSProvider> provider = _gpsProvider;
    const QPointer<GPSCorrectionManager> correctionManager = _correctionManager;
    if (correctionManager) {
        auto registration = correctionManager->registerSource(GPSCorrectionSource::LocalReceiver, sourceInstance);
        if (!provider || _gpsProvider != provider) {
            return;
        }
        _correctionRegistration = std::move(registration);
    }
    const auto token = _correctionRegistration.token();
    const auto current = [this, provider, token, registered = !correctionManager.isNull()]() {
        return provider && _gpsProvider == provider && (!registered || token.valid());
    };
    // Queued callbacks retain the producing session's token.
    (void) connect(
        provider, &GPSProvider::RTCMDataUpdate, this,
        [correctionManager, token](const QByteArray& data, qint64 receivedAtMs) {
            if (!correctionManager) {
                qCWarning(GPSRtkLog) << "Correction manager not ready; dropping" << data.size() << "bytes";
                return;
            }
            const bool valid = RTCM::isValidFrame(data);
            const int messageId =
                data.size() >= 5 ? (static_cast<quint8>(data[3]) << 4) | (static_cast<quint8>(data[4]) >> 4) : 0;
            correctionManager->acceptIngress(
                token.event(data, receivedAtMs, messageId, valid, false,
                            valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame));
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::satelliteInfoUpdate, this,
        [this, current](const satellite_info_s& data) {
            if (current()) {
                _satelliteInfoUpdate(data);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::sensorGpsUpdate, this,
        [this, current](const sensor_gps_s& data) {
            if (current()) {
                _sensorGpsUpdate(data);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::surveyInStatus, this,
        [this, current](const GPSSurveyInStatus& status) {
            if (current()) {
                _onGPSSurveyInStatus(status);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::connectionError, this,
        [this, current](GPSConnectionError error) {
            if (current()) {
                _onGPSDisconnect();
                _onGPSConnectionError(error);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::receiverReady, this,
        [this, current]() {
            if (current()) {
                _onGPSConnect();
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &QThread::finished, this,
        [this, provider]() {
            if (provider && _gpsProvider == provider) {
                _gpsProvider = nullptr;
                _onGPSDisconnect();
            }
        },
        Qt::QueuedConnection);
    (void) connect(provider, &QThread::finished, provider, &QObject::deleteLater);
    provider->start();
}

void GPSRtk::disconnectGPS()
{
    // Invalidate the session before waiting: queued output may still be in the GUI event queue.
    auto* provider = std::exchange(_gpsProvider, nullptr);
    _onGPSDisconnect();
    if (provider) {
        provider->stop();
        if (!provider->wait(_disconnectTimeoutMs)) {
            qCWarning(GPSRtkLog) << "GPS thread did not exit in time; deferring cleanup to finished()";
            // The worker owns its stop flag and must survive destruction of this manager.
            provider->setParent(nullptr);
        }
    }
}

bool GPSRtk::connected() const
{
    return _gpsRtkFactGroup->connected()->rawValue().toBool();
}

FactGroup* GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup;
}

GPSRtk::SatelliteCounts GPSRtk::countSatellites(const satellite_info_s& msg)
{
    SatelliteCounts counts;
    counts.inView = qMin(msg.count, satellite_info_s::SAT_INFO_MAX_SATELLITES);
    for (uint8_t i = 0; i < counts.inView; ++i) {
        if (msg.used[i]) {
            ++counts.used;
        }
    }
    return counts;
}

void GPSRtk::_satelliteInfoUpdate(const satellite_info_s& msg)
{
    const SatelliteCounts counts = countSatellites(msg);
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << QStringLiteral("%1 in view, %2 used").arg(counts.inView).arg(counts.used);
    _gpsRtkFactGroup->numSatellites()->setRawValue(counts.inView);
    _gpsRtkFactGroup->numSatellitesUsed()->setRawValue(counts.used);
}

void GPSRtk::_sensorGpsUpdate(const sensor_gps_s& msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO
                       << QStringLiteral("alt=%1, long=%2, lat=%3")
                              .arg(msg.altitude_msl_m)
                              .arg(msg.longitude_deg)
                              .arg(msg.latitude_deg);
}
