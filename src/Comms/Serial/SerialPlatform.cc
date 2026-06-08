#include "SerialPlatform.h"

#include <utility>

namespace SerialPlatform {

// Test-only port factory, shared by both platform TUs (host/android) so the seam isn't duplicated per platform.
namespace {
SerialPortFactory s_portFactoryOverride;
}  // namespace

void setPortFactoryForTest(SerialPortFactory factory)
{
    s_portFactoryOverride = std::move(factory);
}

const SerialPortFactory &portFactoryOverride()
{
    return s_portFactoryOverride;
}

}  // namespace SerialPlatform
