#include "DataFlashParser.h"
#include "DataFlashUtility.h"
#include "GeoTagData.h"
#include "QGCLoggingCategory.h"

#include <cmath>

QGC_LOGGING_CATEGORY(DataFlashParserLog, "AnalyzeView.DataFlashParser")

namespace DataFlashParser
{

namespace {

GeoTagData extractGeoTagData(const QMap<QString, QVariant> &fields)
{
    GeoTagData feedback;

    // Extract timestamp (microseconds since boot, convert to seconds)
    if (fields.contains(QStringLiteral("TimeUS"))) {
        feedback.timestamp = static_cast<qint64>(fields[QStringLiteral("TimeUS")].toULongLong() / 1000000);
    }

    // Extract image sequence number
    if (fields.contains(QStringLiteral("Img"))) {
        feedback.imageSequence = fields[QStringLiteral("Img")].toUInt();
    }

    // Extract GPS coordinates
    double lat = 0, lon = 0, alt = 0;
    if (fields.contains(QStringLiteral("Lat"))) {
        lat = fields[QStringLiteral("Lat")].toDouble();
    }
    if (fields.contains(QStringLiteral("Lng"))) {
        lon = fields[QStringLiteral("Lng")].toDouble();
    }
    if (fields.contains(QStringLiteral("Alt"))) {
        alt = fields[QStringLiteral("Alt")].toDouble();
    } else if (fields.contains(QStringLiteral("GPSAlt"))) {
        alt = fields[QStringLiteral("GPSAlt")].toDouble();
    }

    feedback.coordinate = QGeoCoordinate(lat, lon, alt);

    // Extract attitude (roll, pitch, yaw) and convert to quaternion
    float roll = 0, pitch = 0, yaw = 0;
    if (fields.contains(QStringLiteral("R"))) {
        roll = static_cast<float>(fields[QStringLiteral("R")].toDouble());
    }
    if (fields.contains(QStringLiteral("P"))) {
        pitch = static_cast<float>(fields[QStringLiteral("P")].toDouble());
    }
    if (fields.contains(QStringLiteral("Y"))) {
        yaw = static_cast<float>(fields[QStringLiteral("Y")].toDouble());
    }

    // Convert Euler angles (degrees) to quaternion
    const float cy = std::cos(yaw * 0.5f * static_cast<float>(M_PI) / 180.0f);
    const float sy = std::sin(yaw * 0.5f * static_cast<float>(M_PI) / 180.0f);
    const float cp = std::cos(pitch * 0.5f * static_cast<float>(M_PI) / 180.0f);
    const float sp = std::sin(pitch * 0.5f * static_cast<float>(M_PI) / 180.0f);
    const float cr = std::cos(roll * 0.5f * static_cast<float>(M_PI) / 180.0f);
    const float sr = std::sin(roll * 0.5f * static_cast<float>(M_PI) / 180.0f);

    feedback.attitude = QQuaternion(
        cy * cp * cr + sy * sp * sr,  // w
        cy * cp * sr - sy * sp * cr,  // x
        sy * cp * sr + cy * sp * cr,  // y
        sy * cp * cr - cy * sp * sr   // z
    );

    // ArduPilot CAM messages indicate successful captures
    feedback.captureResult = GeoTagData::CaptureResult::Success;

    return feedback;
}

} // namespace

bool getTagsFromLog(const char *data, qint64 size, QList<GeoTagData> &cameraFeedback, QString &errorMessage)
{
    cameraFeedback.clear();

    if (!DataFlashUtility::isValidHeader(data, size)) {
        errorMessage = QStringLiteral("Invalid DataFlash log format");
        return false;
    }

    // First pass: Parse FMT messages to learn message formats
    QMap<uint8_t, DataFlashUtility::MessageFormat> formats;
    if (!DataFlashUtility::parseFmtMessages(data, size, formats)) {
        errorMessage = QStringLiteral("No message formats found in log");
        return false;
    }

    // Find CAM message type
    uint8_t camMessageType = 0;
    for (auto it = formats.constBegin(); it != formats.constEnd(); ++it) {
        if (it.value().name == QStringLiteral("CAM")) {
            camMessageType = it.key();
            qCDebug(DataFlashParserLog) << "Found CAM format:" << it.value().format
                                        << "columns:" << it.value().columns;
            break;
        }
    }

    if (camMessageType == 0) {
        errorMessage = QStringLiteral("No CAM (camera) messages found in log");
        return false;
    }

    // Second pass: Extract CAM messages using iterator
    DataFlashUtility::iterateMessages(data, size, formats,
        [&](uint8_t msgType, const char *payload, int, const DataFlashUtility::MessageFormat &fmt) {
            if (msgType == camMessageType) {
                const QMap<QString, QVariant> fields = DataFlashUtility::parseMessage(payload, fmt);
                GeoTagData feedback = extractGeoTagData(fields);

                if (feedback.coordinate.isValid()) {
                    cameraFeedback.append(feedback);
                }
            }
            return true;  // Continue iteration
        });

    if (cameraFeedback.isEmpty()) {
        errorMessage = QStringLiteral("No valid camera capture events found in log");
        return false;
    }

    qCDebug(DataFlashParserLog) << "Parsed" << cameraFeedback.size() << "camera capture events";

    return true;
}

} // namespace DataFlashParser
