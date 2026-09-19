#include "GPSRtk.h"

#include "GPSBaseStationConfig.h"
#include "GPSCorrectionManager.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "QGCLoggingCategory.h"
#include "RTCMFramer.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#include "SerialPortManager.h"
#endif

#include <algorithm>
#include <functional>
#include <utility>

#include <QtCore/QPointer>

QGC_LOGGING_CATEGORY(GPSRtkLog, "GPS.GPSRtk")

namespace {
struct GPSReceiverTypeEntry
{
    QLatin1StringView key;
    GPSType type;
    int manufacturerId;  // RTKSettings::baseReceiverManufacturers enum value
};

constexpr GPSReceiverTypeEntry kGPSReceiverTypeTable[] = {
    {QLatin1StringView("trimble"), GPSType::trimble, 1},
    {QLatin1StringView("septentrio"), GPSType::septentrio, 2},
    {QLatin1StringView("femtomes"), GPSType::femto, 3},
    {QLatin1StringView("blox"), GPSType::ublox, 4},
};
}  // namespace

GPSRtk::GPSRtk(QObject* parent) : QObject(parent), _gpsRtkFactGroup(new GPSRTKFactGroup(this))
{
    qCDebug(GPSRtkLog) << this;
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
    _lastLoggedFixType.reset();
    _correctionRegistration.reset();
    _gpsRtkFactGroup->connected()->setRawValue(false);
    _gpsRtkFactGroup->valid()->setRawValue(false);
    _gpsRtkFactGroup->active()->setRawValue(false);
    _gpsRtkFactGroup->currentDuration()->setRawValue(0);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(qQNaN());
    _gpsRtkFactGroup->currentLatitude()->setRawValue(qQNaN());
    _gpsRtkFactGroup->currentLongitude()->setRawValue(qQNaN());
    _gpsRtkFactGroup->currentAltitude()->setRawValue(qQNaN());
    _gpsRtkFactGroup->numSatellites()->setRawValue(-1);
    _gpsRtkFactGroup->numSatellitesUsed()->setRawValue(-1);
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
    GPSType type = GPSType::ublox;
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

void GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
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
    GPSBaseStationConfig rtkConfig;
    if (useFixedBase) {
        rtkConfig = GPSBaseStationConfig{
            .useFixedBase = true,
            .fixedBaseLatitude = rtkSettings->fixedBasePositionLatitude()->rawValue().toDouble(),
            .fixedBaseLongitude = rtkSettings->fixedBasePositionLongitude()->rawValue().toDouble(),
            .fixedBaseAltitudeMeters = rtkSettings->fixedBasePositionAltitude()->rawValue().toFloat(),
            .fixedBaseAccuracyMeters = rtkSettings->fixedBasePositionAccuracy()->rawValue().toFloat(),
        };
    } else {
        rtkConfig = GPSBaseStationConfig{
            .surveyInAccMeters = rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(),
            .surveyInDurationSecs = rtkSettings->surveyInMinObservationDuration()->rawValue().toLongLong(),
        };
    }
    _gpsProvider = new GPSProvider(std::move(transportFactory), type, GPSReceiverConfig{.base = rtkConfig}, this);
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
    const auto connectCurrent = [this, provider, current]<typename... Args>(void (GPSProvider::*signal)(Args...),
                                                                            auto handler) {
        return connect(
            provider, signal, this,
            [current, handler](Args... args) {
                if (current()) {
                    handler(args...);
                }
            },
            Qt::QueuedConnection);
    };
    // Queued callbacks retain the producing session's token.
    (void) connect(
        provider, &GPSProvider::RTCMDataUpdate, this,
        [correctionManager, token](const QByteArray& data, qint64 receivedAtMs) {
            if (!correctionManager) {
                qCWarning(GPSRtkLog) << "Correction manager not ready; dropping" << data.size() << "bytes";
                return;
            }
            const bool valid = RTCMFramer::isValidFrame(data);
            const int messageId =
                data.size() >= 5 ? (static_cast<quint8>(data[3]) << 4) | (static_cast<quint8>(data[4]) >> 4) : 0;
            correctionManager->acceptIngress(
                token.event(data, receivedAtMs, messageId, valid, false,
                            valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame));
        },
        Qt::QueuedConnection);
    (void) connectCurrent(&GPSProvider::satelliteInfoUpdate, std::bind_front(&GPSRtk::_satelliteInfoUpdate, this));
    (void) connectCurrent(&GPSProvider::satelliteUsageUpdate, std::bind_front(&GPSRtk::_satelliteUsageUpdate, this));
    (void) connectCurrent(&GPSProvider::sensorGpsUpdate, std::bind_front(&GPSRtk::_sensorGpsUpdate, this));
    (void) connectCurrent(&GPSProvider::surveyInStatus, std::bind_front(&GPSRtk::_onGPSSurveyInStatus, this));
    (void) connectCurrent(&GPSProvider::connectionError, [this](GPSConnectionError error) {
        _onGPSDisconnect();
        _onGPSConnectionError(error);
    });
    (void) connectCurrent(&GPSProvider::receiverReady, std::bind_front(&GPSRtk::_onGPSConnect, this));
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

GPSRTKFactGroup* GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup;
}

GPSRtk::SatelliteCounts GPSRtk::countSatellites(const GPSSatelliteReport& msg)
{
    SatelliteCounts counts;
    counts.inView = (std::min) (msg.count, GPSSatelliteReport::MAX_SATELLITES);
    if (msg.count > GPSSatelliteReport::MAX_SATELLITES) {
        return counts;
    }
    counts.used = 0;
    for (uint16_t i = 0; i < counts.inView; ++i) {
        if (!msg.satellites[i].used) {
            counts.used.reset();
            break;
        }
        if (*msg.satellites[i].used) {
            ++*counts.used;
        }
    }
    return counts;
}

void GPSRtk::_satelliteInfoUpdate(const GPSSatelliteReport& msg)
{
    const SatelliteCounts counts = countSatellites(msg);
    qCDebug(GPSRtkLog) << QStringLiteral("%1 in view, %2 used")
                              .arg(counts.inView)
                              .arg(counts.used ? QString::number(*counts.used) : QStringLiteral("unknown"));
    _gpsRtkFactGroup->numSatellites()->setRawValue(counts.inView);
    if (counts.used) {
        _gpsRtkFactGroup->numSatellitesUsed()->setRawValue(*counts.used);
    }
}

void GPSRtk::_sensorGpsUpdate(const GPSPositionReport& msg)
{
    if (_lastLoggedFixType != msg.fixType) {
        _lastLoggedFixType = msg.fixType;
        qCDebug(GPSRtkLog) << "Receiver fix changed:" << static_cast<int>(msg.fixType);
    }
}

void GPSRtk::_satelliteUsageUpdate(const GPSSatelliteUsageReport& msg)
{
    // A count-only observation cannot change the independently reported satellites in view.
    _gpsRtkFactGroup->numSatellitesUsed()->setRawValue(msg.usedCount.value_or(-1));
}
