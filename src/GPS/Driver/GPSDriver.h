#pragma once

#include "GPSType.h"
#include "satellite_info.h"
#include "sensor_gps.h"

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

#include <cstdint>
#include <functional>
#include <memory>

class GPSTransport;
class GPSBaseStationSupport;

/// RTK base-station configuration, decoupled from QGC settings types.
struct GPSReceiverConfig
{
    bool useFixedBase = false;
    double surveyInAccMeters = 0.0;
    int surveyInDurationSecs = 0;
    double fixedBaseLatitude = 0.0;
    double fixedBaseLongitude = 0.0;
    float fixedBaseAltitudeMeters = 0.0f;
    float fixedBaseAccuracyMeters = 0.0f;
    float headingOffsetDeg = 5.0f;  // dual-antenna heading offset; consumed only by the Septentrio (SBF) driver
};

/// Survey-in progress, translated from the px4 SurveyInStatus.
struct GPSSurveyInStatus
{
    double latitude = 0.0;
    double longitude = 0.0;
    float altitude = 0.0f;
    uint32_t meanAccuracyMM = 0;
    uint32_t durationSecs = 0;
    bool valid = false;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSSurveyInStatus)

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
    GPSDriver(GPSType type, GPSTransport &transport, const GPSReceiverConfig &config, GPSDriverSinks sinks);
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
    GPSType _type;
    GPSTransport &_transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;

    std::unique_ptr<GPSBaseStationSupport> _driver;
    sensor_gps_s _sensorGps{};
    satellite_info_s _satelliteInfo{};
};
