/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SettingsFact.h"
#include "QGCCorePlugin.h"
#include "QGCApplication.h"

#include <QSettings>

SettingsFact::SettingsFact(QObject* parent)
    : Fact(parent)
{    
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

SettingsFact::SettingsFact(QString settingGroup, FactMetaData* metaData, QObject* parent)
    : Fact(0, metaData->name(), metaData->type(), parent)
    , _settingGroup(settingGroup)
    , _visible(true)
{
    QSettings settings;

    if (!_settingGroup.isEmpty()) {
        settings.beginGroup(_settingGroup);
    }

    // Allow core plugin a chance to override the default value
    _visible = qgcApp()->toolbox()->corePlugin()->adjustSettingMetaData(*metaData);
    setMetaData(metaData);

    QVariant typedValue;
    QString errorString;
    metaData->convertAndValidateRaw(settings.value(_name, metaData->rawDefaultValue()), true /* conertOnly */, typedValue, errorString);
    _rawValue = typedValue;

    connect(this, &Fact::rawValueChanged, this, &SettingsFact::_rawValueChanged);
}

SettingsFact::SettingsFact(const SettingsFact& other, QObject* parent)
    : Fact(other, parent)
{
    *this = other;
}

const SettingsFact& SettingsFact::operator=(const SettingsFact& other)
{
    Fact::operator=(other);
    
    _settingGroup = other._settingGroup;

    return *this;
}

void SettingsFact::_rawValueChanged(QVariant value)
{
    QSettings settings;

    if (!_settingGroup.isEmpty()) {
        settings.beginGroup(_settingGroup);
    }

    settings.setValue(_name, value);
}
