#include <type_traits>

#include "GPSIOStatus.h"

#if defined(QT_CORE_LIB) || defined(QT_VERSION)
#error GPS I/O statuses must remain usable without Qt.
#endif

static_assert(std::is_enum_v<GPSOpenStatus>);
static_assert(std::is_enum_v<GPSReadStatus>);
static_assert(std::is_enum_v<GPSWriteStatus>);
static_assert(std::is_enum_v<GPSBaudStatus>);
static_assert(GPSReadStatus::Data != GPSReadStatus::TimedOut);
static_assert(GPSWriteStatus::Completed != GPSWriteStatus::Unsupported);

int main()
{
    return 0;
}
