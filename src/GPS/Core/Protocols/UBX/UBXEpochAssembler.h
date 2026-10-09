#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

#include "GPSProtocolEvent.h"

class GPSDecodeContext;

namespace UBX {

/// Two receiver epochs tolerate reordered navigation messages without mixing their fields. Each completed epoch with a
/// NAV-PVT is published as a position to the context it is given.
class EpochAssembler
{
public:
    static constexpr std::chrono::microseconds MAX_AGE = std::chrono::milliseconds(200);

    struct Epoch
    {
        uint32_t tow = 0;
        uint64_t receipt = 0;
        GPSDecodedPosition position;
        /// NAV-PVT arrived; an epoch without it has no position to publish.
        bool hasPvt = false;
        bool highPrecision = false;
    };

    /// The epoch at @a tow, started now when new; nullptr for a time of week already published or too old.
    Epoch* find(uint32_t tow, GPSDecodeContext& context);

    /// Publishes epochs older than MAX_AGE.
    void expire(GPSDecodeContext& context);

    /// Publishes the epoch at @a tow and the ones before it.
    void end(uint32_t tow, GPSDecodeContext& context);

private:
    void _orderOldestFirst();
    void _finish(std::optional<Epoch>& epoch, GPSDecodeContext& context);

    std::array<std::optional<Epoch>, 2> _epochs;
    std::optional<uint32_t> _lastPublished;
};

}  // namespace UBX
