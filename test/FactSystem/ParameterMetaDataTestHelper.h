#pragma once

#include "ParameterMetaData.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryFile>

/// Write a JSON object to a temporary file suitable for ParameterMetaData::loadParameterFactMetaDataFile.
inline bool writeJsonToTempFile(QTemporaryFile &tmpFile, const QByteArray &data)
{
    tmpFile.setFileTemplate(QStringLiteral("XXXXXX.json"));
    if (!tmpFile.open()) {
        return false;
    }
    tmpFile.write(data);
    tmpFile.close();
    return true;
}

/// Instantiate a ParameterMetaData subclass, load JSON data, and return it.
template<typename T>
T *loadMetaDataFromJson(const QByteArray &jsonData, QObject *parent)
{
    QTemporaryFile tmpFile;
    if (!writeJsonToTempFile(tmpFile, jsonData)) {
        return nullptr;
    }

    auto *meta = new T(parent);
    meta->loadParameterFactMetaDataFile(tmpFile.fileName());
    return meta;
}
