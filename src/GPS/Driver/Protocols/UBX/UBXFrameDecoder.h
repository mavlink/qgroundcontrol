#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace UBX {
struct Frame
{
    uint16_t message = 0;
    uint16_t length = 0;
    std::array<uint8_t, 4096> payload{};
};

/// A native frame owns its bytes until both checksum bytes are consumed.
class FrameDecoder
{
public:
    bool idle() const { return _state == State::Sync1; }

    void reset()
    {
        _state = State::Sync1;
        _checksumA = _checksumB = 0;
        _index = 0;
    }

    std::optional<Frame> consume(uint8_t byte)
    {
        switch (_state) {
            case State::Sync1:
                if (byte == 0xb5)
                    _state = State::Sync2;
                break;
            case State::Sync2:
                if (byte == 0x62)
                    _state = State::Class;
                else
                    reset();
                break;
            case State::Class:
                checksum(byte);
                _frame.message = byte;
                _state = State::Id;
                break;
            case State::Id:
                checksum(byte);
                _frame.message |= uint16_t(byte) << 8;
                _state = State::Length1;
                break;
            case State::Length1:
                checksum(byte);
                _frame.length = byte;
                _state = State::Length2;
                break;
            case State::Length2:
                checksum(byte);
                _frame.length |= uint16_t(byte) << 8;
                _index = 0;
                _state = _frame.length ? State::Payload : State::Checksum1;
                break;
            case State::Payload:
                checksum(byte);
                if (_index < _frame.payload.size())
                    _frame.payload[_index] = byte;
                if (++_index == _frame.length)
                    _state = State::Checksum1;
                break;
            case State::Checksum1:
                if (_checksumA == byte)
                    _state = State::Checksum2;
                else
                    reset();
                break;
            case State::Checksum2: {
                const bool valid = _checksumB == byte && _frame.length <= _frame.payload.size();
                reset();
                if (valid)
                    return _frame;
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
        Class,
        Id,
        Length1,
        Length2,
        Payload,
        Checksum1,
        Checksum2
    };

    void checksum(uint8_t byte)
    {
        _checksumA += byte;
        _checksumB += _checksumA;
    }

    Frame _frame;
    State _state = State::Sync1;
    uint16_t _index = 0;
    uint8_t _checksumA = 0;
    uint8_t _checksumB = 0;
};
}  // namespace UBX
