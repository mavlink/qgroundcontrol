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

#include <QSettings>

const char* ComplexMissionItem::jsonComplexItemTypeKey = "complexItemType";

const char* ComplexMissionItem::_presetSettingsKey =        "_presets";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, bool flyView, QObject* parent)
    : VisualMissionItem (vehicle, flyView, parent)
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
    emit presetNamesChanged();
}

QJsonObject ComplexMissionItem::_loadPresetJson(const QString& name)
{
    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    return QJsonDocument::fromBinaryData(settings.value(name).toByteArray()).object();
}
