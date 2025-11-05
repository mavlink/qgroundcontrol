/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraMetaData.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CameraMetaDataLog, "Camera.CameraMetaData")

CameraMetaData::CameraMetaData(const QString &canonicalName,
                               const QString &brand,
                               const QString &model,
                               double sensorWidth,
                               double sensorHeight,
                               double imageWidth,
                               double imageHeight,
                               double focalLength,
                               bool landscape,
                               bool fixedOrientation,
                               double minTriggerInterval,
                               const QString &deprecatedTranslatedName)
    : canonicalName(canonicalName)
    , brand(brand)
    , model(model)
    , sensorWidth(sensorWidth)
    , sensorHeight(sensorHeight)
    , imageWidth(imageWidth)
    , imageHeight(imageHeight)
    , focalLength(focalLength)
    , landscape(landscape)
    , fixedOrientation(fixedOrientation)
    , minTriggerInterval(minTriggerInterval)
    , deprecatedTranslatedName(deprecatedTranslatedName)
{
    qCDebug(CameraMetaDataLog) << this;
}

CameraMetaData::~CameraMetaData()
{
    qCDebug(CameraMetaDataLog) << this;
}

QList<CameraMetaData*> CameraMetaData::parseCameraMetaData()
{
    QList<CameraMetaData*> cameraList;

    QString errorString;
    int version = 0;
    const QJsonObject jsonObject = JsonHelper::openInternalQGCJsonFile(QStringLiteral(":/json/CameraMetaData.json"), "CameraMetaData", 1, 1, version, errorString);
    if (!errorString.isEmpty()) {
        qCWarning(CameraMetaDataLog) << "Internal Error:" << errorString;
        return cameraList;
    }

    static const QList<JsonHelper::KeyValidateInfo> rootKeyInfoList = {
        { "cameraMetaData", QJsonValue::Array, true }
    };
    if (!JsonHelper::validateKeys(jsonObject, rootKeyInfoList, errorString)) {
        qCWarning(CameraMetaDataLog) << errorString;
        return cameraList;
    }

    static const QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
        { "canonicalName", QJsonValue::String, true },
        { "brand", QJsonValue::String, true },
        { "model", QJsonValue::String, true },
        { "sensorWidth", QJsonValue::Double, true },
        { "sensorHeight", QJsonValue::Double, true },
        { "imageWidth", QJsonValue::Double, true },
        { "imageHeight", QJsonValue::Double, true },
        { "focalLength", QJsonValue::Double, true },
        { "landscape", QJsonValue::Bool, true },
        { "fixedOrientation", QJsonValue::Bool, true },
        { "minTriggerInterval", QJsonValue::Double, true },
        { "deprecatedTranslatedName", QJsonValue::String, true },
    };
    const QJsonArray cameraInfo = jsonObject["cameraMetaData"].toArray();
    for (const QJsonValue &jsonValue : cameraInfo) {
        if (!jsonValue.isObject()) {
            qCWarning(CameraMetaDataLog) << "Entry in CameraMetaData array is not object";
            return cameraList;
        }

        const QJsonObject obj = jsonValue.toObject();
        if (!JsonHelper::validateKeys(obj, cameraKeyInfoList, errorString)) {
            qCWarning(CameraMetaDataLog) << errorString;
            return cameraList;
        }

        const QString canonicalName = obj["canonicalName"].toString();
        const QString brand = obj["brand"].toString();
        const QString model = obj["model"].toString();
        const double sensorWidth = obj["sensorWidth"].toDouble();
        const double sensorHeight = obj["sensorHeight"].toDouble();
        const double imageWidth = obj["imageWidth"].toDouble();
        const double imageHeight = obj["imageHeight"].toDouble();
        const double focalLength = obj["focalLength"].toDouble();
        const bool landscape = obj["landscape"].toBool();
        const bool fixedOrientation = obj["fixedOrientation"].toBool();
        const double minTriggerInterval = obj["minTriggerInterval"].toDouble();
        const QString deprecatedTranslatedName = obj["deprecatedTranslatedName"].toString();

        CameraMetaData *metaData = new CameraMetaData(
            canonicalName, brand, model, sensorWidth, sensorHeight,
            imageWidth, imageHeight, focalLength, landscape,
            fixedOrientation, minTriggerInterval, deprecatedTranslatedName);
        cameraList.append(metaData);
    }

    return cameraList;
}
