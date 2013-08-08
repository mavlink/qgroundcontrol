/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Declaration of class QGCParamWidget
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef QGCPARAMWIDGET_H
#define QGCPARAMWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include <QLabel>
#include <QTimer>

#include "QGCUASParamManager.h"
#include "UASInterface.h"

/**
 * @brief Widget to read/set onboard parameters
 */
class QGCParamWidget : public QGCUASParamManager
{
    Q_OBJECT
public:
    QGCParamWidget(UASInterface* uas, QWidget *parent = 0);

protected:
    virtual void setParameterStatusMsg(const QString& msg);

signals:
    /** @brief A parameter was changed in the widget, NOT onboard */
    //void parameterChanged(int component, QString parametername, float value); // defined in QGCUASParamManager already


public slots:
    /** @brief Add a component to the list
     * @param compId Component id of the component
     * @param compName Human friendly name of the component
     */
    void addComponentItem(int compId, QString compName);

    /** @brief Add a parameter to the list with retransmission / safety checks */
//    void receivedParameterUpdate(int uas, int component, int paramCount, int paramId, QString parameterName, QVariant value);

    virtual void handleParameterUpdate(int component, int paramId, const QString& parameterName, QVariant value);
    virtual void handleParameterListUpToDate();

    virtual void handleParamStatusMsgUpdate(QString msg, int level);

    /** @brief Ensure that view of parameter matches data in the model */
    void updateParameterDisplay(int component, QString parameterName, QVariant value);
    /** @brief Request list of parameters from MAV */
    void requestAllParamsUpdate();
    /** @brief Set one parameter, changes value in RAM of MAV */
//    virtual void setParameter(int component, QString parameterName, QVariant value);
    /** @brief Set all parameters, changes the value in RAM of MAV */
    void setParameters();
    /** @brief Write the current parameters to permanent storage (EEPROM/HDD) */
    void writeParameters();
    /** @brief Read the parameters from permanent storage to RAM */
    void readParameters();
    /** @brief Clear the parameter list */
    void clear();
    /** @brief Update when user changes parameters */
    void parameterItemChanged(QTreeWidgetItem* prev, int column);

    /** @brief Store parameters to a file */
    void saveParametersToFile();
    /** @brief Load parameters from a file */
    void loadParametersFromFile();



protected:
    QTreeWidget* tree;   ///< The parameter tree
    QLabel* statusLabel; ///< User-facing parameter status label
    QMap<int, QTreeWidgetItem*>* componentItems; ///< The tree of component items, stored by component ID
    QMap<int, QMap<QString, QTreeWidgetItem*>* > paramGroups; ///< Parameter groups to organize component items


};

#endif // QGCPARAMWIDGET_H
