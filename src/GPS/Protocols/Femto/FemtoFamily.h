#pragma once

#include "Femto/FemtoConfigurator.h"
#include "Femto/FemtoDecoder.h"
#include "GPSFamilyProtocol.h"
#include "GPSReceiverFamily.h"

/// Femtomes receivers: ASCII commands, with GGA sentences and RTCM3 output, set up as RTK base stations.
namespace Femto {

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

    bool receiverReady(const GPSDecodeContext&) const override { return _decoder.session().configured; }

    GPSTask<void> serviceStreaming(GPSCommandChannel& channel) override
    {
        return _configurator.serviceStreaming(channel);
    }

    [[nodiscard]] Decoder& decoder() { return _decoder; }

private:
    Decoder _decoder;
    Configurator _configurator{_decoder};
};

}  // namespace Femto
