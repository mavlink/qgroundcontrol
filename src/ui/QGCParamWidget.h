#ifndef QGCPARAMWIDGET_H
#define QGCPARAMWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMap>

#include "UASInterface.h"

class QGCParamWidget : public QWidget
{
Q_OBJECT
public:
    explicit QGCParamWidget(UASInterface* uas, QWidget *parent = 0);
    UASInterface* getUAS();
signals:

public slots:
    void addComponent(int component, QString componentName);
    void addParameter(int component, QString parameterName, float value);
    void clear();
protected:
    UASInterface* mav;
    QTreeWidget* tree;
    QMap<int, QTreeWidgetItem*>* components;

};

#endif // QGCPARAMWIDGET_H
