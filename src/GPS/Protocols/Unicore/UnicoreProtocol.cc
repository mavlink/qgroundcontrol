#include "UnicoreProtocol.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UnicoreProtocolLog, "GPS.Driver.Protocols.Unicore")

const QLoggingCategory& UnicoreProtocol::logCategory() const
{
    return UnicoreProtocolLog();
}

UnicoreProtocol::UnicoreProtocol(GPSProtocolIO io, bool satelliteInfoEnabled)
    : GPSAsciiProtocol(std::move(io), satelliteInfoEnabled)
{
    setRTCMEnabled(false);
}

std::string UnicoreProtocol::receiverIdentity() const
{
    return _model.empty() || _firmware.empty() ? _model + _firmware : _model + ' ' + _firmware;
}
