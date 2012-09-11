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
 *   @brief Definition of class CommConfigurationWindow
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

enum qgc_link_t {
    QGC_LINK_SERIAL,
    QGC_LINK_UDP,
    QGC_LINK_SIMULATION,
    QGC_LINK_FORWARDING,
#ifdef XBEELINK
	QGC_LINK_XBEE,
#endif
    QGC_LINK_OPAL
};

enum qgc_protocol_t {
    QGC_PROTOCOL_MAVLINK,
};


#ifdef OPAL_RT
#include "OpalLink.h"
#endif

/**
 * @brief Configuration window for communication links
 */
class CommConfigurationWindow : public QWidget
{
    Q_OBJECT

public:
    CommConfigurationWindow(LinkInterface* link, ProtocolInterface* protocol, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    ~CommConfigurationWindow();

    QAction* getAction();

public slots:
    void setLinkType(int linktype);
    /** @brief Set the protocol for this link */
    void setProtocol(int protocol);
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
