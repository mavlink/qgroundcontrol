#include "CompInfoGeneral.h"
#include "JsonHelper.h"
#include "JsonParsing.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>

QGC_LOGGING_CATEGORY(CompInfoGeneralLog, "ComponentInformation.CompInfoGeneral")

CompInfoGeneral::CompInfoGeneral(uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_GENERAL, compId_, vehicle_, parent)
{

}

void CompInfoGeneral::setUris(CompInfo &compInfo) const
{
    const auto& metadataTypeIter = _supportedTypes.constFind(compInfo.type);
    if (metadataTypeIter == _supportedTypes.constEnd()) {
        compInfo._uris = {}; // reset
    } else {
        compInfo._uris = *metadataTypeIter;
    }
}

void CompInfoGeneral::setJson(const QString& metadataJsonFileName)
{
    if (metadataJsonFileName.isEmpty()) {
        return;
    }

    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonParsing::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
        qCWarning(CompInfoGeneralLog) << "Metadata json file open failed: compid:" << compId << errorString;
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,           QJsonValue::Double, true },
        { _jsonMetadataTypesKey,   QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(jsonObj, keyInfoList, errorString)) {
        qCWarning(CompInfoGeneralLog) << "Metadata json validation failed: compid:" << compId << errorString;
        return;
    }

    int version = jsonObj[JsonHelper::jsonVersionKey].toInt();
    if (version != 1) {
        qCWarning(CompInfoGeneralLog) << "Metadata json unsupported version" << version;
        return;
    }

    QJsonArray rgSupportedTypes = jsonObj[_jsonMetadataTypesKey].toArray();
    for (QJsonValue typeValue : rgSupportedTypes) {
        int metadataType = typeValue["type"].toInt(-1);
        if (metadataType == -1)
            continue;
        Uris uris;
        uris.uriMetaData = typeValue["uri"].toString();
        uris.crcMetaData = typeValue["fileCrc"].toVariant().toLongLong(); // Note: can't use toInt(), as it returns 0 when exceeding 2^31
        uris.crcMetaDataValid = typeValue.toObject().contains("fileCrc");
        uris.uriMetaDataFallback = typeValue["uriFallback"].toString();
        uris.crcMetaDataFallback = typeValue["fileCrcFallback"].toVariant().toLongLong();
        uris.crcMetaDataFallbackValid = typeValue.toObject().contains("fileCrcFallback");
        uris.uriTranslation = typeValue["translationUri"].toString();
        uris.uriTranslationFallback = typeValue["translationUriFallback"].toString();

        if (uris.uriMetaData.isEmpty() || !uris.crcMetaDataValid) {
            // The CRC is optional for dynamically updated metadata, and once we want to support that this logic needs
            // to be updated.
            qCDebug(CompInfoGeneralLog) << "Metadata missing fields: type:uri:crcValid" << metadataType <<
                    uris.uriMetaData << uris.crcMetaDataValid;
            continue;
        }

        _supportedTypes[(COMP_METADATA_TYPE)metadataType] = uris;
        qCDebug(CompInfoGeneralLog) << "Metadata type : uri : crc" << metadataType << uris.uriMetaData << uris.crcMetaData;
    }
}
