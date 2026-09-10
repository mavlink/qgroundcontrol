#include <cstdlib>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    gps_test_warnings.clear();
    GPSPositionReport position{};
    GPSSatelliteReport satellites{};
    GPSProtocolIO io;
    io.read = [](auto, auto) -> GPSProtocolReadResult { std::abort(); };
    io.write = [](auto, auto) -> GPSProtocolWriteResult { std::abort(); };
    io.setBaudrate = [](auto) -> GPSBaudStatus { std::abort(); };
    io.nowUs = [] { return uint64_t{1000000}; };
    GPSDriverUBX ubx(io, &position, &satellites, {});
    GPSDriverAshtech ashtech(io, &position, &satellites);
    GPSDriverSBF sbf(io, &position, &satellites);
    GPSDriverFemto femto(io, &position, &satellites);
    for (GPSProtocol* protocol : std::array<GPSProtocol*, 4>{&ubx, &ashtech, &sbf, &femto}) {
        const size_t split = size ? data[0] % (size + 1) : 0;
        protocol->consume({data, split});
        protocol->consume({data + split, size - split});
    }
    return 0;
}
