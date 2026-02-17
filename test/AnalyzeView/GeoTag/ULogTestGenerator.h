#pragma once

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

namespace ULogTestGenerator
{

struct CameraCaptureEvent
{
    uint64_t timestamp_us = 0;
    uint64_t timestamp_utc_us = 0;
    uint32_t seq = 0;
    QGeoCoordinate coordinate;
    float groundDistance = 0.0f;
    float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion
    int8_t result = 1;  // 1 = Success
    uint8_t cameraId = 0;
};

/// Generate a ULog file with camera_capture events
/// @param filename Output file path
/// @param events List of camera capture events
/// @return true if successful
bool generateULog(const QString &filename, const QList<CameraCaptureEvent> &events);

/// Generate sample events for testing (matching timestamps and valid coordinates)
/// @param count Number of events to generate
/// @param startTimestamp_us Starting timestamp in microseconds
/// @param intervalSec Interval between events in seconds
/// @return List of generated events
QList<CameraCaptureEvent> generateSampleEvents(int count, uint64_t startTimestamp_us = 1000000,
                                                double intervalSec = 2.0);

}  // namespace ULogTestGenerator
