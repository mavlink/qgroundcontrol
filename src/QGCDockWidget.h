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

#ifndef QGCDockWidget_h
#define QGCDockWidget_h

#include <QDockWidget>
#include <QAction>
#include <QPointer>

class QGCDockWidget : public QWidget {
    Q_OBJECT
    
public:
    /// Pass in title = QString() and action = NULL when just using as a regular widget
    QGCDockWidget(const QString& title, QAction* action, QWidget *parent = 0);
    
    void loadSettings(void);
    void saveSettings(void);

    void closeEvent(QCloseEvent* event);

protected:
    QString             _title;
    QPointer<QAction>   _action;
    static const char*  _settingsGroup;
};


#endif
