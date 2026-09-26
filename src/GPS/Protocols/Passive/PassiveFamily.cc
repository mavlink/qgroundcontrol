#include "Passive/PassiveFamily.h"

#include <memory>

#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSNMEAStream.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PassiveProtocolLog, "GPS.Driver.Protocols.Passive")

namespace {

constexpr unsigned MINIMUM_BAUD = 1200;
constexpr unsigned MAXIMUM_BAUD = 4000000;

class PassiveProtocol final : public GPSFamilyProtocol
{
public:
    explicit PassiveProtocol(const GPSFamilyOptions& options)
        : _nmea(GPSNMEAStream::Navigation::StandardNMEA, options.satelliteInfoEnabled)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override
    {
        _configured = false;
        _nmea.reset(channel.stream());
        if (config.allowPersistentChanges || config.base != GPSBaseStationConfig{} || baud < MINIMUM_BAUD ||
            baud > MAXIMUM_BAUD) {
            qCWarning(PassiveProtocolLog)
                << "Passive input requires an explicit baud rate and no receiver configuration";
            co_return false;
        }
        if (!co_await channel.setBaudrate(baud)) {
            if (channel.error() != GPSProtocolError::Cancelled) {
                qCWarning(PassiveProtocolLog) << "Could not set the passive input baud rate";
            }
            co_return false;
        }
        _nmea.setRTCMEnabled(true);
        _configured = true;
        co_return true;
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _nmea.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _nmea.flush(context); }

    bool receiverReady(const GPSDecodeContext&) const override { return _configured; }

    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        return _nmea.receive(channel, timeout);
    }

private:
    GPSNMEAStream _nmea;
    bool _configured = false;
};

}  // namespace

namespace Passive {

const GPSReceiverFamily FAMILY{
    .type = GPSType::passive,
    .name = QLatin1StringView("Passive"),
    .logCategory = &PassiveProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .create = &gpsCreateProtocol<PassiveProtocol>,
};

}  // namespace Passive
