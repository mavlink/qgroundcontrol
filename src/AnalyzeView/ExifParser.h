#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

Q_DECLARE_LOGGING_CATEGORY(ExifParserLog)

class ExifParser
{
public:
    ExifParser() = default;
    ~ExifParser() = default;

    double readTime(QByteArray& buf);
    bool write(QByteArray& buf, GeoTagWorker::cameraFeedbackPacket& geotag);

private:
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
    union gpsData_u {
        char c[0xa3];
        readable_s readable;
    };
};
