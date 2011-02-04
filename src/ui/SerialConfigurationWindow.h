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
 *   @brief Definition of configuration window for serial links
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _SERIALCONFIGURATIONWINDOW_H_
#define _SERIALCONFIGURATIONWINDOW_H_

#include <QObject>
#include <QWidget>
#include <QAction>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <LinkInterface.h>
#include <SerialLinkInterface.h>
#include "ui_SerialSettings.h"

class SerialConfigurationWindow : public QWidget
{
    Q_OBJECT

public:
    SerialConfigurationWindow(LinkInterface* link, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    ~SerialConfigurationWindow();

    QAction* getAction();

public slots:
    void configureCommunication();
    void enableFlowControl(bool flow);
    void setParityNone(bool accept);
    void setParityOdd(bool accept);
    void setParityEven(bool accept);
    void setPortName(QString port);
    void setLinkName(QString name);
    void setupPortList();

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    bool userConfigured; ///< Switch to detect if current values are user-selected and shouldn't be overriden

private:

    Ui::serialSettings ui;
    SerialLinkInterface* link;
    QAction* action;
    QTimer* portCheckTimer;

};


#endif // _SERIALCONFIGURATIONWINDOW_H_
