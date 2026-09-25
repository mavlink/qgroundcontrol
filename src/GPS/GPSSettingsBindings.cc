#include "GPSSettingsBindings.h"

#include <chrono>

#include "Fact.h"
#include "GPSCorrectionSettings.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSettingsBindingsLog, "GPS.GPSSettingsBindings")

namespace GPSSettingsBindings {

GPSCorrectionManager::RoutingConfiguration routingConfiguration(GPSCorrectionSettings* settings)
{
    using RoutingPolicy = GPSCorrectionManager::RoutingPolicy;
    GPSCorrectionManager::RoutingConfiguration configuration;
    configuration.instance = settings->correctionSourceInstance()->rawValue().toString();
    switch (settings->correctionSource()->rawValue().toInt()) {
        case GPSCorrectionSettings::LocalReceiver:
            configuration.source = GPSCorrectionSource::LocalReceiver;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Ntrip:
            configuration.source = GPSCorrectionSource::Ntrip;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Udp:
            configuration.source = GPSCorrectionSource::Udp;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Automatic:
            break;
        default:
            qCWarning(GPSSettingsBindingsLog) << "Invalid correction source; using automatic selection";
            break;
    }
    return configuration;
}

GPSCorrectionManager::UdpInputConfiguration udpInputConfiguration(GPSCorrectionSettings* settings)
{
    return {
        .enabled = settings->rtcmUdpInputEnabled()->rawValue().toBool(),
        .port = static_cast<quint16>(settings->rtcmUdpInputPort()->rawValue().toUInt()),
        .validate = settings->rtcmUdpValidate()->rawValue().toBool(),
    };
}

GPSCorrectionManager::UdpOutputConfiguration udpOutputConfiguration(GPSCorrectionSettings* settings)
{
    return {
        .enabled = settings->rtcmUdpOutputEnabled()->rawValue().toBool(),
        .address = settings->rtcmUdpOutputAddress()->rawValue().toString().trimmed(),
        .port = static_cast<quint16>(settings->rtcmUdpOutputPort()->rawValue().toUInt()),
    };
}

NTRIPManager::Configuration ntripConfiguration(NTRIPSettings* settings)
{
    NTRIPManager::Configuration configuration;
    configuration.enabled = settings->ntripServerConnectEnabled()->rawValue().toBool();
    auto& connection = configuration.stream.connection;
    connection.host = settings->ntripServerHostAddress()->rawValue().toString();
    connection.port = settings->ntripServerPort()->rawValue().toInt();
    connection.username = settings->ntripUsername()->rawValue().toString();
    connection.password = settings->ntripPassword()->rawValue().toString();
    connection.mountpoint = settings->ntripMountpoint()->rawValue().toString();
    connection.useTls = settings->ntripUseTls()->rawValue().toBool();
    connection.allowSelfSignedCerts = settings->ntripAllowSelfSignedCerts()->rawValue().toBool();
    configuration.stream.filter.whitelist = settings->ntripWhitelist()->rawValue().toString();
    configuration.gga.source =
        static_cast<NTRIPGgaProvider::PositionSource>(settings->ntripGgaPositionSource()->rawValue().toUInt());
    configuration.gga.interval = std::chrono::seconds(settings->ntripGgaIntervalSec()->rawValue().toUInt());
    return configuration;
}

void bindCorrections(GPSCorrectionSettings* settings, GPSCorrectionManager* corrections)
{
    if (!settings || !corrections) {
        return;
    }
    const auto applyRouting = [settings, corrections]() {
        corrections->applyRoutingConfiguration(routingConfiguration(settings));
    };
    const auto applyUdpInput = [settings, corrections]() {
        corrections->setUdpInputConfiguration(udpInputConfiguration(settings));
    };
    const auto applyUdpOutput = [settings, corrections]() {
        corrections->setUdpOutputConfiguration(udpOutputConfiguration(settings));
    };
    for (const Fact* fact : {settings->correctionSource(), settings->correctionSourceInstance()}) {
        QObject::connect(fact, &Fact::rawValueChanged, corrections, applyRouting);
    }
    for (const Fact* fact :
         {settings->rtcmUdpInputEnabled(), settings->rtcmUdpInputPort(), settings->rtcmUdpValidate()}) {
        QObject::connect(fact, &Fact::rawValueChanged, corrections, applyUdpInput);
    }
    for (const Fact* fact :
         {settings->rtcmUdpOutputEnabled(), settings->rtcmUdpOutputAddress(), settings->rtcmUdpOutputPort()}) {
        QObject::connect(fact, &Fact::rawValueChanged, corrections, applyUdpOutput);
    }
    applyRouting();
    applyUdpInput();
    applyUdpOutput();
}

void bindNtrip(NTRIPSettings* settings, NTRIPManager* ntrip)
{
    if (!settings || !ntrip) {
        return;
    }
    const auto apply = [settings, ntrip]() { ntrip->setConfiguration(ntripConfiguration(settings)); };
    const Fact* facts[] = {
        settings->ntripServerConnectEnabled(),
        settings->ntripServerHostAddress(),
        settings->ntripServerPort(),
        settings->ntripUsername(),
        settings->ntripPassword(),
        settings->ntripMountpoint(),
        settings->ntripWhitelist(),
        settings->ntripUseTls(),
        settings->ntripAllowSelfSignedCerts(),
        settings->ntripGgaPositionSource(),
        settings->ntripGgaIntervalSec(),
    };
    for (const Fact* fact : facts) {
        QObject::connect(fact, &Fact::rawValueChanged, ntrip, apply);
    }
    QObject::connect(ntrip, &NTRIPManager::mountpointChosen, ntrip,
                     [settings](const QString& mountpoint) { settings->ntripMountpoint()->setRawValue(mountpoint); });
    QObject::connect(ntrip, &NTRIPManager::enableRequested, ntrip,
                     [settings]() { settings->ntripServerConnectEnabled()->setRawValue(true); });
    apply();
}

}  // namespace GPSSettingsBindings
