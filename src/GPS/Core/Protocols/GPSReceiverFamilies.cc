#include "GPSReceiverFamilies.h"

#include <algorithm>
#include <array>

namespace {

// A new family adds its FAMILY here.
constexpr std::array FAMILIES{
    &UBX::FAMILY,      // u-blox
    &Ashtech::FAMILY,  // Trimble
    &SBF::FAMILY,      // Septentrio
    &Femto::FAMILY,    // Femtomes
    &Unicore::FAMILY,  // Unicore
    &Quectel::FAMILY,  // Quectel
    &Passive::FAMILY,  // Passive NMEA
};

}  // namespace

std::span<const GPSReceiverFamily* const> gpsReceiverFamilies()
{
    return FAMILIES;
}

const GPSReceiverFamily* gpsReceiverFamily(GPSType type)
{
    const auto found = std::ranges::find(FAMILIES, type, &GPSReceiverFamily::type);
    return found == FAMILIES.end() ? nullptr : *found;
}
