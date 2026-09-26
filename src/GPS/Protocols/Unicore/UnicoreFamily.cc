#include "Unicore/UnicoreFamily.h"

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>
#include <variant>

#include "GPSASCIILog.h"
#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSNMEAStream.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "Unicore/UnicoreConfigurator.h"
#include "Unicore/UnicoreDecoder.h"
#include "Unicore/UnicorePlan.h"

QGC_LOGGING_CATEGORY(UnicoreProtocolLog, "GPS.Driver.Protocols.Unicore")

namespace {

class UnicoreProtocol final : public GPSFamilyProtocol
{
public:
    explicit UnicoreProtocol(const GPSFamilyOptions& options)
        : _decoder(options.satelliteInfoEnabled)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override
    {
        return Unicore::configureBase(channel, _decoder, std::move(config), baud);
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _decoder.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _decoder.flush(context); }

    bool receiverReady(const GPSDecodeContext& context) const override { return _decoder.ready() && !context.failed(); }

    QString identity() const override { return _decoder.identity(); }

    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext&) override
    {
        if (std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode)) {
            return false;
        }
        _decoder.armDecodeOnly(config.base.mode);
        return true;
    }

    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        return _decoder.nmea().receive(channel, timeout);
    }

    [[nodiscard]] Unicore::Decoder& decoder() { return _decoder; }

private:
    Unicore::Decoder _decoder;
};

/// "$command,<command>,response: <status>*hh", whose checksum, unlike NMEA's, includes the '$'.
bool commandReply(std::string_view line)
{
    const size_t star = line.rfind('*');
    if (!line.starts_with("$command,") || line.find(",response: ") == std::string_view::npos ||
        star == std::string_view::npos || star + 3 != line.size()) {
        return false;
    }
    const auto expected = NMEA::number<unsigned>(line.substr(star + 1), NMEA::HEX_BASE);
    return expected && *expected == NMEA::checksum(line.substr(0, star));
}

/// Unicore logs carry the CPU idle time where NovAtel-compatible receivers name the port.
QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    if (commandReply(text)) {
        return QLatin1StringView("Unicore command replies");
    }
    const auto log = gpsASCIILog(text);
    const bool unicore =
        log && !log->source.empty() && std::ranges::all_of(log->source, [](char ch) { return ch >= '0' && ch <= '9'; });
    return unicore ? QLatin1StringView("Unicore ASCII logs") : QLatin1StringView();
}

/// The identity query Unicore::configureBase() probes each rate with. A receiver that reports its model answered, even
/// when its firmware is not supported.
GPSTask<bool> probe(GPSCommandChannel& channel, GPSFamilyProtocol& protocol)
{
    Unicore::Decoder& decoder = static_cast<UnicoreProtocol&>(protocol).decoder();
    const Unicore::Plan::Command identify = Unicore::Plan::identify();
    decoder.expect(identify);
    const QByteArray wire = identify.text + "\r\n";
    const auto result =
        co_await channel.transact({identify.name().toStdString(), Unicore::Plan::COMMAND_TIMEOUT}, wire);
    co_return result.succeeded() || !decoder.model().isEmpty();
}

}  // namespace

namespace Unicore {

const GPSReceiverFamily FAMILY{
    .type = GPSType::unicore,
    .name = QLatin1StringView("Unicore"),
    .logCategory = &UnicoreProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<UnicoreProtocol>,
    .signature = &signature,
    .probe = &probe,
};

}  // namespace Unicore
