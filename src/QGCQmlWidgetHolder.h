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

#ifndef QGCQmlWidgetHolder_h
#define QGCQmlWidgetHolder_h

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include <QWidget>

#include "ui_QGCQmlWidgetHolder.h"
#include "AutoPilotPlugin.h"

namespace Ui {
class QGCQmlWidgetHolder;
}

/// This is used to create widgets which are implemented in QML.

class QGCQmlWidgetHolder : public QWidget
{
    Q_OBJECT

public:
    explicit QGCQmlWidgetHolder(QWidget *parent = 0);
    ~QGCQmlWidgetHolder();
    
    /// Sets the UAS into the widget which in turn will load facts into the context
    void setAutoPilot(AutoPilotPlugin* autoPilot);

    /// Sets the QML into the control
    void setSource(const QUrl& qmlUrl);

private:
    Ui::QGCQmlWidgetHolder _ui;
};

#endif
