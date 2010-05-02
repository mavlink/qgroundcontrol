#include <QHBoxLayout>

#include "QGCParamWidget.h"
#include "UASInterface.h"
#include <QDebug>

QGCParamWidget::QGCParamWidget(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    mav(uas),
    components(new QMap<int, QTreeWidgetItem*>())
{
    // Create tree widget
    tree = new QTreeWidget(this);

    // Set tree widget as widget onto this component
    QHBoxLayout* horizontalLayout;
    //form->setAutoFillBackground(false);
    horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setMargin(0);
    horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);

    horizontalLayout->addWidget(tree);
    this->setLayout(horizontalLayout);

    // Set header
    QStringList headerItems;
    headerItems.append("Parameter");
    headerItems.append("Value");
    tree->setHeaderLabels(headerItems);
    tree->setColumnCount(2);
}

UASInterface* QGCParamWidget::getUAS()
{
    return mav;
}

/**
 *
 * @param uas System which has the component
 * @param component id of the component
 * @param componentName human friendly name of the component
 */
void QGCParamWidget::addComponent(int component, QString componentName)
{
    QStringList list;
    list.append(componentName);
    list.append(QString::number(component));
    QTreeWidgetItem* comp = new QTreeWidgetItem(list);
    bool updated = false;
    if (components->contains(component)) updated = true;
    components->insert(component, comp);
    if (!updated)
    {
        tree->addTopLevelItem(comp);
        tree->update();
    }
}

void QGCParamWidget::addParameter(int component, QString parameterName, float value)
{
    // Insert parameter into map
    //tree->appendParam(component, parameterName, value);
    QStringList plist;
    plist.append(parameterName);
    plist.append(QString::number(value));
    QTreeWidgetItem* item = new QTreeWidgetItem(plist);

    // Get component
    if (!components->contains(component))
    {
        addComponent(component, "Component #" + QString::number(component));
    }
    components->value(component)->addChild(item);
    tree->update();
}

void QGCParamWidget::clear()
{
    tree->clear();
}
