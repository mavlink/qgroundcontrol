/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

class QByteArray;

Q_DECLARE_LOGGING_CATEGORY(ExifParserLog)

namespace ExifParser {
    double readTime(const QByteArray &buf);
    double readTime2(const QByteArray &buf);
    bool write(QByteArray &buf, const GeoTagWorker::cameraFeedbackPacket &geotag);
} // namespace ExifParser
