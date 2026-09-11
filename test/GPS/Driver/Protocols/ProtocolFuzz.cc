#include <cstdlib>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolFeatures.h"
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
#if QGC_GPS_ENABLE_UBX
    GPSDriverUBX ubx(io, &position, &satellites);
    GPSDriverUBX operationalUbx(io, &position, &satellites);
    operationalUbx.setDecodeContext({true, true, true});
    GPSDriverUBX epochUbx(io, &position, &satellites);
    epochUbx.setDecodeContext({true, true, true, true});
#endif
#if QGC_GPS_ENABLE_ASHTECH
    GPSDriverAshtech ashtech(io, &position, &satellites);
#endif
#if QGC_GPS_ENABLE_SBF
    GPSDriverSBF sbf(io, &position, &satellites);
#endif
#if QGC_GPS_ENABLE_FEMTO
    GPSDriverFemto femto(io, &position, &satellites);
#endif
    GPSProtocol* protocols[] = {
#if QGC_GPS_ENABLE_UBX
        &ubx,     &operationalUbx, &epochUbx,
#endif
#if QGC_GPS_ENABLE_ASHTECH
        &ashtech,
#endif
#if QGC_GPS_ENABLE_SBF
        &sbf,
#endif
#if QGC_GPS_ENABLE_FEMTO
        &femto,
#endif
    };
    for (GPSProtocol* protocol : protocols) {
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
