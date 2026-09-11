#pragma once

#include <QtCore/QLatin1StringView>

#include <array>
#include <memory>
#include <span>
#include <type_traits>

#include "GPSDriver.h"
#include "Protocols/GPSBaseProtocol.h"

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
        _driver = std::move(driver);
    }

    virtual int configureReceiver(unsigned& baudrate, const GPSProtocol::GPSConfig& config,
                                  GPSReceiverConfig::OutputProtocol protocol);

    GPSProtocol& driver() { return *_driver; }

    const GPSProtocol& driver() const { return *_driver; }

private:
    std::unique_ptr<GPSProtocol> _driver;
};

struct GPSDriverFamily
{
    using Factory = std::unique_ptr<GPSDriverBackend> (*)(GPSProtocolIO, GPSPositionReport*, GPSSatelliteReport*,
                                                          const GPSReceiverConfig&);
    GPSType type;
    Factory create;
};

std::span<const GPSDriverFamily> gpsDriverFamilies();
const GPSDriverFamily* gpsDriverFamily(GPSType type);
