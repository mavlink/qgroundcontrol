#pragma once

#include <array>
#include <chrono>
#include <cstddef>
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
    static constexpr size_t EPOCHS_IN_FLIGHT = 2;
    /// How far a time of week may trail the last published one and still belong to a published epoch. Receiver time
    /// advances with receipt time, and messages are reordered only among the epochs in flight, each assembled for at
    /// most MAX_AGE, so a late message trails by no more than that span. A time of week further behind, or half a week
    /// or more ahead, is a new time base, as when the receiver resolves time.
    static constexpr uint32_t REORDER_WINDOW_MS = static_cast<uint32_t>(
        EPOCHS_IN_FLIGHT * std::chrono::duration_cast<std::chrono::milliseconds>(MAX_AGE).count());

    struct Epoch
    {
        uint32_t tow = 0;
        uint64_t receipt = 0;
        GPSDecodedPosition position;
        /// NAV-PVT arrived; an epoch without it has no position to publish.
        bool hasPvt = false;
        bool highPrecision = false;
    };

    /// The epoch at @a tow, started now when new; nullptr for a time of week already published or too old. A new time
    /// base publishes the epochs in flight and forgets the last published time of week.
    Epoch* find(uint32_t tow, GPSDecodeContext& context);

    /// Publishes epochs older than MAX_AGE.
    void expire(GPSDecodeContext& context);

    /// Publishes the epoch at @a tow and the ones before it.
    void end(uint32_t tow, GPSDecodeContext& context);

private:
    void _orderOldestFirst();
    void _finish(std::optional<Epoch>& epoch, GPSDecodeContext& context);

    std::array<std::optional<Epoch>, EPOCHS_IN_FLIGHT> _epochs;
    std::optional<uint32_t> _lastPublished;
};

}  // namespace UBX
