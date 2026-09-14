#include "CRC32.h"

#include <array>
#include <limits>

int main()
{
    const std::array<uint8_t, 9> input{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const auto initial = std::numeric_limits<uint32_t>::max();
    const auto bytes = std::span(input);
    const auto prefix = QGC::crc32Update(bytes.first(4), initial);
    if (prefix != 0x641c1f5cu) {
        return 1;
    }
    if ((QGC::crc32Update(bytes.subspan(4), prefix) ^ initial) != 0xcbf43926u) {
        return 2;
    }
    return QGC::crc32Update({}, prefix) == prefix ? 0 : 3;
}
