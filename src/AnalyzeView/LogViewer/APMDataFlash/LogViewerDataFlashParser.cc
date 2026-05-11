#include "LogViewerDataFlashParser.h"

#include "APMDataFlashUtility.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QVariantMap>

#include <algorithm>
#include <limits>

namespace {

QString _vehicleTypeFromMessageText(const QString &messageText)
{
    const QString text = messageText.toLower();
    if (text.contains(QStringLiteral("arducopter"))) { return QStringLiteral("ArduCopter"); }
    if (text.contains(QStringLiteral("arduplane")))  { return QStringLiteral("ArduPlane"); }
    if (text.contains(QStringLiteral("ardurover")))  { return QStringLiteral("ArduRover"); }
    if (text.contains(QStringLiteral("ardusub")))    { return QStringLiteral("ArduSub"); }
    return QString();
}

// Parse "major.minor" from firmware version strings like "ArduCopter V4.3.1" or "V4.5-stable"
void _parseFirmwareVersionFromMessageText(const QString &messageText, int &major, int &minor)
{
    static const QRegularExpression re(QStringLiteral("V(\\d+)\\.(\\d+)"));
    const QRegularExpressionMatch m = re.match(messageText);
    if (m.hasMatch()) {
        major = m.captured(1).toInt();
        minor = m.captured(2).toInt();
    }
}

QString _ardupilotModeName(const QString &vehicleType, int modeNumber)
{
    static const QHash<int, QString> planeModes = {
        {0,QStringLiteral("Manual")},{1,QStringLiteral("CIRCLE")},{2,QStringLiteral("STABILIZE")},
        {3,QStringLiteral("TRAINING")},{4,QStringLiteral("ACRO")},{5,QStringLiteral("FBWA")},
        {6,QStringLiteral("FBWB")},{7,QStringLiteral("CRUISE")},{8,QStringLiteral("AUTOTUNE")},
        {10,QStringLiteral("Auto")},{11,QStringLiteral("RTL")},{12,QStringLiteral("Loiter")},
        {13,QStringLiteral("TAKEOFF")},{14,QStringLiteral("AVOID_ADSB")},{15,QStringLiteral("Guided")},
        {17,QStringLiteral("QSTABILIZE")},{18,QStringLiteral("QHOVER")},{19,QStringLiteral("QLOITER")},
        {20,QStringLiteral("QLAND")},{21,QStringLiteral("QRTL")},{22,QStringLiteral("QAUTOTUNE")},
        {23,QStringLiteral("QACRO")},{24,QStringLiteral("THERMAL")},{25,QStringLiteral("Loiter to QLand")},
        {26,QStringLiteral("AUTOLAND")},
    };
    static const QHash<int, QString> copterModes = {
        {0,QStringLiteral("Stabilize")},{1,QStringLiteral("Acro")},{2,QStringLiteral("AltHold")},
        {3,QStringLiteral("Auto")},{4,QStringLiteral("Guided")},{5,QStringLiteral("Loiter")},
        {6,QStringLiteral("RTL")},{7,QStringLiteral("Circle")},{9,QStringLiteral("Land")},
        {11,QStringLiteral("Drift")},{13,QStringLiteral("Sport")},{14,QStringLiteral("Flip")},
        {15,QStringLiteral("AutoTune")},{16,QStringLiteral("PosHold")},{17,QStringLiteral("Brake")},
        {18,QStringLiteral("Throw")},{19,QStringLiteral("Avoid_ADSB")},{20,QStringLiteral("Guided_NoGPS")},
        {21,QStringLiteral("Smart_RTL")},{22,QStringLiteral("FlowHold")},{23,QStringLiteral("Follow")},
        {24,QStringLiteral("ZigZag")},{25,QStringLiteral("SystemID")},{26,QStringLiteral("Heli_Autorotate")},
        {27,QStringLiteral("Auto RTL")},{28,QStringLiteral("Turtle")},
    };
    if (vehicleType == QStringLiteral("ArduPlane")) {
        return planeModes.value(modeNumber, QStringLiteral("Mode %1").arg(modeNumber));
    }
    if (vehicleType == QStringLiteral("ArduCopter")) {
        return copterModes.value(modeNumber, QStringLiteral("Mode %1").arg(modeNumber));
    }
    return QStringLiteral("Mode %1").arg(modeNumber);
}

QString _ardupilotErrDescription(int subsystem, int ecode)
{
    QString sub;
    switch (subsystem) {
    case 1:  sub = QCoreApplication::translate("LogFileParser", "Main"); break;
    case 2:  sub = QCoreApplication::translate("LogFileParser", "Radio"); break;
    case 3:  sub = QCoreApplication::translate("LogFileParser", "Compass"); break;
    case 4:  sub = QCoreApplication::translate("LogFileParser", "Optflow"); break;
    case 5:  sub = QCoreApplication::translate("LogFileParser", "Radio Failsafe"); break;
    case 6:  sub = QCoreApplication::translate("LogFileParser", "Battery Failsafe"); break;
    case 7:  sub = QCoreApplication::translate("LogFileParser", "GPS Failsafe"); break;
    case 8:  sub = QCoreApplication::translate("LogFileParser", "GCS Failsafe"); break;
    case 9:  sub = QCoreApplication::translate("LogFileParser", "Fence Failsafe"); break;
    case 10: sub = QCoreApplication::translate("LogFileParser", "Flight mode"); break;
    case 11: sub = QCoreApplication::translate("LogFileParser", "GPS"); break;
    case 12: sub = QCoreApplication::translate("LogFileParser", "Crash Check"); break;
    case 13: sub = QCoreApplication::translate("LogFileParser", "Flip"); break;
    case 14: sub = QCoreApplication::translate("LogFileParser", "Autotune"); break;
    case 15: sub = QCoreApplication::translate("LogFileParser", "Parachute"); break;
    case 16: sub = QCoreApplication::translate("LogFileParser", "EKF Check"); break;
    case 17: sub = QCoreApplication::translate("LogFileParser", "EKF Failsafe"); break;
    case 18: sub = QCoreApplication::translate("LogFileParser", "Barometer"); break;
    case 19: sub = QCoreApplication::translate("LogFileParser", "CPU Load Watchdog"); break;
    case 20: sub = QCoreApplication::translate("LogFileParser", "ADSB Failsafe"); break;
    case 21: sub = QCoreApplication::translate("LogFileParser", "Terrain Data"); break;
    case 22: sub = QCoreApplication::translate("LogFileParser", "Navigation"); break;
    case 23: sub = QCoreApplication::translate("LogFileParser", "Terrain Failsafe"); break;
    case 24: sub = QCoreApplication::translate("LogFileParser", "EKF Primary"); break;
    case 25: sub = QCoreApplication::translate("LogFileParser", "Thrust Loss Check"); break;
    case 26: sub = QCoreApplication::translate("LogFileParser", "Sensor Failsafe"); break;
    case 27: sub = QCoreApplication::translate("LogFileParser", "Leak Failsafe"); break;
    case 28: sub = QCoreApplication::translate("LogFileParser", "Pilot Input"); break;
    case 29: sub = QCoreApplication::translate("LogFileParser", "Vibration Failsafe"); break;
    case 30: sub = QCoreApplication::translate("LogFileParser", "Internal Error"); break;
    case 31: sub = QCoreApplication::translate("LogFileParser", "Deadreckon Failsafe"); break;
    default: sub = QCoreApplication::translate("LogFileParser", "Subsystem %1").arg(subsystem); break;
    }
    return QStringLiteral("%1 (%2): Code %3").arg(sub).arg(subsystem).arg(ecode);
}

QString _ardupilotEventDescription(int eventId)
{
    switch (eventId) {
    case 10: return QCoreApplication::translate("LogFileParser", "Armed");
    case 11: return QCoreApplication::translate("LogFileParser", "Disarmed");
    case 15: return QCoreApplication::translate("LogFileParser", "Auto Armed");
    case 17: return QCoreApplication::translate("LogFileParser", "Land Complete Maybe");
    case 18: return QCoreApplication::translate("LogFileParser", "Land Complete");
    case 19: return QCoreApplication::translate("LogFileParser", "Lost GPS");
    case 21: return QCoreApplication::translate("LogFileParser", "Flip Start");
    case 22: return QCoreApplication::translate("LogFileParser", "Flip End");
    case 25: return QCoreApplication::translate("LogFileParser", "Set Home");
    case 26: return QCoreApplication::translate("LogFileParser", "Simple Mode Enabled");
    case 27: return QCoreApplication::translate("LogFileParser", "Simple Mode Disabled");
    case 28: return QCoreApplication::translate("LogFileParser", "Not Landed");
    case 29: return QCoreApplication::translate("LogFileParser", "Super Simple Mode Enabled");
    case 30: return QCoreApplication::translate("LogFileParser", "AutoTune Initialised");
    case 31: return QCoreApplication::translate("LogFileParser", "AutoTune Off");
    case 32: return QCoreApplication::translate("LogFileParser", "AutoTune Restart");
    case 33: return QCoreApplication::translate("LogFileParser", "AutoTune Success");
    case 34: return QCoreApplication::translate("LogFileParser", "AutoTune Failed");
    case 35: return QCoreApplication::translate("LogFileParser", "AutoTune Reached Limit");
    case 36: return QCoreApplication::translate("LogFileParser", "AutoTune Pilot Testing");
    case 37: return QCoreApplication::translate("LogFileParser", "AutoTune Saved Gains");
    case 38: return QCoreApplication::translate("LogFileParser", "Save Trim");
    case 39: return QCoreApplication::translate("LogFileParser", "Save Waypoint Add");
    case 41: return QCoreApplication::translate("LogFileParser", "Fence Enabled");
    case 42: return QCoreApplication::translate("LogFileParser", "Fence Disabled");
    case 43: return QCoreApplication::translate("LogFileParser", "Acro Trainer Off");
    case 44: return QCoreApplication::translate("LogFileParser", "Acro Trainer Leveling");
    case 45: return QCoreApplication::translate("LogFileParser", "Acro Trainer Limited");
    case 46: return QCoreApplication::translate("LogFileParser", "Gripper Grab");
    case 47: return QCoreApplication::translate("LogFileParser", "Gripper Release");
    case 49: return QCoreApplication::translate("LogFileParser", "Parachute Disabled");
    case 50: return QCoreApplication::translate("LogFileParser", "Parachute Enabled");
    case 51: return QCoreApplication::translate("LogFileParser", "Parachute Released");
    case 52: return QCoreApplication::translate("LogFileParser", "Landing Gear Deployed");
    case 53: return QCoreApplication::translate("LogFileParser", "Landing Gear Retracted");
    case 54: return QCoreApplication::translate("LogFileParser", "Motors Emergency Stopped");
    case 55: return QCoreApplication::translate("LogFileParser", "Motors Emergency Stop Cleared");
    case 56: return QCoreApplication::translate("LogFileParser", "Motors Interlock Disabled");
    case 57: return QCoreApplication::translate("LogFileParser", "Motors Interlock Enabled");
    case 58: return QCoreApplication::translate("LogFileParser", "Rotor Runup Complete");
    case 59: return QCoreApplication::translate("LogFileParser", "Rotor Speed Below Critical");
    case 60: return QCoreApplication::translate("LogFileParser", "EKF Altitude Reset");
    case 61: return QCoreApplication::translate("LogFileParser", "Land Cancelled By Pilot");
    case 62: return QCoreApplication::translate("LogFileParser", "EKF Yaw Reset");
    case 63: return QCoreApplication::translate("LogFileParser", "ADSB Avoidance Enabled");
    case 64: return QCoreApplication::translate("LogFileParser", "ADSB Avoidance Disabled");
    case 65: return QCoreApplication::translate("LogFileParser", "Proximity Avoidance Enabled");
    case 66: return QCoreApplication::translate("LogFileParser", "Proximity Avoidance Disabled");
    case 67: return QCoreApplication::translate("LogFileParser", "GPS Primary Changed");
    case 71: return QCoreApplication::translate("LogFileParser", "ZigZag Store A");
    case 72: return QCoreApplication::translate("LogFileParser", "ZigZag Store B");
    case 73: return QCoreApplication::translate("LogFileParser", "Land Repo Active");
    case 74: return QCoreApplication::translate("LogFileParser", "Standby Enabled");
    case 75: return QCoreApplication::translate("LogFileParser", "Standby Disabled");
    case 76: return QCoreApplication::translate("LogFileParser", "Fence Alt Max Enabled");
    case 77: return QCoreApplication::translate("LogFileParser", "Fence Alt Max Disabled");
    case 78: return QCoreApplication::translate("LogFileParser", "Fence Circle Enabled");
    case 79: return QCoreApplication::translate("LogFileParser", "Fence Circle Disabled");
    case 80: return QCoreApplication::translate("LogFileParser", "Fence Alt Min Enabled");
    case 81: return QCoreApplication::translate("LogFileParser", "Fence Alt Min Disabled");
    case 82: return QCoreApplication::translate("LogFileParser", "Fence Polygon Enabled");
    case 83: return QCoreApplication::translate("LogFileParser", "Fence Polygon Disabled");
    case 85: return QCoreApplication::translate("LogFileParser", "EK3 Source Set: Primary");
    case 86: return QCoreApplication::translate("LogFileParser", "EK3 Source Set: Secondary");
    case 87: return QCoreApplication::translate("LogFileParser", "EK3 Source Set: Tertiary");
    case 90: return QCoreApplication::translate("LogFileParser", "Airspeed Primary Changed");
    case 163: return QCoreApplication::translate("LogFileParser", "Surfaced");
    case 164: return QCoreApplication::translate("LogFileParser", "Not Surfaced");
    case 165: return QCoreApplication::translate("LogFileParser", "Bottomed");
    case 166: return QCoreApplication::translate("LogFileParser", "Not Bottomed");
    default: return QCoreApplication::translate("LogFileParser", "Event %1").arg(eventId);
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

} // namespace

namespace DataFlashParser {

LogParseResult parseFile(const QString &filePath)
{
    LogParseResult result;
    result.sourceType = LogParseResult::SourceType::APMDataFlash;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "Failed to open file");
        return result;
    }

    const qint64 fileSize = file.size();
    if (fileSize <= 0) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "File is empty");
        return result;
    }
    if (fileSize > std::numeric_limits<qsizetype>::max()) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "File is too large to parse");
        return result;
    }

    uchar *const mappedData = file.map(0, fileSize);
    if (mappedData == nullptr) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "Failed to memory-map file");
        return result;
    }

    struct ScopedUnmap {
        QFile &file;
        uchar *data = nullptr;
        ~ScopedUnmap() { if (data) { file.unmap(data); } }
    } scopedUnmap{file, mappedData};

    const char *const raw = reinterpret_cast<const char *>(mappedData);

    // Verify DataFlash magic
    if (fileSize < 3 ||
        static_cast<uint8_t>(raw[0]) != 0xA3 ||
        static_cast<uint8_t>(raw[1]) != 0x95) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "File does not appear to be a DataFlash log (invalid header)");
        return result;
    }

    const QByteArray bytes = QByteArray::fromRawData(raw, static_cast<qsizetype>(fileSize));

    QMap<uint8_t, APMDataFlashUtility::MessageFormat> formats;
    if (!APMDataFlashUtility::parseFmtMessages(bytes.constData(), bytes.size(), formats)) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "No valid FMT messages were found");
        return result;
    }

    QSet<QString> fieldSet;
    QSet<QString> plottableFieldSet;
    double minTimestampSecs = -1.0;
    double maxTimestampSecs = -1.0;
    bool hasOpenModeSegment = false;
    double modeSegmentStartSecs = -1.0;
    QString currentModeName;

    QHash<uint8_t, QHash<QString, QString>> fieldNameByCol;
    fieldNameByCol.reserve(formats.size());
    for (auto fmtIt = formats.cbegin(); fmtIt != formats.cend(); ++fmtIt) {
        const APMDataFlashUtility::MessageFormat &fmt = fmtIt.value();
        QHash<QString, QString> &perCol = fieldNameByCol[fmtIt.key()];
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

    APMDataFlashUtility::iterateMessages(bytes.constData(), bytes.size(), formats,
        [&](uint8_t msgType, const char *payload, int, const APMDataFlashUtility::MessageFormat &fmt) {
        const QMap<QString, QVariant> values = APMDataFlashUtility::parseMessage(payload, fmt);
        const double timestampSecs = _extractTimestampSeconds(values);
        if (timestampSecs >= 0.0) {
            if (minTimestampSecs < 0.0 || timestampSecs < minTimestampSecs) { minTimestampSecs = timestampSecs; }
            maxTimestampSecs = std::max(maxTimestampSecs, timestampSecs);
        }

        if (fmt.name == kPARM) {
            const QString paramName = values.value(QStringLiteral("Name")).toString();
            const QVariant paramValue = values.contains(QStringLiteral("Value"))
                ? values.value(QStringLiteral("Value"))
                : values.value(QStringLiteral("Val"));
            if (!paramName.isEmpty()) {
                QVariantMap row;
                row[QStringLiteral("name")]         = paramName;
                row[QStringLiteral("value")]        = paramValue;
                // DataFlash logs don't carry default value metadata
                row[QStringLiteral("isFloat")]      = paramValue.metaType() == QMetaType::fromType<float>()
                                                      || paramValue.metaType() == QMetaType::fromType<double>();
                row[QStringLiteral("hasDefault")]   = false;
                row[QStringLiteral("defaultValue")] = QVariant();
                row[QStringLiteral("isDefault")]    = false;
                result.parameters.append(row);
            }
        } else if (fmt.name == kMSG) {
            const QString text = values.value(QStringLiteral("Message")).toString();
            const QString detected = _vehicleTypeFromMessageText(text);
            if (result.detectedVehicleType.isEmpty() && !detected.isEmpty()) {
                result.detectedVehicleType = detected;
                _parseFirmwareVersionFromMessageText(text, result.firmwareMajorVersion, result.firmwareMinorVersion);
            }
            if (!text.isEmpty()) {
                QVariantMap row;
                row[QStringLiteral("time")] = timestampSecs;
                row[QStringLiteral("text")] = text;
                result.messages.append(row);
            }
        } else if (fmt.name == kMODE) {
            QString modeName = values.value(QStringLiteral("Mode")).toString();
            bool isNumeric = false;
            const int modeNumber = modeName.toInt(&isNumeric);
            if (isNumeric) {
                modeName = _ardupilotModeName(result.detectedVehicleType, modeNumber);
            } else if (modeName.isEmpty()) {
                modeName = QCoreApplication::translate("LogFileParser", "Unknown");
            }
            _appendEvent(result.events, timestampSecs, QStringLiteral("mode"),
                         QCoreApplication::translate("LogFileParser", "Mode: %1").arg(modeName));
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
            _appendEvent(result.events, timestampSecs, QStringLiteral("error"),
                         _ardupilotErrDescription(subsystem, ecode));
        } else if (fmt.name == kEV) {
            const int eventId = values.value(QStringLiteral("Id"), values.value(QStringLiteral("Event"))).toInt();
            _appendEvent(result.events, timestampSecs, QStringLiteral("event"),
                         _ardupilotEventDescription(eventId));
        }

        result.sampleCount++;

        const QHash<QString, QString> &perCol = fieldNameByCol[msgType];
        const bool haveTimestamp = (timestampSecs >= 0.0);
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            const auto nameIt = perCol.constFind(it.key());
            if (nameIt == perCol.constEnd()) { continue; }
            const QString &fieldName = nameIt.value();
            fieldSet.insert(fieldName);
            if (!haveTimestamp) { continue; }
            const int typeId = it.value().metaType().id();
            const bool numeric =
                (typeId == QMetaType::Int) || (typeId == QMetaType::UInt) ||
                (typeId == QMetaType::LongLong) || (typeId == QMetaType::ULongLong) ||
                (typeId == QMetaType::Float) || (typeId == QMetaType::Double);
            if (numeric) {
                result.fieldSamples[fieldName].append(QPointF(timestampSecs, it.value().toDouble()));
                plottableFieldSet.insert(fieldName);
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

    result.availableFields = fieldSet.values();
    std::sort(result.availableFields.begin(), result.availableFields.end());
    result.plottableFields = plottableFieldSet.values();
    std::sort(result.plottableFields.begin(), result.plottableFields.end());
    result.minTimestamp = minTimestampSecs;
    result.maxTimestamp = maxTimestampSecs;
    result.ok = true;
    return result;
}

} // namespace DataFlashParser
