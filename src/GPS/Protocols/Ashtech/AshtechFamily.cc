#include "Ashtech/AshtechFamily.h"

#include <memory>
#include <string_view>
#include <utility>

#include "Ashtech/AshtechPlan.h"
#include "GPSCommandChannel.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AshtechProtocolLog, "GPS.Driver.Protocols.Ashtech")

namespace Ashtech {

namespace {

QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine || !text.starts_with("$PASHR,")) {
        return {};
    }
    const auto sentence = NMEA::frame(text);
    return sentence && sentence->hasValidChecksum() ? QLatin1StringView("$PASHR sentences") : QLatin1StringView();
}

/// The port query Configurator::configure() probes each rate with.
GPSTask<bool> probe(GPSCommandChannel& channel, GPSFamilyProtocol&)
{
    auto query = Plan::command(Plan::line(Plan::PORT_QUERY), [](std::string_view reply) {
        return reply.starts_with("$PASHR,PRT,") ? GPSCommandOutcome::Acknowledged : Plan::rejection(reply);
    });
    query.attempts = Plan::PORT_QUERY_ATTEMPTS;
    const GPSCommandSequence sequence{{std::move(query)}};
    co_return (co_await channel.runSequence(sequence)).succeeded();
}

}  // namespace

const GPSReceiverFamily FAMILY{
    .type = GPSType::trimble,
    .name = QLatin1StringView("Ashtech"),
    .logCategory = &AshtechProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    // Unless a rate is selected, configure at the Trimble default rather than probing BAUD_RATES.
    .autoBaudRate = 115200,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
    .probe = &probe,
};

}  // namespace Ashtech
