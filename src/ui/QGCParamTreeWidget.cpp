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
///     @author Thomas Gubler <thomasgubler@gmail.com>

#include "QGCParamTreeWidget.h"
#include <QMenu>
#include <QTreeWidgetItem>
#include <QDebug>

QGCParamTreeWidget::QGCParamTreeWidget(QWidget *parent) :
    QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(this, &QGCParamTreeWidget::customContextMenuRequested,
                         this, &QGCParamTreeWidget::showContextMenu);
    qDebug() << "create QGCParamTreeWidget";

}

QGCParamTreeWidget::~QGCParamTreeWidget()
{

}

void QGCParamTreeWidget::showContextMenu(const QPoint &pos)
{
    QMenu menu;
    QTreeWidgetItem* item = itemAt(pos);

    // Only show context menu for parameter items and not for group items
    // (show for TEST_P but not for TEST)
    // If a context menu is needed later for the groups then move this 'if'
    // to below where the actions are created and filter out certain actions
    // for the outer nodes
    if (indexOfTopLevelItem(item) > -1 ||
            indexOfTopLevelItem(item->parent()) > -1) {
        return;
    }

    QString param_id = item->data(0, Qt::DisplayRole).toString();

    // Refresh single parameter
    QAction* act = new QAction(tr("Refresh this param"), this);
    act->setProperty("action", "refresh");
    act->setProperty("param_id", param_id);
    connect(act, &QAction::triggered, this,
            &QGCParamTreeWidget::contextMenuAction);
    menu.addAction(act);

    // RC to parameter mapping
    act = new QAction(tr("Map Parameter to RC"), this);
    act->setProperty("action", "maprc");
    act->setProperty("param_id", param_id);
    connect(act, &QAction::triggered, this,
            &QGCParamTreeWidget::contextMenuAction);
    menu.addAction(act);
    menu.exec(mapToGlobal(pos));
}

void QGCParamTreeWidget::contextMenuAction() {
    QString action = qobject_cast<QAction*>(
            sender())->property("action").toString();
    QString param_id = qobject_cast<QAction*>(
            sender())->property("param_id").toString();

    if (action == "refresh") {
        emit refreshParamRequest(param_id);
    } else if (action == "maprc") {
        emit mapRCToParamRequest(param_id);
    } else {
        qDebug() << "Undefined context menu action";
    }
}
