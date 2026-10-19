#include "HealthReportFixtures.h"

#include <cstring>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QtEndian>

#include "MAVLinkLib.h"

namespace TestFixtures {
namespace HealthReport {

namespace {

constexpr uint8_t LibEventsComponentId = MAV_COMP_ID_AUTOPILOT1;
constexpr uint32_t ArmingCheckSummaryEventId = 1;
constexpr uint32_t FailedCheckEventId = 2;
constexpr uint32_t HealthSummaryEventId = 3;

// libevents log levels: external level in the low nibble, internal level in the high nibble
constexpr uint8_t LogLevelsError = 0x33;
constexpr uint8_t LogLevelsInfo = 0x66;

template <typename T>
void appendArgument(QByteArray& arguments, T value)
{
    const T littleEndian = qToLittleEndian(value);
    arguments.append(reinterpret_cast<const char*>(&littleEndian), sizeof(T));
}

}  // namespace

QString writeMetadata(const QString& directory, uint32_t manualCustomMode, uint32_t takeoffCustomMode,
                      uint32_t missionCustomMode)
{
    const QString metadata = QStringLiteral(R"({
    "version": 2,
    "components": {
        "%1": {
            "namespace": "test",
            "supported_protocols": ["health_and_arming_check"],
            "navigation_mode_groups": {
                "groups": {
                    "0": [%2],
                    "1": [%3],
                    "2": [%4]
                }
            },
            "enums": {
                "navigation_mode_group_t": {
                    "type": "uint32_t",
                    "description": "Mode groups",
                    "is_bitfield": true,
                    "entries": {
                        "1": { "name": "manual", "description": "Manual" },
                        "2": { "name": "takeoff", "description": "Takeoff" },
                        "4": { "name": "mission", "description": "Mission" }
                    }
                },
                "health_component_t": {
                    "type": "uint32_t",
                    "description": "Health components",
                    "is_bitfield": true,
                    "entries": {
                        "1": { "name": "none", "description": "None" },
                        "2": { "name": "gps", "description": "GPS" }
                    }
                }
            },
            "event_groups": {
                "arming_check": {
                    "events": {
                        "%5": {
                            "name": "arming_check_summary",
                            "message": "Arming check summary",
                            "type": "summary",
                            "arguments": [
                                { "type": "uint8_t", "name": "chunk_idx" },
                                { "type": "test::health_component_t", "name": "error" },
                                { "type": "test::health_component_t", "name": "warning" },
                                { "type": "test::navigation_mode_group_t", "name": "can_arm" },
                                { "type": "test::navigation_mode_group_t", "name": "can_run" }
                            ]
                        },
                        "%6": {
                            "name": "check_failed",
                            "message": "%8",
                            "arguments": [
                                { "type": "test::navigation_mode_group_t", "name": "modes" },
                                { "type": "uint8_t", "name": "health_component_index" }
                            ]
                        }
                    }
                },
                "health": {
                    "events": {
                        "%7": {
                            "name": "health_summary",
                            "message": "Health summary",
                            "type": "summary",
                            "arguments": [
                                { "type": "uint8_t", "name": "chunk_idx" },
                                { "type": "test::health_component_t", "name": "is_present" },
                                { "type": "test::health_component_t", "name": "error" },
                                { "type": "test::health_component_t", "name": "warning" }
                            ]
                        }
                    }
                }
            }
        }
    }
})")
                                 .arg(LibEventsComponentId)
                                 .arg(manualCustomMode)
                                 .arg(takeoffCustomMode)
                                 .arg(missionCustomMode)
                                 .arg(ArmingCheckSummaryEventId)
                                 .arg(FailedCheckEventId)
                                 .arg(HealthSummaryEventId)
                                 .arg(EventStream::failedCheckMessage());

    const QString path = QDir(directory).filePath(QStringLiteral("events.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || (file.write(metadata.toUtf8()) == -1)) {
        return QString();
    }
    return path;
}

EventStream::EventStream(uint8_t systemId)
    : _systemId(systemId)
{}

QString EventStream::failedCheckMessage()
{
    return QStringLiteral("Test check failed");
}

QList<mavlink_message_t> EventStream::report(uint32_t canArmModeGroups, uint32_t canRunModeGroups)
{
    QList<mavlink_message_t> messages;

    QByteArray arguments;
    appendArgument<uint8_t>(arguments, 0);   // chunk_idx
    appendArgument<uint32_t>(arguments, 0);  // error
    appendArgument<uint32_t>(arguments, 0);  // warning
    appendArgument<uint32_t>(arguments, canArmModeGroups);
    appendArgument<uint32_t>(arguments, canRunModeGroups);
    messages.append(_event(ArmingCheckSummaryEventId, LogLevelsInfo, arguments));

    const uint32_t failedModeGroups = AllModeGroups & ~canArmModeGroups;
    if (failedModeGroups != 0) {
        arguments.clear();
        appendArgument<uint32_t>(arguments, failedModeGroups);
        appendArgument<uint8_t>(arguments, 0);  // health_component_index
        messages.append(_event(FailedCheckEventId, LogLevelsError, arguments));
    }

    arguments.clear();
    appendArgument<uint8_t>(arguments, 0);   // chunk_idx
    appendArgument<uint32_t>(arguments, 0);  // is_present
    appendArgument<uint32_t>(arguments, 0);  // error
    appendArgument<uint32_t>(arguments, 0);  // warning
    messages.append(_event(HealthSummaryEventId, LogLevelsInfo, arguments));

    return messages;
}

mavlink_message_t EventStream::_event(uint32_t eventSubId, uint8_t logLevels, const QByteArray& arguments)
{
    uint8_t argumentBytes[MAVLINK_MSG_EVENT_FIELD_ARGUMENTS_LEN]{};
    std::memcpy(argumentBytes, arguments.constData(), qMin<qsizetype>(arguments.size(), sizeof(argumentBytes)));

    const uint32_t eventId = (static_cast<uint32_t>(LibEventsComponentId) << 24) | eventSubId;

    mavlink_message_t message{};
    (void) mavlink_msg_event_pack(_systemId, LibEventsComponentId, &message, MAV_COMP_ID_ALL, 0, eventId, _timeBootMs++,
                                  _sequence++, logLevels, argumentBytes);
    return message;
}

}  // namespace HealthReport
}  // namespace TestFixtures
