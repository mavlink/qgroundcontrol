#include "QuectelProtocol.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QuectelProtocolLog, "GPS.Driver.Protocols.Quectel")

const QLoggingCategory& QuectelProtocol::logCategory() const
{
    return QuectelProtocolLog();
}

QuectelProtocol::QuectelProtocol(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSAsciiProtocol(std::move(io), satelliteInfoEnabled)
{
    setRTCMEnabled(false);
}
