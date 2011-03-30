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
 *   @brief Definition of class ParameterInterface
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef PARAMETERINTERFACE_H
#define PARAMETERINTERFACE_H

#include <QtGui/QWidget>

#include "ui_ParameterInterface.h"
#include "UASInterface.h"
#include "QGCParamWidget.h"

namespace Ui
{
class ParameterInterface;
}

/**
 * @brief Container class for onboard parameter widgets
 *
 * @see QGCParamWidget
 */
class ParameterInterface : public QWidget
{
    Q_OBJECT
public:
    explicit ParameterInterface(QWidget *parent = 0);
    virtual ~ParameterInterface();

public slots:
    void addUAS(UASInterface* uas);
    void selectUAS(int index);

protected:
    virtual void changeEvent(QEvent *e);
    QMap<int, QGCParamWidget*>* paramWidgets;
    int curr;

private:
    Ui::parameterWidget *m_ui;
};

#endif // PARAMETERINTERFACE_H
