#pragma once

#include "GPSReceiver.h"
#include "ScriptedProvider.h"

/// Receiver configurations and a receiver whose worker is scripted, shared by the receiver suites.
namespace GPSReceiverTestSupport {
inline GPSPositionReport fixReport(GPSFixQuality fixType)
{
    GPSPositionReport report;
    report.navigation.fixType = fixType;
    return report;
}

/// The descriptor ID of the passive family, which the settings select by role rather than by manufacturer.
inline const int kPassiveManufacturer = GPSReceiver::manufacturerForType(GPSType::passive);

inline GPSReceiver::Configuration receiverConfiguration(
    int manufacturer = GPSReceiver::manufacturerForType(GPSType::ublox))
{
    GPSReceiver::Configuration configuration;
    if (manufacturer == kPassiveManufacturer) {
        configuration.receiverRole = GPSReceiver::Passive;
    } else {
        configuration.receiverRole = GPSReceiver::ConfiguredBase;
        configuration.baseReceiverManufacturer = manufacturer;
    }
    return configuration;
}

inline GPSReceiver::Configuration fixedConfiguration(
    int manufacturer = GPSReceiver::manufacturerForType(GPSType::ublox))
{
    auto configuration = receiverConfiguration(manufacturer);
    configuration.baseMode = static_cast<int>(BaseModeDefinition::Mode::BaseFixed);
    configuration.fixedBasePositionLatitude = 47.5;
    configuration.fixedBasePositionLongitude = 8.25;
    configuration.fixedBasePositionAltitude = 512.0f;
    configuration.fixedBasePositionAccuracy = 1.5f;
    return configuration;
}

#ifndef QGC_NO_SERIAL_LINK
inline GPSReceiver::Configuration serialConfiguration(int manufacturer, const QString& device, uint32_t baudRate)
{
    auto configuration = receiverConfiguration(manufacturer);
    configuration.connectionType = GPSReceiver::Serial;
    configuration.serialDevice = device;
    configuration.serialBaudRate = baudRate;
    return configuration;
}
#endif

struct ScriptedGPSReceiver
{
    explicit ScriptedGPSReceiver(RuntimeScheduler* scheduler = nullptr)
        : receiver(nullptr, scheduler)
    {
        receiver.setProviderFactory(providers.providerFactory());
    }

    GPSReceiver receiver;
    ScriptedProviderFactory providers;
};
}  // namespace GPSReceiverTestSupport
