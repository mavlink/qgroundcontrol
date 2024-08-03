#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

class QByteArray;
class QString;

Q_DECLARE_LOGGING_CATEGORY(ULogParserLog)

namespace ULogParser {
    /// Get GeoTags from a ULog
    ///     @return true if failed, errorMessage set
    bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::cameraFeedbackPacket> &cameraFeedback, QString &errorMessage);
} // namespace ULogParser
