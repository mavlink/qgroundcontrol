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
#include <QtCore/QtEndian>

QGC_LOGGING_CATEGORY(ExifParserLog, "qgc.analyzeview.exifparser")

namespace ExifParser
{

uint32_t _readUint32(const QByteArray& buf, size_t offset, bool isLittleEndian)
{
    if (buf.size() < offset + 4) {
        qCWarning(ExifParserLog) << "Buffer too small to read uint32 at offset" << offset;
        return 0;
    }
    uint32_t value = *reinterpret_cast<const uint32_t*>(buf.constData() + offset);
    return isLittleEndian ? qFromLittleEndian(value) : qFromBigEndian(value);
}

uint16_t _readUint16(const QByteArray& buf, size_t offset, bool isLittleEndian)
{
    if (buf.size() < offset + 2) {
        qCWarning(ExifParserLog) << "Buffer too small to read uint16 at offset" << offset;
        return 0;
    }
    uint16_t value = *reinterpret_cast<const uint16_t*>(buf.constData() + offset);
    return isLittleEndian ? qFromLittleEndian(value) : qFromBigEndian(value);
}

bool _readIFDTagValue(const QByteArray& buf, size_t ifdOffset, bool isLittleEndian, uint16_t tag, uint32_t& value)
{
    const size_t bytesNumDirEntries = 2;
    const size_t bytesPerIFDEntry = 12;

    if (buf.size() < ifdOffset + bytesNumDirEntries + bytesPerIFDEntry) {
        qCWarning(ExifParserLog) << "Buffer too small to read IFD tag";
        return false;
    }

    // Read the number of directory entries
    size_t numEntries = _readUint16(buf, ifdOffset, isLittleEndian);
    ifdOffset += bytesNumDirEntries;

    // Iterate through the IFD entries looking for the specified tag
    for (size_t i = 0; i < numEntries; ++i) {
        // IFD entry format: Tag (2 bytes), Type (2 bytes), Count (4 bytes), Value (4 bytes)
        size_t entryOffset = ifdOffset + (i * bytesPerIFDEntry);
        const size_t offsetToValue = 8;

        uint16_t foundTag = _readUint16(buf, entryOffset, isLittleEndian);

        if (foundTag == tag) {
            value = _readUint32(buf, entryOffset + offsetToValue, isLittleEndian);
            return true;
        }
    }

    qCWarning(ExifParserLog) << "EXIF DateTime tag not found";
    return false;
}

QDateTime readTime(const QByteArray& buffer)
{
    // Check for JPEG SOI marker (Start of Image)
    QByteArray jpegHeader("\xFF\xD8", 2);
    if (buffer.size() < 2 || buffer.first(2) != jpegHeader) {
        qCWarning(ExifParserLog) << "Not a valid JPEG file";
        return QDateTime();
    }

    // Search for the APP1 marker (EXIF metadata)
    size_t offset = 2;
    QByteArray app1Marker("\xFF\xE1", 2);
    size_t app1MarkerIndex = buffer.indexOf(app1Marker, offset);
    if (app1MarkerIndex == -1) {
        qCWarning(ExifParserLog) << "APP1 marker not found in JPEG file";
        return QDateTime();
    }

    // Found APP1 marker
    size_t exifStart = offset + 4;

    // Check for "Exif\0\0" header
    QByteArray exifHeader("\x45\x78\x69\x66\x00\x00", 6);
    if (buffer.mid(exifStart, 6) != exifHeader) {
        qCWarning(ExifParserLog) << "EXIF header not found in APP1 segment";
        return QDateTime();
    }

    // Determine endianness (II for little-endian, MM for big-endian)
    size_t tiffHeader = exifStart + 6;
    bool isLittleEndian = (buffer[tiffHeader] == 'I' && buffer[tiffHeader + 1] == 'I');

    // Read the offset to the EXIF IFD from IFD0
    const size_t ifd0Offset = _readUint32(buffer, tiffHeader + 4, isLittleEndian);
    const uint16_t tagExifOffset = 0x8769;
    uint32_t exifIFDOffset = 0;
    if (!_readIFDTagValue(buffer, tiffHeader + ifd0Offset, isLittleEndian, tagExifOffset, exifIFDOffset)) {
        qCWarning(ExifParserLog) << "EXIF IFD0 offset tag not found";
        return QDateTime();
    }

    // Read the offset value for DateTimeDigitized tag (0x0132)
    const uint16_t tagDateTimeDigitized = 0x9004;
    uint32_t dateTimeValueOffset = 0;
    if (!_readIFDTagValue(buffer, tiffHeader + exifIFDOffset, isLittleEndian, tagDateTimeDigitized, dateTimeValueOffset)) {
        qCWarning(ExifParserLog) << "EXIF IFD DateTimeDigitized tag not found";
        return QDateTime();
    }

    QString strDateTime(buffer.constData() + tiffHeader + dateTimeValueOffset);
    return QDateTime::fromString(strDateTime, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
}

bool write(QByteArray &buf, const GeoTagWorker::CameraFeedbackPacket &geotag)
{
    QByteArray app1Header("\xff\xe1", 2);
    uint32_t app1HeaderInd = buf.indexOf(app1Header);
    uint16_t *conversionPointer = reinterpret_cast<uint16_t *>(buf.mid(app1HeaderInd + 2, 2).data());
    uint16_t app1Size = *conversionPointer;
    uint16_t app1SizeEndian = qFromBigEndian(app1Size) + 0xa5;  // change wrong endian
    QByteArray tiffHeader("\x49\x49\x2A", 3);
    uint32_t tiffHeaderInd = buf.indexOf(tiffHeader);
    conversionPointer = reinterpret_cast<uint16_t *>(buf.mid(tiffHeaderInd + 8, 2).data());
    uint16_t numberOfTiffFields  = *conversionPointer;
    uint32_t nextIfdOffsetInd = tiffHeaderInd + 10 + 12 * (numberOfTiffFields);
    conversionPointer = reinterpret_cast<uint16_t *>(buf.mid(nextIfdOffsetInd, 2).data());
    uint16_t nextIfdOffset = *conversionPointer;

    // Definition of useful unions and structs
    union char2uint32_u {
        char c[4];
        uint32_t i;
    };
    union char2uint16_u {
        char c[2];
        uint16_t i;
    };
    // This struct describes a standart field used in exif files
    struct field_s {
        uint16_t tagID;  // Describes which information is added here, e.g. GPS Lat
        uint16_t type;  // Describes the data type, e.g. string, uint8_t,...
        uint32_t size;  // Describes the size
        uint32_t content;  // Either contains the information, or the offset to the exif header where the information is stored (if 32 bits is not enough)
    };
    // This struct contains all the fields that we want to add to the image
    struct fields_s {
        field_s gpsVersion;
        field_s gpsLatRef;
        field_s gpsLat;
        field_s gpsLonRef;
        field_s gpsLon;
        field_s gpsAltRef;
        field_s gpsAlt;
        field_s gpsMapDatum;
        uint32_t finishedDataField;
    };
    // These are the additional information that can not be put into a single uin32_t
    struct extended_s {
        uint32_t gpsLat[6];
        uint32_t gpsLon[6];
        uint32_t gpsAlt[2];
        char mapDatum[7];// = {0x57,0x47,0x53,0x2D,0x38,0x34,0x00};
    };
    // This struct contains all the information we want to add to the image
    struct readable_s {
        fields_s fields;
        extended_s extendedData;
    };

    // This union is used because for writing the information we have to use a char array, but we still want the information to be available in a more descriptive way
    union {
        char c[0xa3];
        readable_s readable;
    } gpsData;


    char2uint32_u gpsIFDInd;
    gpsIFDInd.i = nextIfdOffset;

    // this will stay constant
    QByteArray gpsInfo("\x25\x88\x04\x00\x01\x00\x00\x00", 8);
    gpsInfo.append(gpsIFDInd.c[0]);
    gpsInfo.append(gpsIFDInd.c[1]);
    gpsInfo.append(gpsIFDInd.c[2]);
    gpsInfo.append(gpsIFDInd.c[3]);

    // filling values to gpsData
    uint32_t gpsDataExtInd = gpsIFDInd.i + 2 + sizeof(fields_s);

    // Filling up the fields with the corresponding values
    gpsData.readable.fields.gpsVersion.tagID = 0;
    gpsData.readable.fields.gpsVersion.type = 1;
    gpsData.readable.fields.gpsVersion.size = 4;
    gpsData.readable.fields.gpsVersion.content = 2;

    gpsData.readable.fields.gpsLatRef.tagID = 1;
    gpsData.readable.fields.gpsLatRef.type = 2;
    gpsData.readable.fields.gpsLatRef.size = 2;
    gpsData.readable.fields.gpsLatRef.content = geotag.latitude > 0 ? 'N' : 'S';

    gpsData.readable.fields.gpsLat.tagID = 2;
    gpsData.readable.fields.gpsLat.type = 5;
    gpsData.readable.fields.gpsLat.size = 3;
    gpsData.readable.fields.gpsLat.content = gpsDataExtInd;

    gpsData.readable.fields.gpsLonRef.tagID = 3;
    gpsData.readable.fields.gpsLonRef.type = 2;
    gpsData.readable.fields.gpsLonRef.size = 2;
    gpsData.readable.fields.gpsLonRef.content = geotag.longitude > 0 ? 'E' : 'W';

    gpsData.readable.fields.gpsLon.tagID = 4;
    gpsData.readable.fields.gpsLon.type = 5;
    gpsData.readable.fields.gpsLon.size = 3;
    gpsData.readable.fields.gpsLon.content = gpsDataExtInd + 6 * 4;

    gpsData.readable.fields.gpsAltRef.tagID = 5;
    gpsData.readable.fields.gpsAltRef.type = 1;
    gpsData.readable.fields.gpsAltRef.size = 1;
    gpsData.readable.fields.gpsAltRef.content = 0x00;

    gpsData.readable.fields.gpsAlt.tagID = 6;
    gpsData.readable.fields.gpsAlt.type = 5;
    gpsData.readable.fields.gpsAlt.size = 1;
    gpsData.readable.fields.gpsAlt.content = gpsDataExtInd + 6 * 4 * 2;

    gpsData.readable.fields.gpsMapDatum.tagID = 18;
    gpsData.readable.fields.gpsMapDatum.type = 2;
    gpsData.readable.fields.gpsMapDatum.size = 7;
    gpsData.readable.fields.gpsMapDatum.content = gpsDataExtInd + 6 * 4 * 2 + 2 * 4;

    gpsData.readable.fields.finishedDataField = 0;

    // Filling up the additional information that does not fit into the fields
    gpsData.readable.extendedData.gpsLat[0] = abs(static_cast<int>(geotag.latitude));
    gpsData.readable.extendedData.gpsLat[1] = 1;
    gpsData.readable.extendedData.gpsLat[2] = static_cast<int>((fabs(geotag.latitude) - floor(fabs(geotag.latitude))) * 60.0);
    gpsData.readable.extendedData.gpsLat[3] = 1;
    gpsData.readable.extendedData.gpsLat[4] = static_cast<int>((fabs(geotag.latitude) * 60.0 - floor(fabs(geotag.latitude) * 60.0)) * 60000.0);
    gpsData.readable.extendedData.gpsLat[5] = 1000;

    gpsData.readable.extendedData.gpsLon[0] = abs(static_cast<int>(geotag.longitude));
    gpsData.readable.extendedData.gpsLon[1] = 1;
    gpsData.readable.extendedData.gpsLon[2] = static_cast<int>((fabs(geotag.longitude) - floor(fabs(geotag.longitude))) * 60.0);
    gpsData.readable.extendedData.gpsLon[3] = 1;
    gpsData.readable.extendedData.gpsLon[4] = static_cast<int>((fabs(geotag.longitude) * 60.0 - floor(fabs(geotag.longitude) * 60.0)) * 60000.0);
    gpsData.readable.extendedData.gpsLon[5] = 1000;

    gpsData.readable.extendedData.gpsAlt[0] = geotag.altitude * 100;
    gpsData.readable.extendedData.gpsAlt[1] = 100;
    gpsData.readable.extendedData.mapDatum[0] = 'W';
    gpsData.readable.extendedData.mapDatum[1] = 'G';
    gpsData.readable.extendedData.mapDatum[2] = 'S';
    gpsData.readable.extendedData.mapDatum[3] = '-';
    gpsData.readable.extendedData.mapDatum[4] = '8';
    gpsData.readable.extendedData.mapDatum[5] = '4';
    gpsData.readable.extendedData.mapDatum[6] = 0x00;

    // remove 12 spaces from image description, as otherwise we need to loop through every field and correct the new address values
    buf.remove(nextIfdOffsetInd + 4, 12);
    // TODO correct size in image description
    // insert Gps Info to image file
    buf.insert(nextIfdOffsetInd, gpsInfo.constData(), 12);
    char numberOfFields[2] = {0x08, 0x00};
    // insert number of gps specific fields that we want to add
    buf.insert(gpsIFDInd.i + tiffHeaderInd, numberOfFields, 2);
    // insert the gps data
    buf.insert(gpsIFDInd.i + 2 + tiffHeaderInd, gpsData.c, 0xa3);

    // update the new file size and exif offsets
    char2uint16_u converter;
    converter.i = qToBigEndian(app1SizeEndian);
    buf.replace(app1HeaderInd + 2, 2, converter.c, 2);
    converter.i = nextIfdOffset + 12 + 0xa5;
    buf.replace(nextIfdOffsetInd + 12, 2, converter.c, 2);

    converter.i = (numberOfTiffFields) + 1;
    buf.replace(tiffHeaderInd + 8, 2, converter.c, 2);
    return true;
}

} // namespace ExifParser
