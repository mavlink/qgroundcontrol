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
 *   @author Matthew Coleman <uavflightdirector@gmail.com>
 *
 */

#ifndef QGCSETTINGSGROUP_H
#define QGCSETTINGSGROUP_H

#include <QObject>
#include <QList>
#include <QSettings>


class QGCSettingsGroup
{
public:
    explicit QGCSettingsGroup(QGCSettingsGroup* pparentGroup, QString groupName = "default");

    void setGroupName(QString groupName);
    void loadGroup();
    void saveGroup();

protected:
    virtual void serialize(QSettings* psettings);
    virtual void deserialize(QSettings* psettings);
    virtual QString getGroupName(void);
    virtual void saveChildren(void);
    virtual void loadChildren(void);

    QString getGroupPath();

private:
    QGCSettingsGroup* p_parent;
    QString         _groupName;


signals:

public slots:

};

#endif // QGCSETTINGSGROUP_H
