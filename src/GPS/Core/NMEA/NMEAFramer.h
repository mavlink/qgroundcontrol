#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include <QtCore/QtGlobal>

namespace NMEA {

/// Completes at the second uppercase checksum digit, without requiring a line ending.
/// The caller owns the buffer; a completed frame remains there until the next frame starts.
class Framer
{
public:
    explicit Framer(std::span<uint8_t> buffer)
        : _buffer(buffer)
    {}

    Q_DISABLE_COPY_MOVE(Framer)

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
                const size_t length =
                    _buffer[_size - 2] == _hexDigit(_checksum >> 4) && _buffer[_size - 1] == _hexDigit(_checksum)
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
    static constexpr char _hexDigit(unsigned value) { return "0123456789ABCDEF"[value & 0xf]; }

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

/// Frames printable ASCII lines. A '$' or '#' restarts the line, so a sentence or log is not lost after garbage.
class LineFramer
{
public:
    explicit LineFramer(std::span<char> buffer)
        : _buffer(buffer)
    {}

    /// Returns a completed line without its terminator; the view is valid until the next byte.
    std::optional<std::string_view> addByte(uint8_t byte)
    {
        const char ch = static_cast<char>(byte);
        if (ch == '\r') {
            if (_active && !_discard) {
                _lineEnded = true;
            }
            return std::nullopt;
        }

        if (ch == '\n') {
            std::optional<std::string_view> line;
            if (_active && !_discard && _size > 0) {
                line = std::string_view(_buffer.data(), _size);
            }
            reset();
            return line;
        }

        if (_startsLine(ch)) {
            _active = true;
            _discard = false;
            _lineEnded = false;
            _size = 0;
            _append(ch);
            return std::nullopt;
        }

        _active = true;

        if (_lineEnded || ch < ' ' || ch > '~' || _size == _buffer.size()) {
            _size = 0;
            _discard = true;
            _lineEnded = false;
            return std::nullopt;
        }

        _append(ch);
        return std::nullopt;
    }

    void reset()
    {
        _active = false;
        _discard = false;
        _lineEnded = false;
        _size = 0;
    }

private:
    static bool _startsLine(char ch) { return ch == '$' || ch == '#'; }

    void _append(char ch)
    {
        if (_discard || _size == _buffer.size()) {
            _discard = true;
            _size = 0;
            return;
        }
        _buffer[_size++] = ch;
    }

    std::span<char> _buffer;
    size_t _size = 0;
    bool _active = false;
    bool _discard = false;
    bool _lineEnded = false;
};

}  // namespace NMEA
