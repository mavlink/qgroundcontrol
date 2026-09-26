#pragma once

#include "GPSBaseStationConfig.h"
#include "GPSReceiverFamily.h"
#include "GPSTask.h"

class GPSCommandChannel;

namespace Femto {

class Decoder;

/// Configures a Femtomes receiver from Plan at 115200 baud: stops its logs, identifies it and sets up the base
/// station. A survey-in starts receiver position averaging; RTCM output follows once the decoder sees it finish, or
/// at once for a fixed base.
class Configurator
{
public:
    explicit Configurator(Decoder& decoder)
        : _decoder(decoder)
    {}

    [[nodiscard]] GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud);

    /// Starts RTCM output once the decoder saw position averaging finish.
    GPSTask<void> serviceStreaming(GPSCommandChannel& channel);

private:
    GPSTask<void> _activateCorrectionOutput(GPSCommandChannel& channel);
    GPSTask<void> _activateRTCMOutput(GPSCommandChannel& channel);

    Decoder& _decoder;
    GPSBaseStationConfig _base{};
};

}  // namespace Femto
