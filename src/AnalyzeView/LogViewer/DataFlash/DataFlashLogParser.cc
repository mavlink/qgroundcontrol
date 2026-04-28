#include "DataFlashLogParser.h"

#include "DataFlashUtility.h"
#include "QGCLoggingCategory.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QFile>
#include <QtCore/QFutureWatcher>
#include <QtCore/QHash>
#include <QtCore/QSet>

#include <algorithm>
#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(DataFlashLogParserLog, "AnalyzeView.DataFlashLogParser")

namespace {
struct ParseResult {
    bool ok = false;
    QString errorMessage;
    QStringList availableSignals;
    QStringList plottableSignals;
    QVariantList parameters;
    QVariantList events;
    QVariantList modeSegments;
    QHash<QString, QVector<QPointF>> signalSamples;
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
    case 1:  subsystemName = DataFlashLogParser::tr("Main"); break;
    case 2:  subsystemName = DataFlashLogParser::tr("Radio"); break;
    case 3:  subsystemName = DataFlashLogParser::tr("Compass"); break;
    case 4:  subsystemName = DataFlashLogParser::tr("Optflow"); break;
    case 5:  subsystemName = DataFlashLogParser::tr("Radio Failsafe"); break;
    case 6:  subsystemName = DataFlashLogParser::tr("Battery Failsafe"); break;
    case 7:  subsystemName = DataFlashLogParser::tr("GPS Failsafe"); break;
    case 8:  subsystemName = DataFlashLogParser::tr("GCS Failsafe"); break;
    case 9:  subsystemName = DataFlashLogParser::tr("Fence Failsafe"); break;
    case 10: subsystemName = DataFlashLogParser::tr("Flight mode"); break;
    case 11: subsystemName = DataFlashLogParser::tr("GPS"); break;
    case 12: subsystemName = DataFlashLogParser::tr("Crash Check"); break;
    case 13: subsystemName = DataFlashLogParser::tr("Flip"); break;
    case 14: subsystemName = DataFlashLogParser::tr("Autotune"); break;
    case 15: subsystemName = DataFlashLogParser::tr("Parachute"); break;
    case 16: subsystemName = DataFlashLogParser::tr("EKF Check"); break;
    case 17: subsystemName = DataFlashLogParser::tr("EKF Failsafe"); break;
    case 18: subsystemName = DataFlashLogParser::tr("Barometer"); break;
    case 19: subsystemName = DataFlashLogParser::tr("CPU Load Watchdog"); break;
    case 20: subsystemName = DataFlashLogParser::tr("ADSB Failsafe"); break;
    case 21: subsystemName = DataFlashLogParser::tr("Terrain Data"); break;
    case 22: subsystemName = DataFlashLogParser::tr("Navigation"); break;
    case 23: subsystemName = DataFlashLogParser::tr("Terrain Failsafe"); break;
    case 24: subsystemName = DataFlashLogParser::tr("EKF Primary"); break;
    case 25: subsystemName = DataFlashLogParser::tr("Thrust Loss Check"); break;
    case 26: subsystemName = DataFlashLogParser::tr("Sensor Failsafe"); break;
    case 27: subsystemName = DataFlashLogParser::tr("Leak Failsafe"); break;
    case 28: subsystemName = DataFlashLogParser::tr("Pilot Input"); break;
    case 29: subsystemName = DataFlashLogParser::tr("Vibration Failsafe"); break;
    case 30: subsystemName = DataFlashLogParser::tr("Internal Error"); break;
    case 31: subsystemName = DataFlashLogParser::tr("Deadreckon Failsafe"); break;
    default: subsystemName = DataFlashLogParser::tr("Subsystem %1").arg(subsystem); break;
    }

    QString ecodeText;
    switch (subsystem) {
    case 1: // MAIN
        if (ecode == 1) { ecodeText = DataFlashLogParser::tr("INS delay"); }
        break;
    case 2: // RADIO
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Errors Resolved"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Late Frame"); }
        break;
    case 3: // COMPASS
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Errors Resolved"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Failed to initialise"); }
        else if (ecode == 4) { ecodeText = DataFlashLogParser::tr("Unhealthy"); }
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
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Failsafe Resolved"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Failsafe Triggered"); }
        break;
    case 9: // FAILSAFE_FENCE
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Failsafe Resolved"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Altitude fence breach"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Circular fence breach"); }
        else if (ecode == 3) { ecodeText = DataFlashLogParser::tr("Altitude and circular fence breach"); }
        else if (ecode == 4) { ecodeText = DataFlashLogParser::tr("Polygon fence breach"); }
        break;
    case 10: // FLIGHT_MODE
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Flight mode change failure"); }
        break;
    case 11: // GPS
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Glitch cleared"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("GPS glitch occurred"); }
        break;
    case 12: // CRASH_CHECK
        if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Crash into ground detected"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Loss of control detected"); }
        break;
    case 13: // FLIP
        if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Flip abandoned"); }
        break;
    case 15: // PARACHUTES
        if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Parachute not deployed (too low)"); }
        else if (ecode == 3) { ecodeText = DataFlashLogParser::tr("Parachute not deployed (landed)"); }
        break;
    case 16: // EKFCHECK
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Variance cleared"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Bad variance"); }
        break;
    case 18: // BARO
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Errors Resolved"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Baro glitch"); }
        else if (ecode == 3) { ecodeText = DataFlashLogParser::tr("Bad depth"); }
        else if (ecode == 4) { ecodeText = DataFlashLogParser::tr("Unhealthy"); }
        break;
    case 20: // FAILSAFE_ADSB
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Failsafe Resolved"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("No action report only"); }
        else if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Avoid by climb/descend"); }
        else if (ecode == 3) { ecodeText = DataFlashLogParser::tr("Avoid by horizontal move"); }
        else if (ecode == 4) { ecodeText = DataFlashLogParser::tr("Avoid perpendicular move"); }
        else if (ecode == 5) { ecodeText = DataFlashLogParser::tr("RTL invoked"); }
        break;
    case 21: // TERRAIN
        if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Missing terrain data"); }
        break;
    case 22: // NAVIGATION
        if (ecode == 2) { ecodeText = DataFlashLogParser::tr("Failed to set destination"); }
        else if (ecode == 3) { ecodeText = DataFlashLogParser::tr("RTL restarted"); }
        else if (ecode == 4) { ecodeText = DataFlashLogParser::tr("Circle initialisation failed"); }
        else if (ecode == 5) { ecodeText = DataFlashLogParser::tr("Destination outside fence"); }
        else if (ecode == 6) { ecodeText = DataFlashLogParser::tr("RTL missing rangefinder"); }
        break;
    case 24: // EKF_PRIMARY
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("1st EKF became primary"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("2nd EKF became primary"); }
        break;
    case 25: // THRUST_LOSS_CHECK
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Thrust restored"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Thrust loss detected"); }
        break;
    case 29: // FAILSAFE_VIBE
        if (ecode == 0) { ecodeText = DataFlashLogParser::tr("Excessive vibration compensation de-activated"); }
        else if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Excessive vibration compensation activated"); }
        break;
    case 30: // INTERNAL_ERROR
        if (ecode == 1) { ecodeText = DataFlashLogParser::tr("Internal errors detected"); }
        break;
    default:
        break;
    }

    if (ecodeText.isEmpty()) {
        ecodeText = DataFlashLogParser::tr("Code %1").arg(ecode);
    }

    return QStringLiteral("%1 (%2): %3 (%4)").arg(subsystemName).arg(subsystem).arg(ecodeText).arg(ecode);
}

QString _ardupilotEventDescription(int eventId)
{
    switch (eventId) {
    case 10: return DataFlashLogParser::tr("Armed");
    case 11: return DataFlashLogParser::tr("Disarmed");
    case 15: return DataFlashLogParser::tr("Auto Armed");
    case 17: return DataFlashLogParser::tr("Land Complete Maybe");
    case 18: return DataFlashLogParser::tr("Land Complete");
    case 19: return DataFlashLogParser::tr("Lost GPS");
    case 21: return DataFlashLogParser::tr("Flip Start");
    case 22: return DataFlashLogParser::tr("Flip End");
    case 25: return DataFlashLogParser::tr("Set Home");
    case 26: return DataFlashLogParser::tr("Simple Mode Enabled");
    case 27: return DataFlashLogParser::tr("Simple Mode Disabled");
    case 28: return DataFlashLogParser::tr("Not Landed");
    case 29: return DataFlashLogParser::tr("Super Simple Mode Enabled");
    case 30: return DataFlashLogParser::tr("AutoTune Initialised");
    case 31: return DataFlashLogParser::tr("AutoTune Off");
    case 32: return DataFlashLogParser::tr("AutoTune Restart");
    case 33: return DataFlashLogParser::tr("AutoTune Success");
    case 34: return DataFlashLogParser::tr("AutoTune Failed");
    case 35: return DataFlashLogParser::tr("AutoTune Reached Limit");
    case 36: return DataFlashLogParser::tr("AutoTune Pilot Testing");
    case 37: return DataFlashLogParser::tr("AutoTune Saved Gains");
    case 38: return DataFlashLogParser::tr("Save Trim");
    case 39: return DataFlashLogParser::tr("Save Waypoint Add");
    case 41: return DataFlashLogParser::tr("Fence Enabled");
    case 42: return DataFlashLogParser::tr("Fence Disabled");
    case 43: return DataFlashLogParser::tr("Acro Trainer Off");
    case 44: return DataFlashLogParser::tr("Acro Trainer Leveling");
    case 45: return DataFlashLogParser::tr("Acro Trainer Limited");
    case 46: return DataFlashLogParser::tr("Gripper Grab");
    case 47: return DataFlashLogParser::tr("Gripper Release");
    case 49: return DataFlashLogParser::tr("Parachute Disabled");
    case 50: return DataFlashLogParser::tr("Parachute Enabled");
    case 51: return DataFlashLogParser::tr("Parachute Released");
    case 52: return DataFlashLogParser::tr("Landing Gear Deployed");
    case 53: return DataFlashLogParser::tr("Landing Gear Retracted");
    case 54: return DataFlashLogParser::tr("Motors Emergency Stopped");
    case 55: return DataFlashLogParser::tr("Motors Emergency Stop Cleared");
    case 56: return DataFlashLogParser::tr("Motors Interlock Disabled");
    case 57: return DataFlashLogParser::tr("Motors Interlock Enabled");
    case 58: return DataFlashLogParser::tr("Rotor Runup Complete");
    case 59: return DataFlashLogParser::tr("Rotor Speed Below Critical");
    case 60: return DataFlashLogParser::tr("EKF Altitude Reset");
    case 61: return DataFlashLogParser::tr("Land Cancelled By Pilot");
    case 62: return DataFlashLogParser::tr("EKF Yaw Reset");
    case 63: return DataFlashLogParser::tr("ADSB Avoidance Enabled");
    case 64: return DataFlashLogParser::tr("ADSB Avoidance Disabled");
    case 65: return DataFlashLogParser::tr("Proximity Avoidance Enabled");
    case 66: return DataFlashLogParser::tr("Proximity Avoidance Disabled");
    case 67: return DataFlashLogParser::tr("GPS Primary Changed");
    case 71: return DataFlashLogParser::tr("ZigZag Store A");
    case 72: return DataFlashLogParser::tr("ZigZag Store B");
    case 73: return DataFlashLogParser::tr("Land Repo Active");
    case 74: return DataFlashLogParser::tr("Standby Enabled");
    case 75: return DataFlashLogParser::tr("Standby Disabled");
    case 76: return DataFlashLogParser::tr("Fence Alt Max Enabled");
    case 77: return DataFlashLogParser::tr("Fence Alt Max Disabled");
    case 78: return DataFlashLogParser::tr("Fence Circle Enabled");
    case 79: return DataFlashLogParser::tr("Fence Circle Disabled");
    case 80: return DataFlashLogParser::tr("Fence Alt Min Enabled");
    case 81: return DataFlashLogParser::tr("Fence Alt Min Disabled");
    case 82: return DataFlashLogParser::tr("Fence Polygon Enabled");
    case 83: return DataFlashLogParser::tr("Fence Polygon Disabled");
    case 85: return DataFlashLogParser::tr("EK3 Source Set: Primary");
    case 86: return DataFlashLogParser::tr("EK3 Source Set: Secondary");
    case 87: return DataFlashLogParser::tr("EK3 Source Set: Tertiary");
    case 90: return DataFlashLogParser::tr("Airspeed Primary Changed");
    case 163: return DataFlashLogParser::tr("Surfaced");
    case 164: return DataFlashLogParser::tr("Not Surfaced");
    case 165: return DataFlashLogParser::tr("Bottomed");
    case 166: return DataFlashLogParser::tr("Not Bottomed");
    default: return DataFlashLogParser::tr("Event %1").arg(eventId);
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
        result.errorMessage = DataFlashLogParser::tr("Failed to open DataFlash file");
        return result;
    }

    const qint64 fileSize = file.size();
    if (fileSize <= 0) {
        result.errorMessage = DataFlashLogParser::tr("DataFlash file is empty");
        return result;
    }
    if (fileSize > std::numeric_limits<qsizetype>::max()) {
        result.errorMessage = DataFlashLogParser::tr("DataFlash file is too large to parse");
        return result;
    }

    uchar *const mappedData = file.map(0, fileSize);
    if (mappedData == nullptr) {
        result.errorMessage = DataFlashLogParser::tr("Failed to memory-map DataFlash file");
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

    QMap<uint8_t, DataFlashUtility::MessageFormat> formats;
    if (!DataFlashUtility::parseFmtMessages(bytes.constData(), bytes.size(), formats)) {
        result.errorMessage = DataFlashLogParser::tr("No valid FMT messages were found");
        return result;
    }

    QSet<QString> signalSet;
    QSet<QString> plottableSet;
    double minTimestampSecs = -1.0;
    double maxTimestampSecs = -1.0;
    bool hasOpenModeSegment = false;
    double modeSegmentStartSecs = -1.0;
    QString currentModeName;

    DataFlashUtility::iterateMessages(bytes.constData(), bytes.size(), formats, [&result, &signalSet, &plottableSet, &minTimestampSecs, &maxTimestampSecs, &hasOpenModeSegment, &modeSegmentStartSecs, &currentModeName](uint8_t, const char *payload, int, const DataFlashUtility::MessageFormat &fmt) {
        const QMap<QString, QVariant> values = DataFlashUtility::parseMessage(payload, fmt);
        const double timestampSecs = _extractTimestampSeconds(values);
        if (timestampSecs >= 0.0) {
            if (minTimestampSecs < 0.0 || timestampSecs < minTimestampSecs) {
                minTimestampSecs = timestampSecs;
            }
            maxTimestampSecs = std::max(maxTimestampSecs, timestampSecs);
        }

        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            signalSet.insert(QStringLiteral("%1.%2").arg(fmt.name, it.key()));
        }

        if (fmt.name == QStringLiteral("PARM")) {
            const QString paramName = values.value(QStringLiteral("Name")).toString();
            const QVariant paramValue = values.contains(QStringLiteral("Value")) ? values.value(QStringLiteral("Value")) : values.value(QStringLiteral("Val"));
            if (!paramName.isEmpty()) {
                QVariantMap row;
                row[QStringLiteral("name")] = paramName;
                row[QStringLiteral("value")] = paramValue;
                result.parameters.append(row);
            }
        } else if (fmt.name == QStringLiteral("MSG")) {
            const QString text = values.value(QStringLiteral("Message")).toString();
            const QString detected = _vehicleTypeFromMessageText(text);
            if (result.detectedVehicleType.isEmpty() && !detected.isEmpty()) {
                result.detectedVehicleType = detected;
            }
        } else if (fmt.name == QStringLiteral("MODE")) {
            QString modeName = values.value(QStringLiteral("Mode")).toString();
            bool isNumericMode = false;
            const int modeNumber = modeName.toInt(&isNumericMode);
            if (isNumericMode) {
                modeName = _ardupilotModeName(result.detectedVehicleType, modeNumber);
            } else if (modeName.isEmpty()) {
                modeName = QStringLiteral("Unknown");
            }
            _appendEvent(result.events, timestampSecs, QStringLiteral("mode"), DataFlashLogParser::tr("Mode: %1").arg(modeName));
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
        } else if (fmt.name == QStringLiteral("ERR")) {
            const int subsystem = values.value(QStringLiteral("Subsys")).toInt();
            const int ecode = values.value(QStringLiteral("ECode")).toInt();
            _appendEvent(result.events, timestampSecs, QStringLiteral("error"), _ardupilotErrDescription(subsystem, ecode));
        } else if (fmt.name == QStringLiteral("EV")) {
            const int eventId = values.value(QStringLiteral("Id"), values.value(QStringLiteral("Event"))).toInt();
            _appendEvent(result.events, timestampSecs, QStringLiteral("event"), _ardupilotEventDescription(eventId));
        }

        result.sampleCount++;
        if (timestampSecs >= 0.0) {
            for (auto it = values.cbegin(); it != values.cend(); ++it) {
                const QString signalName = QStringLiteral("%1.%2").arg(fmt.name, it.key());
                const int typeId = it.value().metaType().id();
                const bool numeric = (typeId == QMetaType::Int) || (typeId == QMetaType::UInt) ||
                                     (typeId == QMetaType::LongLong) || (typeId == QMetaType::ULongLong) ||
                                     (typeId == QMetaType::Float) || (typeId == QMetaType::Double);
                if (numeric) {
                    result.signalSamples[signalName].append(QPointF(timestampSecs, it.value().toDouble()));
                    plottableSet.insert(signalName);
                }
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

    result.availableSignals = signalSet.values();
    std::sort(result.availableSignals.begin(), result.availableSignals.end());
    result.plottableSignals = plottableSet.values();
    std::sort(result.plottableSignals.begin(), result.plottableSignals.end());
    result.minTimestamp = minTimestampSecs;
    result.maxTimestamp = maxTimestampSecs;
    result.ok = true;
    return result;
}
} // namespace

DataFlashLogParser::DataFlashLogParser(QObject *parent)
    : QObject(parent)
{
    qCDebug(DataFlashLogParserLog) << this;
}

DataFlashLogParser::~DataFlashLogParser()
{
    qCDebug(DataFlashLogParserLog) << this;
}

bool DataFlashLogParser::parseFile(const QString &filePath)
{
    ++_parseRequestId; // Invalidate any in-flight async parse results.
    clear();
    const ParseResult result = _parseDataFlashFile(filePath);
    if (!result.ok) {
        _setParseError(result.errorMessage);
        return false;
    }

    _availableSignals = result.availableSignals;
    _plottableSignals = result.plottableSignals;
    _parameters = result.parameters;
    _events = result.events;
    _modeSegments = result.modeSegments;
    _signalSamples = result.signalSamples;
    _sampleCount = result.sampleCount;
    _detectedVehicleType = result.detectedVehicleType;
    emit availableSignalsChanged();
    emit plottableSignalsChanged();
    emit parametersChanged();
    emit eventsChanged();
    emit modeSegmentsChanged();
    emit detectedVehicleTypeChanged();
    if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
        _minTimestamp = result.minTimestamp;
        _maxTimestamp = result.maxTimestamp;
        emit timeRangeChanged();
    }
    emit sampleCountChanged();

    _parsed = true;
    emit parsedChanged();
    qCDebug(DataFlashLogParserLog) << "Parsed signals" << _availableSignals.count() << "parameters" << _parameters.count() << "events" << _events.count();
    return true;
}

void DataFlashLogParser::parseFileAsync(const QString &filePath)
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

        _availableSignals = result.availableSignals;
        _plottableSignals = result.plottableSignals;
        _parameters = result.parameters;
        _events = result.events;
        _modeSegments = result.modeSegments;
        _signalSamples = result.signalSamples;
        _sampleCount = result.sampleCount;
        _detectedVehicleType = result.detectedVehicleType;
        emit availableSignalsChanged();
        emit plottableSignalsChanged();
        emit parametersChanged();
        emit eventsChanged();
        emit modeSegmentsChanged();
        emit detectedVehicleTypeChanged();
        if (_minTimestamp != result.minTimestamp || _maxTimestamp != result.maxTimestamp) {
            _minTimestamp = result.minTimestamp;
            _maxTimestamp = result.maxTimestamp;
            emit timeRangeChanged();
        }
        emit sampleCountChanged();

        _parsed = true;
        emit parsedChanged();
        emit parseFileFinished(filePath, true, QString());
    });

    watcher->setFuture(QtConcurrent::run([filePath]() {
        return _parseDataFlashFile(filePath);
    }));
}

void DataFlashLogParser::clear()
{
    const bool oldParsed = _parsed;
    _parsed = false;
    if (oldParsed) {
        emit parsedChanged();
    }

    if (!_parseError.isEmpty()) {
        _parseError.clear();
        emit parseErrorChanged();
    }

    if (!_availableSignals.isEmpty()) {
        _availableSignals.clear();
        emit availableSignalsChanged();
    }

    if (!_parameters.isEmpty()) {
        _parameters.clear();
        emit parametersChanged();
    }

    if (!_events.isEmpty()) {
        _events.clear();
        emit eventsChanged();
    }

    if (!_modeSegments.isEmpty()) {
        _modeSegments.clear();
        emit modeSegmentsChanged();
    }

    if (!_detectedVehicleType.isEmpty()) {
        _detectedVehicleType.clear();
        emit detectedVehicleTypeChanged();
    }

    if (!_plottableSignals.isEmpty()) {
        _plottableSignals.clear();
        emit plottableSignalsChanged();
    }

    _signalSamples.clear();
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

QVariantList DataFlashLogParser::signalSamples(const QString &signalName) const
{
    QVariantList output;
    const auto signalIt = _signalSamples.constFind(signalName);
    if (signalIt == _signalSamples.cend()) {
        return output;
    }

    const QVector<QPointF> &points = signalIt.value();
    output.reserve(points.size());
    for (const QPointF &point : points) {
        QVariantMap row;
        row[QStringLiteral("x")] = point.x();
        row[QStringLiteral("y")] = point.y();
        output.append(row);
    }

    return output;
}

double DataFlashLogParser::signalValueAt(const QString &signalName, double timestampSeconds) const
{
    const auto signalIt = _signalSamples.constFind(signalName);
    if ((signalIt == _signalSamples.cend()) || signalIt->isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const QVector<QPointF> &points = signalIt.value();
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

QString DataFlashLogParser::modeAt(double timestampSeconds) const
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

QVariantList DataFlashLogParser::eventsNear(double timestampSeconds, double thresholdSeconds) const
{
    QVariantList matches;
    const double threshold = std::max(0.0, thresholdSeconds);

    for (const QVariant &variant : _events) {
        const QVariantMap eventItem = variant.toMap();
        const double eventTime = eventItem.value(QStringLiteral("time")).toDouble();
        if (std::fabs(eventTime - timestampSeconds) <= threshold) {
            matches.append(eventItem);
        }
    }

    return matches;
}

void DataFlashLogParser::_setParseError(const QString &error)
{
    if (_parseError != error) {
        _parseError = error;
        emit parseErrorChanged();
    }
    qCWarning(DataFlashLogParserLog) << error;
}
