#include <cstdint>
#include <limits>

#include "GPSBaseStationConfig.h"
#include "GPSConstellation.h"

#if defined(QT_CORE_LIB) || defined(QT_VERSION)
#error Native receiver contracts must not inherit Qt dependencies.
#endif

int main()
{
    const GPSBaseStationConfig config{.surveyInAccMeters = 1,
                                      .surveyInDurationSecs = (std::numeric_limits<uint32_t>::max)()};
    return config.surveyInDurationSecs == 4294967295LL && gpsSatelliteId(GPSConstellation::Galileo, 301) == 1 ? 0 : 1;
}
