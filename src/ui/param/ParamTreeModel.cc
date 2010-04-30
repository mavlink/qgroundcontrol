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
 *   @brief Implementation of class ParamTreeModel
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QtGui>

#include "ParamTreeItem.h"
#include "ParamTreeModel.h"

ParamTreeModel::ParamTreeModel(QObject *parent)
    : QAbstractItemModel(parent),
    components()
{
    QList<QVariant> rootData;
    rootData << tr("Parameter") << tr("Value");
    rootItem = new ParamTreeItem(rootData);

    //String data = "IMU\n ROLL_K_I\t1.255\n PITCH_K_P\t0.621\n PITCH_K_I\t2.5545\n";
    //setupModelData(data.split(QString("\n")), rootItem);
}

ParamTreeModel::ParamTreeModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent),
    components()
{
    QList<QVariant> rootData;
    rootData << tr("Parameter") << tr("Value");
    rootItem = new ParamTreeItem(rootData);
    setupModelData(data.split(QString("\n")), rootItem);
}

ParamTreeModel::~ParamTreeModel()
{
    delete rootItem;
}

int ParamTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<ParamTreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant ParamTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    ParamTreeItem *item = static_cast<ParamTreeItem*>(index.internalPointer());

    return item->data(index.column());
}

bool ParamTreeModel::setData (const QModelIndex & index, const QVariant & value, int role)
{

}

Qt::ItemFlags ParamTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant ParamTreeModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex ParamTreeModel::index(int row, int column, const QModelIndex &parent)
        const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ParamTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ParamTreeItem*>(parent.internalPointer());

    ParamTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ParamTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ParamTreeItem *childItem = static_cast<ParamTreeItem*>(index.internalPointer());
    ParamTreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ParamTreeModel::rowCount(const QModelIndex &parent) const
{
    ParamTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ParamTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

ParamTreeItem* ParamTreeModel::getNodeForComponentId(int id)
{
    return components.value(id);
}

void ParamTreeModel::appendComponent(int componentId, QString name)
{
    if (!components.contains(componentId))
    {
        ParamTreeItem* item = new ParamTreeItem(name + QString("(#") + QString::number(componentId) + QString(")"), 0, rootItem);
        components.insert(componentId, item);
    }
}

void ParamTreeModel::appendComponent(int componentId)
{
    if (!components.contains(componentId))
    {
        ParamTreeItem* item = new ParamTreeItem(QString("Component #") + QString::number(componentId) + QString(""), 0, rootItem);
        components.insert(componentId, item);
    }
    //emit dataChanged();
}

void ParamTreeModel::appendParam(int componentId, QString name, float value)
{
    // If component does not exist yet
    if (!components.contains(componentId))
    {
        appendComponent(componentId);
    }
    ParamTreeItem* comp = components.value(componentId);
    // FIXME Children may be double here
    comp->appendChild(new ParamTreeItem(name, value, comp));
    qDebug() << __FILE__ << __LINE__ << "Added param" << name << value << "for component" << comp->getParamName();
    emit dataChanged(createIndex(0, 0, rootItem), createIndex(0, 0, rootItem));
}

void ParamTreeModel::setupModelData(const QStringList &lines, ParamTreeItem *parent)
{
    QList<ParamTreeItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].mid(position, 1) != " ")
                break;
            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new ParamTreeItem(columnData, parents.last()));
        }

        number++;
    }
}
