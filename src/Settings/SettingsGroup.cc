/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsGroup.h"
#include "QGCCorePlugin.h"
#include "QGCApplication.h"

SettingsGroup::SettingsGroup(const QString& name, const QString& settingsGroup, QObject* parent)
    : QObject       (parent)
    , _name         (name)
    , _settingsGroup(settingsGroup)
    , _visible      (qgcApp()->toolbox()->corePlugin()->overrideSettingsGroupVisibility(_name))
{
    QString jsonNameFormat(":/json/%1.SettingsGroup.json");

    _nameToMetaDataMap = FactMetaData::createMapFromJsonFile(jsonNameFormat.arg(_name), this);
}

SettingsFact* SettingsGroup::_createSettingsFact(const QString& factName)
{
    return new SettingsFact(_settingsGroup, _nameToMetaDataMap[factName], this);
}

