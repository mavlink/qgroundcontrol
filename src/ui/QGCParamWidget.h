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
#include <QMap>
#include <QLabel>
#include <QTimer>
#include "QGCParamTreeWidget.h"

#include "QGCBaseParamWidget.h"


//forward declarations
class QGridLayout;
class UASInterface;

/**
 * @brief Widget to read/set onboard parameters
 */
class QGCParamWidget : public QGCBaseParamWidget
{
    Q_OBJECT
public:
    QGCParamWidget(QWidget *parent = 0);

    /// @brief Sets the list of parameters which should be shown by this editor. Parameter names can be
    ///         wildcarded at the end such as this: "RC*". Which will filter to all parameters which begin
    ///         with "RC". The wildcard (*) can only be at the end of the string.
    void setFilterList(const QStringList& filterList) { _filterList = filterList; }

protected:
    virtual void setParameterStatusMsg(const QString& msg);
    virtual void layoutWidget();///< Layout the appearance of this widget
    virtual void connectViewSignalsAndSlots();///< Connect view signals/slots as needed
    virtual void disconnectViewSignalsAndSlots();///< Connect view signals/slots as needed

    virtual QTreeWidgetItem* getParentWidgetItemForParam(int compId, const QString& paramName);
    virtual QTreeWidgetItem* findChildWidgetItemForParam(QTreeWidgetItem* parentItem, const QString& paramName);

    /** @brief Add a component item as a child of this widget
     * @param compId Component id of the component
     * @param compName Human friendly name of the component
     */
    void addComponentItem(int compId, QString compName);

    virtual void addActionButtonsToLayout(QGridLayout* layout);

public slots:
    virtual void handleOnboardParamUpdate(int component,const QString& parameterName, QVariant value);
    virtual void handlePendingParamUpdate(int compId, const QString& paramName, QVariant value, bool isPending);
    virtual void handleOnboardParameterListUpToDate();
    virtual void handleParamStatusMsgUpdate(QString msg, int level);

    virtual void clearOnboardParamDisplay();
    virtual void clearPendingParamDisplay();

    /** @brief Adds parameter at the correct location by a alphapetical comparison of the parameter names */
    void insertParamAlphabetical(int indexLowerBound, int indexUpperBound, QTreeWidgetItem* parentItem, QTreeWidgetItem* paramItem);

    /** @brief Ensure that view of parameter matches data in the model */
    QTreeWidgetItem* updateParameterDisplay(int component, QString parameterName, QVariant value);

    /** @brief Update when user changes parameters */
    void parameterItemChanged(QTreeWidgetItem* prev, int column);

    /** Promt configuration for param map config from user */
    void configureRCToParam(QString param_id);



protected:
    QMap<int, QTreeWidgetItem*>* componentItems; ///< The tree of component items, stored by component ID
    QMap<int, QMap<QString, QTreeWidgetItem*>* > paramGroups; ///< Parameter groups to organize component items
    QLabel* statusLabel; ///< User-facing parameter status label
    QGCParamTreeWidget* tree;   ///< The parameter tree
    QStringList _filterList;

private:
    bool _fullParamListLoaded;
};

#endif // QGCPARAMWIDGET_H
