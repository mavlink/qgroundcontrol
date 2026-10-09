// Message classes and ids, and the frame layout, as the u-blox interface descriptions define them.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "Checksums.h"

namespace UBX {

/// A message class and id. value() is the two header bytes as one little-endian word, as FrameDecoder
/// reports a frame's message.
struct MessageId
{
    uint8_t cls;
    uint8_t id;

    [[nodiscard]] constexpr uint16_t value() const { return static_cast<uint16_t>(cls | (id << 8)); }
};

}  // namespace UBX

namespace UBX::MsgClass {
inline constexpr uint8_t NAV = 0x01;
inline constexpr uint8_t RXM = 0x02;
inline constexpr uint8_t INF = 0x04;
inline constexpr uint8_t ACK = 0x05;
inline constexpr uint8_t CFG = 0x06;
inline constexpr uint8_t MON = 0x0a;
inline constexpr uint8_t SEC = 0x27;
inline constexpr uint8_t RTCM3 = 0xf5;
}  // namespace UBX::MsgClass

namespace UBX::Msg {
inline constexpr MessageId NAV_STATUS{MsgClass::NAV, 0x03};
inline constexpr MessageId NAV_DOP{MsgClass::NAV, 0x04};
inline constexpr MessageId NAV_PVT{MsgClass::NAV, 0x07};
inline constexpr MessageId NAV_HPPOSLLH{MsgClass::NAV, 0x14};
inline constexpr MessageId NAV_TIMEGPS{MsgClass::NAV, 0x20};
inline constexpr MessageId NAV_SVINFO{MsgClass::NAV, 0x30};
inline constexpr MessageId NAV_SAT{MsgClass::NAV, 0x35};
inline constexpr MessageId NAV_SVIN{MsgClass::NAV, 0x3b};
inline constexpr MessageId NAV_EOE{MsgClass::NAV, 0x61};
inline constexpr MessageId RXM_SFRBX{MsgClass::RXM, 0x13};
inline constexpr MessageId RXM_RAWX{MsgClass::RXM, 0x15};
inline constexpr MessageId INF_ERROR{MsgClass::INF, 0x00};
inline constexpr MessageId INF_WARNING{MsgClass::INF, 0x01};
inline constexpr MessageId INF_NOTICE{MsgClass::INF, 0x02};
inline constexpr MessageId INF_DEBUG{MsgClass::INF, 0x04};
inline constexpr MessageId ACK_NAK{MsgClass::ACK, 0x00};
inline constexpr MessageId ACK_ACK{MsgClass::ACK, 0x01};
inline constexpr MessageId CFG_PRT{MsgClass::CFG, 0x00};
inline constexpr MessageId CFG_MSG{MsgClass::CFG, 0x01};
inline constexpr MessageId CFG_RATE{MsgClass::CFG, 0x08};
inline constexpr MessageId CFG_NAV5{MsgClass::CFG, 0x24};
inline constexpr MessageId CFG_TMODE3{MsgClass::CFG, 0x71};
inline constexpr MessageId CFG_VALSET{MsgClass::CFG, 0x8a};
inline constexpr MessageId CFG_VALGET{MsgClass::CFG, 0x8b};
inline constexpr MessageId MON_VER{MsgClass::MON, 0x04};
inline constexpr MessageId MON_HW{MsgClass::MON, 0x09};
inline constexpr MessageId MON_RF{MsgClass::MON, 0x38};
inline constexpr MessageId SEC_SIG{MsgClass::SEC, 0x09};
inline constexpr MessageId RTCM3_1005{MsgClass::RTCM3, 0x05};
inline constexpr MessageId RTCM3_1074{MsgClass::RTCM3, 0x4a};
inline constexpr MessageId RTCM3_1077{MsgClass::RTCM3, 0x4d};
inline constexpr MessageId RTCM3_1084{MsgClass::RTCM3, 0x54};
inline constexpr MessageId RTCM3_1087{MsgClass::RTCM3, 0x57};
inline constexpr MessageId RTCM3_1094{MsgClass::RTCM3, 0x5e};
inline constexpr MessageId RTCM3_1097{MsgClass::RTCM3, 0x61};
inline constexpr MessageId RTCM3_1124{MsgClass::RTCM3, 0x7c};
inline constexpr MessageId RTCM3_1127{MsgClass::RTCM3, 0x7f};
inline constexpr MessageId RTCM3_1230{MsgClass::RTCM3, 0xe6};
}  // namespace UBX::Msg

namespace UBX {

/// Readable names of the messages above, as the interface description writes them.
inline constexpr std::pair<MessageId, std::string_view> MESSAGE_NAMES[] = {
    {Msg::NAV_STATUS, "UBX-NAV-STATUS"},     {Msg::NAV_DOP, "UBX-NAV-DOP"},         {Msg::NAV_PVT, "UBX-NAV-PVT"},
    {Msg::NAV_HPPOSLLH, "UBX-NAV-HPPOSLLH"}, {Msg::NAV_TIMEGPS, "UBX-NAV-TIMEGPS"}, {Msg::NAV_SVINFO, "UBX-NAV-SVINFO"},
    {Msg::NAV_SAT, "UBX-NAV-SAT"},           {Msg::NAV_SVIN, "UBX-NAV-SVIN"},       {Msg::NAV_EOE, "UBX-NAV-EOE"},
    {Msg::RXM_SFRBX, "UBX-RXM-SFRBX"},       {Msg::RXM_RAWX, "UBX-RXM-RAWX"},       {Msg::INF_ERROR, "UBX-INF-ERROR"},
    {Msg::INF_WARNING, "UBX-INF-WARNING"},   {Msg::INF_NOTICE, "UBX-INF-NOTICE"},   {Msg::INF_DEBUG, "UBX-INF-DEBUG"},
    {Msg::ACK_NAK, "UBX-ACK-NAK"},           {Msg::ACK_ACK, "UBX-ACK-ACK"},         {Msg::CFG_PRT, "UBX-CFG-PRT"},
    {Msg::CFG_MSG, "UBX-CFG-MSG"},           {Msg::CFG_RATE, "UBX-CFG-RATE"},       {Msg::CFG_NAV5, "UBX-CFG-NAV5"},
    {Msg::CFG_TMODE3, "UBX-CFG-TMODE3"},     {Msg::CFG_VALSET, "UBX-CFG-VALSET"},   {Msg::CFG_VALGET, "UBX-CFG-VALGET"},
    {Msg::MON_VER, "UBX-MON-VER"},           {Msg::MON_HW, "UBX-MON-HW"},           {Msg::MON_RF, "UBX-MON-RF"},
    {Msg::SEC_SIG, "UBX-SEC-SIG"},           {Msg::RTCM3_1005, "RTCM3-1005"},       {Msg::RTCM3_1074, "RTCM3-1074"},
    {Msg::RTCM3_1077, "RTCM3-1077"},         {Msg::RTCM3_1084, "RTCM3-1084"},       {Msg::RTCM3_1087, "RTCM3-1087"},
    {Msg::RTCM3_1094, "RTCM3-1094"},         {Msg::RTCM3_1097, "RTCM3-1097"},       {Msg::RTCM3_1124, "RTCM3-1124"},
    {Msg::RTCM3_1127, "RTCM3-1127"},         {Msg::RTCM3_1230, "RTCM3-1230"},
};

/// The name of @a message, such as "UBX-CFG-VALSET", or its class and id in hex when it has none.
[[nodiscard]] inline QByteArray messageName(MessageId message)
{
    for (const auto& [id, name] : MESSAGE_NAMES) {
        if (id.value() == message.value()) {
            return QByteArrayView(name).toByteArray();
        }
    }
    return QByteArrayLiteral("UBX-0x") + QByteArray::number(message.cls, 16) + "-0x" +
           QByteArray::number(message.id, 16);
}

}  // namespace UBX

namespace UBX {
inline constexpr uint8_t SYNC1 = 0xb5;
inline constexpr uint8_t SYNC2 = 0x62;
inline constexpr size_t HEADER_SIZE = 6;
/// Header and checksum.
inline constexpr size_t FRAME_OVERHEAD = HEADER_SIZE + 2;
/// The longest payload decoded.
inline constexpr size_t MAX_PAYLOAD_SIZE = 4096;
/// A longer payload, up to this size, is read to its end and dropped, so the stream stays aligned on the next frame.
/// It covers every documented message, the largest being RXM-RAWX at 8176 bytes; a longer claim is taken as noise.
inline constexpr size_t MAX_SKIPPED_PAYLOAD_SIZE = 8192;

/// Writes the frame of @a payload to the front of @a out. @return the frame, or empty when it does not fit.
[[nodiscard]] constexpr std::span<const uint8_t> encodeFrame(MessageId message, std::span<const uint8_t> payload,
                                                             std::span<uint8_t> out)
{
    const size_t size = payload.size() + FRAME_OVERHEAD;
    if (payload.size() > MAX_PAYLOAD_SIZE || size > out.size()) {
        return {};
    }
    out[0] = SYNC1;
    out[1] = SYNC2;
    out[2] = message.cls;
    out[3] = message.id;
    out[4] = static_cast<uint8_t>(payload.size());
    out[5] = static_cast<uint8_t>(payload.size() >> 8);
    std::copy(payload.begin(), payload.end(), out.begin() + HEADER_SIZE);
    const auto checksum = QGC::fletcher8(out.subspan(2, size - 4));
    out[size - 2] = checksum.a;
    out[size - 1] = checksum.b;
    return out.first(size);
}

/// A received frame. Its views borrow the decoder's storage until the next consume().
struct Frame
{
    uint16_t message = 0;
    /// From the sync bytes to the checksum.
    std::span<const uint8_t> bytes;
    std::span<const uint8_t> payload;
};

class FrameDecoder
{
public:
    [[nodiscard]] bool idle() const { return _state == State::Sync1; }

    void reset()
    {
        _state = State::Sync1;
        _checksum = {};
    }

    [[nodiscard]] std::optional<Frame> consume(uint8_t byte)
    {
        switch (_state) {
            case State::Sync1:
                if (byte == SYNC1) {
                    _state = State::Sync2;
                }
                break;
            case State::Sync2:
                if (byte == SYNC2) {
                    _size = 2;
                    _state = State::Header;
                } else if (byte != SYNC1) {
                    reset();
                }
                break;
            case State::Header:
                _add(byte);
                if (_size == HEADER_SIZE) {
                    _length = static_cast<uint16_t>(_bytes[4] | (_bytes[5] << 8));
                    _payloadRead = 0;
                    if (_length > MAX_SKIPPED_PAYLOAD_SIZE) {
                        reset();
                    } else {
                        _state = _length ? State::Payload : State::Checksum1;
                    }
                }
                break;
            case State::Payload:
                if (_length <= MAX_PAYLOAD_SIZE) {
                    _add(byte);
                } else {
                    _checksum = QGC::fletcher8({&byte, 1}, _checksum);
                }
                if (++_payloadRead == _length) {
                    _state = State::Checksum1;
                }
                break;
            case State::Checksum1:
                if (byte == _checksum.a) {
                    _bytes[_size++] = byte;
                    _state = State::Checksum2;
                } else {
                    reset();
                }
                break;
            case State::Checksum2: {
                const bool valid = byte == _checksum.b && _length <= MAX_PAYLOAD_SIZE;
                reset();
                if (valid) {
                    _bytes[_size++] = byte;
                    const std::span<const uint8_t> bytes(_bytes.data(), _size);
                    return Frame{static_cast<uint16_t>(_bytes[2] | (_bytes[3] << 8)), bytes,
                                 bytes.subspan(HEADER_SIZE, _length)};
                }
                break;
            }
        }
        return std::nullopt;
    }

private:
    enum class State
    {
        Sync1,
        Sync2,
        Header,
        Payload,
        Checksum1,
        Checksum2
    };

    void _add(uint8_t byte)
    {
        _checksum = QGC::fletcher8({&byte, 1}, _checksum);
        _bytes[_size++] = byte;
    }

    std::array<uint8_t, MAX_PAYLOAD_SIZE + FRAME_OVERHEAD> _bytes{SYNC1, SYNC2};
    size_t _size = 0;
    uint16_t _length = 0;
    uint16_t _payloadRead = 0;
    State _state = State::Sync1;
    QGC::Fletcher8 _checksum;
};
}  // namespace UBX
