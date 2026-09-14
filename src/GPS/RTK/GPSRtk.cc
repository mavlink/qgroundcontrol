#include "GPSRtk.h"

#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSType.h"
#include "NTRIPManager.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
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
struct GPSTypeEntry
{
    QLatin1StringView key;
    GPSType type;
    int manufacturerId;  // RTKSettings::baseReceiverManufacturers enum value
};

constexpr GPSTypeEntry kGPSTypeTable[] = {
    {QLatin1StringView("trimble"), GPSType::trimble, 1},
    {QLatin1StringView("septentrio"), GPSType::septentrio, 2},
    {QLatin1StringView("femtomes"), GPSType::femto, 3},
    {QLatin1StringView("blox"), GPSType::u_blox, 4},
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
    _gpsRtkFactGroup->currentDuration()->setRawValue(status.durationSecs);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(status.meanAccuracyMM) / 1000.0);
    _gpsRtkFactGroup->currentLatitude()->setRawValue(status.latitude);
    _gpsRtkFactGroup->currentLongitude()->setRawValue(status.longitude);
    _gpsRtkFactGroup->currentAltitude()->setRawValue(status.altitude);
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
    GPSType type = GPSType::u_blox;
    for (const GPSTypeEntry& entry : kGPSTypeTable) {
        if (gps_type.contains(entry.key, Qt::CaseInsensitive)) {
            type = entry.type;
            break;
        }
    }
    connectReceiver(type, [device, reservation](const std::atomic_bool& requestStop) {
        return std::make_unique<SerialGPSTransport>(device, requestStop);
    });
}
#endif

void GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory)
{
    RTKSettings* const rtkSettings = SettingsManager::instance()->rtkSettings();
    for (const GPSTypeEntry& entry : kGPSTypeTable) {
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
    const GPSReceiverConfig rtkConfig = {
        .useFixedBase = useFixedBase,
        .surveyInAccMeters = rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(),
        .surveyInDurationSecs = rtkSettings->surveyInMinObservationDuration()->rawValue().toInt(),
        .fixedBaseLatitude = rtkSettings->fixedBasePositionLatitude()->rawValue().toDouble(),
        .fixedBaseLongitude = rtkSettings->fixedBasePositionLongitude()->rawValue().toDouble(),
        .fixedBaseAltitudeMeters = rtkSettings->fixedBasePositionAltitude()->rawValue().toFloat(),
        .fixedBaseAccuracyMeters = rtkSettings->fixedBasePositionAccuracy()->rawValue().toFloat(),
    };
    _gpsProvider = new GPSProvider(std::move(transportFactory), type, rtkConfig, this);
    const QPointer<GPSProvider> provider = _gpsProvider;
    // Always queue worker callbacks and reject retired sessions, including already queued events.
    (void) connect(
        provider, &GPSProvider::RTCMDataUpdate, this,
        [this, provider](const QByteArray& data) {
            if (provider && _gpsProvider == provider) {
                if (auto* rtcm = NTRIPManager::instance()->rtcmMavlink()) {
                    rtcm->RTCMDataUpdate(data);
                }
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::satelliteInfoUpdate, this,
        [this, provider](const satellite_info_s& data) {
            if (provider && _gpsProvider == provider) {
                _satelliteInfoUpdate(data);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::sensorGpsUpdate, this,
        [this, provider](const sensor_gps_s& data) {
            if (provider && _gpsProvider == provider) {
                _sensorGpsUpdate(data);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::surveyInStatus, this,
        [this, provider](const GPSSurveyInStatus& status) {
            if (provider && _gpsProvider == provider) {
                _onGPSSurveyInStatus(status);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::connectionError, this,
        [this, provider](GPSConnectionError error) {
            if (provider && _gpsProvider == provider) {
                _onGPSDisconnect();
                _onGPSConnectionError(error);
            }
        },
        Qt::QueuedConnection);
    (void) connect(
        provider, &GPSProvider::receiverReady, this,
        [this, provider]() {
            if (provider && _gpsProvider == provider) {
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
