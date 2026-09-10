#pragma once

#include <array>
#include <cstdint>

namespace UBX {
enum class Board : uint8_t
{
    unknown = 0,
    u_blox5 = 5,
    u_blox6 = 6,
    u_blox7 = 7,
    u_blox8 = 8,
    u_blox9 = 9,
    u_blox9_F9P_L1L2 = 10,
    u_blox10 = 11,
    u_blox9_F9P_L1L5 = 12,
    u_blox10_L1L5 = 13,
    u_blox_X20 = 14
};

struct ReceiverProfile
{
    Board board;
    bool usb;
    bool rtcmOutput;
    bool constellationSelection;
    bool baseCapabilityKnown;
};

inline constexpr std::array RECEIVER_PROFILES = {
    ReceiverProfile{Board::unknown, true, false, false, false},
    ReceiverProfile{Board::u_blox5, true, false, false, false},
    ReceiverProfile{Board::u_blox6, true, false, false, false},
    ReceiverProfile{Board::u_blox7, true, false, false, false},
    ReceiverProfile{Board::u_blox8, true, false, false, false},
    ReceiverProfile{Board::u_blox9, true, false, true, true},
    ReceiverProfile{Board::u_blox9_F9P_L1L2, true, true, true, true},
    ReceiverProfile{Board::u_blox10, false, false, false, true},
    ReceiverProfile{Board::u_blox9_F9P_L1L5, true, true, false, true},
    ReceiverProfile{Board::u_blox10_L1L5, false, false, false, true},
    ReceiverProfile{Board::u_blox_X20, true, true, false, true},
};

constexpr ReceiverProfile receiverProfile(Board board)
{
    for (auto profile : RECEIVER_PROFILES)
        if (profile.board == board)
            return profile;
    return RECEIVER_PROFILES.front();
}

struct OutputPort
{
    unsigned messageKeyOffset;
    bool requiresUsb;
};

inline constexpr std::array OUTPUT_PORTS = {OutputPort{1, false}, OutputPort{3, true}};

constexpr unsigned configurationValueBytes(uint32_t key)
{
    const unsigned type = key >> 28;
    return type == 1 || type == 2 ? 1 : type == 3 ? 2 : type == 4 ? 4 : 0;
}
}  // namespace UBX
