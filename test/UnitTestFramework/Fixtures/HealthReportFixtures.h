#pragma once

#include <cstdint>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include "MAVLinkMessageType.h"

/// @file
/// @brief libevents health and arming check report fixtures

namespace TestFixtures {
namespace HealthReport {

/// Mode groups defined by the fixture metadata
constexpr int ManualModeGroup = 0;
constexpr int TakeoffModeGroup = 1;
constexpr int MissionModeGroup = 2;

constexpr uint32_t modeGroupBit(int modeGroup)
{
    return 1u << modeGroup;
}

constexpr uint32_t AllModeGroups =
    modeGroupBit(ManualModeGroup) | modeGroupBit(TakeoffModeGroup) | modeGroupBit(MissionModeGroup);

/// Writes autopilot events metadata which supports the health and arming check protocol and maps each
/// mode group to a custom mode.
/// @return Metadata file path, or an empty string on failure
QString writeMetadata(const QString& directory, uint32_t manualCustomMode, uint32_t takeoffCustomMode,
                      uint32_t missionCustomMode);

/// Builds the EVENT message sequences of successive health and arming check reports from the autopilot
class EventStream
{
public:
    explicit EventStream(uint8_t systemId);

    /// Message text of the check reported for the mode groups which cannot arm
    static QString failedCheckMessage();

    /// @param canArmModeGroups Mode group bits in which arming would be accepted
    /// @param canRunModeGroups Mode group bits which can be switched into while armed
    QList<mavlink_message_t> report(uint32_t canArmModeGroups, uint32_t canRunModeGroups);

private:
    mavlink_message_t _event(uint32_t eventSubId, uint8_t logLevels, const QByteArray& arguments);

    uint8_t _systemId = 0;
    uint16_t _sequence = 0;
    uint32_t _timeBootMs = 1000;
};

}  // namespace HealthReport
}  // namespace TestFixtures
