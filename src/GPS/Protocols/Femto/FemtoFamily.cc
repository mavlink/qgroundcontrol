#include "Femto/FemtoFamily.h"

#include <array>
#include <memory>
#include <string_view>

#include "Femto/FemtoPlan.h"
#include "GPSASCIILog.h"
#include "GPSCommandChannel.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(FemtoProtocolLog, "GPS.Driver.Protocols.Femto")

namespace Femto {

namespace {

bool portName(std::string_view field)
{
    const size_t digits = field.find_first_of("0123456789");
    if (digits == 0 || digits == std::string_view::npos) {
        return false;
    }
    for (size_t index = 0; index < field.size(); ++index) {
        const char ch = field[index];
        if (index < digits ? !(ch >= 'A' && ch <= 'Z') : !(ch >= '0' && ch <= '9')) {
            return false;
        }
    }
    return true;
}

/// Femtomes receivers speak the NovAtel command set: abbreviated "<... OK" replies, and ASCII logs whose header names
/// the port, unlike Unicore's.
QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine) {
        return {};
    }
    if (text.starts_with('<') && (text.ends_with(" OK") || text.starts_with(Plan::REJECTED))) {
        return QLatin1StringView("abbreviated ASCII replies");
    }
    const auto log = gpsASCIILog(text);
    return log && portName(log->source) ? QLatin1StringView("NovAtel-format ASCII logs") : QLatin1StringView();
}

/// The read-only half of Plan::IDENTIFY, in as many rounds: UNLOGALL would stop the receiver's logs.
constexpr std::array VERSION_QUERY{Plan::IDENTIFY[1]};

GPSTask<bool> probe(GPSCommandChannel& channel, GPSFamilyProtocol&)
{
    bool identified = false;
    for (unsigned round = 0; round < Plan::IDENTIFY_ROUNDS && !identified && !channel.failed(); ++round) {
        identified = (co_await channel.runSequence(Plan::sequence(VERSION_QUERY))).succeeded();
    }
    co_return identified;
}

}  // namespace

const GPSReceiverFamily FAMILY{
    .type = GPSType::femto,
    .name = QLatin1StringView("Femto"),
    .logCategory = &FemtoProtocolLog,
    // RTCM3 is framed once the receiver answered (see Configurator::configure()).
    .stream = {.framers = GPSFrameKind::NMEASentence | GPSFrameKind::RTCM3,
               .enabled = GPSFrameKind::NMEASentence,
               .sentenceBufferSize = 600},
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
    .probe = &probe,
};

}  // namespace Femto
