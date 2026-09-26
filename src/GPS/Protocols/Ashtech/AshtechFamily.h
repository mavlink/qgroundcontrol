#pragma once

#include <chrono>

#include "Ashtech/AshtechConfigurator.h"
#include "Ashtech/AshtechDecoder.h"
#include "GPSFamilyProtocol.h"
#include "GPSReceiverFamily.h"

/// Ashtech/Trimble receivers: proprietary $PASH commands and $PASHR,POS positions over the shared NMEA and RTCM3
/// stream. An MB-Two is set up as an RTK base station.
namespace Ashtech {

extern const GPSReceiverFamily FAMILY;

/// The family protocol: the decoder, and the configurator that drives the receiver.
class Protocol final : public GPSFamilyProtocol
{
public:
    explicit Protocol(const GPSFamilyOptions& options)
        : _decoder(options.satelliteInfoEnabled)
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

    bool receiverReady(const GPSDecodeContext&) const override { return _decoder.session().configured; }

    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        return _decoder.nmea().receive(channel, timeout);
    }

    GPSTask<void> serviceStreaming(GPSCommandChannel& channel) override
    {
        return _configurator.serviceStreaming(channel);
    }

    [[nodiscard]] Decoder& decoder() { return _decoder; }

private:
    Decoder _decoder;
    Configurator _configurator{_decoder};
};

}  // namespace Ashtech
