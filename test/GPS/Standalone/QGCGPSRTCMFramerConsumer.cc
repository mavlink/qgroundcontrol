#include <algorithm>
#include <array>
#include <cstdint>

#include "RTCMFramer.h"

#if defined(QT_CORE_LIB) || defined(QT_VERSION)
#error RTCM framing must not inherit Qt dependencies.
#endif

int main()
{
    constexpr std::array<uint8_t, 8> frame{0xd3, 0x00, 0x02, 0x3e, 0xd0, 0xa4, 0xe0, 0x00};
    RTCMFramer framer;
    for (const auto byte : frame) {
        framer.addByte(byte);
    }
    if (!framer.valid() || framer.messageId() != 1005 || !std::ranges::equal(framer.frame(), frame)) {
        return 1;
    }
    if (framer.nextFrame() || framer.hasPartialFrame() || !framer.frame().empty()) {
        return 2;
    }
    framer.addByte(RTCMFramer::PREAMBLE);
    if (!framer.hasPartialFrame()) {
        return 3;
    }
    framer.reset();
    return framer.hasPartialFrame() || framer.bufferedSize() != 0 ? 4 : 0;
}
