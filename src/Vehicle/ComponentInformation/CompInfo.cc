#include "CompInfo.h"

#include "JsonParsing.h"
#include "JsonSchemaValidator.h"
#include "VehicleFileFetch.h"

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

CompInfo::ParsedJson CompInfo::parseJsonFile(const QString& fileName, const QString& schemaResourcePath)
{
    ParsedJson json;
    json.read = JsonParsing::isJsonFile(fileName, json.document, json.readError, VehicleFileFetch::kMaxSmallFileBytes);
    if (json.read) {
        (void) JsonSchemaValidator::validate(json.document, schemaResourcePath, json.schemaError);
    }
    return json;
}

CompInfo::FileParser CompInfo::fileParser() const
{
    return [schema = jsonSchema()](const QString& fileName) { return parseJsonFile(fileName, schema); };
}
