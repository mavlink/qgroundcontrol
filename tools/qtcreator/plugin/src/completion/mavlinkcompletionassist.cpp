/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "mavlinkcompletionassist.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <utils/icon.h>

#include <QIcon>
#include <QRegularExpression>
#include <QTextDocument>

namespace QGC::Internal {

// Static member initialization
QList<MAVLinkCompletionProcessor::MAVLinkMessage> MAVLinkCompletionProcessor::s_messages;
bool MAVLinkCompletionProcessor::s_messagesLoaded = false;

// Common MAVLink messages used in QGC
static const QList<MAVLinkCompletionProcessor::MAVLinkMessage> kCommonMessages = {
    // System messages
    {"HEARTBEAT", 0, "Vehicle heartbeat - basic status",
     {"type", "autopilot", "base_mode", "custom_mode", "system_status", "mavlink_version"}},
    {"SYS_STATUS", 1, "System status including battery and sensors",
     {"onboard_control_sensors_present", "onboard_control_sensors_enabled",
      "onboard_control_sensors_health", "load", "voltage_battery", "current_battery",
      "battery_remaining", "drop_rate_comm", "errors_comm"}},
    {"SYSTEM_TIME", 2, "System time sync",
     {"time_unix_usec", "time_boot_ms"}},
    {"PING", 4, "Ping for latency measurement",
     {"time_usec", "seq", "target_system", "target_component"}},

    // Attitude and position
    {"ATTITUDE", 30, "Vehicle attitude (roll/pitch/yaw)",
     {"time_boot_ms", "roll", "pitch", "yaw", "rollspeed", "pitchspeed", "yawspeed"}},
    {"ATTITUDE_QUATERNION", 31, "Attitude as quaternion",
     {"time_boot_ms", "q1", "q2", "q3", "q4", "rollspeed", "pitchspeed", "yawspeed"}},
    {"LOCAL_POSITION_NED", 32, "Local position in NED frame",
     {"time_boot_ms", "x", "y", "z", "vx", "vy", "vz"}},
    {"GLOBAL_POSITION_INT", 33, "Global position (lat/lon/alt)",
     {"time_boot_ms", "lat", "lon", "alt", "relative_alt", "vx", "vy", "vz", "hdg"}},
    {"GPS_RAW_INT", 24, "Raw GPS data",
     {"time_usec", "fix_type", "lat", "lon", "alt", "eph", "epv", "vel", "cog", "satellites_visible"}},

    // Navigation
    {"NAV_CONTROLLER_OUTPUT", 62, "Navigation controller output",
     {"nav_roll", "nav_pitch", "nav_bearing", "target_bearing", "wp_dist", "alt_error",
      "aspd_error", "xtrack_error"}},
    {"MISSION_CURRENT", 42, "Current active mission item",
     {"seq"}},
    {"MISSION_ITEM_REACHED", 46, "Mission item reached notification",
     {"seq"}},

    // RC and control
    {"RC_CHANNELS", 65, "RC channel values",
     {"time_boot_ms", "chancount", "chan1_raw", "chan2_raw", "chan3_raw", "chan4_raw",
      "chan5_raw", "chan6_raw", "chan7_raw", "chan8_raw", "rssi"}},
    {"RC_CHANNELS_OVERRIDE", 70, "Override RC channels from GCS",
     {"target_system", "target_component", "chan1_raw", "chan2_raw", "chan3_raw", "chan4_raw"}},
    {"SERVO_OUTPUT_RAW", 36, "Servo/motor output values",
     {"time_usec", "port", "servo1_raw", "servo2_raw", "servo3_raw", "servo4_raw"}},

    // Parameters
    {"PARAM_VALUE", 22, "Parameter value response",
     {"param_id", "param_value", "param_type", "param_count", "param_index"}},
    {"PARAM_SET", 23, "Set parameter request",
     {"target_system", "target_component", "param_id", "param_value", "param_type"}},

    // Commands
    {"COMMAND_LONG", 76, "Long command",
     {"target_system", "target_component", "command", "confirmation",
      "param1", "param2", "param3", "param4", "param5", "param6", "param7"}},
    {"COMMAND_ACK", 77, "Command acknowledgement",
     {"command", "result", "progress", "result_param2"}},

    // Telemetry
    {"VFR_HUD", 74, "VFR HUD data",
     {"airspeed", "groundspeed", "heading", "throttle", "alt", "climb"}},
    {"ALTITUDE", 141, "Altitude data",
     {"time_usec", "altitude_monotonic", "altitude_amsl", "altitude_local",
      "altitude_relative", "altitude_terrain", "bottom_clearance"}},
    {"BATTERY_STATUS", 147, "Battery status",
     {"id", "battery_function", "type", "temperature", "voltages", "current_battery",
      "current_consumed", "energy_consumed", "battery_remaining"}},

    // Sensors
    {"SCALED_IMU", 26, "Scaled IMU data",
     {"time_boot_ms", "xacc", "yacc", "zacc", "xgyro", "ygyro", "zgyro", "xmag", "ymag", "zmag"}},
    {"SCALED_PRESSURE", 29, "Scaled pressure data",
     {"time_boot_ms", "press_abs", "press_diff", "temperature"}},
    {"WIND", 168, "Wind estimation (ArduPilot)",
     {"direction", "speed", "speed_z"}},

    // Status text
    {"STATUSTEXT", 253, "Status text message",
     {"severity", "text"}},

    // High latency
    {"HIGH_LATENCY", 234, "High latency link summary",
     {"base_mode", "custom_mode", "landed_state", "roll", "pitch", "heading",
      "throttle", "heading_sp", "latitude", "longitude", "altitude_amsl",
      "altitude_sp", "airspeed", "airspeed_sp", "groundspeed", "climb_rate",
      "gps_nsat", "gps_fix_type", "battery_remaining", "temperature", "temperature_air"}},
    {"HIGH_LATENCY2", 235, "High latency link summary v2",
     {"timestamp", "type", "autopilot", "custom_mode", "latitude", "longitude",
      "altitude", "target_altitude", "heading", "target_heading", "target_distance"}},

    // Extended system state
    {"EXTENDED_SYS_STATE", 245, "Extended system state",
     {"vtol_state", "landed_state"}},
};

MAVLinkCompletionProvider::MAVLinkCompletionProvider(QObject *parent)
    : TextEditor::CompletionAssistProvider(parent)
{
}

int MAVLinkCompletionProvider::activationCharSequenceLength() const
{
    // Activate on "MAVLINK_" or "mavlink_" prefix
    return 1;
}

bool MAVLinkCompletionProvider::isActivationCharSequence(const QString &sequence) const
{
    // Activate after typing 'M' (for MAVLINK_MSG_ID_) or 'm' (for mavlink_msg_)
    return sequence == QLatin1String("M") || sequence == QLatin1String("m");
}

bool MAVLinkCompletionProvider::isContinuationChar(const QChar &c) const
{
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

TextEditor::IAssistProcessor *MAVLinkCompletionProvider::createProcessor(
    const TextEditor::AssistInterface *assistInterface) const
{
    Q_UNUSED(assistInterface)
    return new MAVLinkCompletionProcessor;
}

// MAVLinkCompletionProcessor implementation

MAVLinkCompletionProcessor::MAVLinkCompletionProcessor()
{
    loadMAVLinkMessages();
}

MAVLinkCompletionProcessor::~MAVLinkCompletionProcessor() = default;

void MAVLinkCompletionProcessor::loadMAVLinkMessages()
{
    if (s_messagesLoaded)
        return;

    s_messages = kCommonMessages;
    s_messagesLoaded = true;

    // TODO: Parse actual MAVLink definitions from project if available
    // This would require finding the mavlink submodule and parsing XML
}

QString MAVLinkCompletionProcessor::getCurrentPrefix() const
{
    if (!interface())
        return {};

    int pos = interface()->position();
    const QString &text = interface()->textDocument()->toPlainText();

    // Walk backwards to find the start of the identifier
    int start = pos;
    while (start > 0) {
        QChar c = text.at(start - 1);
        if (!c.isLetterOrNumber() && c != QLatin1Char('_'))
            break;
        --start;
    }

    return text.mid(start, pos - start);
}

QString MAVLinkCompletionProcessor::getCurrentContext() const
{
    if (!interface())
        return {};

    int pos = interface()->position();
    const QString &text = interface()->textDocument()->toPlainText();

    // Get the current line for context analysis
    int lineStart = text.lastIndexOf(QLatin1Char('\n'), pos - 1) + 1;
    int lineEnd = text.indexOf(QLatin1Char('\n'), pos);
    if (lineEnd < 0) lineEnd = text.length();

    return text.mid(lineStart, lineEnd - lineStart);
}

TextEditor::IAssistProposal *MAVLinkCompletionProcessor::perform()
{
    if (!interface())
        return nullptr;

    QString prefix = getCurrentPrefix();
    QString context = getCurrentContext();

    // Check what kind of completion to provide
    if (prefix.startsWith(QLatin1String("MAVLINK_MSG_ID_"), Qt::CaseInsensitive)) {
        return createMessageIdProposal(prefix.mid(15)); // Remove "MAVLINK_MSG_ID_"
    }

    if (prefix.startsWith(QLatin1String("mavlink_msg_"), Qt::CaseInsensitive)) {
        return createDecodeProposal(prefix.mid(12)); // Remove "mavlink_msg_"
    }

    // Check if we're in a switch statement on message.msgid
    static QRegularExpression switchRegex(
        QStringLiteral(R"(switch\s*\(\s*\w+\.msgid\s*\))"));
    if (context.contains(switchRegex) ||
        context.contains(QLatin1String("case MAVLINK"))) {
        return createMessageIdProposal(prefix);
    }

    // Check if typing just "MAVLINK" or "mavlink"
    if (prefix.compare(QLatin1String("MAVLINK"), Qt::CaseInsensitive) == 0 ||
        prefix.startsWith(QLatin1String("MAVLINK_"), Qt::CaseInsensitive)) {
        return createMessageIdProposal({});
    }

    if (prefix.compare(QLatin1String("mavlink"), Qt::CaseInsensitive) == 0 ||
        prefix.startsWith(QLatin1String("mavlink_"), Qt::CaseInsensitive)) {
        return createDecodeProposal({});
    }

    return nullptr;
}

TextEditor::IAssistProposal *MAVLinkCompletionProcessor::createMessageIdProposal(
    const QString &prefix)
{
    QList<TextEditor::AssistProposalItemInterface *> items;

    for (const auto &msg : std::as_const(s_messages)) {
        if (!prefix.isEmpty() &&
            !msg.name.startsWith(prefix, Qt::CaseInsensitive)) {
            continue;
        }

        auto *item = new TextEditor::AssistProposalItem;
        item->setText(QStringLiteral("MAVLINK_MSG_ID_%1").arg(msg.name));
        item->setDetail(QStringLiteral("[%1] %2").arg(msg.id).arg(msg.description));
        item->setIcon(QIcon::fromTheme(QStringLiteral("code-variable")));
        items.append(item);
    }

    if (items.isEmpty())
        return nullptr;

    auto *model = new TextEditor::GenericProposalModel;
    model->loadContent(items);

    // Calculate base position for the proposal
    int basePosition = interface()->position() - getCurrentPrefix().length();

    return new TextEditor::GenericProposal(basePosition, model);
}

TextEditor::IAssistProposal *MAVLinkCompletionProcessor::createDecodeProposal(
    const QString &prefix)
{
    QList<TextEditor::AssistProposalItemInterface *> items;

    for (const auto &msg : std::as_const(s_messages)) {
        QString lowerName = msg.name.toLower();

        if (!prefix.isEmpty() &&
            !lowerName.startsWith(prefix, Qt::CaseInsensitive)) {
            continue;
        }

        // Add decode function
        auto *decodeItem = new TextEditor::AssistProposalItem;
        decodeItem->setText(QStringLiteral("mavlink_msg_%1_decode").arg(lowerName));
        decodeItem->setDetail(QStringLiteral("Decode %1 message").arg(msg.name));
        decodeItem->setIcon(QIcon::fromTheme(QStringLiteral("code-function")));
        items.append(decodeItem);

        // Add getter functions for each field
        for (const QString &field : msg.fields) {
            auto *fieldItem = new TextEditor::AssistProposalItem;
            fieldItem->setText(QStringLiteral("mavlink_msg_%1_get_%2").arg(lowerName, field));
            fieldItem->setDetail(QStringLiteral("Get %1.%2").arg(msg.name, field));
            fieldItem->setIcon(QIcon::fromTheme(QStringLiteral("code-function")));
            items.append(fieldItem);
        }
    }

    if (items.isEmpty())
        return nullptr;

    auto *model = new TextEditor::GenericProposalModel;
    model->loadContent(items);

    int basePosition = interface()->position() - getCurrentPrefix().length();

    return new TextEditor::GenericProposal(basePosition, model);
}

TextEditor::IAssistProposal *MAVLinkCompletionProcessor::createFieldProposal(
    const QString &messageName)
{
    QList<TextEditor::AssistProposalItemInterface *> items;

    for (const auto &msg : std::as_const(s_messages)) {
        if (msg.name.compare(messageName, Qt::CaseInsensitive) != 0)
            continue;

        for (const QString &field : msg.fields) {
            auto *item = new TextEditor::AssistProposalItem;
            item->setText(field);
            item->setDetail(QStringLiteral("Field of %1").arg(msg.name));
            item->setIcon(QIcon::fromTheme(QStringLiteral("code-field")));
            items.append(item);
        }
        break;
    }

    if (items.isEmpty())
        return nullptr;

    auto *model = new TextEditor::GenericProposalModel;
    model->loadContent(items);

    int basePosition = interface()->position() - getCurrentPrefix().length();

    return new TextEditor::GenericProposal(basePosition, model);
}

} // namespace QGC::Internal
