// The only translation unit that enables the dialect's message-info helpers. It skips the precompiled header and
// unity builds, which would include MAVLinkLib.h before this definition.
#define MAVLINK_USE_MESSAGE_INFO

#include "MAVLinkMessageInfo.h"

#include "MAVLinkLib.h"

namespace MAVLinkMessageInfo {

const mavlink_message_info_t* byId(uint32_t msgId)
{
    return mavlink_get_message_info_by_id(msgId);
}

}  // namespace MAVLinkMessageInfo
