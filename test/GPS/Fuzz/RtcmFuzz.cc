#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "RTCMParser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > 65536) {
        return 0;
    }
    RTCMParser parser;
    for (size_t i = 0; i < size; ++i) {
        if (!parser.addByte(data[i])) {
            continue;
        }
        const auto frame = parser.currentFrame();
        if (frame.size() != parser.messageLength() + 6 || frame.size() > 1029 || frame.front() != char(0xd3)) {
            std::abort();
        }
        if (parser.validateCrc()) {
            const auto computed =
                RTCMParser::crc24q(reinterpret_cast<const uint8_t*>(frame.constData()), frame.size() - 3);
            const auto crc = (quint32(quint8(frame[frame.size() - 3])) << 16) |
                             (quint32(quint8(frame[frame.size() - 2])) << 8) | quint8(frame.back());
            if (computed != crc || parser.messageId() > 4095) {
                std::abort();
            }
        }
        parser.reset();
    }
    return 0;
}
