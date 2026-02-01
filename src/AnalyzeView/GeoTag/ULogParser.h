#pragma once

#include "GeoTagData.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include <ulog_cpp/subscription.hpp>


namespace ULogParser {

/// Parse a TypedDataView sample into GeoTagData
/// @param sample The typed data view from a camera_capture message
/// @return Populated GeoTagData structure
GeoTagData parseGeoTagData(const ulog_cpp::TypedDataView &sample);

/// Get GeoTags from a ULog using streamed parsing (raw memory version)
/// @param data Pointer to the ULog data (can be memory-mapped)
/// @param size Size of the data in bytes
/// @param cameraFeedback Output list of camera capture events
/// @param errorMessage Output error message if parsing fails
/// @return true on success, false on failure with errorMessage set
bool getTagsFromLog(const char *data, qint64 size, QList<GeoTagData> &cameraFeedback, QString &errorMessage);

/// Get GeoTags from a ULog using streamed parsing (QByteArray version)
/// @return true on success, false on failure with errorMessage set
inline bool getTagsFromLog(const QByteArray &log, QList<GeoTagData> &cameraFeedback, QString &errorMessage)
{
    return getTagsFromLog(log.constData(), log.size(), cameraFeedback, errorMessage);
}

} // namespace ULogParser
