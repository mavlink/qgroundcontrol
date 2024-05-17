#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

Q_DECLARE_LOGGING_CATEGORY(ULogParserLog)

namespace ULogParser {
    /// @return true: failed, errorMessage set
    bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::cameraFeedbackPacket> &cameraFeedback, QString &errorMessage);
}
