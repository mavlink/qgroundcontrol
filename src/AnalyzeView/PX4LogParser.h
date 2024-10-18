/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

Q_DECLARE_LOGGING_CATEGORY(PX4LogParserLog)

namespace PX4LogParser {
    bool getTagsFromLog(const QByteArray &log, QList<GeoTagWorker::CameraFeedbackPacket> &cameraFeedback);
}
