#include "NMEAUtils.h"

#if defined(QT_NETWORK_LIB) || defined(QT_QML_LIB) || defined(QT_SERIALPORT_LIB)
#error NMEA formatting must not inherit transport or application dependencies.
#endif

int main()
{
    const auto sentence = NMEAUtils::makeGGA(QGeoCoordinate(47, 8, 500), 500);
    return NMEAUtils::verifyChecksum(sentence) && NMEAUtils::repairChecksum(sentence) == sentence ? 0 : 1;
}
