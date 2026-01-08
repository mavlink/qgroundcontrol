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

    _nameToMetaDataMap = FactMetaData::createMapFromJsonFile(QString(kJsonFileTemplate).arg(name), this);
}

SettingsFact* SettingsGroup::_createSettingsFact(const QString& factName)
{
    FactMetaData* m = _nameToMetaDataMap[factName];
    if(!m) {
        qCritical() << "Fact name " << factName << "not found in" << QString(kJsonFileTemplate).arg(_name);
        exit(-1);
    }
    return new SettingsFact(_settingsGroup, m, this);
}
