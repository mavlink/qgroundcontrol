#include "Quectel/QuectelFamily.h"

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>

#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSNMEAStream.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "Quectel/QuectelCodec_p.h"
#include "Quectel/QuectelConfigurator.h"
#include "Quectel/QuectelDecoder.h"
#include "Quectel/QuectelPlan.h"

QGC_LOGGING_CATEGORY(QuectelProtocolLog, "GPS.Driver.Protocols.Quectel")

namespace {

/// Stale survey status is revoked before each read, however long the caller's receive.
constexpr std::chrono::milliseconds MAXIMUM_RECEIVE{1000};

class QuectelProtocol final : public GPSFamilyProtocol
{
public:
    explicit QuectelProtocol(const GPSFamilyOptions& options)
        : _decoder(options.satelliteInfoEnabled)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override
    {
        return Quectel::configureBase(channel, _decoder, std::move(config), baud);
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _decoder.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _decoder.flush(context); }

    bool receiverReady(const GPSDecodeContext& context) const override
    {
        return _decoder.configured() && !context.failed();
    }

    QString identity() const override { return _decoder.identity(); }

    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context) override
    {
        _decoder.setBase(config.base.mode);
        _decoder.monitorSurvey(context);
        _decoder.finishSession(context);
        return true;
    }

    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        _decoder.expireSurvey(channel.context());
        const GPSReceiveUpdates updates = co_await channel.receiveCycle(
            _decoder.limitReceiveTimeout(std::min(timeout, MAXIMUM_RECEIVE), channel.nowUs()));
        co_await channel.serviceControls();
        if (channel.failed()) {
            _decoder.endSession(channel.context());
            (void) channel.flush();
        }
        co_return updates;
    }

private:
    Quectel::Decoder _decoder;
};

QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine || !text.starts_with("$PQTM")) {
        return {};
    }
    const auto sentence = NMEA::frame(text);
    return sentence && sentence->hasValidChecksum() ? QLatin1StringView("$PQTM sentences") : QLatin1StringView();
}

/// The firmware query the configurator probes each rate with. Any firmware answers, qualified or not.
GPSTask<bool> probe(GPSCommandChannel& channel, GPSFamilyProtocol&)
{
    auto query = Quectel::Plan::command(Quectel::Plan::IDENTIFY, [](std::string_view body) {
        const QuectelCodec::Fields reply(body);
        return reply.size() == 4 && reply[0] == Quectel::Plan::IDENTIFY ? GPSCommandOutcome::Acknowledged
                                                                        : GPSCommandOutcome::Pending;
    });
    const auto result = co_await channel.transact(query.step, query.wire, std::get<GPSTextMatcher>(query.reply));
    co_return result.succeeded();
}

}  // namespace

namespace Quectel {

const GPSReceiverFamily FAMILY{
    .type = GPSType::quectel,
    .name = QLatin1StringView("Quectel"),
    .logCategory = &QuectelProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<QuectelProtocol>,
    .signature = &signature,
    .probe = &probe,
};

}  // namespace Quectel
