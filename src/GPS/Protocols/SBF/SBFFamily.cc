#include "SBF/SBFFamily.h"

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "Checksums.h"
#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSStreamDemux.h"
#include "QGCLoggingCategory.h"
#include "SBF/Generated/SBFBlocks.h"
#include "SBF/SBFConfigurator.h"
#include "SBF/SBFDecoder.h"
#include "SBF/SBFPlan.h"
#include "WireFields.h"

QGC_LOGGING_CATEGORY(SBFProtocolLog, "GPS.Driver.Protocols.SBF")

namespace {

constexpr std::array<unsigned, 1> BAUD_RATES{SBF::Plan::BAUD_RATE};

class SBFProtocol final : public GPSFamilyProtocol
{
public:
    explicit SBFProtocol(const GPSFamilyOptions& options)
        : _decoder(options.satelliteInfoEnabled)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _decoder.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _decoder.flush(context); }

    bool receiverReady(const GPSDecodeContext&) const override { return _decoder.baseRunning(); }

    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context) override
    {
        context.stream().setEnabled(GPSFrameKind::RTCM3, true);
        _decoder.startBase(config.base, context.nowUs());
        return true;
    }

    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override;

private:
    SBF::Decoder _decoder;
};

GPSTask<bool> SBFProtocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _decoder.stopBase();
    if (!channel.validateConfiguration(config)) {
        co_return false;
    }
    _decoder.resetEpochs();
    channel.stream().setEnabled(GPSFrameKind::RTCM3, false);
    channel.stream().reset(GPSFrameKind::SBF);
    if (!co_await SBF::configureBase(channel, config.base, baud)) {
        co_return false;
    }
    _decoder.startBase(config.base, channel.nowUs());
    co_return true;
}

GPSTask<GPSReceiveUpdates> SBFProtocol::receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
{
    if (!_decoder.baseRunning() || channel.failed()) {
        co_return GPSReceiveUpdates{};
    }
    co_return co_await channel.receiveCycle(timeout);
}

bool upper(char ch)
{
    return ch >= 'A' && ch <= 'Z';
}

bool digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

/// The command prompt: a connection descriptor such as "USB1", "COM2" or "IP10" before '>', at the start of the
/// text or after a separator. Unlike the configurator, which trusts any '>' from a receiver it already expects, the
/// probe must not take line noise at a wrong rate for a prompt.
bool prompted(QByteArrayView text)
{
    for (qsizetype end = text.indexOf('>'); end >= 0; end = text.indexOf('>', end + 1)) {
        if (end < 4) {
            continue;
        }
        const QByteArrayView descriptor = text.sliced(end - 4, 4);
        const bool separated = end == 4 || !(upper(text[end - 5]) || digit(text[end - 5]));
        const bool tail = (upper(descriptor[2]) || digit(descriptor[2])) && digit(descriptor[3]);
        if (separated && upper(descriptor[0]) && upper(descriptor[1]) && tail) {
            return true;
        }
    }
    return false;
}

QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    if (frame.kind == GPSFrameKind::ASCIILine) {
        return text.starts_with("$R:") || text.starts_with("$R?") ? QLatin1StringView("Septentrio command replies")
                                                                  : QLatin1StringView();
    }
    // A candidate the framer truncated at its capture limit cannot be checked, so only short blocks count. The CRC
    // covers everything from the id field to the end of the block.
    constexpr size_t CRC_START = 4;
    if (frame.kind != GPSFrameKind::SBF || frame.bytes.size() < Wire::SIZE<SBF::BlockHeader>) {
        return {};
    }
    const auto header = Wire::decode<SBF::BlockHeader>(frame.bytes);
    if (header.length < Wire::SIZE<SBF::BlockHeader> || header.length > frame.bytes.size() || header.length % 4 != 0 ||
        header.crc != QGC::crc16Ccitt(frame.bytes.subspan(CRC_START, header.length - CRC_START))) {
        return {};
    }
    return QLatin1StringView("SBF blocks");
}

/// The prompt SBF::configureBase() reads the connection descriptor from, without forcing command input first.
GPSTask<bool> probe(GPSCommandChannel& channel, GPSFamilyProtocol&)
{
    const auto scope = channel.deadlineScope(SBF::Plan::PROMPT_TIMEOUT);
    const QByteArrayView prompt = SBF::Plan::PROMPT;
    channel.beginCommand({std::string(prompt.data(), static_cast<size_t>(prompt.size())), SBF::Plan::PROMPT_TIMEOUT});
    if (!co_await channel.write(prompt)) {
        channel.finishCommand(channel.failed() ? channel.failureOutcome() : GPSCommandOutcome::TransportError);
        co_return false;
    }
    QByteArray received;
    std::array<uint8_t, GPSCommandChannel::READ_CHUNK_SIZE> chunk{};
    bool answered = false;
    const uint64_t deadline = channel.commandDeadline().untilUs;
    while (!answered && channel.nowUs() < deadline) {
        const int count = co_await channel.read(chunk, channel.remainingUntil(deadline));
        if (count < 0) {
            break;
        }
        received.append(reinterpret_cast<const char*>(chunk.data()), count);
        answered = prompted(received);
        if (received.size() > 2 * static_cast<qsizetype>(chunk.size())) {
            received = received.last(8);
        }
    }
    channel.finishCommand(answered           ? GPSCommandOutcome::Acknowledged
                          : channel.failed() ? channel.failureOutcome()
                                             : GPSCommandOutcome::TimedOut);
    co_return answered;
}

}  // namespace

namespace SBF {

const GPSReceiverFamily FAMILY{
    .type = GPSType::septentrio,
    .name = QLatin1StringView("Septentrio"),
    .logCategory = &SBFProtocolLog,
    .stream = {.framers = GPSFrameKind::SBF | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::SBF},
    .baudCandidates = BAUD_RATES,
    .create = &gpsCreateProtocol<SBFProtocol>,
    .signature = &signature,
    .probe = &probe,
};

}  // namespace SBF
