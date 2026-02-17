#include "CompInfo.h"

CompInfo::CompInfo(COMP_METADATA_TYPE type_, uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : QObject   (parent)
    , type      (type_)
    , vehicle   (vehicle_)
    , compId    (compId_)
{

}

void CompInfo::setUriMetaData(const QString &uri, uint32_t crc)
{
    _uris.uriMetaData = uri;
    _uris.crcMetaData = crc;
    _uris.crcMetaDataValid = true;
}
