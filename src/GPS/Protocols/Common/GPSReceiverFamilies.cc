#include "GPSReceiverFamilies.h"

#include <algorithm>
#include <array>

#include "Ashtech/AshtechFamily.h"
#include "Femto/FemtoFamily.h"
#include "Passive/PassiveFamily.h"
#include "Quectel/QuectelFamily.h"
#include "SBF/SBFFamily.h"
#include "UBX/UBXFamily.h"
#include "Unicore/UnicoreFamily.h"

namespace {

// A new family adds its descriptor here.
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
