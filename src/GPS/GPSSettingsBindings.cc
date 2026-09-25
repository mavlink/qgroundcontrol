#include "GPSSettingsBindings.h"

#include <chrono>

#include "Fact.h"
#include "GPSCorrectionSettings.h"
#include "GPSRtk.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"

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

GPSRtk::Configuration rtkConfiguration(RTKSettings* settings)
{
    GPSRtk::Configuration configuration;
    configuration.receiverRole = static_cast<GPSRtk::ReceiverRole>(settings->receiverRole()->rawValue().toInt());
    configuration.baseReceiverManufacturer = settings->baseReceiverManufacturers()->rawValue().toInt();
    configuration.connectionType = static_cast<GPSRtk::ConnectionType>(settings->connectionType()->rawValue().toInt());
    configuration.tcpHost = settings->tcpHost()->rawValue().toString();
    configuration.tcpPort = settings->tcpPort()->rawValue().toUInt();
    configuration.udpPort = settings->udpPort()->rawValue().toUInt();
    configuration.serialDevice = settings->serialDevice()->rawValue().toString();
    configuration.serialBaudRate = settings->serialBaudRate()->rawValue().toUInt();
    configuration.baseMode = settings->useFixedBasePosition()->rawValue().toInt();
    configuration.fixedBasePositionLatitude = settings->fixedBasePositionLatitude()->rawValue().toDouble();
    configuration.fixedBasePositionLongitude = settings->fixedBasePositionLongitude()->rawValue().toDouble();
    configuration.fixedBasePositionAltitude = settings->fixedBasePositionAltitude()->rawValue().toFloat();
    configuration.fixedBasePositionAccuracy = settings->fixedBasePositionAccuracy()->rawValue().toFloat();
    configuration.surveyInAccuracyLimit = settings->surveyInAccuracyLimit()->rawValue().toDouble();
    configuration.surveyInMinObservationDuration = settings->surveyInMinObservationDuration()->rawValue().toLongLong();
    configuration.receiverAveragingDuration = settings->receiverAveragingDuration()->rawValue().toUInt();
    configuration.compactRtcmCorrections = settings->compactRtcmCorrections()->rawValue().toBool();
    configuration.autoConnect = settings->autoConnect()->rawValue().toBool();
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

void bindRtk(RTKSettings* settings, GPSRtk* rtk)
{
    if (!settings || !rtk) {
        return;
    }
    const auto apply = [settings, rtk]() { rtk->setConfiguration(rtkConfiguration(settings)); };
    const Fact* facts[] = {
        settings->receiverRole(),
        settings->baseReceiverManufacturers(),
        settings->surveyInAccuracyLimit(),
        settings->surveyInMinObservationDuration(),
        settings->receiverAveragingDuration(),
        settings->connectionType(),
        settings->tcpHost(),
        settings->tcpPort(),
        settings->udpPort(),
        settings->autoConnect(),
        settings->serialDevice(),
        settings->serialBaudRate(),
        settings->useFixedBasePosition(),
        settings->fixedBasePositionLatitude(),
        settings->fixedBasePositionLongitude(),
        settings->fixedBasePositionAltitude(),
        settings->fixedBasePositionAccuracy(),
        settings->compactRtcmCorrections(),
    };
    for (const Fact* fact : facts) {
        QObject::connect(fact, &Fact::rawValueChanged, rtk, apply);
    }
    QObject::connect(rtk, &GPSRtk::autoConnectDisabled, rtk,
                     [settings]() { settings->autoConnect()->setRawValue(false); });
    QObject::connect(rtk, &GPSRtk::baseManufacturerDetected, rtk, [settings](int manufacturer) {
        settings->baseReceiverManufacturers()->setRawValue(manufacturer);
    });
    apply();
}

}  // namespace GPSSettingsBindings
