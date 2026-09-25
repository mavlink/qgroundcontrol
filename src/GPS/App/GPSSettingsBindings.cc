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
using RTKConfig = GPSRTK::Configuration;

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
    {&GPSCorrectionSettings::rtcmUdpValidate, assign<&UdpInputConfig::validate>},
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

constexpr Binding<RTKSettings, RTKConfig> kRTKBindings[] = {
    {&RTKSettings::receiverRole, assign<&RTKConfig::receiverRole>},
    {&RTKSettings::baseReceiverManufacturers, assign<&RTKConfig::baseReceiverManufacturer>},
    {&RTKSettings::connectionType, assign<&RTKConfig::connectionType>},
    {&RTKSettings::tcpHost, assign<&RTKConfig::tcpHost>},
    {&RTKSettings::tcpPort, assign<&RTKConfig::tcpPort>},
    {&RTKSettings::udpPort, assign<&RTKConfig::udpPort>},
    {&RTKSettings::serialDevice, assign<&RTKConfig::serialDevice>},
    {&RTKSettings::serialBaudRate, assign<&RTKConfig::serialBaudRate>},
    {&RTKSettings::useFixedBasePosition, assign<&RTKConfig::baseMode>},
    {&RTKSettings::fixedBasePositionLatitude, assign<&RTKConfig::fixedBasePositionLatitude>},
    {&RTKSettings::fixedBasePositionLongitude, assign<&RTKConfig::fixedBasePositionLongitude>},
    {&RTKSettings::fixedBasePositionAltitude, assign<&RTKConfig::fixedBasePositionAltitude>},
    {&RTKSettings::fixedBasePositionAccuracy, assign<&RTKConfig::fixedBasePositionAccuracy>},
    {&RTKSettings::surveyInAccuracyLimit, assign<&RTKConfig::surveyInAccuracyLimit>},
    {&RTKSettings::surveyInMinObservationDuration,
     [](RTKConfig& configuration, const QVariant& rawValue) {
         configuration.surveyInMinObservationDuration = std::chrono::seconds(rawValue.value<int64_t>());
     }},
    {&RTKSettings::receiverAveragingDuration,
     [](RTKConfig& configuration, const QVariant& rawValue) {
         configuration.receiverAveragingDuration = std::chrono::seconds(rawValue.value<uint32_t>());
     }},
    {&RTKSettings::compactRtcmCorrections, assign<&RTKConfig::compactRtcmCorrections>},
    {&RTKSettings::autoConnect, assign<&RTKConfig::autoConnect>},
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

GPSRTK::Configuration rtkConfiguration(RTKSettings* settings)
{
    return build(settings, kRTKBindings);
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
    return facts(settings, kRTKBindings);
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

void bindRtk(RTKSettings* settings, GPSRTK* rtk)
{
    if (!settings || !rtk) {
        return;
    }
    const auto apply = [settings, rtk]() { rtk->setConfiguration(rtkConfiguration(settings)); };
    watch(settings, kRTKBindings, rtk, apply);
    QObject::connect(rtk, &GPSRTK::autoConnectDisabled, rtk,
                     [settings]() { settings->autoConnect()->setRawValue(false); });
    QObject::connect(rtk, &GPSRTK::baseManufacturerDetected, rtk, [settings](int manufacturer) {
        settings->baseReceiverManufacturers()->setRawValue(manufacturer);
    });
    apply();
}

}  // namespace GPSSettingsBindings
