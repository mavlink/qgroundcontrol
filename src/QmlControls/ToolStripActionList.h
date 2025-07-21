/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtQml/QQmlListProperty>
#include <QtQmlIntegration/QtQmlIntegration>

class ToolStripActionList : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit ToolStripActionList(QObject* parent = nullptr);
    
    Q_PROPERTY(QQmlListProperty<QObject> model READ model NOTIFY modelChanged)

    QQmlListProperty<QObject> model();

signals:
    void modelChanged(void);

private:
    static void         append  (QQmlListProperty<QObject>* qmlListProperty, QObject* value);
    static qsizetype    count   (QQmlListProperty<QObject>* qmlListProperty);
    static QObject*     at      (QQmlListProperty<QObject>*, qsizetype index);
    static void         clear   (QQmlListProperty<QObject>* qmlListProperty);

    QList<QObject*> _objectList;
};
