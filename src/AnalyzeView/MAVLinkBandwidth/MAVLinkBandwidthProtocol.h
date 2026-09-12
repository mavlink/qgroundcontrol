#pragma once

#include <QtCore/QByteArray>

#include <array>
#include <cstddef>
#include <cstdint>

namespace MAVLinkBandwidth {

static constexpr uint16_t kTunnelPayloadType = 0;
static constexpr uint8_t kProtocolVersion = 1;
static constexpr std::size_t kPayloadSize = 128;
static constexpr std::size_t kDataOffset = 38;
static constexpr std::size_t kDataCapacity = kPayloadSize - kDataOffset;

enum class OpCode : uint8_t
{
    Hello = 1,
    HelloAck,
    Start,
    Data,
    Report,
    Stop,
    Abort,
};

enum class Direction : uint8_t
{
    QgcToVehicle = 1,
    VehicleToQgc,
};

struct Packet
{
    OpCode opCode = OpCode::Hello;
    Direction direction = Direction::QgcToVehicle;
    uint8_t flags = 0;
    uint32_t sessionId = 0;
    uint32_t sequence = 0;
    std::array<uint32_t, 5> values{};
    QByteArray data;
};

using Payload = std::array<uint8_t, kPayloadSize>;

[[nodiscard]] Payload encode(const Packet& packet);
[[nodiscard]] bool decode(const uint8_t* payload, std::size_t payloadLength, Packet& packet);

}  // namespace MAVLinkBandwidth
