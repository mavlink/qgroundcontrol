#include "UBX/UBXFamily.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>

#include <QtCore/QByteArrayView>

#include "Checksums.h"
#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "UBX/Generated/UBXMessageIds.h"
#include "UBX/UBXConfigurator.h"
#include "UBX/UBXDecoder.h"
#include "UBX/UBXPlan.h"

QGC_LOGGING_CATEGORY(UBXProtocolLog, "GPS.Driver.Protocols.UBX")

namespace {

class UBXFamilyProtocol final : public GPSFamilyProtocol
{
public:
    explicit UBXFamilyProtocol(const GPSFamilyOptions& options)
        : _decoder(options.satelliteInfoEnabled)
        , _configurator(_decoder)
    {}

    GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override
    {
        return _configurator.configure(channel, config, baud);
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _decoder.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _decoder.flush(context); }

    bool receiverReady(const GPSDecodeContext&) const override { return _decoder.state().configured; }

    QString identity() const override
    {
        const auto& identity = _decoder.state().identity;
        const char* separator = identity.model.isEmpty() || identity.firmware.isEmpty() ? "" : " ";
        return QString::fromUtf8(identity.model + separator + identity.firmware);
    }

    bool completeReceiveCycle(GPSReceiveUpdates handled) override { return _decoder.completeReceiveCycle(handled); }

    std::chrono::milliseconds nextReadSlice(std::chrono::milliseconds timeout) const override
    {
        return _decoder.nextReadSlice(timeout);
    }

    GPSTask<void> serviceStreaming(GPSCommandChannel& channel) override
    {
        return _configurator.serviceStreaming(channel);
    }

    UBXDecoder& decoder() { return _decoder; }

private:
    UBXDecoder _decoder;
    UBXConfigurator _configurator;
};

constexpr std::array<uint8_t, 8> monVerPoll()
{
    std::array<uint8_t, 8> frame{0xb5, 0x62, UBX::Msg::MON_VER.cls, UBX::Msg::MON_VER.id, 0, 0, 0, 0};
    const auto checksum = QGC::fletcher8(std::span<const uint8_t>(frame).subspan(2, 4));
    frame[6] = checksum.a;
    frame[7] = checksum.b;
    return frame;
}

constexpr std::array<uint8_t, 8> MON_VER_POLL = monVerPoll();

QLatin1StringView signature(const GPSFrame& frame)
{
    if (frame.kind == GPSFrameKind::UBX) {
        return QLatin1StringView("UBX frames");
    }
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine || !text.starts_with("$PUBX,")) {
        return {};
    }
    const auto sentence = NMEA::frame(text);
    return sentence && sentence->hasValidChecksum() ? QLatin1StringView("$PUBX sentences") : QLatin1StringView();
}

/// The MON-VER poll UBXConfigurator identifies receivers with, under the same label and timeout.
GPSTask<bool> probe(GPSCommandChannel& channel, GPSFamilyProtocol&)
{
    const auto result = co_await channel.transact(
        {std::to_string(UBX::Msg::MON_VER.value()), UBX::Plan::IDENTITY_TIMEOUT},
        QByteArrayView(MON_VER_POLL.data(), static_cast<qsizetype>(MON_VER_POLL.size())),
        GPSFrameMatcher([](const GPSFrame& frame) {
            return frame.kind == GPSFrameKind::UBX && frame.messageId == UBX::Msg::MON_VER.value()
                       ? GPSCommandOutcome::Acknowledged
                       : GPSCommandOutcome::Pending;
        }));
    co_return result.succeeded();
}

}  // namespace

namespace UBX {

const GPSReceiverFamily FAMILY{
    .type = GPSType::ublox,
    .name = QLatin1StringView("u-blox"),
    .logCategory = &UBXProtocolLog,
    .stream = {.framers = GPSFrameKind::UBX | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::UBX},
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<UBXFamilyProtocol>,
    .signature = &signature,
    .probe = &probe,
};

UBXDecoder& decoder(GPSFamilyProtocol& protocol)
{
    return static_cast<UBXFamilyProtocol&>(protocol).decoder();
}

}  // namespace UBX
