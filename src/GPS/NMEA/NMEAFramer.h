#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "NMEAFields.h"

namespace NMEA {

/// Completes at the second uppercase checksum digit, without requiring a line ending.
/// The caller owns the buffer; a completed frame remains there until the next frame starts.
class Framer
{
public:
    explicit Framer(std::span<uint8_t> buffer)
        : _buffer(buffer)
    {}

    Framer(const Framer&) = delete;
    Framer& operator=(const Framer&) = delete;
    Framer(Framer&&) = delete;
    Framer& operator=(Framer&&) = delete;

    size_t addByte(uint8_t byte)
    {
        if (_buffer.size() < 6) {
            return 0;
        }
        switch (_state) {
            case State::Waiting:
                if (byte == '$') {
                    _size = 1;
                    _buffer[0] = byte;
                    _checksum = 0;
                    _state = State::Body;
                }
                break;
            case State::Body:
                if (byte == '$') {
                    _size = 0;
                    _checksum = 0;
                } else if (byte == '*') {
                    _state = State::ChecksumHigh;
                } else {
                    _checksum ^= byte;
                }
                // Preserve the receiver framers' original payload limit and checksum headroom.
                if (_size >= _buffer.size() - 5) {
                    reset();
                } else {
                    _buffer[_size++] = byte;
                }
                break;
            case State::ChecksumHigh:
                _buffer[_size++] = byte;
                _state = State::ChecksumLow;
                break;
            case State::ChecksumLow: {
                _buffer[_size++] = byte;
                const size_t length = _buffer[_size - 2] == NMEAFields::hexDigit(_checksum >> 4) &&
                                              _buffer[_size - 1] == NMEAFields::hexDigit(_checksum)
                                          ? _size
                                          : 0;
                reset();
                return length;
            }
        }
        return 0;
    }

    void reset()
    {
        _state = State::Waiting;
        _size = 0;
        _checksum = 0;
    }

private:
    enum class State
    {
        Waiting,
        Body,
        ChecksumHigh,
        ChecksumLow
    };
    std::span<uint8_t> _buffer;
    size_t _size = 0;
    uint8_t _checksum = 0;
    State _state = State::Waiting;
};

}  // namespace NMEA
