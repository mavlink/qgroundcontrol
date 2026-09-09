#include "GPSDriverBackend.h"

#include <QtCore/QCoreApplication>

#include <numbers>

#include "PX4/ashtech.h"
#include "PX4/femtomes.h"
#include "PX4/sbf.h"
#include "PX4/ubx.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSDriverBackendLog, "GPS.Driver.GPSDriverBackend")

GPSDriverBackend::GPSDriverBackend()
{
    qCDebug(GPSDriverBackendLog) << this;
}

GPSDriverBackend::~GPSDriverBackend()
{
    qCDebug(GPSDriverBackendLog) << this;
}

namespace {
class UbxBackend final : public GPSDriverBackend
{
public:
    UbxBackend(GPSCallbackPtr callback, void* user, sensor_gps_s* position, satellite_info_s* satellites,
               const GPSReceiverConfig& config)
    {
        const GPSDriverUBX::Settings settings = {
            .dynamic_model = static_cast<uint8_t>(config.dynamicModel),
            .dgnss_timeout = 0,
            .min_cno = 0,
            .min_elev = 0,
            .output_rate = static_cast<uint8_t>(config.outputRateHz),
            .heading_offset = 0.0f,
            .uart1_baudrate = 0,
            .uart2_baudrate = 57600,
            .ppk_output = false,
            .jam_det_sensitivity_hi = false,
            .mode = GPSDriverUBX::UBXMode::Normal,
        };
        setDriver(std::make_unique<GPSDriverUBX>(GPSDriverUBX::Interface::UART, callback, user, position, satellites,
                                                 settings));
    }

    void updateCapabilities(GPSReceiverCapabilities& capabilities) const override
    {
        const auto& receiver = static_cast<const GPSDriverUBX&>(driver());
        capabilities.model = QString::fromLatin1(receiver.modelName());
        capabilities.firmware = QString::fromLatin1(receiver.firmwareVersion());
        using Support = GPSReceiverCapabilities::Support;
        if (!capabilities.model.isEmpty()) {
            capabilities.constellationSelection =
                receiver.supportsConstellationSelection() ? Support::Supported : Support::Unsupported;
            capabilities.outputRateSelection =
                receiver.supportsOutputRateSelection() ? Support::Supported : Support::Unsupported;
        }
        switch (receiver.baseStationCapability()) {
            case GPSDriverUBX::BaseStationCapability::Supported:
                capabilities.rtkBase = Support::Supported;
                capabilities.correctionInput = Support::Supported;
                break;
            case GPSDriverUBX::BaseStationCapability::Unsupported:
                capabilities.rtkBase = Support::Unsupported;
                // Only advertise correction injection for models whose RTCM configuration is known.
                capabilities.correctionInput = Support::Unsupported;
                break;
            case GPSDriverUBX::BaseStationCapability::Unknown:
                break;
        }
    }

    QString configurationError() const override
    {
        if (static_cast<const GPSDriverUBX&>(driver()).constellationConfigurationRejected()) {
            return QCoreApplication::translate("GPSDriver",
                                               "Receiver did not accept the requested constellation configuration");
        }
        return {};
    }

private:
    int configureReceiver(unsigned& baudrate, const GPSHelper::GPSConfig& config,
                          GPSReceiverConfig::OutputProtocol protocol) override
    {
        return static_cast<GPSDriverUBX&>(driver()).configure(baudrate, config,
                                                              protocol == GPSReceiverConfig::OutputProtocol::NMEA
                                                                  ? GPSDriverUBX::OutputProtocol::NMEA
                                                                  : GPSDriverUBX::OutputProtocol::Native);
    }
};

class AshtechBackend final : public GPSDriverBackend
{
public:
    AshtechBackend(GPSCallbackPtr callback, void* user, sensor_gps_s* position, satellite_info_s* satellites,
                   const GPSReceiverConfig&)
    {
        setDriver(std::make_unique<GPSDriverAshtech>(callback, user, position, satellites));
    }

private:
    int configureReceiver(unsigned& baudrate, const GPSHelper::GPSConfig& config,
                          GPSReceiverConfig::OutputProtocol protocol) override
    {
        baudrate = 115200;
        return GPSDriverBackend::configureReceiver(baudrate, config, protocol);
    }
};

class SbfBackend final : public GPSDriverBackend
{
public:
    SbfBackend(GPSCallbackPtr callback, void* user, sensor_gps_s* position, satellite_info_s* satellites,
               const GPSReceiverConfig& config)
    {
        setDriver(std::make_unique<GPSDriverSBF>(callback, user, position, satellites,
                                                 config.headingOffsetDeg * std::numbers::pi_v<float> / 180.0f));
    }
};

class FemtoBackend final : public GPSDriverBackend
{
public:
    FemtoBackend(GPSCallbackPtr callback, void* user, sensor_gps_s* position, satellite_info_s* satellites,
                 const GPSReceiverConfig&)
    {
        setDriver(std::make_unique<GPSDriverFemto>(callback, user, position, satellites));
    }
};

template <class Backend>
std::unique_ptr<GPSDriverBackend> createBackend(GPSCallbackPtr callback, void* user, sensor_gps_s* position,
                                                satellite_info_s* satellites, const GPSReceiverConfig& config)
{
    return std::make_unique<Backend>(callback, user, position, satellites, config);
}

using Support = GPSReceiverCapabilities::Support;
constexpr std::array families = {
    GPSDriverFamily{GPSType::u_blox,
                    QLatin1StringView("U-blox"),
                    {QLatin1StringView("blox"), QLatin1StringView("ubx"), QLatin1StringView("u-blox")},
                    4,
                    Support::Unknown,
                    Support::Supported,
                    Support::Unknown,
                    &createBackend<UbxBackend>,
                    Support::Unknown,
                    Support::Supported,
                    Support::Unknown,
                    Support::Unsupported},
    GPSDriverFamily{GPSType::trimble,
                    QLatin1StringView("Trimble"),
                    {QLatin1StringView("trimble"), QLatin1StringView("ashtech"), QLatin1StringView("spectra")},
                    1,
                    Support::Supported,
                    Support::Unsupported,
                    Support::Unknown,
                    &createBackend<AshtechBackend>},
    GPSDriverFamily{GPSType::septentrio,
                    QLatin1StringView("Septentrio"),
                    {QLatin1StringView("septentrio"), QLatin1StringView("sbf"), QLatin1StringView()},
                    2,
                    Support::Supported,
                    Support::Unsupported,
                    Support::Supported,
                    &createBackend<SbfBackend>,
                    Support::Unsupported,
                    Support::Unsupported,
                    Support::Unsupported,
                    Support::Supported},
    GPSDriverFamily{GPSType::femto,
                    QLatin1StringView("Femtomes"),
                    {QLatin1StringView("femtomes"), QLatin1StringView("femto"), QLatin1StringView()},
                    3,
                    Support::Supported,
                    Support::Unsupported,
                    Support::Unknown,
                    &createBackend<FemtoBackend>},
};
}  // namespace

int GPSDriverBackend::configure(unsigned& baudrate, const GPSReceiverConfig& config)
{
    if (!_driver || !config.validationError().isEmpty()) {
        return -1;
    }
    if (config.role == GPSReceiverConfig::Role::RTKBase) {
        if (!_baseStation) {
            return -1;
        }
        if (config.base.useFixedBase) {
            _baseStation->setBasePosition(config.base.fixedBaseLatitude, config.base.fixedBaseLongitude,
                                          config.base.fixedBaseAltitudeMeters,
                                          config.base.fixedBaseAccuracyMeters * 1000.0f);
        } else {
            _baseStation->setSurveyInSpecs(static_cast<uint32_t>(config.base.surveyInAccMeters * 10000.0),
                                           static_cast<uint32_t>(config.base.surveyInDurationSecs));
        }
    }
    GPSHelper::GPSConfig nativeConfig = {};
    nativeConfig.output_mode =
        config.role == GPSReceiverConfig::Role::RTKBase ? GPSHelper::OutputMode::RTCM : GPSHelper::OutputMode::GPS;
    nativeConfig.gnss_systems = static_cast<GPSHelper::GNSSSystemsMask>(config.constellationMask);
    nativeConfig.require_gnss_config = config.constellationMask != 0;
    const int result = configureReceiver(baudrate, nativeConfig, config.outputProtocol);
    return _driver->ioError() ? _driver->ioError() : result;
}

int GPSDriverBackend::configureReceiver(unsigned& baudrate, const GPSHelper::GPSConfig& config,
                                        GPSReceiverConfig::OutputProtocol protocol)
{
    return protocol == GPSReceiverConfig::OutputProtocol::Native ? _driver->configure(baudrate, config) : -1;
}

std::span<const GPSDriverFamily> gpsDriverFamilies()
{
    return families;
}

const GPSDriverFamily* gpsDriverFamily(GPSType type)
{
    for (const auto& family : families) {
        if (family.type == type) {
            return &family;
        }
    }
    return nullptr;
}
