/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QQmlEngine>

#include "CustomActionManager.h"
#include "CustomAction.h"
#include "JsonHelper.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

CustomActionManager::CustomActionManager(QObject* parent)
    : QObject(parent)
{

}

CustomActionManager::CustomActionManager(Fact* actionFileNameFact, QObject* parent)
    : QObject(parent)
{
    setActionFileNameFact(actionFileNameFact);
}

void CustomActionManager::setActionFileNameFact(Fact* actionFileNameFact) 
{
    _actionFileNameFact = actionFileNameFact;
    emit actionFileNameFactChanged();
    connect(_actionFileNameFact, &Fact::rawValueChanged, this, &CustomActionManager::_loadActionsFile);

    _loadActionsFile();
}

void CustomActionManager::_loadActionsFile() 
{
    _actions.clearAndDeleteContents();
    QString actionFileName = _actionFileNameFact->rawValue().toString();
    if (actionFileName.isEmpty()) {
        return;
    }

    // Custom actions are always loaded from the custom actions save path
    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->customActionsSavePath();
    QDir saveDir(savePath);
    QString fullPath = saveDir.absoluteFilePath(actionFileName);

    // It's ok for the file to not exist
    QFileInfo fileInfo(fullPath);
    if (!fileInfo.exists()) {
        return;
    }

    const char* kQgcFileType = "CustomActions";
    const char* kActionListKey = "actions";

    _actions.clearAndDeleteContents();

    QString errorString;
    int version;
    QJsonObject jsonObject = JsonHelper::openInternalQGCJsonFile(fullPath, kQgcFileType, 1, 1, version, errorString);
    if (!errorString.isEmpty()) {
        qgcApp()->showAppMessage(tr("Failed to load custom actions file: `%1` error: `%2`").arg(fullPath).arg(errorString));
        return;
    }

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { kActionListKey, QJsonValue::Array, /* required= */ true },
    };
    if (!JsonHelper::validateKeys(jsonObject, keyInfoList, errorString)) {
        qgcApp()->showAppMessage(tr("Custom actions file - incorrect format: %1").arg(errorString));
        return;
    }

    QJsonArray actionList = jsonObject[kActionListKey].toArray();
    for (auto actionJson: actionList) {
        if (!actionJson.isObject()) {
            qgcApp()->showAppMessage(tr("Custom actions file - incorrect format: JsonValue not an object"));
            _actions.clearAndDeleteContents();
            return;
        }

        auto actionObj = actionJson.toObject();

        QList<JsonHelper::KeyValidateInfo> actionKeyInfoList = {
            { "label",          QJsonValue::String, /* required= */ true },
            { "description",    QJsonValue::String, /* required= */ true },
            { "mavCmd",         QJsonValue::Double, /* required= */ true },

            { "compId",         QJsonValue::Double, /* required= */ false },
            { "param1",         QJsonValue::Double, /* required= */ false },
            { "param2",         QJsonValue::Double, /* required= */ false },
            { "param3",         QJsonValue::Double, /* required= */ false },
            { "param4",         QJsonValue::Double, /* required= */ false },
            { "param5",         QJsonValue::Double, /* required= */ false },
            { "param6",         QJsonValue::Double, /* required= */ false },
            { "param7",         QJsonValue::Double, /* required= */ false },
        };
        if (!JsonHelper::validateKeys(actionObj, actionKeyInfoList, errorString)) {
            qgcApp()->showAppMessage(tr("Custom actions file - incorrect format: %1").arg(errorString));
            _actions.clearAndDeleteContents();
            return;
        }

        auto label = actionObj["label"].toString();
        auto description = actionObj["description"].toString();
        auto mavCmd = (MAV_CMD)actionObj["mavCmd"].toInt();
        auto compId = (MAV_COMPONENT)actionObj["compId"].toInt(MAV_COMP_ID_AUTOPILOT1);
        auto param1 = actionObj["param1"].toDouble(0.0);
        auto param2 = actionObj["param2"].toDouble(0.0);
        auto param3 = actionObj["param3"].toDouble(0.0);
        auto param4 = actionObj["param4"].toDouble(0.0);
        auto param5 = actionObj["param5"].toDouble(0.0);
        auto param6 = actionObj["param6"].toDouble(0.0);
        auto param7 = actionObj["param7"].toDouble(0.0);

        CustomAction* action = new CustomAction(label, description, mavCmd, compId, param1, param2, param3, param4, param5, param6, param7, this);
        QQmlEngine::setObjectOwnership(action, QQmlEngine::CppOwnership);
        _actions.append(action);
    }
}
