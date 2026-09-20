#include "GPSRtk.h"

#include "AutoConnectSettings.h"
#include "GPSBaseStationConfig.h"
#include "GPSCorrectionManager.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfigValidation.h"
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
    {QLatin1StringView("trimble"), GPSType::trimble, 1}, {QLatin1StringView("septentrio"), GPSType::septentrio, 2},
    {QLatin1StringView("femtomes"), GPSType::femto, 3},  {QLatin1StringView("blox"), GPSType::ublox, 4},
    {QLatin1StringView("unicore"), GPSType::unicore, 5}, {QLatin1StringView("quectel"), GPSType::quectel, 6},
    {QLatin1StringView("passive"), GPSType::passive, 7},
};
}  // namespace

GPSRtk::GPSRtk(QObject* parent) : QObject(parent), _gpsRtkFactGroup(new GPSRTKFactGroup(this))
{
    qCDebug(GPSRtkLog) << this;
#ifndef QGC_NO_SERIAL_LINK
    _serialTransportFactory = [](const QString& device, const std::atomic_bool& stop) {
        return std::make_unique<SerialGPSTransport>(device, stop);
    };
    setSerialPortManager(SerialPortManager::instance());
#endif
}

GPSRtk::~GPSRtk()
{
    disconnectGPS();

    qCDebug(GPSRtkLog) << this;
}

void GPSRtk::_onGPSConnect()
{
    _setError(GPSConnectionError::None);
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
    _activeManufacturer = 0;
    _activeBaseMode = -1;
    _activeSerialDevice.clear();
    emit receiverChanged();
}

void GPSRtk::_onGPSConnectionError(GPSConnectionError error)
{
    switch (error) {
        case GPSConnectionError::OpenFailed:
            qCWarning(GPSRtkLog) << "Failed to open GPS receiver transport";
            _setError(error, tr("Failed to open the receiver. Check the device, permissions, and other connections."));
            break;
        case GPSConnectionError::ConfigFailed:
            qCWarning(GPSRtkLog) << "GPS receiver did not accept configuration";
            if (_gpsRtkFactGroup->lastError()->rawValue().toInt() != static_cast<int>(error) ||
                _errorMessage.isEmpty()) {
                _setError(error,
                          tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode."));
            }
            break;
        case GPSConnectionError::DeviceError:
            qCWarning(GPSRtkLog) << "GPS device error, connection lost";
            _setError(error, tr("Receiver connection lost. Check the device and reconnect."));
            break;
        case GPSConnectionError::None:
            _setError(error);
            break;
    }
}

void GPSRtk::_setError(GPSConnectionError error, const QString& message)
{
    _gpsRtkFactGroup->lastError()->setRawValue(static_cast<int>(error));
    if (_errorMessage != message) {
        _errorMessage = message;
        emit errorMessageChanged();
    }
}

void GPSRtk::_onGPSSurveyInStatus(const GPSSurveyInStatus& status)
{
    if (_activeManufacturer == manufacturerForType(GPSType::passive)) {
        return;
    }
    _gpsRtkFactGroup->currentDuration()->setRawValue(static_cast<qint64>(status.duration.count()));
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(status.meanAccuracyMeters.value_or(qQNaN()));
    _gpsRtkFactGroup->currentLatitude()->setRawValue(status.coordinate.latitude());
    _gpsRtkFactGroup->currentLongitude()->setRawValue(status.coordinate.longitude());
    _gpsRtkFactGroup->currentAltitude()->setRawValue(status.altitudeEllipsoidMeters);
    _gpsRtkFactGroup->valid()->setRawValue(status.valid);
    _gpsRtkFactGroup->active()->setRawValue(status.active);
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtk::setSerialPortManager(SerialPortManager* serialPorts)
{
    if (hasReceiver()) {
        return;
    }
    QObject::disconnect(_portEnumerationConnection);
    _serialPorts = serialPorts;
    if (_serialPorts) {
        _portEnumerationConnection =
            connect(_serialPorts, &SerialPortManager::portsEnumerated, this, [this](const QStringList& availablePorts) {
                if (!_activeSerialDevice.isEmpty() && !availablePorts.contains(_activeSerialDevice)) {
                    disconnectGPS();
                    _setError(GPSConnectionError::DeviceError,
                              tr("Receiver unplugged. Select a device and reconnect."));
                }
            });
    }
}

bool GPSRtk::connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate, bool allowPersistentChanges)
{
    std::optional<GPSType> type;
    for (const GPSReceiverTypeEntry& entry : kGPSReceiverTypeTable) {
        if (gps_type.contains(entry.key, Qt::CaseInsensitive)) {
            type = entry.type;
            break;
        }
    }
    if (!type) {
        _setError(GPSConnectionError::ConfigFailed, tr("Select a specific receiver type before connecting."));
        return false;
    }
    const QString endpoint = device.trimmed();
    if (endpoint.isEmpty() || !_serialPorts) {
        _setError(GPSConnectionError::OpenFailed, tr("Select an available serial device."));
        return false;
    }
    auto reservation = _serialPorts->reservePort(endpoint);
    if (!reservation) {
        _setError(GPSConnectionError::OpenFailed, tr("The selected serial device is already in use."));
        return false;
    }
    const bool started = connectReceiver(
        *type,
        [endpoint, reservation, factory = _serialTransportFactory](const std::atomic_bool& requestStop) {
            return factory(endpoint, requestStop);
        },
        QStringLiteral("serial:%1").arg(endpoint), baudRate, allowPersistentChanges);
    if (started) {
        _activeSerialDevice = endpoint;
        emit receiverChanged();
    }
    return started;
}
#endif

bool GPSRtk::serialSupported() const
{
#ifdef QGC_NO_SERIAL_LINK
    return false;
#else
    return true;
#endif
}

std::optional<GPSType> GPSRtk::typeForManufacturer(int manufacturer)
{
    for (const auto& entry : kGPSReceiverTypeTable) {
        if (entry.manufacturerId == manufacturer) {
            return entry.type;
        }
    }
    return std::nullopt;
}

int GPSRtk::manufacturerForType(GPSType type)
{
    for (const auto& entry : kGPSReceiverTypeTable) {
        if (entry.type == type) {
            return entry.manufacturerId;
        }
    }
    return 0;
}

QVariantMap GPSRtk::capabilitiesForManufacturer(int manufacturer) const
{
    GPSReceiverCapabilities capabilities;
    for (const auto& entry : kGPSReceiverTypeTable) {
        if (manufacturer != 0 && manufacturer != entry.manufacturerId) {
            continue;
        }
        const auto receiverCapabilities = gpsReceiverCapabilities(entry.type, GPSReceiverConfig::Role::RTKBase);
        capabilities.recognized |= receiverCapabilities.recognized;
        capabilities.rtkBase |= receiverCapabilities.rtkBase;
        capabilities.surveyIn |= receiverCapabilities.surveyIn;
        capabilities.receiverAveraging |= receiverCapabilities.receiverAveraging;
        capabilities.passive |= manufacturer != 0 && receiverCapabilities.passive;
    }
    return {{QStringLiteral("recognized"), capabilities.recognized},
            {QStringLiteral("rtkBase"), capabilities.rtkBase},
            {QStringLiteral("surveyIn"), capabilities.surveyIn},
            {QStringLiteral("receiverAveraging"), capabilities.receiverAveraging},
            {QStringLiteral("passive"), capabilities.passive}};
}

bool GPSRtk::connectConfiguredGPS(bool allowPersistentChanges)
{
#ifndef QGC_NO_SERIAL_LINK
    auto* settings = SettingsManager::instance()->rtkSettings();
    const auto type = typeForManufacturer(settings->baseReceiverManufacturers()->rawValue().toInt());
    if (!type) {
        _setError(GPSConnectionError::ConfigFailed, tr("Select a specific receiver type before connecting."));
        return false;
    }
    if (hasReceiver()) {
        _setError(GPSConnectionError::OpenFailed, tr("Disconnect the current receiver before connecting another."));
        return false;
    }
    const QString device = settings->serialDevice()->rawValue().toString().trimmed();
    const auto ports = _serialPorts ? _serialPorts->availablePorts() : QList<SerialPortManager::Port>{};
    const auto port = std::find_if(ports.cbegin(), ports.cend(),
                                   [&device](const auto& candidate) { return candidate.systemLocation == device; });
    if (device.isEmpty() || port == ports.cend() || port->bootloader) {
        _setError(GPSConnectionError::OpenFailed, tr("Select an available serial device that is not a bootloader."));
        return false;
    }
    const auto baud = settings->serialBaudRate()->rawValue().toULongLong();
    if (baud < 1200 || baud > 4000000) {
        _setError(GPSConnectionError::ConfigFailed, tr("Select a baud rate between 1200 and 4000000."));
        return false;
    }
    emit manualConnectionRequested();
    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    for (const auto& entry : kGPSReceiverTypeTable) {
        if (entry.type == *type) {
            return connectGPS(device, entry.key.toString(), static_cast<uint32_t>(baud), allowPersistentChanges);
        }
    }
#else
    Q_UNUSED(allowPersistentChanges);
#endif
    _setError(GPSConnectionError::OpenFailed, tr("Serial receiver connections are unavailable in this build."));
    return false;
}

void GPSRtk::disconnectConfiguredGPS()
{
    emit manualConnectionRequested();
    SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS()->setRawValue(false);
    disconnectGPS();
    _setError(GPSConnectionError::None);
}

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

QString GPSRtk::_receiverConfig(GPSType type, RTKSettings* settings, uint32_t baudRate, GPSReceiverConfig& config,
                                bool allowPersistentChanges)
{
    config = GPSReceiverConfig{.baudRate = baudRate, .allowPersistentChanges = allowPersistentChanges};
    if (type == GPSType::passive) {
        config.role = GPSReceiverConfig::Role::Passive;
        return gpsReceiverConfigError(type, config);
    }
    switch (static_cast<BaseModeDefinition::Mode>(settings->useFixedBasePosition()->rawValue().toInt())) {
        case BaseModeDefinition::Mode::BaseFixed:
            config.base = GPSBaseStationConfig{
                .useFixedBase = true,
                .fixedBaseLatitude = settings->fixedBasePositionLatitude()->rawValue().toDouble(),
                .fixedBaseLongitude = settings->fixedBasePositionLongitude()->rawValue().toDouble(),
                .fixedBaseAltitudeMeters = settings->fixedBasePositionAltitude()->rawValue().toFloat(),
                .fixedBaseAccuracyMeters = settings->fixedBasePositionAccuracy()->rawValue().toFloat(),
            };
            break;
        case BaseModeDefinition::Mode::BaseSurveyIn:
            config.base = GPSBaseStationConfig{
                .surveyInAccMeters = settings->surveyInAccuracyLimit()->rawValue().toDouble(),
                .surveyInDurationSecs = settings->surveyInMinObservationDuration()->rawValue().toLongLong(),
            };
            break;
        case BaseModeDefinition::Mode::BaseReceiverAveraging:
            config.base.surveyMode = GPSBaseStationConfig::SurveyMode::ReceiverManaged;
            config.base.receiverAveragingDurationSecs = settings->receiverAveragingDuration()->rawValue().toUInt();
            break;
        default:
            return tr("Select a supported base mode.");
    }
    return gpsReceiverConfigError(type, config);
}

bool GPSRtk::connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                             const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges)
{
    RTKSettings* const settings = SettingsManager::instance()->rtkSettings();
    GPSReceiverConfig config;
    const QString configError = _receiverConfig(type, settings, baudRate, config, allowPersistentChanges);
    if (!configError.isEmpty()) {
        _setError(GPSConnectionError::ConfigFailed, configError);
        return false;
    }
    disconnectGPS();
    settings->baseReceiverManufacturers()->setRawValue(manufacturerForType(type));
    _setError(GPSConnectionError::None);
    _activeManufacturer = manufacturerForType(type);
    _activeBaseMode =
        config.role == GPSReceiverConfig::Role::Passive ? -1 : settings->useFixedBasePosition()->rawValue().toInt();
    _gpsProvider = new GPSProvider(std::move(transportFactory), type, config, this);
    const QPointer<GPSProvider> provider = _gpsProvider;
    const QPointer<GPSCorrectionManager> correctionManager = _correctionManager;
    if (correctionManager) {
        auto registration = correctionManager->registerSource(GPSCorrectionSource::LocalReceiver, sourceInstance);
        if (!provider || _gpsProvider != provider) {
            return false;
        }
        _correctionRegistration = std::move(registration);
    }
    const auto token = _correctionRegistration.token();
    const auto current = [this, provider, token, registered = !correctionManager.isNull()]() {
        return provider && _gpsProvider == provider && _activeManufacturer != 0 && (!registered || token.valid());
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
    (void) connectCurrent(&GPSProvider::configurationError, [this](const QString& detail) {
        if (!detail.isEmpty()) {
            _setError(GPSConnectionError::ConfigFailed, tr("Receiver configuration failed: %1").arg(detail));
        }
    });
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
    emit receiverChanged();
    return provider && _gpsProvider == provider;
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
