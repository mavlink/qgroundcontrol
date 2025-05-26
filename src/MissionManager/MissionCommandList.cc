/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionCommandList.h"
#include "JsonHelper.h"
#include "MissionCommandUIInfo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>

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

    QString errorString;
    int version;
    QJsonObject jsonObject = JsonHelper::openInternalQGCJsonFile(jsonFilename, qgcFileType, 1, 1, version, errorString);
    if (!errorString.isEmpty()) {
        qWarning() << "Internal Error: " << errorString;
        return;
    }

    QJsonValue jsonValue = jsonObject.value(_mavCmdInfoJsonKey);
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
