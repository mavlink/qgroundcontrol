#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace NMEA {

class LineFramer
{
public:
    struct Options
    {
        bool requireStart = true;
        bool hashStartsLine = false;
    };

    struct Update
    {
        bool started = false;
        bool endedWithCarriageReturn = false;
        std::optional<std::string_view> line;
    };

    explicit LineFramer(std::span<char> buffer)
        : LineFramer(buffer, Options{})
    {}

    LineFramer(std::span<char> buffer, Options options)
        : _buffer(buffer)
        , _options(options)
    {}

    Update addByte(uint8_t byte)
    {
        const char ch = static_cast<char>(byte);
        if (ch == '\r') {
            if (_active && !_discard) {
                _lineEnded = true;
            }
            return {};
        }

        if (ch == '\n') {
            Update update;
            if (_active && !_discard && _size > 0) {
                update.endedWithCarriageReturn = _lineEnded;
                update.line = std::string_view(_buffer.data(), _size);
            }
            reset();
            return update;
        }

        if (_startsLine(ch)) {
            _active = true;
            _discard = false;
            _lineEnded = false;
            _size = 0;
            return _append(ch, true);
        }

        if (!_active) {
            if (_options.requireStart) {
                return {};
            }
            _active = true;
        }

        if (_lineEnded || ch < ' ' || ch > '~' || _size == _buffer.size()) {
            _size = 0;
            _discard = true;
            _lineEnded = false;
            return {};
        }

        return _append(ch, false);
    }

    void reset()
    {
        _active = false;
        _discard = false;
        _lineEnded = false;
        _size = 0;
    }

private:
    bool _startsLine(char ch) const { return ch == '$' || (_options.hashStartsLine && ch == '#'); }

    Update _append(char ch, bool started)
    {
        if (_discard || _size == _buffer.size()) {
            _discard = true;
            _size = 0;
            return {.started = started, .endedWithCarriageReturn = false, .line = std::nullopt};
        }
        _buffer[_size++] = ch;
        return {.started = started, .endedWithCarriageReturn = false, .line = std::nullopt};
    }

    std::span<char> _buffer;
    Options _options;
    size_t _size = 0;
    bool _active = false;
    bool _discard = false;
    bool _lineEnded = false;
};

}  // namespace NMEA
