#include "GPSDriverBackend.h"

#include <QtCore/QCoreApplication>

#include <cmath>
#include <numbers>

#include "Protocols/Ashtech/GPSDriverAshtech.h"
#include "Protocols/Femto/GPSDriverFemto.h"
#include "Protocols/SBF/GPSDriverSBF.h"
#include "Protocols/UBX/GPSDriverUBX.h"
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
    UbxBackend(GPSProtocolIO io, GPSPositionReport* position, GPSSatelliteReport* satellites,
               const GPSReceiverConfig& config)
    {
        const GPSDriverUBX::Settings settings = {
            .dynamic_model = static_cast<uint8_t>(config.dynamicModel),
            .output_rate = static_cast<uint8_t>(config.outputRateHz),
        };
        setDriver(std::make_unique<GPSDriverUBX>(std::move(io), position, satellites, settings));
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

    void completeConfigurationReport(const GPSReceiverConfig& config, bool configured,
                                     GPSConfigurationReport& report) override
    {
        auto& receiver = static_cast<GPSDriverUBX&>(driver());
        const auto tr = [](const char* text) { return QCoreApplication::translate("GPSDriver", text); };
        for (auto& setting : report.settings) {
            if (setting.id == GPSReceiverSetting::ConstellationMask && receiver.constellationConfigurationRejected()) {
                if (receiver.constellationRequestRejected()) {
                    setting.requestState = GPSSettingReport::RequestState::Rejected;
                }
                setting.detail = configurationError();
            } else if (configured &&
                       (setting.id == GPSReceiverSetting::DynamicModel ||
                        setting.id == GPSReceiverSetting::OutputRateHz ||
                        (setting.id == GPSReceiverSetting::ConstellationMask && config.constellationMask))) {
                setting.requestState = GPSSettingReport::RequestState::Acknowledged;
                setting.detail = tr("Configuration acknowledged; receiver readback is unavailable");
            }
        }
        GPSDriverUBX::ConfigurationReadback values;
        if (!configured || config.outputProtocol != GPSReceiverConfig::OutputProtocol::Native ||
            !receiver.readConfiguration(values, 500)) {
            return;
        }
        for (auto& setting : report.settings) {
            if (setting.id == GPSReceiverSetting::DynamicModel) {
                setting.reportedValue = values.dynamic_model;
                setting.comparisonApplicable = true;
            } else if (setting.id == GPSReceiverSetting::OutputRateHz && values.measurement_interval_ms &&
                       values.navigation_rate) {
                setting.reportedValue = 1000.0 / (double(values.measurement_interval_ms) * values.navigation_rate);
                setting.comparisonApplicable = config.outputRateHz != 0;
            } else if (setting.id == GPSReceiverSetting::ConstellationMask && values.constellations_reported) {
                setting.reportedValue = values.constellation_mask;
                setting.comparisonApplicable = config.constellationMask != 0;
            } else {
                continue;
            }
            setting.readbackState = GPSSettingReport::ReadbackState::Reported;
            setting.matchesRequested =
                setting.comparisonApplicable &&
                std::abs(setting.requestedValue.toDouble() - setting.reportedValue.toDouble()) < 1e-9;
            setting.detail = !setting.comparisonApplicable
                                 ? tr("Receiver defaults requested; current receiver value reported")
                             : setting.matchesRequested ? tr("Receiver readback matches the request")
                                                        : tr("Receiver reports a value different from the request");
        }
    }

private:
    int configureReceiver(unsigned& baudrate, const GPSProtocol::GPSConfig& config,
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
    AshtechBackend(GPSProtocolIO io, GPSPositionReport* position, GPSSatelliteReport* satellites,
                   const GPSReceiverConfig&)
    {
        setDriver(std::make_unique<GPSDriverAshtech>(std::move(io), position, satellites));
    }

private:
    int configureReceiver(unsigned& baudrate, const GPSProtocol::GPSConfig& config,
                          GPSReceiverConfig::OutputProtocol protocol) override
    {
        baudrate = 115200;
        return GPSDriverBackend::configureReceiver(baudrate, config, protocol);
    }
};

class SbfBackend final : public GPSDriverBackend
{
public:
    SbfBackend(GPSProtocolIO io, GPSPositionReport* position, GPSSatelliteReport* satellites,
               const GPSReceiverConfig& config)
    {
        setDriver(std::make_unique<GPSDriverSBF>(std::move(io), position, satellites,
                                                 config.headingOffsetDeg * std::numbers::pi_v<float> / 180.0f));
    }

    void completeConfigurationReport(const GPSReceiverConfig&, bool configured, GPSConfigurationReport& report) override
    {
        for (auto& setting : report.settings) {
            if (configured && setting.id == GPSReceiverSetting::HeadingOffsetDeg) {
                setting.requestState = GPSSettingReport::RequestState::Acknowledged;
                setting.detail = QCoreApplication::translate(
                    "GPSDriver", "Configuration acknowledged; receiver readback is unavailable");
            }
        }
    }
};

class FemtoBackend final : public GPSDriverBackend
{
public:
    FemtoBackend(GPSProtocolIO io, GPSPositionReport* position, GPSSatelliteReport* satellites,
                 const GPSReceiverConfig&)
    {
        setDriver(std::make_unique<GPSDriverFemto>(std::move(io), position, satellites));
    }
};

template <class Backend>
std::unique_ptr<GPSDriverBackend> createBackend(GPSProtocolIO io, GPSPositionReport* position,
                                                GPSSatelliteReport* satellites, const GPSReceiverConfig& config)
{
    return std::make_unique<Backend>(std::move(io), position, satellites, config);
}

constexpr std::array families = {
    GPSDriverFamily{GPSType::u_blox, &createBackend<UbxBackend>},
    GPSDriverFamily{GPSType::trimble, &createBackend<AshtechBackend>},
    GPSDriverFamily{GPSType::septentrio, &createBackend<SbfBackend>},
    GPSDriverFamily{GPSType::femto, &createBackend<FemtoBackend>},
};
}  // namespace

int GPSDriverBackend::configure(unsigned& baudrate, const GPSReceiverConfig& config)
{
    if (!_driver || !config.validationError().isEmpty()) {
        return -1;
    }
    GPSProtocol::GPSConfig nativeConfig = {};
    nativeConfig.base = config.base;
    nativeConfig.output_mode =
        config.role == GPSReceiverConfig::Role::RTKBase ? GPSProtocol::OutputMode::RTCM : GPSProtocol::OutputMode::GPS;
    nativeConfig.gnss_systems = static_cast<GPSProtocol::GNSSSystemsMask>(config.constellationMask);
    nativeConfig.require_gnss_config = config.constellationMask != 0;
    const int result = configureReceiver(baudrate, nativeConfig, config.outputProtocol);
    return _driver->ioError() ? _driver->ioError() : result;
}

int GPSDriverBackend::configureReceiver(unsigned& baudrate, const GPSProtocol::GPSConfig& config,
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
