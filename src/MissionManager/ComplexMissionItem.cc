/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
const char* ComplexMissionItem::_presetNameKey =            "complexItemPresetName";
const char* ComplexMissionItem::_saveCameraInPresetKey =    "complexItemCameraSavedInPreset";
const char* ComplexMissionItem::_builtInPresetKey =         "complexItemBuiltInPreset";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, bool flyView, QObject* parent)
    : VisualMissionItem         (vehicle, flyView, parent)
    , _cameraInPreset           (true)
{

}

const ComplexMissionItem& ComplexMissionItem::operator=(const ComplexMissionItem& other)
{
    VisualMissionItem::operator=(other);

    _cameraInPreset = other._cameraInPreset;

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

void ComplexMissionItem::clearCurrentPreset(void)
{
    _currentPreset.clear();
    emit currentPresetChanged(_currentPreset);
}

void ComplexMissionItem::deleteCurrentPreset(void)
{
    qDebug() << "deleteCurrentPreset" << _currentPreset;
    if (!_currentPreset.isEmpty()) {
        QSettings settings;

        settings.beginGroup(presetsSettingsGroup());
        settings.beginGroup(_presetSettingsKey);
        settings.remove(_currentPreset);
        emit presetNamesChanged();

        clearCurrentPreset();
    }
}

void ComplexMissionItem::_savePresetJson(const QString& name, QJsonObject& presetObject)
{
    presetObject[_presetNameKey] = name;

    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    settings.setValue(name, QJsonDocument(presetObject).toBinaryData());
    emit presetNamesChanged();

    _currentPreset = name;
    emit currentPresetChanged(name);
}

QJsonObject ComplexMissionItem::_loadPresetJson(const QString& name)
{
    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    return QJsonDocument::fromBinaryData(settings.value(name).toByteArray()).object();
}

void ComplexMissionItem::_saveItem(QJsonObject& saveObject)
{
    qDebug() << "_saveItem" << _cameraInPreset;
    saveObject[_presetNameKey] =            _currentPreset;
    saveObject[_saveCameraInPresetKey] =    _cameraInPreset;
    saveObject[_builtInPresetKey] =         _builtInPreset;
}

void ComplexMissionItem::_loadItem(const QJsonObject& saveObject)
{
    _currentPreset =    saveObject[_presetNameKey].toString();
    _cameraInPreset =   saveObject[_saveCameraInPresetKey].toBool(false);
    _builtInPreset =    saveObject[_builtInPresetKey].toBool(false);

    if (!presetNames().contains(_currentPreset)) {
        _currentPreset.clear();
    }

    emit cameraInPresetChanged  (_cameraInPreset);
    emit currentPresetChanged   (_currentPreset);
    emit builtInPresetChanged   (_builtInPreset);
}

void ComplexMissionItem::setCameraInPreset(bool cameraInPreset)
{
    if (cameraInPreset != _cameraInPreset) {
        _cameraInPreset = cameraInPreset;
        emit cameraInPresetChanged(_cameraInPreset);
    }
}

void ComplexMissionItem::setBuiltInPreset(bool builtInPreset)
{
    if (builtInPreset != _builtInPreset) {
        _builtInPreset = builtInPreset;
        emit builtInPresetChanged(_builtInPreset);
    }
}
