/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SettingsFact.h"

#include <QSettings>

SettingsFact::SettingsFact(QObject* parent)
    : Fact(parent)
{    

}

SettingsFact::SettingsFact(QString settingGroup, QString settingName, FactMetaData::ValueType_t type, const QVariant& defaultValue, QObject* parent)
    : Fact(0, settingName, type, parent)
    , _settingGroup(settingGroup)
{
    QSettings settings;

    if (!_settingGroup.isEmpty()) {
        settings.beginGroup(_settingGroup);
    }

    _rawValue = settings.value(_name, defaultValue);

    connect(this, &Fact::valueChanged, this, &SettingsFact::_valueChanged);
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

void SettingsFact::_valueChanged(QVariant value)
{
    QSettings settings;

    if (!_settingGroup.isEmpty()) {
        settings.beginGroup(_settingGroup);
    }

    settings.setValue(_name, value);
}
