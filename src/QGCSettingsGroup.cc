/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief QGCSettingsGroup management of settings serialization into nexted groups
 *
 *   This class is inherited by any object that needs to serialize itself in a tree structure
 *   The tree root (NULL parent group) is typically a singleton.
 *
 *   @author Matthew Coleman <uavflightdirector@gmail.com>
 *
 */

#include "QGCSettingsGroup.h"
#include <QSettings>
#include <QDebug>

QGCSettingsGroup::QGCSettingsGroup(QGCSettingsGroup* pparentGroup, QString groupName)
{
    p_parent = pparentGroup;
    _groupName = groupName;
}


void QGCSettingsGroup::loadGroup(){
    QSettings settings;
    QString groupPath = getGroupPath();
    settings.beginGroup(groupPath);
    settings.sync();
    deserialize(&settings);
    settings.endGroup();
    loadChildren();
    qDebug() << "Loaded settings group: " << groupPath;
}

void QGCSettingsGroup::saveGroup(){
    QSettings settings;
    QString groupPath = getGroupPath();
    settings.remove(groupPath);
    settings.beginGroup(groupPath);
    serialize(&settings);
    settings.endGroup();
    settings.sync();
    saveChildren();
    qDebug() << "Saved settings group: " << groupPath;
}


void QGCSettingsGroup::setGroupName(QString groupName){
    _groupName = groupName;
    if(p_parent != NULL)
        p_parent->saveGroup();
    else
        this->saveGroup();
}

QString QGCSettingsGroup::getGroupPath(){
    if(p_parent != NULL)
    {
        return p_parent->getGroupPath() + "/" + getGroupName();
    }
    else
        return getGroupName();
}

void QGCSettingsGroup::serialize(QSettings* psettings){
    psettings = psettings;
}

void QGCSettingsGroup::deserialize(QSettings* psettings){
    psettings = psettings;
}

QString QGCSettingsGroup::getGroupName(void){
    return _groupName;
}

void QGCSettingsGroup::saveChildren(void) {};
void QGCSettingsGroup::loadChildren(void) {};
