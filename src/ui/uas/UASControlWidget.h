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
 *   @brief Definition of class UASControlWidget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UASCONTROLWIDGET_H_
#define _UASCONTROLWIDGET_H_

#include <QWidget>
#include <QLineEdit>
#include <QString>
#include <QPushButton>
#include <ui_UASControl.h>
#include <UASInterface.h>

/**
 * @brief Widget controlling one MAV
 */
class UASControlWidget : public QWidget
{
    Q_OBJECT

public:
    UASControlWidget(QWidget *parent = 0);
    ~UASControlWidget();

public slots:
    /** @brief Set the system this widget controls */
    void setUAS(UASInterface* uas);
    /** @brief Trigger next context action */
    void cycleContextButton();
    /** @brief Set the operation mode of the MAV */
    void setMode(int mode);
    /** @brief Transmit the operation mode */
    void transmitMode();
    /** @brief Update the mode */
    void updateMode(int uas,QString mode,QString description);
    /** @brief Update state */
    void updateState(int state);
    /** @brief Update internal state machine */
    void updateStatemachine();

signals:
    void changedMode(int);


protected slots:
    /** @brief Set the background color for the widget */
    void setBackgroundColor(QColor color);

protected:
    int uas;              ///< Reference to the current uas
    unsigned int uasMode; ///< Current uas mode
    bool engineOn;        ///< Engine state

private:
    Ui::uasControl ui;

};

#endif // _UASCONTROLWIDGET_H_
