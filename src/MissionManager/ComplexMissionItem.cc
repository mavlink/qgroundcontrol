/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComplexMissionItem.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"

#include <QSettings>

const char* ComplexMissionItem::jsonComplexItemTypeKey = "complexItemType";

const char* ComplexMissionItem::_presetSettingsKey =        "_presets";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, bool flyView, QObject* parent)
    : VisualMissionItem (vehicle, flyView, parent)
    , _toolbox(qgcApp()->toolbox())
    , _settingsManager(_toolbox->settingsManager())
{

}

const ComplexMissionItem& ComplexMissionItem::operator=(const ComplexMissionItem& other)
{
    VisualMissionItem::operator=(other);

    return *this;
}

QStringList ComplexMissionItem::presetNames(void)
{
    QStringList names;

    QSettings settings;

    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    return settings.childKeys();
}

void ComplexMissionItem::loadPreset(const QString& name)
{
    Q_UNUSED(name);
    qgcApp()->showMessage(tr("This Pattern does not support Presets."));
}

void ComplexMissionItem::savePreset(const QString& name)
{
    Q_UNUSED(name);
    qgcApp()->showMessage(tr("This Pattern does not support Presets."));
}

void ComplexMissionItem::deletePreset(const QString& name)
{
    if (qgcApp()->toolbox()->corePlugin()->options()->surveyBuiltInPresetNames().contains(name)) {
        qgcApp()->showMessage(tr("'%1' is a built-in preset which cannot be deleted.").arg(name));
        return;
    }

    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    settings.remove(name);
    emit presetNamesChanged();
}

void ComplexMissionItem::_savePresetJson(const QString& name, QJsonObject& presetObject)
{
    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    settings.setValue(name, QJsonDocument(presetObject).toBinaryData());

    // Use this to save a survey preset as a JSON file to be included in the build
    // as a built-in survey preset that cannot be deleted.
    #if 0
    QString savePath = _settingsManager->appSettings()->missionSavePath();
    QDir saveDir(savePath);

    QString fileName = saveDir.absoluteFilePath(name);
    fileName.append(".json");
    QFile jsonFile(fileName);

    if (!jsonFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Couldn't open .json file.";
    }

    qDebug() << "Saving survey preset to JSON";
    auto jsonDoc = QJsonDocument(jsonObj);
    jsonFile.write(jsonDoc.toJson());
    #endif

    emit presetNamesChanged();
}

QJsonObject ComplexMissionItem::_loadPresetJson(const QString& name)
{
    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    return QJsonDocument::fromBinaryData(settings.value(name).toByteArray()).object();
}
