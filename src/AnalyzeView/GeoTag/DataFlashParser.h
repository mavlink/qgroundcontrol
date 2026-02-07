#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QtGlobal>


struct GeoTagData;

/// Parser for ArduPilot DataFlash binary logs (.bin files)
/// Extracts camera trigger events (CAM messages) with GPS coordinates
namespace DataFlashParser
{
    /// Parse DataFlash log from raw memory and extract camera capture events
    /// @param data Pointer to the binary log data (can be memory-mapped)
    /// @param size Size of the data in bytes
    /// @param cameraFeedback Output list of camera capture events
    /// @param errorMessage Output error message if parsing fails
    /// @return true if parsing succeeded
    bool getTagsFromLog(const char *data, qint64 size, QList<GeoTagData> &cameraFeedback, QString &errorMessage);

    /// Parse DataFlash log and extract camera capture events
    /// @param logBuffer The binary log file contents
    /// @param cameraFeedback Output list of camera capture events
    /// @param errorMessage Output error message if parsing fails
    /// @return true if parsing succeeded
    inline bool getTagsFromLog(const QByteArray &logBuffer, QList<GeoTagData> &cameraFeedback, QString &errorMessage)
    {
        return getTagsFromLog(logBuffer.constData(), logBuffer.size(), cameraFeedback, errorMessage);
    }
}
