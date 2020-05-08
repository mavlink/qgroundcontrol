/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ToolStripActionList.h"

ToolStripActionList::ToolStripActionList(QObject* parent)
    : QObject(parent)
{

}

QQmlListProperty<QObject> ToolStripActionList::model(void)
{
    return QQmlListProperty<QObject>(this, this,
             &ToolStripActionList::append,
             &ToolStripActionList::count,
             &ToolStripActionList::at,
             &ToolStripActionList::clear);
}

void ToolStripActionList::append(QQmlListProperty<QObject>* qmlListProperty, QObject* value) {
    reinterpret_cast<ToolStripActionList*>(qmlListProperty->data)->_objectList.append(value);
}

void ToolStripActionList::clear(QQmlListProperty<QObject>* qmlListProperty) {
    reinterpret_cast<ToolStripActionList*>(qmlListProperty->data)->_objectList.clear();
}

QObject* ToolStripActionList::at(QQmlListProperty<QObject>* qmlListProperty, int index) {
    return reinterpret_cast<ToolStripActionList*>(qmlListProperty->data)->_objectList[index];
}

int ToolStripActionList::count(QQmlListProperty<QObject>* qmlListProperty) {
    return reinterpret_cast<ToolStripActionList*>(qmlListProperty->data)->_objectList.count();
}
