#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QMetaType>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "GPSConfigurationReport.h"
#include "GPSDeadline.h"
#include "GPSExecutionContext.h"
#include "GPSIntegrityObservation.h"
#include "GPSObservation.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "GPSSurveyInStatus.h"
#include "GPSTransportResult.h"
#include "GPSType.h"

class GPSTransport;
struct GPSProtocolIO;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receive().
struct GPSDriverSinks
{
    std::function<void(const GPSObservation&)> onPosition;
    std::function<void(const GPSIntegrityObservation&)> onIntegrity;
    std::function<void(const GPSSatelliteObservation&)> onSatelliteInfo;
    std::function<void(const GPSRelativeObservation&)> onRelativePosition;
    std::function<void(const QByteArray&)> onRTCM;
    std::function<void(const GPSSurveyInStatus&)> onSurveyIn;
};

/// Receiver protocol facade: selects and configures the receiver
/// driver, connects typed protocol services to a GPSTransport plus the supplied sinks, and
/// pumps its receive loop. Keeps protocol implementation types out of callers.
class GPSDriver
{
    friend class GPSDriverTest;

public:
    using ConfigurationStatus = GPSConfigurationStatus;
    using ConfigurationResult = GPSConfigurationResult;

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
        std::optional<GPSReadResult> transportRead = {};
    };

    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks,
              GPSExecutionContext clock = {});
    ~GPSDriver();

    GPSDriver(const GPSDriver&) = delete;
    GPSDriver& operator=(const GPSDriver&) = delete;

    /// Create and configure the underlying driver. Returns false on failure.
    bool configure();

    const ConfigurationResult& configurationResult() const { return _configurationResult; }

    const GPSReceiverCapabilities& capabilities() const { return _capabilities; }

    const GPSConfigurationReport& configurationReport() const { return _configurationReport; }

    /// Pump one receive cycle and publish validated observations.
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
        qsizetype bytesAccepted = 0;
        qsizetype bytesUncertain = 0;
    };

    /// Worker-thread-only: configuration and receive calls must not run concurrently.
    bool readyForCorrections() const;
    CorrectionResult injectCorrections(const QByteArray& data, GPSDeadline deadline = {});
    CorrectionResult injectCorrections(const QByteArray& data, QDeadlineTimer deadline);

    unsigned baudrate() const { return _baudrate; }

private:
    void _updateCapabilities();
    GPSProtocolIO _protocolIO();

    GPSExecutionContext _clock;
    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;
    unsigned _baudrate = 0;
    GPSReceiverCapabilities _capabilities;
    ConfigurationResult _configurationResult;
    GPSConfigurationReport _configurationReport;

    struct Private;
    std::unique_ptr<Private> _private;
};
