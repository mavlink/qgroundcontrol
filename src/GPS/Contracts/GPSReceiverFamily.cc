#include "GPSReceiverFamily.h"

#include "GPSProtocolFeatures.h"

namespace {
using Support = GPSReceiverCapabilities::Support;
constexpr std::array<GPSReceiverFamily,
                     QGC_GPS_ENABLE_UBX + QGC_GPS_ENABLE_ASHTECH + QGC_GPS_ENABLE_SBF + QGC_GPS_ENABLE_FEMTO>
    families = {
#if QGC_GPS_ENABLE_UBX
        GPSReceiverFamily{GPSType::u_blox,
                          QLatin1StringView("U-blox"),
                          {QLatin1StringView("blox"), QLatin1StringView("ubx"), QLatin1StringView("u-blox")},
                          4,
                          Support::Unknown,
                          Support::Supported,
                          Support::Unknown,
                          Support::Unknown,
                          Support::Supported,
                          Support::Unknown,
                          Support::Unsupported},
#endif
#if QGC_GPS_ENABLE_ASHTECH
        GPSReceiverFamily{
            GPSType::trimble,
            QLatin1StringView("Trimble"),
            {QLatin1StringView("trimble"), QLatin1StringView("ashtech"), QLatin1StringView("spectra")},
            1,
            Support::Supported,
            Support::Unsupported,
            Support::Unknown,
        },
#endif
#if QGC_GPS_ENABLE_SBF
        GPSReceiverFamily{GPSType::septentrio,
                          QLatin1StringView("Septentrio"),
                          {QLatin1StringView("septentrio"), QLatin1StringView("sbf"), QLatin1StringView()},
                          2,
                          Support::Supported,
                          Support::Unsupported,
                          Support::Supported,
                          Support::Unsupported,
                          Support::Unsupported,
                          Support::Unsupported,
                          Support::Supported},
#endif
#if QGC_GPS_ENABLE_FEMTO
        GPSReceiverFamily{
            GPSType::femto,
            QLatin1StringView("Femtomes"),
            {QLatin1StringView("femtomes"), QLatin1StringView("femto"), QLatin1StringView()},
            3,
            Support::Supported,
            Support::Unsupported,
            Support::Unknown,
        },
#endif
};
}  // namespace

std::span<const GPSReceiverFamily> gpsReceiverFamilies()
{
    return families;
}
