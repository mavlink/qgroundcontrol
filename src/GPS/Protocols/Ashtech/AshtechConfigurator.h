#pragma once

#include <string_view>

#include "GPSCommandTransaction.h"
#include "GPSReceiverFamily.h"
#include "GPSTask.h"

class GPSCommandChannel;

namespace Ashtech {

class Decoder;

/// Configures an Ashtech receiver from Plan: finds its port and rate, moves the link to 115200 baud, identifies the
/// board and selects the navigation output. A configured MB-Two becomes a base station once it reports a position:
/// the streaming services start the survey-in or set the fixed position, and start RTCM output once the base
/// position is known.
class Configurator
{
public:
    explicit Configurator(Decoder& decoder)
        : _decoder(decoder)
    {}

    [[nodiscard]] GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud);

    /// Sends the base-station commands the decoder scheduled.
    GPSTask<void> serviceStreaming(GPSCommandChannel& channel);

private:
    GPSTask<bool> _queryPort(GPSCommandChannel& channel, unsigned attempts);
    GPSTask<void> _activateCorrectionOutput(GPSCommandChannel& channel);
    GPSTask<void> _activateRTCMOutput(GPSCommandChannel& channel);
    GPSCommandOutcome _portReply(std::string_view reply);
    GPSCommandOutcome _boardReply(std::string_view reply);

    Decoder& _decoder;
    /// Port the receiver reported, such as 'A'.
    char _port = 'A';
};

}  // namespace Ashtech
