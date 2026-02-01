#include "ExifParser.h"
#include "ExifUtility.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>

Q_STATIC_LOGGING_CATEGORY(ExifParserLog, "AnalyzeView.ExifParser")

namespace ExifParser
{

QDateTime readTime(const QByteArray &buffer)
{
    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    if (!data) {
        qCWarning(ExifParserLog) << "Failed to parse EXIF data";
        return QDateTime();
    }

    const QString dateTimeDigitized = ExifUtility::readString(data, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_IFD_EXIF);
    exif_data_unref(data);

    if (dateTimeDigitized.isEmpty()) {
        qCWarning(ExifParserLog) << "DateTimeDigitized tag not found";
        return QDateTime();
    }

    return QDateTime::fromString(dateTimeDigitized, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
}

bool write(QByteArray &buffer, const GeoTagData &geotag)
{
    ExifData *data = ExifUtility::loadFromBuffer(buffer);
    if (!data) {
        data = ExifUtility::createNew();
        if (!data) {
            qCWarning(ExifParserLog) << "Failed to create EXIF data";
            return false;
        }
    }

    const ExifByteOrder order = exif_data_get_byte_order(data);

    // Helper to get existing or create new GPS entry
    auto getOrCreateEntry = [&](ExifTag tag, ExifFormat format, unsigned long components) -> ExifEntry* {
        ExifEntry *entry = exif_content_get_entry(data->ifd[EXIF_IFD_GPS], tag);
        if (!entry) {
            entry = ExifUtility::createTag(data, EXIF_IFD_GPS, tag, format, components);
        }
        return entry;
    };

    // GPS Version ID (2.3.0.0)
    ExifEntry *versionEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_VERSION_ID), EXIF_FORMAT_BYTE, 4);
    if (versionEntry && versionEntry->data) {
        versionEntry->data[0] = 2;
        versionEntry->data[1] = 3;
        versionEntry->data[2] = 0;
        versionEntry->data[3] = 0;
    }

    // Latitude
    ExifEntry *latRefEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF), EXIF_FORMAT_ASCII, 2);
    ExifUtility::writeGpsRef(latRefEntry, geotag.coordinate.latitude() >= 0 ? 'N' : 'S');

    ExifEntry *latEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE), EXIF_FORMAT_RATIONAL, 3);
    ExifUtility::writeGpsCoordinate(latEntry, order, geotag.coordinate.latitude());

    // Longitude
    ExifEntry *lonRefEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF), EXIF_FORMAT_ASCII, 2);
    ExifUtility::writeGpsRef(lonRefEntry, geotag.coordinate.longitude() >= 0 ? 'E' : 'W');

    ExifEntry *lonEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE), EXIF_FORMAT_RATIONAL, 3);
    ExifUtility::writeGpsCoordinate(lonEntry, order, geotag.coordinate.longitude());

    // Altitude
    ExifEntry *altRefEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE_REF), EXIF_FORMAT_BYTE, 1);
    ExifUtility::writeGpsAltRef(altRefEntry, geotag.coordinate.altitude() < 0 ? 1 : 0);

    ExifEntry *altEntry = getOrCreateEntry(static_cast<ExifTag>(EXIF_TAG_GPS_ALTITUDE), EXIF_FORMAT_RATIONAL, 1);
    ExifUtility::writeRational(altEntry, order, geotag.coordinate.altitude(), 100);

    // Save back to buffer
    const bool success = ExifUtility::saveToBuffer(data, buffer);
    exif_data_unref(data);

    return success;
}

} // namespace ExifParser
