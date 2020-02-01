/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionCommandList.h"
#include "FactMetaData.h"
#include "Vehicle.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
#include "JsonHelper.h"
#include "MissionCommandUIInfo.h"

#include <QStringList>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QDebug>
#include <QFile>

const char* MissionCommandList::_versionJsonKey =       "version";
const char* MissionCommandList::_mavCmdInfoJsonKey =    "mavCmdInfo";

MissionCommandList::MissionCommandList(const QString& jsonFilename, bool baseCommandList, QObject* parent)
    : QObject(parent)
{
    _loadMavCmdInfoJson(jsonFilename, baseCommandList);
}

void MissionCommandList::_loadMavCmdInfoJson(const QString& jsonFilename, bool baseCommandList)
{
    if (jsonFilename.isEmpty()) {
        return;
    }

    qCDebug(MissionCommandsLog) << "Loading" << jsonFilename;

    QFile jsonFile(jsonFilename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file" << jsonFilename << jsonFile.errorString();
        return;
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() << jsonFilename << "Unable to open json document" << jsonParseError.errorString();
        return;
    }

    QJsonObject json = doc.object();

    int version = json.value(_versionJsonKey).toInt();
    if (version != 1) {
        qWarning() << jsonFilename << "Invalid version" << version;
        return;
    }

    QJsonValue jsonValue = json.value(_mavCmdInfoJsonKey);
    if (!jsonValue.isArray()) {
        qWarning() << jsonFilename << "mavCmdInfo not array";
        return;
    }

    // Iterate over MissionCommandUIInfo objects
    QJsonArray jsonArray = jsonValue.toArray();
    for(QJsonValue info: jsonArray) {
        if (!info.isObject()) {
            qWarning() << jsonFilename << "mavCmdArray should contain objects";
            return;
        }

        MissionCommandUIInfo* uiInfo = new MissionCommandUIInfo(this);

        QString errorString;
        if (!uiInfo->loadJsonInfo(info.toObject(), baseCommandList, errorString)) {
            uiInfo->deleteLater();
            qWarning() << jsonFilename << errorString;
            return;
        }

        // Update list of known categories
        QString newCategory = uiInfo->category();
        if (!newCategory.isEmpty() && !_categories.contains(newCategory)) {
            _categories.append(newCategory);
        }

        _infoMap[uiInfo->command()] = uiInfo;
    }

    // Build id list
    for (MAV_CMD id: _infoMap.keys()) {
        _ids << id;
    }
}

MissionCommandUIInfo* MissionCommandList::getUIInfo(MAV_CMD command) const
{
    if (!_infoMap.contains(command)) {
        return nullptr;
    }

    return _infoMap[command];
}
