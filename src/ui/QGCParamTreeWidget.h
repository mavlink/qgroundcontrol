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
///     @brief A treeview with context menus for parameters
///     @author Thomas Gubler <thomasgubler@gmail.com>

#ifndef QGCPARAMTREEWIDGET_H
#define QGCPARAMTREEWIDGET_H

#include <QTreeWidget>

/// Implements individual context menus for the QTreeWidgetItems
class QGCParamTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    QGCParamTreeWidget(QWidget *parent = 0);
    ~QGCParamTreeWidget();

signals:
    void mapRCToParamRequest(QString param_id);

public slots:
    void showContextMenu(const QPoint &pos);
    void contextMenuAction();

};

#endif // QGCPARAMTREEWIDGET_H
