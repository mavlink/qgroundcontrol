/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief QGC Main Map Display
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef QGCMAPDISPLAY_H
#define QGCMAPDISPLAY_H

#include "QGCQmlWidgetHolder.h"

class UASInterface;
class UASWaypointManager;

class QGCMapDisplay : public QGCQmlWidgetHolder
{
    Q_OBJECT
public:
    QGCMapDisplay(QWidget* parent = NULL);
    ~QGCMapDisplay();

    Q_INVOKABLE void    saveSetting (const QString &key, const QString& value);
    Q_INVOKABLE QString loadSetting (const QString &key, const QString& defaultValue);

    /** @brief Start updating widget */
    void showEvent(QShowEvent* event);
    /** @brief Stop updating widget */
    void hideEvent(QHideEvent* event);

signals:

private slots:

private:

private:
    UASWaypointManager* WPM;

};


#endif // QGCFLIGHTDISPLAY_H
