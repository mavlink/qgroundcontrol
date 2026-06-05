#include "APMDataFlashLogParser.h"

#include "APMDataFlashUtility.h"
#include "QGCLoggingCategory.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFile>
#include <QtCore/QFutureWatcher>
#include <QtCore/QHash>
#include <QtCore/QSet>

#include <algorithm>
#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(APMDataFlashLogParserLog, "AnalyzeView.APMDataFlashLogParser")

namespace {
struct ParseResult {
    bool ok = false;
    QString errorMessage;
    QStringList availableFields;
    QStringList plottableFields;
    QVariantList parameters;
    QVariantList events;
    QVariantList messages;
    QVariantList modeSegments;
    QHash<QString, QVector<QPointF>> fieldSamples;
    double minTimestamp = -1.0;
    double maxTimestamp = -1.0;
    int sampleCount = 0;
    QString detectedVehicleType;
};

QString _vehicleTypeFromMessageText(const QString &messageText)
{
    const QString text = messageText.toLower();
    if (text.contains(QStringLiteral("arducopter"))) {
        return QStringLiteral("ArduCopter");
    }
    if (text.contains(QStringLiteral("arduplane"))) {
        return QStringLiteral("ArduPlane");
    }
    if (text.contains(QStringLiteral("ardurover"))) {
        return QStringLiteral("ArduRover");
    }
    if (text.contains(QStringLiteral("ardusub"))) {
        return QStringLiteral("ArduSub");
    }

    return QString();
}

QString _ardupilotModeName(const QString &vehicleType, int modeNumber)
{
    static const QHash<int, QString> planeModes = {
        {0, QStringLiteral("Manual")},
        {1, QStringLiteral("CIRCLE")},
        {2, QStringLiteral("STABILIZE")},
        {3, QStringLiteral("TRAINING")},
        {4, QStringLiteral("ACRO")},
        {5, QStringLiteral("FBWA")},
        {6, QStringLiteral("FBWB")},
        {7, QStringLiteral("CRUISE")},
        {8, QStringLiteral("AUTOTUNE")},
        {10, QStringLiteral("Auto")},
        {11, QStringLiteral("RTL")},
        {12, QStringLiteral("Loiter")},
        {13, QStringLiteral("TAKEOFF")},
        {14, QStringLiteral("AVOID_ADSB")},
        {15, QStringLiteral("Guided")},
        {17, QStringLiteral("QSTABILIZE")},
        {18, QStringLiteral("QHOVER")},
        {19, QStringLiteral("QLOITER")},
        {20, QStringLiteral("QLAND")},
        {21, QStringLiteral("QRTL")},
        {22, QStringLiteral("QAUTOTUNE")},
        {23, QStringLiteral("QACRO")},
        {24, QStringLiteral("THERMAL")},
        {25, QStringLiteral("Loiter to QLand")},
        {26, QStringLiteral("AUTOLAND")},
    };
    static const QHash<int, QString> copterModes = {
        {0, QStringLiteral("Stabilize")},
        {1, QStringLiteral("Acro")},
        {2, QStringLiteral("AltHold")},
        {3, QStringLiteral("Auto")},
        {4, QStringLiteral("Guided")},
        {5, QStringLiteral("Loiter")},
        {6, QStringLiteral("RTL")},
        {7, QStringLiteral("Circle")},
        {9, QStringLiteral("Land")},
        {11, QStringLiteral("Drift")},
        {13, QStringLiteral("Sport")},
        {14, QStringLiteral("Flip")},
        {15, QStringLiteral("AutoTune")},
        {16, QStringLiteral("PosHold")},
        {17, QStringLiteral("Brake")},
        {18, QStringLiteral("Throw")},
        {19, QStringLiteral("Avoid_ADSB")},
        {20, QStringLiteral("Guided_NoGPS")},
        {21, QStringLiteral("Smart_RTL")},
        {22, QStringLiteral("FlowHold")},
        {23, QStringLiteral("Follow")},
        {24, QStringLiteral("ZigZag")},
        {25, QStringLiteral("SystemID")},
        {26, QStringLiteral("Heli_Autorotate")},
        {27, QStringLiteral("Auto RTL")},
        {28, QStringLiteral("Turtle")},
    };

    if (vehicleType == QStringLiteral("ArduPlane")) {
        return planeModes.value(modeNumber, QStringLiteral("Mode %1").arg(modeNumber));
    }
    if (vehicleType == QStringLiteral("ArduCopter")) {
        return copterModes.value(modeNumber, QStringLiteral("Mode %1").arg(modeNumber));
    }

    // Numeric ArduPilot mode values are vehicle-specific. Without reliable
    // vehicle-type detection, preserve numeric representation.
    return QStringLiteral("Mode %1").arg(modeNumber);
}

QString _ardupilotErrDescription(int subsystem, int ecode)
{
    QString subsystemName;
    switch (subsystem) {
    case 1:  subsystemName = APMDataFlashLogParser::tr("Main"); break;
    case 2:  subsystemName = APMDataFlashLogParser::tr("Radio"); break;
    case 3:  subsystemName = APMDataFlashLogParser::tr("Compass"); break;
    case 4:  subsystemName = APMDataFlashLogParser::tr("Optflow"); break;
    case 5:  subsystemName = APMDataFlashLogParser::tr("Radio Failsafe"); break;
    case 6:  subsystemName = APMDataFlashLogParser::tr("Battery Failsafe"); break;
    case 7:  subsystemName = APMDataFlashLogParser::tr("GPS Failsafe"); break;
    case 8:  subsystemName = APMDataFlashLogParser::tr("GCS Failsafe"); break;
    case 9:  subsystemName = APMDataFlashLogParser::tr("Fence Failsafe"); break;
    case 10: subsystemName = APMDataFlashLogParser::tr("Flight mode"); break;
    case 11: subsystemName = APMDataFlashLogParser::tr("GPS"); break;
    case 12: subsystemName = APMDataFlashLogParser::tr("Crash Check"); break;
    case 13: subsystemName = APMDataFlashLogParser::tr("Flip"); break;
    case 14: subsystemName = APMDataFlashLogParser::tr("Autotune"); break;
    case 15: subsystemName = APMDataFlashLogParser::tr("Parachute"); break;
    case 16: subsystemName = APMDataFlashLogParser::tr("EKF Check"); break;
    case 17: subsystemName = APMDataFlashLogParser::tr("EKF Failsafe"); break;
    case 18: subsystemName = APMDataFlashLogParser::tr("Barometer"); break;
    case 19: subsystemName = APMDataFlashLogParser::tr("CPU Load Watchdog"); break;
    case 20: subsystemName = APMDataFlashLogParser::tr("ADSB Failsafe"); break;
    case 21: subsystemName = APMDataFlashLogParser::tr("Terrain Data"); break;
    case 22: subsystemName = APMDataFlashLogParser::tr("Navigation"); break;
    case 23: subsystemName = APMDataFlashLogParser::tr("Terrain Failsafe"); break;
    case 24: subsystemName = APMDataFlashLogParser::tr("EKF Primary"); break;
    case 25: subsystemName = APMDataFlashLogParser::tr("Thrust Loss Check"); break;
    case 26: subsystemName = APMDataFlashLogParser::tr("Sensor Failsafe"); break;
    case 27: subsystemName = APMDataFlashLogParser::tr("Leak Failsafe"); break;
    case 28: subsystemName = APMDataFlashLogParser::tr("Pilot Input"); break;
    case 29: subsystemName = APMDataFlashLogParser::tr("Vibration Failsafe"); break;
    case 30: subsystemName = APMDataFlashLogParser::tr("Internal Error"); break;
    case 31: subsystemName = APMDataFlashLogParser::tr("Deadreckon Failsafe"); break;
    default: subsystemName = APMDataFlashLogParser::tr("Subsystem %1").arg(subsystem); break;
    }

    QString ecodeText;
    switch (subsystem) {
    case 1: // MAIN
        if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("INS delay"); }
        break;
    case 2: // RADIO
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Errors Resolved"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Late Frame"); }
        break;
    case 3: // COMPASS
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Errors Resolved"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Failed to initialise"); }
        else if (ecode == 4) { ecodeText = APMDataFlashLogParser::tr("Unhealthy"); }
        break;
    case 5: // FAILSAFE_RADIO
    case 6: // FAILSAFE_BATT
    case 7: // FAILSAFE_GPS
    case 8: // FAILSAFE_GCS
    case 17: // FAILSAFE_EKFINAV
    case 19: // CPU
    case 23: // FAILSAFE_TERRAIN
    case 26: // FAILSAFE_SENSORS
    case 27: // FAILSAFE_LEAK
    case 28: // PILOT_INPUT
    case 31: // FAILSAFE_DEADRECKON
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Failsafe Resolved"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Failsafe Triggered"); }
        break;
    case 9: // FAILSAFE_FENCE
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Failsafe Resolved"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Altitude fence breach"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Circular fence breach"); }
        else if (ecode == 3) { ecodeText = APMDataFlashLogParser::tr("Altitude and circular fence breach"); }
        else if (ecode == 4) { ecodeText = APMDataFlashLogParser::tr("Polygon fence breach"); }
        break;
    case 10: // FLIGHT_MODE
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Flight mode change failure"); }
        break;
    case 11: // GPS
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Glitch cleared"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("GPS glitch occurred"); }
        break;
    case 12: // CRASH_CHECK
        if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Crash into ground detected"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Loss of control detected"); }
        break;
    case 13: // FLIP
        if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Flip abandoned"); }
        break;
    case 15: // PARACHUTES
        if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Parachute not deployed (too low)"); }
        else if (ecode == 3) { ecodeText = APMDataFlashLogParser::tr("Parachute not deployed (landed)"); }
        break;
    case 16: // EKFCHECK
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Variance cleared"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Bad variance"); }
        break;
    case 18: // BARO
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Errors Resolved"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Baro glitch"); }
        else if (ecode == 3) { ecodeText = APMDataFlashLogParser::tr("Bad depth"); }
        else if (ecode == 4) { ecodeText = APMDataFlashLogParser::tr("Unhealthy"); }
        break;
    case 20: // FAILSAFE_ADSB
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Failsafe Resolved"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("No action report only"); }
        else if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Avoid by climb/descend"); }
        else if (ecode == 3) { ecodeText = APMDataFlashLogParser::tr("Avoid by horizontal move"); }
        else if (ecode == 4) { ecodeText = APMDataFlashLogParser::tr("Avoid perpendicular move"); }
        else if (ecode == 5) { ecodeText = APMDataFlashLogParser::tr("RTL invoked"); }
        break;
    case 21: // TERRAIN
        if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Missing terrain data"); }
        break;
    case 22: // NAVIGATION
        if (ecode == 2) { ecodeText = APMDataFlashLogParser::tr("Failed to set destination"); }
        else if (ecode == 3) { ecodeText = APMDataFlashLogParser::tr("RTL restarted"); }
        else if (ecode == 4) { ecodeText = APMDataFlashLogParser::tr("Circle initialisation failed"); }
        else if (ecode == 5) { ecodeText = APMDataFlashLogParser::tr("Destination outside fence"); }
        else if (ecode == 6) { ecodeText = APMDataFlashLogParser::tr("RTL missing rangefinder"); }
        break;
    case 24: // EKF_PRIMARY
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("1st EKF became primary"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("2nd EKF became primary"); }
        break;
    case 25: // THRUST_LOSS_CHECK
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Thrust restored"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Thrust loss detected"); }
        break;
    case 29: // FAILSAFE_VIBE
        if (ecode == 0) { ecodeText = APMDataFlashLogParser::tr("Excessive vibration compensation de-activated"); }
        else if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Excessive vibration compensation activated"); }
        break;
    case 30: // INTERNAL_ERROR
        if (ecode == 1) { ecodeText = APMDataFlashLogParser::tr("Internal errors detected"); }
        break;
    default:
        break;
    }

    if (ecodeText.isEmpty()) {
        ecodeText = APMDataFlashLogParser::tr("Code %1").arg(ecode);
    }

    return QStringLiteral("%1 (%2): %3 (%4)").arg(subsystemName).arg(subsystem).arg(ecodeText).arg(ecode);
}

QString _ardupilotEventDescription(int eventId)
{
    switch (eventId) {
    case 10: return APMDataFlashLogParser::tr("Armed");
    case 11: return APMDataFlashLogParser::tr("Disarmed");
    case 15: return APMDataFlashLogParser::tr("Auto Armed");
    case 17: return APMDataFlashLogParser::tr("Land Complete Maybe");
    case 18: return APMDataFlashLogParser::tr("Land Complete");
    case 19: return APMDataFlashLogParser::tr("Lost GPS");
    case 21: return APMDataFlashLogParser::tr("Flip Start");
    case 22: return APMDataFlashLogParser::tr("Flip End");
    case 25: return APMDataFlashLogParser::tr("Set Home");
    case 26: return APMDataFlashLogParser::tr("Simple Mode Enabled");
    case 27: return APMDataFlashLogParser::tr("Simple Mode Disabled");
    case 28: return APMDataFlashLogParser::tr("Not Landed");
    case 29: return APMDataFlashLogParser::tr("Super Simple Mode Enabled");
    case 30: return APMDataFlashLogParser::tr("AutoTune Initialised");
    case 31: return APMDataFlashLogParser::tr("AutoTune Off");
    case 32: return APMDataFlashLogParser::tr("AutoTune Restart");
    case 33: return APMDataFlashLogParser::tr("AutoTune Success");
    case 34: return APMDataFlashLogParser::tr("AutoTune Failed");
    case 35: return APMDataFlashLogParser::tr("AutoTune Reached Limit");
    case 36: return APMDataFlashLogParser::tr("AutoTune Pilot Testing");
    case 37: return APMDataFlashLogParser::tr("AutoTune Saved Gains");
    case 38: return APMDataFlashLogParser::tr("Save Trim");
    case 39: return APMDataFlashLogParser::tr("Save Waypoint Add");
    case 41: return APMDataFlashLogParser::tr("Fence Enabled");
    case 42: return APMDataFlashLogParser::tr("Fence Disabled");
    case 43: return APMDataFlashLogParser::tr("Acro Trainer Off");
    case 44: return APMDataFlashLogParser::tr("Acro Trainer Leveling");
    case 45: return APMDataFlashLogParser::tr("Acro Trainer Limited");
    case 46: return APMDataFlashLogParser::tr("Gripper Grab");
    case 47: return APMDataFlashLogParser::tr("Gripper Release");
    case 49: return APMDataFlashLogParser::tr("Parachute Disabled");
    case 50: return APMDataFlashLogParser::tr("Parachute Enabled");
    case 51: return APMDataFlashLogParser::tr("Parachute Released");
    case 52: return APMDataFlashLogParser::tr("Landing Gear Deployed");
    case 53: return APMDataFlashLogParser::tr("Landing Gear Retracted");
    case 54: return APMDataFlashLogParser::tr("Motors Emergency Stopped");
    case 55: return APMDataFlashLogParser::tr("Motors Emergency Stop Cleared");
    case 56: return APMDataFlashLogParser::tr("Motors Interlock Disabled");
    case 57: return APMDataFlashLogParser::tr("Motors Interlock Enabled");
    case 58: return APMDataFlashLogParser::tr("Rotor Runup Complete");
    case 59: return APMDataFlashLogParser::tr("Rotor Speed Below Critical");
    case 60: return APMDataFlashLogParser::tr("EKF Altitude Reset");
    case 61: return APMDataFlashLogParser::tr("Land Cancelled By Pilot");
    case 62: return APMDataFlashLogParser::tr("EKF Yaw Reset");
    case 63: return APMDataFlashLogParser::tr("ADSB Avoidance Enabled");
    case 64: return APMDataFlashLogParser::tr("ADSB Avoidance Disabled");
    case 65: return APMDataFlashLogParser::tr("Proximity Avoidance Enabled");
    case 66: return APMDataFlashLogParser::tr("Proximity Avoidance Disabled");
    case 67: return APMDataFlashLogParser::tr("GPS Primary Changed");
    case 71: return APMDataFlashLogParser::tr("ZigZag Store A");
    case 72: return APMDataFlashLogParser::tr("ZigZag Store B");
    case 73: return APMDataFlashLogParser::tr("Land Repo Active");
    case 74: return APMDataFlashLogParser::tr("Standby Enabled");
    case 75: return APMDataFlashLogParser::tr("Standby Disabled");
    case 76: return APMDataFlashLogParser::tr("Fence Alt Max Enabled");
    case 77: return APMDataFlashLogParser::tr("Fence Alt Max Disabled");
    case 78: return APMDataFlashLogParser::tr("Fence Circle Enabled");
    case 79: return APMDataFlashLogParser::tr("Fence Circle Disabled");
    case 80: return APMDataFlashLogParser::tr("Fence Alt Min Enabled");
    case 81: return APMDataFlashLogParser::tr("Fence Alt Min Disabled");
    case 82: return APMDataFlashLogParser::tr("Fence Polygon Enabled");
    case 83: return APMDataFlashLogParser::tr("Fence Polygon Disabled");
    case 85: return APMDataFlashLogParser::tr("EK3 Source Set: Primary");
    case 86: return APMDataFlashLogParser::tr("EK3 Source Set: Secondary");
    case 87: return APMDataFlashLogParser::tr("EK3 Source Set: Tertiary");
    case 90: return APMDataFlashLogParser::tr("Airspeed Primary Changed");
    case 163: return APMDataFlashLogParser::tr("Surfaced");
    case 164: return APMDataFlashLogParser::tr("Not Surfaced");
    case 165: return APMDataFlashLogParser::tr("Bottomed");
    case 166: return APMDataFlashLogParser::tr("Not Bottomed");
    default: return APMDataFlashLogParser::tr("Event %1").arg(eventId);
    }
}

double _extractTimestampSeconds(const QMap<QString, QVariant> &values)
{
    if (values.contains(QStringLiteral("TimeUS"))) {
        return values.value(QStringLiteral("TimeUS")).toDouble() / 1000000.0;
    }
    if (values.contains(QStringLiteral("TimeMS"))) {
        return values.value(QStringLiteral("TimeMS")).toDouble() / 1000.0;
    }
    if (values.contains(QStringLiteral("Time"))) {
        return values.value(QStringLiteral("Time")).toDouble() / 1000.0;
    }

    return -1.0;
}

void _appendEvent(QVariantList &events, double timestampSecs, const QString &type, const QString &description)
{
    if ((timestampSecs < 0.0) || description.isEmpty()) {
        return;
    }

    QVariantMap eventRow;
    eventRow[QStringLiteral("time")] = timestampSecs;
    eventRow[QStringLiteral("type")] = type;
    eventRow[QStringLiteral("description")] = description;
    events.append(eventRow);
}

ParseResult _parseDataFlashFile(const QString &filePath)
{
    ParseResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = APMDataFlashLogParser::tr("Failed to open DataFlash file");
        return result;
    }

    const qint64 fileSize = file.size();
    if (fileSize <= 0) {
        result.errorMessage = APMDataFlashLogParser::tr("DataFlash file is empty");
        return result;
    }
    if (fileSize > std::numeric_limits<qsizetype>::max()) {
        result.errorMessage = APMDataFlashLogParser::tr("DataFlash file is too large to parse");
        return result;
    }

    uchar *const mappedData = file.map(0, fileSize);
    if (mappedData == nullptr) {
        result.errorMessage = APMDataFlashLogParser::tr("Failed to memory-map DataFlash file");
        return result;
    }

    struct ScopedFileUnmap {
        QFile &file;
        uchar *data = nullptr;
        ~ScopedFileUnmap()
        {
            if (data != nullptr) {
                file.unmap(data);
            }
        }
    } scopedFileUnmap{file, mappedData};

    const QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char *>(mappedData), static_cast<qsizetype>(fileSize));

    QMap<uint8_t, APMDataFlashUtility::MessageFormat> formats;
    if (!APMDataFlashUtility::parseFmtMessages(bytes.constData(), bytes.size(), formats)) {
        result.errorMessage = APMDataFlashLogParser::tr("No valid FMT messages were found");
        return result;
    }

    QSet<QString> signalSet;
    QSet<QString> plottableSet;
    double minTimestampSecs = -1.0;
    double maxTimestampSecs = -1.0;
    bool hasOpenModeSegment = false;
    double modeSegmentStartSecs = -1.0;
    QString currentModeName;

    // Precompute, once per format, the fully-qualified signal names ("<msg>.<col>")
    // and a lookup map from column name to that pre-formatted signal name. Building
    // these names per-message via QString::arg() previously dominated parse cost.
    QHash<uint8_t, QHash<QString, QString>> signalNameByCol;
    signalNameByCol.reserve(formats.size());
    for (auto fmtIt = formats.cbegin(); fmtIt != formats.cend(); ++fmtIt) {
        const APMDataFlashUtility::MessageFormat &fmt = fmtIt.value();
        QHash<QString, QString> &perCol = signalNameByCol[fmtIt.key()];
        perCol.reserve(fmt.columns.size());
        for (const QString &col : fmt.columns) {
            perCol.insert(col, fmt.name + QLatin1Char('.') + col);
        }
    }

    static const QString kPARM = QStringLiteral("PARM");
    static const QString kMSG  = QStringLiteral("MSG");
    static const QString kMODE = QStringLiteral("MODE");
    static const QString kERR  = QStringLiteral("ERR");
    static const QString kEV   = QStringLiteral("EV");

    APMDataFlashUtility::iterateMessages(bytes.constData(), bytes.size(), formats, [&result, &signalSet, &plottableSet, &minTimestampSecs, &maxTimestampSecs, &hasOpenModeSegment, &modeSegmentStartSecs, &currentModeName, &signalNameByCol](uint8_t msgType, const char *payload, int, const APMDataFlashUtility::MessageFormat &fmt) {
        const QMap<QString, QVariant> values = APMDataFlashUtility::parseMessage(payload, fmt);
        const double timestampSecs = _extractTimestampSeconds(values);
        if (timestampSecs >= 0.0) {
            if (minTimestampSecs < 0.0 || timestampSecs < minTimestampSecs) {
                minTimestampSecs = timestampSecs;
            }
            maxTimestampSecs = std::max(maxTimestampSecs, timestampSecs);
        }

        if (fmt.name == kPARM) {
            const QString paramName = values.value(QStringLiteral("Name")).toString();
            const QVariant paramValue = values.contains(QStringLiteral("Value")) ? values.value(QStringLiteral("Value")) : values.value(QStringLiteral("Val"));
            if (!paramName.isEmpty()) {
                QVariantMap row;
                row[QStringLiteral("name")] = paramName;
                row[QStringLiteral("value")] = paramValue;
                result.parameters.append(row);
            }
        } else if (fmt.name == kMSG) {
            const QString text = values.value(QStringLiteral("Message")).toString();
            const QString detected = _vehicleTypeFromMessageText(text);
            if (result.detectedVehicleType.isEmpty() && !detected.isEmpty()) {
                result.detectedVehicleType = detected;
            }
            if (!text.isEmpty()) {
                QVariantMap row;
                row[QStringLiteral("time")] = timestampSecs;
                row[QStringLiteral("text")] = text;
                result.messages.append(row);
            }
        } else if (fmt.name == kMODE) {
            QString modeName = values.value(QStringLiteral("Mode")).toString();
            bool isNumericMode = false;
            const int modeNumber = modeName.toInt(&isNumericMode);
            if (isNumericMode) {
                modeName = _ardupilotModeName(result.detectedVehicleType, modeNumber);
            } else if (modeName.isEmpty()) {
                modeName = APMDataFlashLogParser::tr("Unknown");
            }
            _appendEvent(result.events, timestampSecs, QStringLiteral("mode"), APMDataFlashLogParser::tr("Mode: %1").arg(modeName));
            if (timestampSecs >= 0.0) {
                if (hasOpenModeSegment && (timestampSecs > modeSegmentStartSecs)) {
                    QVariantMap segment;
                    segment[QStringLiteral("mode")] = currentModeName;
                    segment[QStringLiteral("start")] = modeSegmentStartSecs;
                    segment[QStringLiteral("end")] = timestampSecs;
                    result.modeSegments.append(segment);
                }
                hasOpenModeSegment = true;
                modeSegmentStartSecs = timestampSecs;
                currentModeName = modeName;
            }
        } else if (fmt.name == kERR) {
            const int subsystem = values.value(QStringLiteral("Subsys")).toInt();
            const int ecode = values.value(QStringLiteral("ECode")).toInt();
            _appendEvent(result.events, timestampSecs, QStringLiteral("error"), _ardupilotErrDescription(subsystem, ecode));
        } else if (fmt.name == kEV) {
            const int eventId = values.value(QStringLiteral("Id"), values.value(QStringLiteral("Event"))).toInt();
            _appendEvent(result.events, timestampSecs, QStringLiteral("event"), _ardupilotEventDescription(eventId));
        }

        result.sampleCount++;

        // Single pass: lookup precomputed signal names, populate signal/plottable
        // sets, and append numeric samples in one walk over the values map.
        const QHash<QString, QString> &perCol = signalNameByCol[msgType];
        const bool haveTimestamp = (timestampSecs >= 0.0);
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            const auto nameIt = perCol.constFind(it.key());
            if (nameIt == perCol.constEnd()) {
                continue;
            }
            const QString &signalName = nameIt.value();
            signalSet.insert(signalName);

            if (!haveTimestamp) {
                continue;
            }
            const int typeId = it.value().metaType().id();
            const bool numeric = (typeId == QMetaType::Int) || (typeId == QMetaType::UInt) ||
                                 (typeId == QMetaType::LongLong) || (typeId == QMetaType::ULongLong) ||
                                 (typeId == QMetaType::Float) || (typeId == QMetaType::Double);
            if (numeric) {
                result.fieldSamples[signalName].append(QPointF(timestampSecs, it.value().toDouble()));
                plottableSet.insert(signalName);
            }
        }

        return true;
    });

    if (hasOpenModeSegment && (maxTimestampSecs >= modeSegmentStartSecs)) {
        QVariantMap segment;
        segment[QStringLiteral("mode")] = currentModeName;
        segment[QStringLiteral("start")] = modeSegmentStartSecs;
        segment[QStringLiteral("end")] = maxTimestampSecs;
        result.modeSegments.append(segment);
    }

    result.availableFields = signalSet.values();
    std::sort(result.availableFields.begin(), result.availableFields.end());
    result.plottableFields = plottableSet.values();
    std::sort(result.plottableFields.begin(), result.plottableFields.end());
    result.minTimestamp = minTimestampSecs;
    result.maxTimestamp = maxTimestampSecs;
    result.ok = true;
    return result;
}
} // namespace

APMDataFlashLogParser::APMDataFlashLogParser(QObject *parent)
    : QObject(parent)
{
    qCDebug(APMDataFlashLogParserLog) << this;
}

APMDataFlashLogParser::~APMDataFlashLogParser()
{
    qCDebug(APMDataFlashLogParserLog) << this;
}

bool APMDataFlashLogParser::parseFile(const QString &filePath)
{
    ++_parseRequestId; // Invalidate any in-flight async parse results.
    clear();
    const ParseResult result = _parseDataFlashFile(filePath);
    if (!result.ok) {
        _setParseError(result.errorMessage);
        return false;
    }

    _availableFields = result.availableFields;
    _plottableFields = result.plottableFields;
    _parameters = result.parameters;
    _events = result.events;
    _messages = result.messages;
    _modeSegments = result.modeSegments;
    _fieldSamples = result.fieldSamples;
    _sampleCount = result.sampleCount;
    _detectedVehicleType = result.detectedVehicleType;
    emit availableFieldsChanged();
    emit plottableFieldsChanged();
    emit parametersChanged();
    emit eventsChanged();
    emit messagesChanged();
    emit modeSegmentsChanged();
    emit detectedVehicleTypeChanged();
    if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
        _minTimestamp = result.minTimestamp;
        _maxTimestamp = result.maxTimestamp;
        emit timeRangeChanged();
    }
    emit sampleCountChanged();

    _parseComplete = true;
    emit parseCompleteChanged();
    qCDebug(APMDataFlashLogParserLog) << "Parsed fields" << _availableFields.count() << "parameters" << _parameters.count() << "events" << _events.count();
    return true;
}

void APMDataFlashLogParser::parseFileAsync(const QString &filePath)
{
    const quint64 requestId = ++_parseRequestId;
    clear();

    auto *watcher = new QFutureWatcher<ParseResult>(this);
    (void) connect(watcher, &QFutureWatcher<ParseResult>::finished, this, [this, watcher, filePath, requestId]() {
        const ParseResult result = watcher->result();
        watcher->deleteLater();

        if (requestId != _parseRequestId) {
            // A newer parse request has superseded this result.
            return;
        }

        if (!result.ok) {
            _setParseError(result.errorMessage);
            emit parseFileFinished(filePath, false, result.errorMessage);
            return;
        }

        _availableFields = result.availableFields;
        _plottableFields = result.plottableFields;
        _parameters = result.parameters;
        _events = result.events;
        _messages = result.messages;
        _modeSegments = result.modeSegments;
        _fieldSamples = result.fieldSamples;
        _sampleCount = result.sampleCount;
        _detectedVehicleType = result.detectedVehicleType;
        emit availableFieldsChanged();
        emit plottableFieldsChanged();
        emit parametersChanged();
        emit eventsChanged();
        emit messagesChanged();
        emit modeSegmentsChanged();
        emit detectedVehicleTypeChanged();
        if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
            _minTimestamp = result.minTimestamp;
            _maxTimestamp = result.maxTimestamp;
            emit timeRangeChanged();
        }
        emit sampleCountChanged();

        _parseComplete = true;
        emit parseCompleteChanged();
        emit parseFileFinished(filePath, true, QString());
    });

    watcher->setFuture(QtConcurrent::run([filePath]() {
        return _parseDataFlashFile(filePath);
    }));
}

void APMDataFlashLogParser::clear()
{
    const bool oldParseComplete = _parseComplete;
    _parseComplete = false;
    if (oldParseComplete) {
        emit parseCompleteChanged();
    }

    if (!_parseError.isEmpty()) {
        _parseError.clear();
        emit parseErrorChanged();
    }

    if (!_availableFields.isEmpty()) {
        _availableFields.clear();
        emit availableFieldsChanged();
    }

    if (!_parameters.isEmpty()) {
        _parameters.clear();
        emit parametersChanged();
    }

    if (!_events.isEmpty()) {
        _events.clear();
        emit eventsChanged();
    }

    if (!_messages.isEmpty()) {
        _messages.clear();
        emit messagesChanged();
    }

    if (!_modeSegments.isEmpty()) {
        _modeSegments.clear();
        emit modeSegmentsChanged();
    }

    if (!_detectedVehicleType.isEmpty()) {
        _detectedVehicleType.clear();
        emit detectedVehicleTypeChanged();
    }

    if (!_plottableFields.isEmpty()) {
        _plottableFields.clear();
        emit plottableFieldsChanged();
    }

    _fieldSamples.clear();
    if (_minTimestamp != -1.0 || _maxTimestamp != -1.0) {
        _minTimestamp = -1.0;
        _maxTimestamp = -1.0;
        emit timeRangeChanged();
    }

    if (_sampleCount != 0) {
        _sampleCount = 0;
        emit sampleCountChanged();
    }
}

QVariantList APMDataFlashLogParser::fieldSamples(const QString &fieldName) const
{
    QVariantList output;
    const auto fieldIt = _fieldSamples.constFind(fieldName);
    if (fieldIt == _fieldSamples.cend()) {
        return output;
    }

    const QVector<QPointF> &points = fieldIt.value();
    output.reserve(points.size());
    for (const QPointF &point : points) {
        output.append(point);
    }

    return output;
}

double APMDataFlashLogParser::fieldValueAt(const QString &fieldName, double timestampSeconds) const
{
    const auto fieldIt = _fieldSamples.constFind(fieldName);
    if ((fieldIt == _fieldSamples.cend()) || fieldIt->isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const QVector<QPointF> &points = fieldIt.value();
    const auto lower = std::lower_bound(points.cbegin(), points.cend(), timestampSeconds,
        [](const QPointF &point, double timestamp) {
            return point.x() < timestamp;
        });

    if (lower == points.cbegin()) {
        return lower->y();
    }

    if (lower == points.cend()) {
        return points.constLast().y();
    }

    const auto previous = std::prev(lower);
    const double lowerDistance = std::fabs(lower->x() - timestampSeconds);
    const double previousDistance = std::fabs(previous->x() - timestampSeconds);

    return (previousDistance <= lowerDistance)
        ? previous->y()
        : lower->y();
}

QString APMDataFlashLogParser::modeAt(double timestampSeconds) const
{
    for (const QVariant &variant : _modeSegments) {
        const QVariantMap segment = variant.toMap();
        const double start = segment.value(QStringLiteral("start")).toDouble();
        const double end = segment.value(QStringLiteral("end")).toDouble();
        if (timestampSeconds >= start && timestampSeconds <= end) {
            return segment.value(QStringLiteral("mode")).toString();
        }
    }

    return QString();
}

QVariantList APMDataFlashLogParser::eventsNear(double timestampSeconds, double thresholdSeconds) const
{
    QVariantList matches;
    const double threshold = std::max(0.0, thresholdSeconds);

    const auto lower = std::lower_bound(_events.cbegin(), _events.cend(), timestampSeconds - threshold,
        [](const QVariant &v, double t) {
            return v.toMap().value(QStringLiteral("time")).toDouble() < t;
        });
    for (auto it = lower; it != _events.cend(); ++it) {
        const QVariantMap eventItem = it->toMap();
        const double eventTime = eventItem.value(QStringLiteral("time")).toDouble();
        if (eventTime > timestampSeconds + threshold) {
            break;
        }
        matches.append(eventItem);
    }

    return matches;
}

void APMDataFlashLogParser::_setParseError(const QString &error)
{
    if (_parseError != error) {
        _parseError = error;
        emit parseErrorChanged();
    }
    qCWarning(APMDataFlashLogParserLog) << error;
}
