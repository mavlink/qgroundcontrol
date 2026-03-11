#pragma once

#include <QtCore/QLoggingCategory>

#include "GeoTagData.h"

class QByteArray;

Q_DECLARE_LOGGING_CATEGORY(ExifParserLog)

namespace ExifParser
{
    QDateTime readTime(const QByteArray &buf);
    bool write(QByteArray &buf, const GeoTagData &geotag);
}
