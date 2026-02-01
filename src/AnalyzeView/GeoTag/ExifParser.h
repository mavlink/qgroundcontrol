#pragma once


#include "GeoTagData.h"

class QByteArray;


namespace ExifParser
{
    QDateTime readTime(const QByteArray &buf);
    bool write(QByteArray &buf, const GeoTagData &geotag);
}
