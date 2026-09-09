#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

#include <cstdint>
#include <functional>
#include <memory>

#include "GPSObservation.h"
#include "GPSReceiverCapabilities.h"
#include "GPSType.h"

class GPSTransport;

/// Configuration used only by the RTK base-station role.
struct GPSBaseStationConfig
{
    bool useFixedBase = false;
    double surveyInAccMeters = 0.0;
    int surveyInDurationSecs = 0;
    double fixedBaseLatitude = 0.0;
    double fixedBaseLongitude = 0.0;
    float fixedBaseAltitudeMeters = 0.0f;
    float fixedBaseAccuracyMeters = 0.0f;
};

/// Receiver configuration, decoupled from QGC settings types.
struct GPSReceiverConfig
{
    enum class Role
    {
        RTKBase = 0,
        Position = 1
    };

    enum class OutputProtocol
    {
        Native,
        NMEA
    };

    Role role = Role::RTKBase;
    OutputProtocol outputProtocol = OutputProtocol::Native;
    GPSBaseStationConfig base;
    QString validationError() const;

    float headingOffsetDeg = 5.0f;  // dual-antenna heading offset; consumed only by the Septentrio (SBF) driver
};

Q_DECLARE_METATYPE(GPSReceiverConfig)

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
    std::function<void(const GPSObservation&)> onPosition;
    std::function<void(const GPSSatelliteObservation&)> onSatelliteInfo;
    std::function<void(const GPSRelativeObservation&)> onRelativePosition;
    std::function<void(const QByteArray &)> onRTCM;
    std::function<void(const GPSSurveyInStatus &)> onSurveyIn;
};

/// Facade over the px4-gpsdrivers library: selects and configures the receiver
/// driver, bridges its callbacks to a GPSTransport plus the supplied sinks, and
/// pumps its receive loop. Keeps all px4 headers and types out of callers.
class GPSDriver
{
public:
    enum class ConfigurationStatus
    {
        NotConfigured,
        Ready,
        Unsupported,
        Cancelled,
        TransportError,
        Failed,
    };

    struct ConfigurationResult
    {
        ConfigurationStatus status = ConfigurationStatus::NotConfigured;
        QString error;
    };

    enum class ReceiveStatus
    {
        Data,
        Idle,
        Cancelled,
        DeviceError,
        NotConfigured,
    };

    struct ReceiveResult
    {
        ReceiveStatus status = ReceiveStatus::NotConfigured;
        bool positionUpdated = false;
        bool satellitesUpdated = false;
    };

    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks);
    ~GPSDriver();

    GPSDriver(const GPSDriver&) = delete;
    GPSDriver& operator=(const GPSDriver&) = delete;

    /// Create and configure the underlying driver. Returns false on failure.
    bool configure();

    const ConfigurationResult& configurationResult() const { return _configurationResult; }

    const GPSReceiverCapabilities& capabilities() const { return _capabilities; }

    /// Pump one receive cycle, invoking the position/satellite sinks as data
    /// arrives. Returns the px4 bitset (<0 error, bit0 position, bit1 satellite),
    /// or <0 if not configured.
    int receive(unsigned timeoutMs);
    ReceiveResult receiveResult(unsigned timeoutMs);

    enum class CorrectionStatus
    {
        Submitted,
        NotReady,
        Unsupported,
        Cancelled,
        TransportError,
        InvalidData,
    };

    struct CorrectionResult
    {
        CorrectionStatus status = CorrectionStatus::NotReady;
        qsizetype bytesWritten = 0;
    };

    /// Worker-thread-only: configuration and receive calls must not run concurrently.
    bool readyForCorrections() const;
    CorrectionResult injectCorrections(const QByteArray& data);

    unsigned baudrate() const { return _baudrate; }

    /// Trampoline target for the px4 callback; `type` is a GPSCallbackType value.
    /// Public only so the file-local C callback can reach it — not for callers.
    int handleCallback(int type, void* data1, int data2);

private:
    void _updateCapabilities();

    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;
    unsigned _baudrate = 0;
    GPSReceiverCapabilities _capabilities;
    ConfigurationResult _configurationResult;

    struct Private;
    std::unique_ptr<Private> _private;
};
