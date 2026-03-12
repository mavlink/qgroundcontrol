
#pragma once

#include <cstdint>

namespace events
{
struct EventType {
    EventType() = default;

#ifdef MAVLINK_MSG_ID_EVENT
    explicit EventType(const mavlink_event_t& mavlink_event)
        : id(mavlink_event.id), log_levels(mavlink_event.log_levels) {
        static_assert(sizeof(mavlink_event.arguments) <= sizeof(arguments), "Message too small");
        memcpy(arguments, mavlink_event.arguments, sizeof(mavlink_event.arguments));
    }
#endif

    uint32_t id{};
    uint8_t log_levels{};
    uint8_t arguments[40];
};

}  // namespace events
