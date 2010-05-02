#ifndef QGCPARAMWIDGET_H
#define QGCPARAMWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMap>

#include "UASInterface.h"

/**
 * @brief Widget to read/set onboard parameters
 */
class QGCParamWidget : public QWidget
{
Q_OBJECT
public:
    explicit QGCParamWidget(UASInterface* uas, QWidget *parent = 0);
    /** @brief Get the UAS of this widget */
    UASInterface* getUAS();
signals:
    void parameterChanged(int component, QString parametername, float value);
public slots:
    /** @brief Add a component to the list */
    void addComponent(int uas, int component, QString componentName);
    /** @brief Add a parameter to the list */
    void addParameter(int uas, int component, QString parameterName, float value);
    /** @brief Request list of parameters from MAV */
    void requestParameterList();
    /** @brief Set one parameter, changes value in RAM of MAV */
    void setParameter(int component, QString parameterName, float value);
    /** @brief Set all parameters, changes the value in RAM of MAV */
    void setParameters();
    /** @brief Write the current parameters to permanent storage (EEPROM/HDD) */
    void writeParameters();
    /** @brief Clear the parameter list */
    void clear();
protected:
    UASInterface* mav;  ///< The MAV this widget is controlling
    QTreeWidget* tree;  ///< The parameter tree
    QMap<int, QTreeWidgetItem*>* components; ///< The list of components

};

#endif // QGCPARAMWIDGET_H
