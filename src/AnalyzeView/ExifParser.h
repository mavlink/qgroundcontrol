#pragma once

#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

class QByteArray;

Q_DECLARE_LOGGING_CATEGORY(ExifParserLog)

namespace ExifParser {
    double readTime(const QByteArray &buf);
    bool write(QByteArray &buf, const GeoTagWorker::cameraFeedbackPacket &geotag);
} // namespace ExifParser
