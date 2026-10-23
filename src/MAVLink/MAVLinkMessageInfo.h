#pragma once

#include <cstdint>

#include "MAVLinkMessageType.h"

/// Message metadata (name and field layout) from the MAVLink dialect.
///
/// Use this instead of the dialect's inline mavlink_get_message_info*() helpers, which put a copy of the whole
/// metadata table into every translation unit that calls them.
namespace MAVLinkMessageInfo {

/// Returns nullptr if the dialect has no message with this id.
[[nodiscard]] const mavlink_message_info_t* byId(uint32_t msgId);

}  // namespace MAVLinkMessageInfo
