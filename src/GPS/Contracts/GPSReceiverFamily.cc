#include "GPSReceiverFamily.h"

namespace {
using Support = GPSReceiverCapabilities::Support;
constexpr std::array families = {
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
    GPSReceiverFamily{
        GPSType::trimble,
        QLatin1StringView("Trimble"),
        {QLatin1StringView("trimble"), QLatin1StringView("ashtech"), QLatin1StringView("spectra")},
        1,
        Support::Supported,
        Support::Unsupported,
        Support::Unknown,
    },
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
    GPSReceiverFamily{
        GPSType::femto,
        QLatin1StringView("Femtomes"),
        {QLatin1StringView("femtomes"), QLatin1StringView("femto"), QLatin1StringView()},
        3,
        Support::Supported,
        Support::Unsupported,
        Support::Unknown,
    },
};
}  // namespace

std::span<const GPSReceiverFamily> gpsReceiverFamilies()
{
    return families;
}
