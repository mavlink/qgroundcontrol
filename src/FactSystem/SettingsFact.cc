/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
