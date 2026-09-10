#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "RTCMFrameDecoder.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > 65536) {
        return 0;
    }
    RTCMFrameDecoder parser;
    for (size_t i = 0; i < size; ++i) {
        const auto decoded = parser.addByte(data[i], 1000);
        if (!decoded) {
            continue;
        }
        const auto frame = decoded->data;
        if (frame.size() > 1029 || frame.front() != char(0xd3)) {
            std::abort();
        }
        if (decoded->valid) {
            const auto computed = RTCMFramer::crc24q(
                {reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(frame.size() - 3)});
            const auto crc = (quint32(quint8(frame[frame.size() - 3])) << 16) |
                             (quint32(quint8(frame[frame.size() - 2])) << 8) | quint8(frame.back());
            if (computed != crc || decoded->messageId > 4095) {
                std::abort();
            }
        }
        parser.reset();
    }
    return 0;
}
