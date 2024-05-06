#include "ExifParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtEndian>
#include <QtCore/QDateTime>

QGC_LOGGING_CATEGORY(ExifParserLog, "qgc.analyzeview.exifparser")

double ExifParser::readTime(QByteArray& buf)
{
    QByteArray tiffHeader("\x49\x49\x2A", 3);
    QByteArray createDateHeader("\x04\x90\x02", 3);

    // find header position
    uint32_t tiffHeaderIndex = buf.indexOf(tiffHeader);

    // find creation date header index
    uint32_t createDateHeaderIndex = buf.indexOf(createDateHeader);

    // extract size of date-time string, -1 accounting for null-termination
    uint32_t* sizeString = reinterpret_cast<uint32_t*>(buf.mid(createDateHeaderIndex + 4, 4).data());
    uint32_t createDateStringSize = qFromLittleEndian(*sizeString) - 1;

    // extract location of date-time string
    uint32_t* dataIndex = reinterpret_cast<uint32_t*>(buf.mid(createDateHeaderIndex + 8, 4).data());
    uint32_t createDateStringDataIndex = qFromLittleEndian(*dataIndex) + tiffHeaderIndex;

    // read out data of create date-time field
    QString createDate = buf.mid(createDateStringDataIndex, createDateStringSize);

    QStringList createDateList = createDate.split(' ');
    if (createDateList.count() < 2) {
        qCWarning(ExifParserLog) << "Could not decode creation time and date: " << createDateList;
        return -1.0;
    }
    QStringList dateList = createDateList[0].split(':');
    if (dateList.count() < 3) {
        qCWarning(ExifParserLog) << "Could not decode creation date: " << dateList;
        return -1.0;
    }
    QStringList timeList = createDateList[1].split(':');
    if (timeList.count() < 3) {
        qCWarning(ExifParserLog) << "Could not decode creation time: " << timeList;
        return -1.0;
    }
    QDate date(dateList[0].toInt(), dateList[1].toInt(), dateList[2].toInt());
    QTime time(timeList[0].toInt(), timeList[1].toInt(), timeList[2].toInt());
    QDateTime tagTime(date, time);
    return tagTime.toMSecsSinceEpoch()/1000.0;
}

bool ExifParser::write(QByteArray& buf, GeoTagWorker::cameraFeedbackPacket& geotag)
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

    gpsData_u gpsData;

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
    buf.insert(nextIfdOffsetInd, gpsInfo, 12);
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
