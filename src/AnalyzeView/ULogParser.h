/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
    bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::CameraFeedbackPacket> &cameraFeedback, QString &errorMessage);
} // namespace ULogParser
