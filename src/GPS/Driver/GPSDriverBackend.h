#pragma once

#include <QtCore/QLatin1StringView>

#include <array>
#include <memory>
#include <span>
#include <type_traits>

#include "GPSDriver.h"
#include "PX4/base_station.h"

/// Private integration boundary: a receiver need not implement base-station operations.
class GPSDriverBackend
{
public:
    GPSDriverBackend();
    virtual ~GPSDriverBackend();

    int configure(unsigned& baudrate, const GPSReceiverConfig& config);

    int receive(unsigned timeoutMs) { return _driver ? _driver->receive(timeoutMs) : -1; }

    int ioError() const { return _driver ? _driver->ioError() : 0; }

    bool receiverReady() const { return _driver && _driver->receiverReady(); }

    virtual void updateCapabilities(GPSReceiverCapabilities&) const {}

    virtual QString configurationError() const { return {}; }

    virtual void completeConfigurationReport(const GPSReceiverConfig&, bool, GPSConfigurationReport&) {}

protected:
    template <class Driver>
    void setDriver(std::unique_ptr<Driver> driver)
    {
        _baseStation = nullptr;
        if constexpr (std::is_base_of_v<GPSBaseStationSupport, Driver>) {
            _baseStation = driver.get();
        }
        _driver = std::move(driver);
    }

    virtual int configureReceiver(unsigned& baudrate, const GPSHelper::GPSConfig& config,
                                  GPSReceiverConfig::OutputProtocol protocol);

    GPSHelper& driver() { return *_driver; }

    const GPSHelper& driver() const { return *_driver; }

private:
    std::unique_ptr<GPSHelper> _driver;
    GPSBaseStationSupport* _baseStation = nullptr;
};

struct GPSDriverFamily
{
    using Factory = std::unique_ptr<GPSDriverBackend> (*)(GPSCallbackPtr, void*, sensor_gps_s*, satellite_info_s*,
                                                          const GPSReceiverConfig&);
    GPSType type;
    QLatin1StringView name;
    std::array<QLatin1StringView, 3> aliases;
    int manufacturerId;
    GPSReceiverCapabilities::Support baseSupport;
    GPSReceiverCapabilities::Support nmeaSupport;
    GPSReceiverCapabilities::Support correctionInput;
    Factory create;
    GPSReceiverCapabilities::Support constellationSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support dynamicModelSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support outputRateSelection = GPSReceiverCapabilities::Support::Unsupported;
    GPSReceiverCapabilities::Support headingOffsetSelection = GPSReceiverCapabilities::Support::Unsupported;
};

std::span<const GPSDriverFamily> gpsDriverFamilies();
const GPSDriverFamily* gpsDriverFamily(GPSType type);
