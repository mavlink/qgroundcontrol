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
#include <QFileDialog>
#include <QFile>

#include "QGCParamWidget.h"
#include "UASInterface.h"
#include <QDebug>
#include "QGC.h"

/**
 * @param uas MAV to set the parameters on
 * @param parent Parent widget
 */
QGCParamWidget::QGCParamWidget(UASInterface* uas, QWidget *parent) :
        QWidget(parent),
        mav(uas),
        components(new QMap<int, QTreeWidgetItem*>()),
        paramGroups(),
        changedValues(),
        parameters()
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
    QPushButton* refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(requestParameterList()));
    horizontalLayout->addWidget(refreshButton, 1, 0);

    QPushButton* setButton = new QPushButton(tr("Transmit"));
    connect(setButton, SIGNAL(clicked()), this, SLOT(setParameters()));
    horizontalLayout->addWidget(setButton, 1, 1);

    QPushButton* writeButton = new QPushButton(tr("Write (ROM)"));
    connect(writeButton, SIGNAL(clicked()), this, SLOT(writeParameters()));
    horizontalLayout->addWidget(writeButton, 1, 2);

    QPushButton* readButton = new QPushButton(tr("Read (ROM)"));
    connect(readButton, SIGNAL(clicked()), this, SLOT(readParameters()));
    horizontalLayout->addWidget(readButton, 2, 2);

    QPushButton* loadFileButton = new QPushButton(tr("Load File"));
    connect(loadFileButton, SIGNAL(clicked()), this, SLOT(loadParameters()));
    horizontalLayout->addWidget(loadFileButton, 2, 0);

    QPushButton* saveFileButton = new QPushButton(tr("Save File"));
    connect(saveFileButton, SIGNAL(clicked()), this, SLOT(saveParameters()));
    horizontalLayout->addWidget(saveFileButton, 2, 1);

    // Set layout
    this->setLayout(horizontalLayout);

    // Set header
    QStringList headerItems;
    headerItems.append("Parameter");
    headerItems.append("Value");
    tree->setHeaderLabels(headerItems);
    tree->setColumnCount(2);
    tree->setExpandsOnDoubleClick(true);

    // Connect signals/slots
    connect(this, SIGNAL(parameterChanged(int,QString,float)), mav, SLOT(setParameter(int,QString,float)));
    connect(tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(parameterItemChanged(QTreeWidgetItem*,int)));

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
    if (components->contains(component))
    {
        // Update existing
        components->value(component)->setData(0, Qt::DisplayRole, componentName);
        components->value(component)->setData(1, Qt::DisplayRole, QString::number(component));
    }
    else
    {
        // Add new
        QStringList list;
        list.append(componentName);
        list.append(QString::number(component));
        QTreeWidgetItem* comp = new QTreeWidgetItem(list);
        components->insert(component, comp);
        // Create grouping and update maps
        paramGroups.insert(component, new QMap<QString, QTreeWidgetItem*>());
        tree->addTopLevelItem(comp);
        tree->update();
        // Create map in parameters
        if (!parameters.contains(component))
        {
            parameters.insert(component, new QMap<QString, float>());
        }
        // Create map in changed parameters
        if (!changedValues.contains(component))
        {
            changedValues.insert(component, new QMap<QString, float>());
        }
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
    // Reference to item in tree
    QTreeWidgetItem* parameterItem = NULL;

    // Get component
    if (!components->contains(component))
    {
        addComponent(uas, component, "Component #" + QString::number(component));
    }

    // Replace value in map

    // FIXME
    if (parameters.value(component)->contains(parameterName)) parameters.value(component)->remove(parameterName);
    parameters.value(component)->insert(parameterName, value);


    QString splitToken = "_";
    // Check if auto-grouping can work
    if (parameterName.contains(splitToken))
    {
        QString parent = parameterName.section(splitToken, 0, 0, QString::SectionSkipEmpty);
        QMap<QString, QTreeWidgetItem*>* compParamGroups = paramGroups.value(component);
        if (!compParamGroups->contains(parent))
        {
            // Insert group item
            QStringList glist;
            glist.append(parent);
            QTreeWidgetItem* item = new QTreeWidgetItem(glist);
            compParamGroups->insert(parent, item);
            components->value(component)->addChild(item);
        }

        // Append child to group
        bool found = false;
        QTreeWidgetItem* parentItem = compParamGroups->value(parent);
        for (int i = 0; i < parentItem->childCount(); i++)
        {
            QTreeWidgetItem* child = parentItem->child(i);
            QString key = child->data(0, Qt::DisplayRole).toString();
            if (key == parameterName)
            {
                //qDebug() << "UPDATED CHILD";
                parameterItem = child;
                parameterItem->setData(1, Qt::DisplayRole, value);
                found = true;
            }
        }

        if (!found)
        {
            // Insert parameter into map
            QStringList plist;
            plist.append(parameterName);
            plist.append(QString::number(value));
            parameterItem = new QTreeWidgetItem(plist);

            compParamGroups->value(parent)->addChild(parameterItem);
            parameterItem->setFlags(parameterItem->flags() | Qt::ItemIsEditable);
        }
    }
    else
    {
        bool found = false;
        QTreeWidgetItem* parent = components->value(component);
        for (int i = 0; i < parent->childCount(); i++)
        {
            QTreeWidgetItem* child = parent->child(i);
            QString key = child->data(0, Qt::DisplayRole).toString();
            if (key == parameterName)
            {
                //qDebug() << "UPDATED CHILD";
                parameterItem = child;
                parameterItem->setData(1, Qt::DisplayRole, value);
                found = true;
            }
        }

        if (!found)
        {
            // Insert parameter into map
            QStringList plist;
            plist.append(parameterName);
            plist.append(QString::number(value));
            parameterItem = new QTreeWidgetItem(plist);

            components->value(component)->addChild(parameterItem);
            parameterItem->setFlags(parameterItem->flags() | Qt::ItemIsEditable);
        }
        //tree->expandAll();
    }
    // Reset background color
    parameterItem->setBackground(0, QBrush(QColor(0, 0, 0)));
    parameterItem->setBackground(1, Qt::NoBrush);
    //tree->update();
    if (changedValues.contains(component)) changedValues.value(component)->remove(parameterName);
}

/**
 * Send a request to deliver the list of onboard parameters
 * to the MAV.
 */
void QGCParamWidget::requestParameterList()
{
    // Clear view and request param list
    clear();
    parameters.clear();
    received.clear();
    mav->requestParameters();
}

void QGCParamWidget::parameterItemChanged(QTreeWidgetItem* current, int column)
{
    if (current && column > 0)
    {
        QTreeWidgetItem* parent = current->parent();
        while (parent->parent() != NULL)
        {
            parent = parent->parent();
        }
        // Parent is now top-level component
        int key = components->key(parent);
        if (!changedValues.contains(key))
        {
            changedValues.insert(key, new QMap<QString, float>());
        }
        QMap<QString, float>* map = changedValues.value(key, NULL);
        if (map)
        {
            bool ok;
            QString str = current->data(0, Qt::DisplayRole).toString();
            float value = current->data(1, Qt::DisplayRole).toDouble(&ok);
            // Send parameter to MAV
            if (ok)
            {
                if (ok)
                {
                    qDebug() << "PARAM CHANGED: COMP:" << key << "KEY:" << str << "VALUE:" << value;
                    // Changed values list
                    if (map->contains(str)) map->remove(str);
                    map->insert(str, value);

                    // Check if the value was numerically changed
                    if (!parameters.value(key)->contains(str) || parameters.value(key)->value(str, 0.0f) != value)
                    {
                        current->setBackground(0, QBrush(QColor(QGC::colorGreen)));
                        current->setBackground(1, QBrush(QColor(QGC::colorGreen)));
                    }

                    // All parameters list
                    if (parameters.value(key)->contains(str)) parameters.value(key)->remove(str);
                    parameters.value(key)->insert(str, value);
                }
            }
        }
    }
}

void QGCParamWidget::saveParameters()
{    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "./parameters.txt", tr("Parameter File (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);

    in << "# Onboard parameters for system " << mav->getUASName() << "\n";
    in << "#\n";
    in << "# MAV ID  COMPONENT ID  PARAM NAME  VALUE (FLOAT)\n";

    // Iterate through all components, through all parameters and emit them
    QMap<int, QMap<QString, float>*>::iterator i;
    for (i = parameters.begin(); i != parameters.end(); ++i)
    {
        // Iterate through the parameters of the component
        int compid = i.key();
        QMap<QString, float>* comp = i.value();
        {
            QMap<QString, float>::iterator j;
            for (j = comp->begin(); j != comp->end(); ++j)
            {
                in << mav->getUASID() << "\t" << compid << "\t" << j.key() << "\t" << j.value() << "\n";
                in.flush();
            }
        }
    }
    file.close();
}

void QGCParamWidget::loadParameters()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Parameter file (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // Clear list
    clear();

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (!line.startsWith("#"))
        {
            QStringList wpParams = line.split("\t");
            if (wpParams.size() == 4)
            {
                // Only load parameters for right mav
                if (mav->getUASID() == wpParams.at(0).toInt())
                {

                    bool changed = false;
                    int component = wpParams.at(1).toInt();
                    QString parameterName = wpParams.at(2);
                    if (!parameters.contains(component) || parameters.value(component)->value(parameterName, 0.0f) != (float)wpParams.at(3).toDouble())
                    {
                        changed = true;
                    }

                    // Set parameter value
                    addParameter(wpParams.at(0).toInt(), wpParams.at(1).toInt(), wpParams.at(2), wpParams.at(3).toDouble());

                    if (changed)
                    {
                        // Create changed values data structure if necessary
                        if (!changedValues.contains(wpParams.at(1).toInt()))
                        {
                            changedValues.insert(wpParams.at(1).toInt(), new QMap<QString, float>());
                        }

                        // Add to changed values
                        if (changedValues.value(wpParams.at(1).toInt())->contains(wpParams.at(2)))
                        {
                            changedValues.value(wpParams.at(1).toInt())->remove(wpParams.at(2));
                        }

                        changedValues.value(wpParams.at(1).toInt())->insert(wpParams.at(2), (float)wpParams.at(3).toDouble());

                        //qDebug() << "MARKING COMP" << wpParams.at(1).toInt() << "PARAM" << wpParams.at(2) << "VALUE" << (float)wpParams.at(3).toDouble() << "AS CHANGED";

                        // Mark in UI


                    }
                }
            }
        }
    }
    file.close();

}


/**
 * @param component the subsystem which has the parameter
 * @param parameterName name of the parameter, as delivered by the system
 * @param value value of the parameter
 */
void QGCParamWidget::setParameter(int component, QString parameterName, float value)
{
    emit parameterChanged(component, parameterName, value);
    qDebug() << "SENT PARAM" << parameterName << value;
}

/**
 * Set all parameter in the parameter tree on the MAV
 */
void QGCParamWidget::setParameters()
{
    // Iterate through all components, through all parameters and emit them
    QMap<int, QMap<QString, float>*>::iterator i;
    for (i = changedValues.begin(); i != changedValues.end(); ++i)
    {
        // Iterate through the parameters of the component
        int compid = i.key();
        QMap<QString, float>* comp = i.value();
        {
            QMap<QString, float>::iterator j;
            for (j = comp->begin(); j != comp->end(); ++j)
            {
                setParameter(compid, j.key(), j.value());
            }
        }
    }

    changedValues.clear();
    qDebug() << __FILE__ << __LINE__ << "SETTING ALL PARAMETERS";
}

/**
 * Write the current onboard parameters from RAM into
 * permanent storage, e.g. EEPROM or harddisk
 */
void QGCParamWidget::writeParameters()
{
    mav->writeParametersToStorage();
}

void QGCParamWidget::readParameters()
{
    mav->readParametersFromStorage();
}

/**
 * Clear all data in the parameter widget
 */
void QGCParamWidget::clear()
{
    tree->clear();
    components->clear();
}
