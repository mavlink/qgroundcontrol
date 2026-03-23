#include "PowerModulePresetController.h"
#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

QGC_LOGGING_CATEGORY(PowerModulePresetControllerLog, "AutoPilotPlugins.PowerModulePresetController")

PowerModulePresetController::PowerModulePresetController(QObject *parent)
    : FactPanelController(parent)
{
}

QVariantList PowerModulePresetController::powerModulePresets() const
{
    int version;
    QString errorString;
    const QJsonObject root = JsonHelper::openInternalQGCJsonFile(
        QStringLiteral(":/json/PowerModulePresets.json"),
        QStringLiteral("PowerModulePresets"), 1, 1, version, errorString);
    if (root.isEmpty()) {
        qCCritical(PowerModulePresetControllerLog) << errorString;
        return {};
    }

    if (!root.value(QStringLiteral("powerModules")).isArray()) {
        qCCritical(PowerModulePresetControllerLog) << "Missing or invalid 'powerModules' array";
        return {};
    }

    static const QList<JsonHelper::KeyValidateInfo> keyInfo = {
        { "name",       QJsonValue::String, true },
        { "voltMult",   QJsonValue::Double, true },
        { "ampPerVolt", QJsonValue::Double, true },
        { "ampOffset",  QJsonValue::Double, true },
    };

    const QJsonArray modules = root.value(QStringLiteral("powerModules")).toArray();
    QVariantList result;
    result.reserve(modules.size());

    for (int i = 0; i < modules.size(); ++i) {
        if (!modules.at(i).isObject()) {
            qCCritical(PowerModulePresetControllerLog) << "powerModules[" << i << "] is not an object";
            return {};
        }
        const QJsonObject obj = modules.at(i).toObject();

        errorString.clear();
        if (!JsonHelper::validateKeysStrict(obj, keyInfo, errorString)) {
            qCCritical(PowerModulePresetControllerLog) << "powerModules[" << i << "]:" << errorString;
            return {};
        }

        result.append(obj.toVariantMap());
    }

    return result;
}
