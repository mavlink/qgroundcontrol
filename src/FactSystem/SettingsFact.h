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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef SettingsFact_H
#define SettingsFact_H

#include "Fact.h"

/// @brief A SettingsFact is Fact which holds a QSettings value.
class SettingsFact : public Fact
{
    Q_OBJECT
    
public:
    SettingsFact(QObject* parent = NULL);
    SettingsFact(QString settingGroup, QString settingName, FactMetaData::ValueType_t type, const QVariant& defaultValue, QObject* parent = NULL);
    SettingsFact(const SettingsFact& other, QObject* parent = NULL);

    const SettingsFact& operator=(const SettingsFact& other);

private slots:
    void _valueChanged(QVariant value);

private:
    QString _settingGroup;
};

#endif
