#include "Fact.h"
#include "NTRIPSettings.h"
#include "NTRIPTransportConfig.h"

NTRIPTransportConfig NTRIPTransportConfig::fromSettings(NTRIPSettings& settings)
{
    const auto read = [](Fact* fact, const QVariant& fallback) { return fact ? fact->rawValue() : fallback; };

    NTRIPTransportConfig config;
    config.host = read(settings.ntripServerHostAddress(), config.host).toString();
    config.port = read(settings.ntripServerPort(), config.port).toInt();
    config.username = read(settings.ntripUsername(), config.username).toString();
    config.password = read(settings.ntripPassword(), config.password).toString();
    config.mountpoint = read(settings.ntripMountpoint(), config.mountpoint).toString();
    config.whitelist = read(settings.ntripWhitelist(), config.whitelist).toString();
    config.useTls = read(settings.ntripUseTls(), config.useTls).toBool();
    config.allowSelfSignedCerts = read(settings.ntripAllowSelfSignedCerts(), config.allowSelfSignedCerts).toBool();
    return config;
}
