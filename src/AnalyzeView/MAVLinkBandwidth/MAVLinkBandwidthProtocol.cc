#include "MAVLinkBandwidthProtocol.h"

#include <QtCore/QtEndian>

#include <algorithm>

namespace MAVLinkBandwidth {
namespace {

static constexpr std::array<uint8_t, 4> kMagic{'Q', 'G', 'B', 'W'};
static constexpr std::size_t kVersionOffset = 4;
static constexpr std::size_t kOpCodeOffset = 5;
static constexpr std::size_t kDirectionOffset = 6;
static constexpr std::size_t kFlagsOffset = 7;
static constexpr std::size_t kSessionOffset = 8;
static constexpr std::size_t kSequenceOffset = 12;
static constexpr std::size_t kValuesOffset = 16;
static constexpr std::size_t kDataLengthOffset = 36;
static constexpr uint8_t kPaddingByte = 0xA5;

void writeUint16(Payload& payload, std::size_t offset, uint16_t value)
{
    qToLittleEndian(value, payload.data() + offset);
}

void writeUint32(Payload& payload, std::size_t offset, uint32_t value)
{
    qToLittleEndian(value, payload.data() + offset);
}

uint16_t readUint16(const uint8_t* payload, std::size_t offset)
{
    return qFromLittleEndian<uint16_t>(payload + offset);
}

uint32_t readUint32(const uint8_t* payload, std::size_t offset)
{
    return qFromLittleEndian<uint32_t>(payload + offset);
}

bool validOpCode(uint8_t opCode)
{
    return (opCode >= static_cast<uint8_t>(OpCode::Hello)) && (opCode <= static_cast<uint8_t>(OpCode::Abort));
}

bool validDirection(uint8_t direction)
{
    return (direction == static_cast<uint8_t>(Direction::QgcToVehicle)) ||
           (direction == static_cast<uint8_t>(Direction::VehicleToQgc));
}

}  // namespace

Payload encode(const Packet& packet)
{
    Payload payload{};
    payload.fill(kPaddingByte);

    std::copy(kMagic.cbegin(), kMagic.cend(), payload.begin());
    payload[kVersionOffset] = kProtocolVersion;
    payload[kOpCodeOffset] = static_cast<uint8_t>(packet.opCode);
    payload[kDirectionOffset] = static_cast<uint8_t>(packet.direction);
    payload[kFlagsOffset] = packet.flags;
    writeUint32(payload, kSessionOffset, packet.sessionId);
    writeUint32(payload, kSequenceOffset, packet.sequence);

    for (std::size_t index = 0; index < packet.values.size(); ++index) {
        writeUint32(payload, kValuesOffset + (index * sizeof(uint32_t)), packet.values[index]);
    }

    const auto dataLength = static_cast<uint16_t>(std::min<std::size_t>(packet.data.size(), kDataCapacity));
    writeUint16(payload, kDataLengthOffset, dataLength);
    if (dataLength > 0) {
        std::copy_n(reinterpret_cast<const uint8_t*>(packet.data.constData()), dataLength,
                    payload.begin() + kDataOffset);
    }

    return payload;
}

bool decode(const uint8_t* payload, std::size_t payloadLength, Packet& packet)
{
    if (!payload || (payloadLength < kPayloadSize) || !std::equal(kMagic.cbegin(), kMagic.cend(), payload) ||
        (payload[kVersionOffset] != kProtocolVersion) || !validOpCode(payload[kOpCodeOffset]) ||
        !validDirection(payload[kDirectionOffset])) {
        return false;
    }

    const uint16_t dataLength = readUint16(payload, kDataLengthOffset);
    if (dataLength > kDataCapacity) {
        return false;
    }

    packet.opCode = static_cast<OpCode>(payload[kOpCodeOffset]);
    packet.direction = static_cast<Direction>(payload[kDirectionOffset]);
    packet.flags = payload[kFlagsOffset];
    packet.sessionId = readUint32(payload, kSessionOffset);
    packet.sequence = readUint32(payload, kSequenceOffset);
    for (std::size_t index = 0; index < packet.values.size(); ++index) {
        packet.values[index] = readUint32(payload, kValuesOffset + (index * sizeof(uint32_t)));
    }
    packet.data = QByteArray(reinterpret_cast<const char*>(payload + kDataOffset), dataLength);

    return true;
}

}  // namespace MAVLinkBandwidth
