#include "GPSConfigurationReport.h"
#include "GPSConnectionError.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverProfile.h"
#include "GPSTransportResult.h"

int main()
{
    GPSReceiverProfile profile;
    profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
    profile.endpoint.host = QStringLiteral(" localhost ");
    profile.endpoint.port = 2947;
    const GPSReceiverProfile normalized = profile.normalized();
    if (!normalized.validationError().isEmpty() || normalized.networkHost() != QStringLiteral("localhost")) {
        return 1;
    }
    profile.endpoint.port = 0;
    if (profile.validationError().isEmpty()) {
        return 2;
    }
    const GPSWriteResult result{.status = GPSWriteStatus::Completed, .acceptedBytes = 3, .writtenBytes = 3};
    return result.writtenBytes == result.acceptedBytes ? 0 : 3;
}
