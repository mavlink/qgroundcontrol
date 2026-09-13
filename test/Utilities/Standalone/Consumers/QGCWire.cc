#include <array>

#include "CRC32.h"
#include "LittleEndian.h"

int main()
{
    std::array<uint8_t, 4> bytes{};
    if (!LittleEndian::write<uint32_t>(bytes, 0, 0x34333231))
        return 1;
    if (LittleEndian::read<uint32_t>(bytes).value_or(0) != 0x34333231)
        return 2;
    return QGC::crc32Update(bytes, 0xffffffffu) == 0x641c1f5cu ? 0 : 3;
}
