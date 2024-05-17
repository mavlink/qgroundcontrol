#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

Q_DECLARE_LOGGING_CATEGORY(PX4LogParserLog)

namespace PX4LogParser {
    bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::cameraFeedbackPacket> &cameraFeedback);
}
