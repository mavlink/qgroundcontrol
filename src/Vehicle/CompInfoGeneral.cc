/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoGeneral.h"

#include "JsonHelper.h"
#include "FactMetaData.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(CompInfoGeneralLog, "CompInfoGeneralLog")

const char* CompInfoGeneral::_jsonMetadataTypesKey = "metadataTypes";

CompInfoGeneral::CompInfoGeneral(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_GENERAL, compId, vehicle, parent)
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

void CompInfoGeneral::setJson(const QString& metadataJsonFileName, const QString& /*translationJsonFileName*/)
{
    if (metadataJsonFileName.isEmpty()) {
        return;
    }

    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonHelper::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
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
        int type = typeValue["type"].toInt(-1);
        if (type == -1)
            continue;
        Uris uris;
        uris.uriMetaData = typeValue["uri"].toString();
        uris.crcMetaData = typeValue["fileCrc"].toVariant().toLongLong(); // Note: can't use toInt(), as it returns 0 when exceeding 2^31
        uris.crcMetaDataValid = typeValue.toObject().contains("fileCrc");
        uris.uriMetaDataFallback = typeValue["uriFallback"].toString();
        uris.crcMetaDataFallback = typeValue["fileCrcFallback"].toVariant().toLongLong();
        uris.crcMetaDataFallbackValid = typeValue.toObject().contains("fileCrcFallback");
        uris.uriTranslation = typeValue["translationUri"].toString();
        uris.crcTranslation = typeValue["translationFileCrc"].toVariant().toLongLong();
        uris.crcTranslationValid = typeValue.toObject().contains("translationFileCrc");
        uris.uriTranslationFallback = typeValue["translationUriFallback"].toString();
        uris.crcTranslationFallback = typeValue["translationFileCrcFallback"].toVariant().toLongLong();
        uris.crcTranslationFallbackValid = typeValue.toObject().contains("translationFileCrcFallback");

        if (uris.uriMetaData.isEmpty() || !uris.crcMetaDataValid) {
            qCWarning(CompInfoGeneralLog) << "Metadata missing required fields: type:uri:crcValid" << type <<
                    uris.uriMetaData << uris.crcMetaDataValid;
            continue;
        }

        _supportedTypes[(COMP_METADATA_TYPE)type] = uris;
        qCDebug(CompInfoGeneralLog) << "Metadata type : uri : crc" << type << uris.uriMetaData << uris.crcMetaData;
    }
}
