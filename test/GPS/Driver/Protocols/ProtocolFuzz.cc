#include <cstdlib>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolTestIO.h"
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
    uint64_t clock = 1000000;
    io.nowUs = [&clock] { return clock; };
    GPSDriverUBX ubx(io, &position, &satellites);
    GPSDriverUBX operationalUbx(io, &position, &satellites);
    operationalUbx.setDecodeContext({true, true, true});
    GPSDriverUBX epochUbx(io, &position, &satellites);
    epochUbx.setDecodeContext({true, true, true, true});
    GPSDriverAshtech ashtech(io, &position, &satellites);
    GPSDriverSBF sbf(io, &position, &satellites);
    GPSDriverFemto femto(io, &position, &satellites);
    for (GPSProtocol* protocol :
         std::array<GPSProtocol*, 6>{&ubx, &operationalUbx, &epochUbx, &ashtech, &sbf, &femto}) {
        const size_t split = size ? data[0] % (size + 1) : 0;
        for (auto bytes :
             {std::span<const uint8_t>(data, split), std::span<const uint8_t>(data + split, size - split)}) {
            do {
                auto result = protocol->decode(bytes);
                if (result.batch.events.size() > GPSDecodedBatch::MAX_EVENTS)
                    std::abort();
                bytes = bytes.subspan(result.bytesConsumed);
            } while (!bytes.empty());
        }
        clock += UBXNavigationEpoch::MAX_AGE_US;
        if (protocol->decode({}).batch.events.size() > GPSDecodedBatch::MAX_EVENTS)
            std::abort();
    }
    return 0;
}
