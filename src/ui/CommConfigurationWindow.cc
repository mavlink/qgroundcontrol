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
 *   @brief Implementation of CommConfigurationWindow
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QDebug>

#include <QDir>
#include <QFileInfoList>
#include <QBoxLayout>
#include <QWidget>

#include "CommConfigurationWindow.h"
#include "SerialConfigurationWindow.h"
#include "SerialLink.h"
#include "UDPLink.h"
#include "MAVLinkSimulationLink.h"
#ifdef XBEELINK
#include "XbeeLink.h"
#include "XbeeConfigurationWindow.h"
#endif // XBEELINK
#ifdef OPAL_RT
#include "OpalLink.h"
#include "OpalLinkConfigurationWindow.h"
#endif
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "QGCUDPLinkConfiguration.h"
#include "LinkManager.h"
#include "MainWindow.h"

CommConfigurationWindow::CommConfigurationWindow(LinkInterface* link, ProtocolInterface* protocol, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
    this->link = link;

    // Setup the user interface according to link type
    ui.setupUi(this);

    // add link types
    ui.linkType->addItem(tr("Serial"), QGC_LINK_SERIAL);
    ui.linkType->addItem(tr("UDP"), QGC_LINK_UDP);
    ui.linkType->addItem(tr("Simulation"), QGC_LINK_SIMULATION);
    ui.linkType->addItem(tr("Opal-RT Link"), QGC_LINK_OPAL);
#ifdef XBEELINK
	ui.linkType->addItem(tr("Xbee API"),QGC_LINK_XBEE);
#endif // XBEELINK
    ui.linkType->setEditable(false);

    ui.connectionType->addItem("MAVLink", QGC_PROTOCOL_MAVLINK);

    // Create action to open this menu
    // Create configuration action for this link
    // Connect the current UAS
    action = new QAction(QIcon(":/files/images/devices/network-wireless.svg"), "", this);
    LinkManager::instance()->add(link);
	action->setData(link->getId());
    action->setEnabled(true);
    action->setVisible(true);
    setLinkName(link->getName());
    connect(action, SIGNAL(triggered()), this, SLOT(show()));

    // Make sure that a change in the link name will be reflected in the UI
    connect(link, SIGNAL(nameChanged(QString)), this, SLOT(setLinkName(QString)));

    // Setup user actions and link notifications
    connect(ui.connectButton, SIGNAL(clicked()), this, SLOT(setConnection()));
    connect(ui.closeButton, SIGNAL(clicked()), this->window(), SLOT(close()));
    connect(ui.deleteButton, SIGNAL(clicked()), this, SLOT(remove()));

    connect(this->link, SIGNAL(connected(bool)), this, SLOT(connectionState(bool)));


    // Fill in the current data
    if(this->link->isConnected()) ui.connectButton->setChecked(true);
    //connect(this->link, SIGNAL(connected(bool)), ui.connectButton, SLOT(setChecked(bool)));

    if(this->link->isConnected()) {
        ui.connectionStatusLabel->setText(tr("Connected"));

        // TODO Deactivate all settings to force user to manually disconnect first
    } else {
        ui.connectionStatusLabel->setText(tr("Disconnected"));
    }

    // TODO Move these calls to each link so that dynamic casts vanish

    // Open details pane for serial link if necessary
    SerialLink* serial = dynamic_cast<SerialLink*>(link);
    if(serial != 0) {
        QWidget* conf = new SerialConfigurationWindow(serial, this);
        ui.linkScrollArea->setWidget(conf);
        ui.linkGroupBox->setTitle(tr("Serial Link"));
        ui.linkType->setCurrentIndex(0);
    }
    UDPLink* udp = dynamic_cast<UDPLink*>(link);
    if (udp != 0) {
        QWidget* conf = new QGCUDPLinkConfiguration(udp, this);
        ui.linkScrollArea->setWidget(conf);
        ui.linkGroupBox->setTitle(tr("UDP Link"));
        ui.linkType->setCurrentIndex(1);
    }
    MAVLinkSimulationLink* sim = dynamic_cast<MAVLinkSimulationLink*>(link);
    if (sim != 0) {
        ui.linkType->setCurrentIndex(2);
        ui.linkGroupBox->setTitle(tr("MAVLink Simulation Link"));
    }
#ifdef OPAL_RT
    OpalLink* opal = dynamic_cast<OpalLink*>(link);
    if (opal != 0) {
        QWidget* conf = new OpalLinkConfigurationWindow(opal, this);
        QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, ui.linkGroupBox);
        layout->addWidget(conf);
        ui.linkGroupBox->setLayout(layout);
        ui.linkType->setCurrentIndex(3);
        ui.linkGroupBox->setTitle(tr("Opal-RT Link"));
    }
#endif
#ifdef XBEELINK
	XbeeLink* xbee = dynamic_cast<XbeeLink*>(link); // new Konrad
	if(xbee != 0)
	{
		QWidget* conf = new XbeeConfigurationWindow(xbee,this); 
		ui.linkScrollArea->setWidget(conf);
		ui.linkGroupBox->setTitle(tr("Xbee Link"));
		ui.linkType->setCurrentIndex(4);
		connect(xbee,SIGNAL(tryConnectBegin(bool)),ui.actionConnect,SLOT(setDisabled(bool)));
		connect(xbee,SIGNAL(tryConnectEnd(bool)),ui.actionConnect,SLOT(setEnabled(bool)));
	}
#endif // XBEELINK
    if (serial == 0 && udp == 0 && sim == 0
#ifdef OPAL_RT
            && opal == 0
#endif
#ifdef XBEELINK
			&& xbee == 0
#endif // XBEELINK
       ) {
        qDebug() << "Link is NOT a known link, can't open configuration window";
    }

#ifdef XBEELINK
	connect(ui.linkType,SIGNAL(currentIndexChanged(int)),this,SLOT(setLinkType(int)));
#endif // XBEELINK

    // Open details pane for MAVLink if necessary
    MAVLinkProtocol* mavlink = dynamic_cast<MAVLinkProtocol*>(protocol);
    if (mavlink != 0) {
        QWidget* conf = new MAVLinkSettingsWidget(mavlink, this);
        ui.protocolScrollArea->setWidget(conf);
        ui.protocolGroupBox->setTitle(protocol->getName()+" (Global Settings)");
    } else {
        qDebug() << "Protocol is NOT MAVLink, can't open configuration window";
    }

    // Open details for UDP link if necessary
    // TODO

    // Display the widget
    this->window()->setWindowTitle(tr("Settings for ") + this->link->getName());
    this->hide();
}

CommConfigurationWindow::~CommConfigurationWindow()
{

}

QAction* CommConfigurationWindow::getAction()
{
    return action;
}

void CommConfigurationWindow::setLinkType(int linktype)
{
	if(link->isConnected())
	{
		// close old configuration window
		this->window()->close();
	}
	else
	{
		// delete old configuration window
		this->remove();
	}

	LinkInterface *tmpLink(NULL);
	switch(linktype)
	{
#ifdef XBEELINK
		case 4:
			{
				XbeeLink *xbee = new XbeeLink();
				tmpLink = xbee;
				MainWindow::instance()->addLink(tmpLink);
				break;
			}
#endif // XBEELINK
		case 1:
			{
				UDPLink *udp = new UDPLink();
				tmpLink = udp;
				MainWindow::instance()->addLink(tmpLink);
				break;
			}
			
#ifdef OPAL_RT
		case 3:
			{
				OpalLink* opal = new OpalLink();
				tmpLink = opal;
				MainWindow::instance()->addLink(tmpLink);
				break;
			}
#endif // OPAL_RT
		default:
			{
			}
		case 0:
			{
				SerialLink *serial = new SerialLink();
				tmpLink = serial;
				MainWindow::instance()->addLink(tmpLink);
				break;
			}
	}
	// trigger new window

	const int32_t& linkIndex(LinkManager::instance()->getLinks().indexOf(tmpLink));
	const int32_t& linkID(LinkManager::instance()->getLinks()[linkIndex]->getId());

	QList<QAction*> actions = MainWindow::instance()->listLinkMenuActions();
	foreach (QAction* act, actions) 
	{
        if (act->data().toInt() == linkID) 
        {
            act->trigger();
            break;
        }
    }
}

void CommConfigurationWindow::setProtocol(int protocol)
{
    qDebug() << "Changing to protocol" << protocol;
}

void CommConfigurationWindow::setConnection()
{
    if(!link->isConnected()) {
        link->connect();
    } else {
        link->disconnect();
    }
}

void CommConfigurationWindow::setLinkName(QString name)
{
    action->setText(tr("%1 Settings").arg(name));
    action->setStatusTip(tr("Adjust setting for link %1").arg(name));
    this->window()->setWindowTitle(tr("Settings for %1").arg(name));
}

void CommConfigurationWindow::remove()
{
    if(action) delete action; //delete action first since it has a pointer to link
    action=NULL;

    if(link) {
        LinkManager::instance()->removeLink(link); //remove link from LinkManager list
        link->disconnect(); //disconnect port, and also calls terminate() to stop the thread
        if (link->isRunning()) link->terminate(); // terminate() the serial thread just in case it is still running
        link->wait(); // wait() until thread is stoped before deleting
        link->deleteLater();
    }
    link=NULL;

    this->window()->close();
    this->deleteLater();
}

void CommConfigurationWindow::connectionState(bool connect)
{
    ui.connectButton->setChecked(connect);
    if(connect) {
        ui.connectionStatusLabel->setText(tr("Connected"));
        ui.connectButton->setText(tr("Disconnect"));
    } else {
        ui.connectionStatusLabel->setText(tr("Disconnected"));
        ui.connectButton->setText(tr("Connect"));
    }
}
