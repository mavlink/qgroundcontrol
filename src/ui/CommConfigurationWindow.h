/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
Please see our website at <http://pixhawk.ethz.ch>

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of configuration window for serial links
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _COMMCONFIGURATIONWINDOW_H_
#define _COMMCONFIGURATIONWINDOW_H_

#include <QObject>
#include <QWidget>
#include <QAction>
#include "LinkInterface.h"
#include "ProtocolInterface.h"
#include "ui_CommSettings.h"

#ifdef OPAL_RT
#include "OpalLink.h"
#endif

class CommConfigurationWindow : public QWidget
{
    Q_OBJECT

public:
    CommConfigurationWindow(LinkInterface* link, ProtocolInterface* protocol, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    ~CommConfigurationWindow();

    QAction* getAction();

public slots:
    void setLinkType(int linktype);
    void setConnection();
    void connectionState(bool connect);
    void setLinkName(QString name);
    /** @brief Disconnects the associated link, removes it from all menus and closes the window. */
    void remove();

private:

    Ui::commSettings ui;
    LinkInterface* link;
    QAction* action;
};


#endif // _COMMCONFIGURATIONWINDOW_H_
