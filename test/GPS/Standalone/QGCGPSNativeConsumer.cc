#include <array>
#include <iostream>
#include <utility>

#include "GPSProtocolFeatures.h"
#include "GPSProtocolIO.h"

#if defined(QT_NETWORK_LIB) || defined(QT_POSITIONING_LIB) || defined(QT_QML_LIB) || defined(QT_SERIALPORT_LIB)
#error Native receiver protocols must not inherit concrete transport or application dependencies.
#endif

#if QGC_GPS_ENABLE_UBX
#include "UBX/GPSDriverUBX.h"
#endif
#if QGC_GPS_ENABLE_ASHTECH
#include "Ashtech/GPSDriverAshtech.h"
#endif
#if QGC_GPS_ENABLE_SBF
#include "SBF/GPSDriverSBF.h"
#endif
#if QGC_GPS_ENABLE_FEMTO
#include "Femto/GPSDriverFemto.h"
#endif
#if QGC_GPS_ENABLE_UNICORE
#include "Unicore/GPSDriverUnicore.h"
#endif
#if QGC_GPS_ENABLE_QUECTEL
#include "Quectel/GPSDriverQuectel.h"
#endif
#if QGC_GPS_ENABLE_PASSIVE
#include "Passive/GPSDriverPassive.h"
#endif

namespace {
template <typename Driver>
bool decodeWithoutDevice(const char* family)
{
    int operations = 0;
    GPSProtocolIO io;
    io.nowUs = [] { return uint64_t{1000000}; };
    io.read = [&](std::span<uint8_t>, GPSDeadline) {
        ++operations;
        return GPSReadResult{};
    };
    io.write = [&](std::span<const uint8_t>, GPSDeadline) {
        ++operations;
        return GPSWriteResult{};
    };
    io.setBaudrate = [&](unsigned) {
        ++operations;
        return GPSBaudStatus::Unsupported;
    };
    io.wait = [&](std::chrono::microseconds) {
        ++operations;
        return false;
    };
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    Driver driver(std::move(io), &position, &satellites);
    constexpr std::array<uint8_t, 8> noise{0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00};
    const auto decoded = driver.decode(noise);
    const auto empty = driver.decode({});
    if (decoded.bytesConsumed != noise.size() || !decoded.batch.events.empty() || empty.bytesConsumed != 0 ||
        !empty.batch.events.empty() || operations != 0) {
        std::cerr << family << ": decoding noise must not manufacture observations or invoke device I/O\n";
        return false;
    }
    return true;
}
}  // namespace

int main()
{
    bool valid = true;
#if QGC_GPS_ENABLE_UBX
    valid &= decodeWithoutDevice<GPSNativeUBX>("UBX");
#endif
#if QGC_GPS_ENABLE_ASHTECH
    valid &= decodeWithoutDevice<GPSNativeAshtech>("Ashtech");
#endif
#if QGC_GPS_ENABLE_SBF
    valid &= decodeWithoutDevice<GPSNativeSBF>("SBF");
#endif
#if QGC_GPS_ENABLE_FEMTO
    valid &= decodeWithoutDevice<GPSNativeFemto>("Femto");
#endif
#if QGC_GPS_ENABLE_UNICORE
    valid &= decodeWithoutDevice<GPSNativeUnicore>("Unicore");
#endif
#if QGC_GPS_ENABLE_QUECTEL
    valid &= decodeWithoutDevice<GPSNativeQuectel>("Quectel");
#endif
#if QGC_GPS_ENABLE_PASSIVE
    valid &= decodeWithoutDevice<GPSNativePassive>("Passive");
#endif
    return valid ? 0 : 1;
}
