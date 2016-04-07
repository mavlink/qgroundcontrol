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

#include "QGCDockWidget.h"

#include <QCloseEvent>
#include <QSettings>

const char*  QGCDockWidget::_settingsGroup = "DockWidgets";

QGCDockWidget::QGCDockWidget(const QString& title, QAction* action, QWidget* parent)
    : QWidget(parent)
    , _title(title)
	, _action(action)
{
    if (action) {
        setWindowTitle(title);
        setWindowFlags(Qt::Tool);
        loadSettings();
    }
}

// Instead of destroying the widget just hide it
void QGCDockWidget::closeEvent(QCloseEvent* event)
{
    if (_action) {
        saveSettings();
        event->ignore();
        _action->trigger();
    } else {
        QWidget::closeEvent(event);
    }
}

void QGCDockWidget::loadSettings(void)
{
    // TODO: This is crashing for some reason. Disabled until sorted out.
    if (0 /*_action*/) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);
        if (settings.contains(_title)) {
            restoreGeometry(settings.value(_title).toByteArray());
        }
        settings.endGroup();
    }
}

void QGCDockWidget::saveSettings(void)
{
    // TODO: This is crashing for some reason. Disabled until sorted out.
    if (0 /*_action*/) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);
        settings.setValue(_title, saveGeometry());
        settings.endGroup();
    }
}
