#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

#include <cstdint>
#include <functional>
#include <memory>

#include "GPSReceiverTypes.h"
#include "satellite_info.h"
#include "sensor_gps.h"

class GPSTransport;
class GPSBaseStationSupport;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receive().
struct GPSDriverSinks
{
    std::function<void(const sensor_gps_s &)> onPosition;
    std::function<void(const satellite_info_s &)> onSatelliteInfo;
    std::function<void(const QByteArray &)> onRTCM;
    std::function<void(const GPSSurveyInStatus &)> onSurveyIn;
};

/// Facade over the px4-gpsdrivers library: selects and configures the receiver
/// driver, bridges its callbacks to a GPSTransport plus the supplied sinks, and
/// pumps its receive loop. Keeps all px4 headers and types out of callers.
class GPSDriver
{
public:
    GPSDriver(GPSReceiverType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks);
    ~GPSDriver();

    GPSDriver(const GPSDriver &) = delete;
    GPSDriver &operator=(const GPSDriver &) = delete;

    /// Create and configure the underlying driver. Returns false on failure.
    bool configure();

    /// Pump one receive cycle, invoking the position/satellite sinks as data
    /// arrives. Returns the px4 bitset (<0 error, bit0 position, bit1 satellite),
    /// or <0 if not configured.
    int receive(unsigned timeoutMs);

    /// Trampoline target for the px4 callback; `type` is a GPSCallbackType value.
    /// Public only so the file-local C callback can reach it — not for callers.
    int handleCallback(int type, void *data1, int data2);

private:
    GPSReceiverType _type;
    GPSTransport &_transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;

    std::unique_ptr<GPSBaseStationSupport> _driver;
    sensor_gps_s _sensorGps{};
    satellite_info_s _satelliteInfo{};
};
