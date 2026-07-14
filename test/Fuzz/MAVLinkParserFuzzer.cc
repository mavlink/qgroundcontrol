#include <cstddef>
#include <cstdint>
#include <mavlink.h>

#ifdef QGC_MAVLINK_SEED_GENERATOR

#include <fstream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 1;
    }

    mavlink_message_t message = {};
    mavlink_msg_heartbeat_pack(1, 1, &message, MAV_TYPE_GENERIC, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_UNINIT);

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
    const uint16_t length = mavlink_msg_to_send_buffer(buffer, &message);
    std::ofstream output(argv[1], std::ios::binary);
    output.write(reinterpret_cast<const char*>(buffer), length);
    return output ? 0 : 1;
}

#else

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    mavlink_reset_channel_status(MAVLINK_COMM_0);

    mavlink_message_t message = {};
    mavlink_status_t status = {};
    for (size_t index = 0; index < size; ++index) {
        if (mavlink_parse_char(MAVLINK_COMM_0, data[index], &message, &status) != MAVLINK_FRAMING_INCOMPLETE) {
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
            mavlink_msg_to_send_buffer(buffer, &message);
        }
    }

    mavlink_reset_channel_status(MAVLINK_COMM_0);
    return 0;
}

#endif
