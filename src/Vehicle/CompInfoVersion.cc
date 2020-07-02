/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoVersion.h"
#include "JsonHelper.h"
#include "FactMetaData.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(CompInfoVersionLog, "CompInfoVersionLog")

const char* CompInfoVersion::_jsonSupportedCompMetadataTypesKey = "supportedCompMetadataTypes";

CompInfoVersion::CompInfoVersion(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo  (COMP_METADATA_TYPE_VERSION, compId, vehicle, parent)
{

}

void CompInfoVersion::setJson(const QString& metadataJsonFileName, const QString& /*translationJsonFileName*/)
{
    if (metadataJsonFileName.isEmpty()) {
        return;
    }

    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonHelper::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
        qCWarning(CompInfoVersionLog) << "Metadata json file open failed: compid:" << compId << errorString;
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,           QJsonValue::Double, true },
        { _jsonSupportedCompMetadataTypesKey,   QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(jsonObj, keyInfoList, errorString)) {
        qCWarning(CompInfoVersionLog) << "Metadata json validation failed: compid:" << compId << errorString;
        return;
    }

    int version = jsonObj[JsonHelper::jsonVersionKey].toInt();
    if (version != 1) {
        qCWarning(CompInfoVersionLog) << "Metadata json unsupported version" << version;
        return;
    }

    QJsonArray rgSupportedTypes = jsonObj[_jsonSupportedCompMetadataTypesKey].toArray();
    for (const QJsonValue& typeValue: rgSupportedTypes) {
        _supportedTypes.append(static_cast<COMP_METADATA_TYPE>(typeValue.toInt()));
    }
}
