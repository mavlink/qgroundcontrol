/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of class MainWindow
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QStringList>

#include "ParamTreeItem.h"

ParamTreeItem::ParamTreeItem(const QList<QVariant> &data, ParamTreeItem *parent)
{
    parentItem = parent;
    itemData = data;
}

ParamTreeItem::~ParamTreeItem()
{
    qDeleteAll(childItems);
}

void ParamTreeItem::appendChild(ParamTreeItem *item)
{
    childItems.append(item);
}

ParamTreeItem *ParamTreeItem::child(int row)
{
    return childItems.value(row);
}

int ParamTreeItem::childCount() const
{
    return childItems.count();
}

int ParamTreeItem::columnCount() const
{
    return itemData.count();
}

QVariant ParamTreeItem::data(int column) const
{
    return itemData.value(column);
}

ParamTreeItem *ParamTreeItem::parent()
{
    return parentItem;
}

int ParamTreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<ParamTreeItem*>(this));

    return 0;
}
