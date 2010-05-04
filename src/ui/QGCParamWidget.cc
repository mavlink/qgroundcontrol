/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL/PIXHAWK PROJECT
<http://www.qgroundcontrol.org>
<http://pixhawk.ethz.ch>

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

/**
 * @file
 *   @brief Implementation of class QGCParamWidget
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <QGridLayout>
#include <QPushButton>

#include "QGCParamWidget.h"
#include "UASInterface.h"
#include <QDebug>

/**
 * @param uas MAV to set the parameters on
 * @param parent Parent widget
 */
QGCParamWidget::QGCParamWidget(UASInterface* uas, QWidget *parent) :
        QWidget(parent),
        mav(uas),
        components(new QMap<int, QTreeWidgetItem*>())
{
    // Create tree widget
    tree = new QTreeWidget(this);
    tree->setColumnWidth(0, 150);

    // Set tree widget as widget onto this component
    QGridLayout* horizontalLayout;
    //form->setAutoFillBackground(false);
    horizontalLayout = new QGridLayout(this);
    horizontalLayout->setSpacing(6);
    horizontalLayout->setMargin(0);
    horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);

    horizontalLayout->addWidget(tree, 0, 0, 1, 3);
    QPushButton* readButton = new QPushButton(tr("Read"));
    connect(readButton, SIGNAL(clicked()), this, SLOT(requestParameterList()));
    horizontalLayout->addWidget(readButton, 1, 0);

    QPushButton* setButton = new QPushButton(tr("Set (RAM)"));
    connect(setButton, SIGNAL(clicked()), this, SLOT(setParameters()));
    horizontalLayout->addWidget(setButton, 1, 1);

    QPushButton* writeButton = new QPushButton(tr("Write (Disk)"));
    connect(writeButton, SIGNAL(clicked()), this, SLOT(writeParameters()));
    horizontalLayout->addWidget(writeButton, 1, 2);

    // Set layout
    this->setLayout(horizontalLayout);

    // Set header
    QStringList headerItems;
    headerItems.append("Parameter");
    headerItems.append("Value");
    tree->setHeaderLabels(headerItems);
    tree->setColumnCount(2);

    // Connect signals/slots
    connect(this, SIGNAL(parameterChanged(int,QString,float)), mav, SLOT(setParameter(int,QString,float)));
    // New parameters from UAS
    connect(uas, SIGNAL(parameterChanged(int,int,QString,float)), this, SLOT(addParameter(int,int,QString,float)));
}

/**
 * @return The MAV of this widget. Unless the MAV object has been destroyed, this
 *         pointer is never zero.
 */
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
void QGCParamWidget::addComponent(int uas, int component, QString componentName)
{
    Q_UNUSED(uas);
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

/**
 * @param uas System which has the component
 * @param component id of the component
 * @param parameterName human friendly name of the parameter
 */
void QGCParamWidget::addParameter(int uas, int component, QString parameterName, float value)
{
    Q_UNUSED(uas);
    // Insert parameter into map
    //tree->appendParam(component, parameterName, value);
    QStringList plist;
    plist.append(parameterName);
    plist.append(QString::number(value));
    QTreeWidgetItem* item = new QTreeWidgetItem(plist);

    // Get component
    if (!components->contains(component))
    {
        addComponent(uas, component, "Component #" + QString::number(component));
    }
    components->value(component)->addChild(item);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    //connect(item, SIGNAL())
    tree->expandAll();
    tree->update();
}

/**
 * Send a request to deliver the list of onboard parameters
 * to the MAV.
 */
void QGCParamWidget::requestParameterList()
{
    // Clear view and request param list
    clear();
    mav->requestParameters();
}

/**
 * @param component the subsystem which has the parameter
 * @param parameterName name of the parameter, as delivered by the system
 * @param value value of the parameter
 */
void QGCParamWidget::setParameter(int component, QString parameterName, float value)
{
    emit parameterChanged(component, parameterName, value);
}

/**
 * Set all parameter in the parameter tree on the MAV
 */
void QGCParamWidget::setParameters()
{
    //mav->setParameter(component, parameterName, value);
    // Iterate through all components, through all parameters and emit them
    QMap<int, QTreeWidgetItem*>::iterator i;
    // Iterate through all components / subsystems
    for (i = components->begin(); i != components->end(); ++i)
    {
        // Get all parameters of this component
        int compid = i.key();
        QTreeWidgetItem* item = i.value();
        for (int j = 0; j < item->childCount(); ++j)
        {
            QTreeWidgetItem* param = item->child(j);
            // First column is name, second column value
            bool ok = true;
            QString key = param->data(0, Qt::DisplayRole).toString();
            float value = param->data(1, Qt::DisplayRole).toDouble(&ok);
            // Send parameter to MAV
            if (ok)
            {
                emit parameterChanged(compid, key, value);
                qDebug() << "KEY:" << key << "VALUE:" << value;
            }
            else
            {
                qDebug() << __FILE__ << __LINE__ << "CONVERSION ERROR!";
            }
        }
    }
    clear();
    //mav->requestParameters();
    qDebug() << __FILE__ << __LINE__ << "SETTING ALL PARAMETERS";
}

/**
 * Write the current onboard parameters from RAM into
 * permanent storage, e.g. EEPROM or harddisk
 */
void QGCParamWidget::writeParameters()
{
    mav->writeParameters();
}

/**
 * Clear all data in the parameter widget
 */
void QGCParamWidget::clear()
{
    tree->clear();
    components->clear();
}
