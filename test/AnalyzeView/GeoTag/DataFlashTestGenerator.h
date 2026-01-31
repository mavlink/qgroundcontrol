#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

namespace DataFlashTestGenerator
{

struct CameraCaptureEvent
{
    uint64_t timestamp_us = 0;
    uint32_t seq = 0;
    QGeoCoordinate coordinate;
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
};

/// Generate a DataFlash log file with CAM messages
/// @param filename Output file path
/// @param events List of camera capture events
/// @return true if successful
bool generateDataFlashLog(const QString &filename, const QList<CameraCaptureEvent> &events);

/// Generate sample events for testing
/// @param count Number of events to generate
/// @param startTimestamp_us Starting timestamp in microseconds
/// @param intervalSec Interval between events in seconds
/// @return List of generated events
QList<CameraCaptureEvent> generateSampleEvents(int count, uint64_t startTimestamp_us = 1000000,
                                                double intervalSec = 2.0);

}  // namespace DataFlashTestGenerator
