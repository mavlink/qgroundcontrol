#include "ULogTestGenerator.h"

#include <ulog_cpp/simple_writer.hpp>

namespace ULogTestGenerator
{

// Struct matching the camera_capture message format
// Fields must be ordered by decreasing size to avoid padding
#pragma pack(push, 1)
struct CameraCaptureMessage
{
    uint64_t timestamp;
    uint64_t timestamp_utc;
    double lat;
    double lon;
    float alt;
    float ground_distance;
    float q[4];
    uint32_t seq;
    int8_t result;
    uint8_t camera_id;
    uint8_t _padding[2];  // Align to 8 bytes
};
#pragma pack(pop)

bool generateULog(const QString &filename, const QList<CameraCaptureEvent> &events)
{
    if (events.isEmpty()) {
        qWarning() << "No events to write";
        return false;
    }

    try {
        // Create writer with start timestamp
        ulog_cpp::SimpleWriter writer(filename.toStdString(), events.first().timestamp_us);

        // Write header info
        writer.writeInfo("sys_name", std::string("QGC_TEST"));
        writer.writeInfo("ver_hw", std::string("TEST"));

        // Define camera_capture message format
        // Fields ordered by decreasing size to avoid padding
        std::vector<ulog_cpp::Field> fields = {
            {"uint64_t", "timestamp"},
            {"uint64_t", "timestamp_utc"},
            {"double", "lat"},
            {"double", "lon"},
            {"float", "alt"},
            {"float", "ground_distance"},
            {"float", "q", 4},  // Array of 4 floats
            {"uint32_t", "seq"},
            {"int8_t", "result"},
            {"uint8_t", "camera_id"},
        };
        writer.writeMessageFormat("camera_capture", fields);

        // Complete header
        writer.headerComplete();

        // Subscribe to camera_capture
        const uint16_t msgId = writer.writeAddLoggedMessage("camera_capture");

        // Write events
        for (const CameraCaptureEvent &event : events) {
            CameraCaptureMessage msg{};
            msg.timestamp = event.timestamp_us;
            msg.timestamp_utc = event.timestamp_utc_us;
            msg.lat = event.coordinate.latitude();
            msg.lon = event.coordinate.longitude();
            msg.alt = static_cast<float>(event.coordinate.altitude());
            msg.ground_distance = event.groundDistance;
            msg.q[0] = event.q[0];
            msg.q[1] = event.q[1];
            msg.q[2] = event.q[2];
            msg.q[3] = event.q[3];
            msg.seq = event.seq;
            msg.result = event.result;
            msg.camera_id = event.cameraId;

            writer.writeData(msgId, msg);
        }

        writer.fsync();
        return true;

    } catch (const std::exception &e) {
        qWarning() << "Failed to generate ULog:" << e.what();
        return false;
    }
}

QList<CameraCaptureEvent> generateSampleEvents(int count, uint64_t startTimestamp_us, double intervalSec)
{
    QList<CameraCaptureEvent> events;
    events.reserve(count);

    // Sample coordinates along a flight path
    const double startLat = 47.397742;  // Zurich area
    const double startLon = 8.545594;
    const double altitude = 100.0;

    const uint64_t intervalUs = static_cast<uint64_t>(intervalSec * 1e6);

    for (int i = 0; i < count; ++i) {
        CameraCaptureEvent event;
        event.timestamp_us = startTimestamp_us + (i * intervalUs);
        event.timestamp_utc_us = event.timestamp_us;  // Same for simplicity
        event.seq = static_cast<uint32_t>(i);

        // Move slightly for each capture
        event.coordinate = QGeoCoordinate(
            startLat + (i * 0.0001),  // ~11m per step
            startLon + (i * 0.0001),
            altitude + (i * 0.5)  // Slight altitude variation
        );

        event.groundDistance = static_cast<float>(altitude);
        event.result = 1;  // Success
        event.cameraId = 0;

        // Identity quaternion (level attitude)
        event.q[0] = 1.0f;
        event.q[1] = 0.0f;
        event.q[2] = 0.0f;
        event.q[3] = 0.0f;

        events.append(event);
    }

    return events;
}

}  // namespace ULogTestGenerator
