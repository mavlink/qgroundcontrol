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
 *   @brief Definition of class ParamTreeItem
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef PARAMTREEITEM_H
#define PARAMTREEITEM_H

#include <QList>
#include <QVariant>

/**
 * @brief One item in the onboard parameter tree
 */
class ParamTreeItem
{
public:
    ParamTreeItem(int id, QString name, float value, ParamTreeItem* parent = 0);
    ParamTreeItem(const QList<QVariant> &data, ParamTreeItem *parent = 0);
    ~ParamTreeItem();

    void appendChild(ParamTreeItem *child);

    ParamTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    ParamTreeItem *parent();

private:
    QList<ParamTreeItem*> childItems;
    QList<QVariant> itemData;
    ParamTreeItem *parentItem;
};

#endif // PARAMTREEITEM_H
