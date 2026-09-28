#include "GPSSettingsBindings.h"

#include <chrono>
#include <type_traits>

#include "Fact.h"
#include "GPSCorrectionSettings.h"
#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"
#include "RTKSettings.h"

QGC_LOGGING_CATEGORY(GPSSettingsBindingsLog, "GPS.GPSSettingsBindings")

namespace GPSSettingsBindings {

namespace {

using RoutingConfig = GPSCorrectionManager::RoutingConfiguration;
using UdpInputConfig = GPSCorrectionManager::UdpInputConfiguration;
using UdpOutputConfig = GPSCorrectionManager::UdpOutputConfiguration;
using NTRIPConfig = NTRIPManager::Configuration;
using ReceiverConfig = GPSReceiver::Configuration;
using PositionConfig = PositionManager::Configuration;

/// One setting: the Fact it is read from and how its raw value is stored in the configuration.
template <typename Settings, typename Configuration>
struct Binding
{
    Fact* (Settings::*fact)();
    void (*apply)(Configuration& configuration, const QVariant& rawValue);
};

template <typename Object>
Object& memberAt(Object& object)
{
    return object;
}

template <typename Object, typename Member, typename... Path>
auto& memberAt(Object& object, Member Object::* member, Path... path)
{
    return memberAt(object.*member, path...);
}

/// Stores the raw value at a member path: assign<&A::b, &B::c> writes a.b.c.
template <auto... Path>
constexpr auto assign = [](auto& configuration, const QVariant& rawValue) {
    auto& destination = memberAt(configuration, Path...);
    using Value = std::remove_reference_t<decltype(destination)>;
    if constexpr (std::is_enum_v<Value>) {
        destination = static_cast<Value>(rawValue.toInt());
    } else {
        destination = rawValue.value<Value>();
    }
};

template <auto Member>
constexpr auto assignConnection = assign<&NTRIPConfig::stream, &NTRIPConfiguration::connection, Member>;

void assignCorrectionSource(RoutingConfig& configuration, const QVariant& rawValue)
{
    using RoutingPolicy = GPSCorrectionManager::RoutingPolicy;
    switch (rawValue.toInt()) {
        case GPSCorrectionSettings::LocalReceiver:
            configuration.source = GPSCorrectionSource::LocalReceiver;
            configuration.policy = RoutingPolicy::Manual;
            break;
        case GPSCorrectionSettings::Ntrip:
            configuration.source = GPSCorrectionSource::NTRIP;
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
}

constexpr Binding<GPSCorrectionSettings, RoutingConfig> kRoutingBindings[] = {
    {&GPSCorrectionSettings::correctionSourceInstance, assign<&RoutingConfig::instance>},
    {&GPSCorrectionSettings::correctionSource, assignCorrectionSource},
};

constexpr Binding<GPSCorrectionSettings, UdpInputConfig> kUdpInputBindings[] = {
    {&GPSCorrectionSettings::rtcmUdpInputEnabled, assign<&UdpInputConfig::enabled>},
    {&GPSCorrectionSettings::rtcmUdpInputPort, assign<&UdpInputConfig::port>},
};

constexpr Binding<GPSCorrectionSettings, UdpOutputConfig> kUdpOutputBindings[] = {
    {&GPSCorrectionSettings::rtcmUdpOutputEnabled, assign<&UdpOutputConfig::enabled>},
    {&GPSCorrectionSettings::rtcmUdpOutputAddress,
     [](UdpOutputConfig& configuration, const QVariant& rawValue) {
         configuration.address = rawValue.toString().trimmed();
     }},
    {&GPSCorrectionSettings::rtcmUdpOutputPort, assign<&UdpOutputConfig::port>},
};

constexpr Binding<NTRIPSettings, NTRIPConfig> kNTRIPBindings[] = {
    {&NTRIPSettings::ntripServerConnectEnabled, assign<&NTRIPConfig::enabled>},
    {&NTRIPSettings::ntripServerHostAddress, assignConnection<&NTRIPConnectionConfig::host>},
    {&NTRIPSettings::ntripServerPort, assignConnection<&NTRIPConnectionConfig::port>},
    {&NTRIPSettings::ntripUsername, assignConnection<&NTRIPConnectionConfig::username>},
    {&NTRIPSettings::ntripPassword, assignConnection<&NTRIPConnectionConfig::password>},
    {&NTRIPSettings::ntripMountpoint, assignConnection<&NTRIPConnectionConfig::mountpoint>},
    {&NTRIPSettings::ntripUseTls, assignConnection<&NTRIPConnectionConfig::useTls>},
    {&NTRIPSettings::ntripAllowSelfSignedCerts, assignConnection<&NTRIPConnectionConfig::allowSelfSignedCerts>},
    {&NTRIPSettings::ntripPinnedCertificate, assignConnection<&NTRIPConnectionConfig::pinnedCertificate>},
    {&NTRIPSettings::ntripWhitelist,
     assign<&NTRIPConfig::stream, &NTRIPConfiguration::filter, &NTRIPRTCMFilterConfig::whitelist>},
    {&NTRIPSettings::ntripGgaPositionSource, assign<&NTRIPConfig::gga, &NTRIPGgaProvider::Configuration::source>},
    {&NTRIPSettings::ntripGgaIntervalSec,
     [](NTRIPConfig& configuration, const QVariant& rawValue) {
         configuration.gga.interval = std::chrono::seconds(rawValue.toUInt());
     }},
};

constexpr Binding<RTKSettings, ReceiverConfig> kReceiverBindings[] = {
    {&RTKSettings::receiverRole, assign<&ReceiverConfig::receiverRole>},
    {&RTKSettings::forwardReceiverRtcm, assign<&ReceiverConfig::forwardReceiverRtcm>},
    {&RTKSettings::baseReceiverManufacturers, assign<&ReceiverConfig::baseReceiverManufacturer>},
    {&RTKSettings::connectionType, assign<&ReceiverConfig::connectionType>},
    {&RTKSettings::tcpHost, assign<&ReceiverConfig::tcpHost>},
    {&RTKSettings::tcpPort, assign<&ReceiverConfig::tcpPort>},
    {&RTKSettings::udpPort, assign<&ReceiverConfig::udpPort>},
    {&RTKSettings::serialDevice, assign<&ReceiverConfig::serialDevice>},
    {&RTKSettings::serialBaudRate, assign<&ReceiverConfig::serialBaudRate>},
    {&RTKSettings::useFixedBasePosition, assign<&ReceiverConfig::baseMode>},
    {&RTKSettings::fixedBasePositionLatitude, assign<&ReceiverConfig::fixedBasePositionLatitude>},
    {&RTKSettings::fixedBasePositionLongitude, assign<&ReceiverConfig::fixedBasePositionLongitude>},
    {&RTKSettings::fixedBasePositionAltitude, assign<&ReceiverConfig::fixedBasePositionAltitude>},
    {&RTKSettings::fixedBasePositionAccuracy, assign<&ReceiverConfig::fixedBasePositionAccuracy>},
    {&RTKSettings::surveyInAccuracyLimit, assign<&ReceiverConfig::surveyInAccuracyLimit>},
    {&RTKSettings::surveyInMinObservationDuration,
     [](ReceiverConfig& configuration, const QVariant& rawValue) {
         configuration.surveyInMinObservationDuration = std::chrono::seconds(rawValue.value<int64_t>());
     }},
    {&RTKSettings::receiverAveragingDuration,
     [](ReceiverConfig& configuration, const QVariant& rawValue) {
         configuration.receiverAveragingDuration = std::chrono::seconds(rawValue.value<uint32_t>());
     }},
    {&RTKSettings::compactRtcmCorrections, assign<&ReceiverConfig::compactRtcmCorrections>},
    {&RTKSettings::autoConnect, assign<&ReceiverConfig::autoConnect>},
};

constexpr Binding<RTKSettings, PositionConfig> kPositionBindings[] = {
    {&RTKSettings::gcsPositionSource, assign<&PositionConfig::sourceMode>},
};

template <typename Settings, typename Configuration, std::size_t N>
Configuration build(Settings* settings, const Binding<Settings, Configuration> (&table)[N])
{
    Configuration configuration;
    for (const auto& binding : table) {
        binding.apply(configuration, (settings->*binding.fact)()->rawValue());
    }
    return configuration;
}

template <typename Settings, typename Configuration, std::size_t N>
QList<Fact*> facts(Settings* settings, const Binding<Settings, Configuration> (&table)[N])
{
    QList<Fact*> result;
    result.reserve(N);
    for (const auto& binding : table) {
        result.append((settings->*binding.fact)());
    }
    return result;
}

template <typename Settings, typename Configuration, std::size_t N, typename Callback>
void watch(Settings* settings, const Binding<Settings, Configuration> (&table)[N], const QObject* receiver,
           const Callback& callback)
{
    for (const Fact* fact : facts(settings, table)) {
        QObject::connect(fact, &Fact::rawValueChanged, receiver, callback);
    }
}

}  // namespace

GPSCorrectionManager::RoutingConfiguration routingConfiguration(GPSCorrectionSettings* settings)
{
    return build(settings, kRoutingBindings);
}

GPSCorrectionManager::UdpInputConfiguration udpInputConfiguration(GPSCorrectionSettings* settings)
{
    return build(settings, kUdpInputBindings);
}

GPSCorrectionManager::UdpOutputConfiguration udpOutputConfiguration(GPSCorrectionSettings* settings)
{
    return build(settings, kUdpOutputBindings);
}

NTRIPManager::Configuration ntripConfiguration(NTRIPSettings* settings)
{
    return build(settings, kNTRIPBindings);
}

GPSReceiver::Configuration rtkConfiguration(RTKSettings* settings)
{
    return build(settings, kReceiverBindings);
}

PositionManager::Configuration positionConfiguration(RTKSettings* settings)
{
    return build(settings, kPositionBindings);
}

QList<Fact*> boundFacts(GPSCorrectionSettings* settings)
{
    return facts(settings, kRoutingBindings) + facts(settings, kUdpInputBindings) + facts(settings, kUdpOutputBindings);
}

QList<Fact*> boundFacts(NTRIPSettings* settings)
{
    return facts(settings, kNTRIPBindings);
}

QList<Fact*> boundFacts(RTKSettings* settings)
{
    return facts(settings, kReceiverBindings) + facts(settings, kPositionBindings);
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
    watch(settings, kRoutingBindings, corrections, applyRouting);
    watch(settings, kUdpInputBindings, corrections, applyUdpInput);
    watch(settings, kUdpOutputBindings, corrections, applyUdpOutput);
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
    watch(settings, kNTRIPBindings, ntrip, apply);
    QObject::connect(ntrip, &NTRIPManager::mountpointChosen, ntrip,
                     [settings](const QString& mountpoint) { settings->ntripMountpoint()->setRawValue(mountpoint); });
    QObject::connect(ntrip, &NTRIPManager::enableRequested, ntrip,
                     [settings]() { settings->ntripServerConnectEnabled()->setRawValue(true); });
    QObject::connect(ntrip, &NTRIPManager::certificatePinChanged, ntrip,
                     [settings](const QString& pin) { settings->ntripPinnedCertificate()->setRawValue(pin); });
    apply();
}

void bindRtk(RTKSettings* settings, GPSReceiver* rtk)
{
    if (!settings || !rtk) {
        return;
    }
    const auto apply = [settings, rtk]() { rtk->setConfiguration(rtkConfiguration(settings)); };
    watch(settings, kReceiverBindings, rtk, apply);
    QObject::connect(rtk, &GPSReceiver::autoConnectDisabled, rtk,
                     [settings]() { settings->autoConnect()->setRawValue(false); });
    apply();
}

void bindPosition(RTKSettings* settings, PositionManager* positions)
{
    if (!settings || !positions) {
        return;
    }
    const auto apply = [settings, positions]() { positions->setConfiguration(positionConfiguration(settings)); };
    watch(settings, kPositionBindings, positions, apply);
    apply();
}

}  // namespace GPSSettingsBindings
