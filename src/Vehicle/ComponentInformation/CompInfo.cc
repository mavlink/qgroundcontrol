#include "CompInfo.h"

CompInfo::CompInfo(COMP_METADATA_TYPE type, uint8_t compId, Vehicle* vehicle, QObject* parent)
    : QObject   (parent)
    , type      (type)
    , vehicle   (vehicle)
    , compId    (compId)
{

}

void CompInfo::setUriMetaData(const QString &uri, uint32_t crc)
{
    _uris.uriMetaData = uri;
    _uris.crcMetaData = crc;
    _uris.crcMetaDataValid = true;
}
