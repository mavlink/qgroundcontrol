/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QQmlListProperty>

class ToolStripActionList : public QObject
{
    Q_OBJECT
    
public:
    ToolStripActionList(QObject* parent = nullptr);
    
    Q_PROPERTY(QQmlListProperty<QObject> model READ model NOTIFY modelChanged)

    QQmlListProperty<QObject> model();

signals:
    void modelChanged(void);

private:
    static void     append  (QQmlListProperty<QObject>* qmlListProperty, QObject* value);
    static int      count   (QQmlListProperty<QObject>* qmlListProperty);
    static QObject* at      (QQmlListProperty<QObject>*, int index);
    static void     clear   (QQmlListProperty<QObject>* qmlListProperty);

    QList<QObject*> _objectList;
};
