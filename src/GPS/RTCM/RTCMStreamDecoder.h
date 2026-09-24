#pragma once

#include "RTCMFramer.h"

/// Owns a binary candidate and its recovery suffix until all completed frames are delivered.
class RTCMStreamDecoder
{
public:
    void reset()
    {
        _framer.reset();
        _pending = false;
    }

    bool ownsByte(uint8_t byte) const { return _pending || _framer.hasPartialFrame() || byte == RTCMFramer::PREAMBLE; }

    void addByte(uint8_t byte) { _pending = _framer.addByte(byte); }

    /// Returning false from publish defers delivery until the caller has space in its next batch.
    template <typename Publish>
    void drain(Publish publish)
    {
        while (_pending) {
            if (_framer.valid() && !publish(_framer.frame())) {
                return;
            }
            _pending = _framer.nextFrame();
        }
    }

private:
    RTCMFramer _framer;
    bool _pending = false;
};
