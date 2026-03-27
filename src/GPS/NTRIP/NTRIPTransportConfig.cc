#include "NTRIPTransportConfig.h"
#include "NTRIPSettings.h"
#include "Fact.h"

NTRIPTransportConfig NTRIPTransportConfig::fromSettings(NTRIPSettings &settings)
{
    NTRIPTransportConfig config;
    if (settings.ntripServerHostAddress()) config.host       = settings.ntripServerHostAddress()->rawValue().toString();
    if (settings.ntripServerPort())        config.port       = settings.ntripServerPort()->rawValue().toInt();
    if (settings.ntripUsername())           config.username   = settings.ntripUsername()->rawValue().toString();
    if (settings.ntripPassword())          config.password   = settings.ntripPassword()->rawValue().toString();
    if (settings.ntripMountpoint())        config.mountpoint = settings.ntripMountpoint()->rawValue().toString();
    if (settings.ntripWhitelist())         config.whitelist  = settings.ntripWhitelist()->rawValue().toString();
    if (settings.ntripUseTls())            config.useTls     = settings.ntripUseTls()->rawValue().toBool();
    if (settings.ntripAllowSelfSignedCerts()) config.allowSelfSignedCerts = settings.ntripAllowSelfSignedCerts()->rawValue().toBool();
    if (settings.ntripUdpForwardEnabled())   config.udpForwardEnabled   = settings.ntripUdpForwardEnabled()->rawValue().toBool();
    if (settings.ntripUdpTargetAddress())    config.udpTargetAddress    = settings.ntripUdpTargetAddress()->rawValue().toString();
    if (settings.ntripUdpTargetPort())       config.udpTargetPort       = static_cast<quint16>(settings.ntripUdpTargetPort()->rawValue().toUInt());
    return config;
}
