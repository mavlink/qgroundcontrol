#pragma once

#include <cstdint>
#include <initializer_list>

/// Stable identities shared by validation, command reporting, and presentation.
enum class GPSReceiverSetting
{
    Unknown,
    ConstellationMask,
    DynamicModel,
    OutputRateHz,
    HeadingOffsetDeg,
};

class GPSReceiverSettingSet
{
public:
    constexpr GPSReceiverSettingSet() = default;

    constexpr GPSReceiverSettingSet(std::initializer_list<GPSReceiverSetting> settings)
    {
        for (auto setting : settings)
            add(setting);
    }

    constexpr void add(GPSReceiverSetting setting)
    {
        if (setting != GPSReceiverSetting::Unknown)
            _bits |= uint32_t{1} << static_cast<unsigned>(setting);
    }

    constexpr bool contains(GPSReceiverSetting setting) const
    {
        return setting != GPSReceiverSetting::Unknown && (_bits & (uint32_t{1} << static_cast<unsigned>(setting)));
    }

private:
    uint32_t _bits = 0;
};
