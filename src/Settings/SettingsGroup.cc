/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsGroup.h"
#include "QGCCorePlugin.h"

#include <QtQml/QQmlEngine>

SettingsGroup::SettingsGroup(const QString& name, const QString& settingsGroup, QObject* parent)
    : QObject       (parent)
    , _visible      (QGCCorePlugin::instance()->overrideSettingsGroupVisibility(name))
    , _name         (name)
    , _settingsGroup(settingsGroup)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    _nameToMetaDataMap = FactMetaData::createMapFromJsonFile(QString(kJsonFile).arg(name), this);
}

SettingsFact* SettingsGroup::_createSettingsFact(const QString& factName)
{
    if (_factCache.contains(factName)) {
        return _factCache[factName];
    }

    FactMetaData* const metaData = _nameToMetaDataMap.value(factName, nullptr);
    if (!metaData) {
        qCritical() << "Fact name" << factName << "not found in" << QString(kJsonFile).arg(_name);
        exit(-1);
    }

    SettingsFact* const fact = new SettingsFact(_settingsGroup, metaData, this);
    _factCache.insert(factName, fact);
    return fact;
}

SettingsFact* SettingsGroup::fact(const QString& factName)
{
    return _createSettingsFact(factName);
}

