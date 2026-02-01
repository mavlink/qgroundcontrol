#include "ULogParser.h"
#include "ULogUtility.h"
#include <QtCore/QLoggingCategory>

#include <cmath>

Q_STATIC_LOGGING_CATEGORY(ULogParserLog, "AnalyzeView.ULogParser")

namespace ULogParser {

GeoTagData parseGeoTagData(const ulog_cpp::TypedDataView &sample)
{
    GeoTagData feedback;

    // Required fields (timestamp in microseconds, convert to seconds)
    feedback.timestamp = static_cast<qint64>(sample.at("timestamp").as<uint64_t>() / 1000000);
    feedback.imageSequence = sample.at("seq").as<uint32_t>();

    // Coordinate fields
    double longitude = sample.at("lon").as<double>();
    longitude = fmod(180.0 + longitude, 360.0) - 180.0;
    feedback.coordinate = QGeoCoordinate(
        sample.at("lat").as<double>(),
        longitude,
        sample.at("alt").as<float>()
    );

    // Optional fields with hasField() checks
    if (sample.hasField("timestamp_utc")) {
        feedback.timestampUTC = static_cast<qint64>(sample.at("timestamp_utc").as<uint64_t>() / 1000000);
    }

    if (sample.hasField("ground_distance")) {
        feedback.groundDistance = sample.at("ground_distance").as<float>();
    }

    if (sample.hasField("q")) {
        feedback.attitude = QQuaternion(
            sample.at("q")[0].as<float>(),
            sample.at("q")[1].as<float>(),
            sample.at("q")[2].as<float>(),
            sample.at("q")[3].as<float>()
        );
    }

    if (sample.hasField("result")) {
        feedback.captureResult = static_cast<GeoTagData::CaptureResult>(sample.at("result").as<int8_t>());
    } else {
        // For backwards compatibility with older ULog files that don't have the result field,
        // assume success if we got valid coordinate data
        feedback.captureResult = GeoTagData::CaptureResult::Success;
    }

    return feedback;
}

bool getTagsFromLog(const char *data, qint64 size, QList<GeoTagData> &cameraFeedback, QString &errorMessage)
{
    cameraFeedback.clear();

    const bool success = ULogUtility::iterateMessages(data, size, "camera_capture",
        [&](const ulog_cpp::TypedDataView &sample) {
            cameraFeedback.append(parseGeoTagData(sample));
            return true;
        }, errorMessage);

    if (!success) {
        return false;
    }

    if (cameraFeedback.isEmpty()) {
        errorMessage = QStringLiteral("Could not detect camera_capture packets in ULog");
        return false;
    }

    return true;
}

} // namespace ULogParser
