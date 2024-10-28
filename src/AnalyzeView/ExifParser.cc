/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ExifParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>

#include <exiv2/exiv2.hpp>

QGC_LOGGING_CATEGORY(ExifParserLog, "qgc.analyzeview.exifparser")

namespace ExifParser
{

void init()
{
    Exiv2::XmpParser::initialize();
    ::atexit(Exiv2::XmpParser::terminate);
}

QDateTime readTime(const QByteArray &buf)
{
    try {
        // Convert QByteArray to std::string for Exiv2
        const Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(reinterpret_cast<const Exiv2::byte*>(buf.constData()), buf.size());
        image->readMetadata();

        const Exiv2::ExifData &exifData = image->exifData();
        if (exifData.empty()) {
            qCWarning(ExifParserLog) << "No EXIF data found in the image.";
            return QDateTime();
        }

        // Read DateTimeOriginal
        const Exiv2::ExifKey key("Exif.Photo.DateTimeOriginal");
        // Exiv2::ExifData::const_iterator it = dateTimeOriginal(exifData);
        const Exiv2::ExifData::const_iterator pos = exifData.findKey(key);
        if (pos == exifData.end()) {
            qCWarning(ExifParserLog) << "No DateTimeOriginal found.";
            return QDateTime();
        }

        const std::string dateTimeOriginal = pos->toString();
        const QString createDate = QString::fromStdString(dateTimeOriginal);
        const QStringList createDateList = createDate.split(' ');

        if (createDateList.size() < 2) {
            qCWarning(ExifParserLog) << "Invalid date/time format: " << createDateList;
            return QDateTime();
        }

        const QStringList dateList = createDateList[0].split(':');
        const QStringList timeList = createDateList[1].split(':');

        if ((dateList.size() < 3) || (timeList.size() < 3)) {
            qCWarning(ExifParserLog) << "Could not parse creation date/time: " << dateList << " " << timeList;
            return QDateTime();
        }

        const QDate date(dateList[0].toInt(), dateList[1].toInt(), dateList[2].toInt());
        const QTime time(timeList[0].toInt(), timeList[1].toInt(), timeList[2].toInt());

        const QDateTime tagTime(date, time);

        return tagTime;
    } catch (const Exiv2::Error &e) {
        qCWarning(ExifParserLog) << "Error reading EXIF data:" << e.what();
        return QDateTime();
    }
}

bool write(QByteArray &buf, const GeoTagWorker::CameraFeedbackPacket &geotag)
{
    try {
        // Convert QByteArray to std::string for Exiv2
        const Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(reinterpret_cast<const Exiv2::byte*>(buf.constData()), buf.size());
        image->readMetadata();

        Exiv2::ExifData &exifData = image->exifData();

        // Set GPSVersionID
        exifData["Exif.GPSInfo.GPSVersionID"] = "2 2 0 0";

        // Set GPS map datum
        exifData["Exif.GPSInfo.GPSMapDatum"] = "WGS-84";

        // Latitude in degrees, minutes, seconds
        const double latitude = std::fabs(geotag.latitude); // Absolute value for conversion
        const int latDegrees = static_cast<int>(latitude);
        const int latMinutes = static_cast<int>((latitude - latDegrees) * 60);
        const double latSeconds = (latitude - latDegrees - latMinutes / 60.0) * 3600.0;

        // Set GPS latitude
        exifData["Exif.GPSInfo.GPSLatitudeRef"] = (geotag.latitude > 0) ? "N" : "S";
        exifData["Exif.GPSInfo.GPSLatitude"] =
            std::to_string(latDegrees) + "/1 " +
            std::to_string(latMinutes) + "/1 " +
            std::to_string(static_cast<int>(latSeconds * 1000)) + "/1000";

        // Longitude in degrees, minutes, seconds
        const double longitude = std::fabs(geotag.longitude);
        const int lonDegrees = static_cast<int>(longitude);
        const int lonMinutes = static_cast<int>((longitude - lonDegrees) * 60);
        const double lonSeconds = (longitude - lonDegrees - lonMinutes / 60.0) * 3600.0;

        // Set GPS longitude
        exifData["Exif.GPSInfo.GPSLongitudeRef"] = (geotag.longitude > 0) ? "E" : "W";
        exifData["Exif.GPSInfo.GPSLongitude"] =
            std::to_string(lonDegrees) + "/1 " +
            std::to_string(lonMinutes) + "/1 " +
            std::to_string(static_cast<int>(lonSeconds * 1000)) + "/1000";

        // Set GPS altitude
        exifData["Exif.GPSInfo.GPSAltitudeRef"] = (geotag.altitude < 0) ? 1 : 0;
        exifData["Exif.GPSInfo.GPSAltitude"] = std::to_string(static_cast<uint32_t>(geotag.altitude * 100)) + "/100";

        // Write the updated metadata back to the buffer
        image->setExifData(exifData);
        image->writeMetadata();

        // Update the buffer with new image data
        buf = QByteArray(reinterpret_cast<const char*>(image->io().mmap()), image->io().size());
        return true;
    } catch (Exiv2::Error& e) {
        qCWarning(ExifParserLog) << "Error writing EXIF GPS data:" << e.what();
        return false;
    }
}

} // namespace ExifParser
