#ifndef EXIFPARSER_H
#define EXIFPARSER_H

#include <QGeoCoordinate>
#include <QDebug>

class ExifParser
{
public:
    ExifParser();
    ~ExifParser();
    double readTime(QByteArray& buf);
    bool write(QByteArray& data, QGeoCoordinate coordinate);
};

#endif // EXIFPARSER_H
