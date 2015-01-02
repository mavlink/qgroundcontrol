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
#include "TCPLink.h"
#include "MAVLinkSimulationLink.h"
#ifdef UNITTEST_BUILD
#include "MockLink.h"
#endif
#ifdef QGC_XBEE_ENABLED
#include "XbeeLink.h"
#include "XbeeConfigurationWindow.h"
#endif // QGC_XBEE_ENABLED
#ifdef QGC_RTLAB_ENABLED
#include "OpalLink.h"
#include "OpalLinkConfigurationWindow.h"
#endif
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "QGCUDPLinkConfiguration.h"
#include "QGCTCPLinkConfiguration.h"
#include "LinkManager.h"
#include "MainWindow.h"

CommConfigurationWindow::CommConfigurationWindow(LinkInterface* link, QWidget *parent) : QDialog(parent)
{
    this->link = link;

    // Setup the user interface according to link type
    ui.setupUi(this);

    // Initialize basic ui state

    // Do not allow changes here unless advanced is checked
    ui.connectionType->setEnabled(false);
    ui.protocolGroupBox->setVisible(false);
    ui.protocolTypeGroupBox->setVisible(false);

    // Connect UI element visibility to checkbox
    //connect(ui.advancedOptionsCheckBox, SIGNAL(clicked(bool)), ui.connectionType, SLOT(setEnabled(bool)));
    //connect(ui.advancedOptionsCheckBox, SIGNAL(clicked(bool)), ui.linkType, SLOT(setEnabled(bool)));
    //connect(ui.advancedOptionsCheckBox, SIGNAL(clicked(bool)), ui.protocolGroupBox, SLOT(setVisible(bool)));
    ui.advancedOptionsCheckBox->setVisible(false);
    //connect(ui.advCheckBox,SIGNAL(clicked(bool)),ui.advancedOptionsCheckBox,SLOT(setChecked(bool)));
    connect(ui.advCheckBox,SIGNAL(clicked(bool)),ui.protocolTypeGroupBox,SLOT(setVisible(bool)));
    connect(ui.advCheckBox, SIGNAL(clicked(bool)), ui.connectionType, SLOT(setEnabled(bool)));
    connect(ui.advCheckBox, SIGNAL(clicked(bool)), ui.protocolGroupBox, SLOT(setVisible(bool)));

    // add link types
    ui.linkType->addItem(tr("Serial"), QGC_LINK_SERIAL);
    ui.linkType->addItem(tr("UDP"), QGC_LINK_UDP);
    ui.linkType->addItem(tr("TCP"), QGC_LINK_TCP);
    
    if(dynamic_cast<MAVLinkSimulationLink*>(link)) {
        //Only show simulation option if already setup elsewhere as a simulation
        ui.linkType->addItem(tr("Simulation"), QGC_LINK_SIMULATION);
    }
    
#ifdef UNITTEST_BUILD
    ui.linkType->addItem(tr("Mock"), QGC_LINK_MOCK);
#endif

#ifdef QGC_RTLAB_ENABLED
    ui.linkType->addItem(tr("Opal-RT Link"), QGC_LINK_OPAL);
#endif
#ifdef QGC_XBEE_ENABLED
	ui.linkType->addItem(tr("Xbee API"),QGC_LINK_XBEE);
#endif // QGC_XBEE_ENABLED
    ui.linkType->setEditable(false);

    ui.connectionType->addItem("MAVLink", QGC_PROTOCOL_MAVLINK);

    // Create action to open this menu
    // Create configuration action for this link
    // Connect the current UAS
    action = new QAction(QIcon(":/files/images/devices/network-wireless.svg"), "", this);
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

    connect(ui.autoConnectCheckBox, SIGNAL(clicked(bool)), this, SLOT(_setAutoConnect(bool)) );

    connect(link, &LinkInterface::connected, this, &CommConfigurationWindow::_linkConnected);
    connect(link, &LinkInterface::disconnected, this, &CommConfigurationWindow::_linkDisconnected);

    // Fill in the current data
    if(this->link->isConnected()) ui.connectButton->setChecked(true);

    ui.autoConnectCheckBox->setChecked(this->link->getAutoConnect());       // auto connect at startup

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
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_SERIAL));
    }
    
    UDPLink* udp = dynamic_cast<UDPLink*>(link);
    if (udp != 0) {
        QWidget* conf = new QGCUDPLinkConfiguration(udp, this);
        ui.linkScrollArea->setWidget(conf);
        ui.linkGroupBox->setTitle(tr("UDP Link"));
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_UDP));
    }
    
    TCPLink* tcp = dynamic_cast<TCPLink*>(link);
    if (tcp != 0) {
        QWidget* conf = new QGCTCPLinkConfiguration(tcp, this);
        ui.linkScrollArea->setWidget(conf);
        ui.linkGroupBox->setTitle(tr("TCP Link"));
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_TCP));
    }
    
    MAVLinkSimulationLink* sim = dynamic_cast<MAVLinkSimulationLink*>(link);
    if (sim != 0) {
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_SIMULATION));
        ui.linkType->setEnabled(false); //Don't allow the user to change to a non-simulation
        ui.linkGroupBox->setTitle(tr("MAVLink Simulation Link"));
    }
    
#ifdef UNITTEST_BUILD
    MockLink* mock = dynamic_cast<MockLink*>(link);
    if (mock != 0) {
        ui.linkGroupBox->setTitle(tr("Mock Link"));
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_MOCK));
    }
#endif
    
#ifdef QGC_RTLAB_ENABLED
    OpalLink* opal = dynamic_cast<OpalLink*>(link);
    if (opal != 0) {
        QWidget* conf = new OpalLinkConfigurationWindow(opal, this);
        QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, ui.linkGroupBox);
        layout->addWidget(conf);
        ui.linkGroupBox->setLayout(layout);
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_OPAL));
        ui.linkGroupBox->setTitle(tr("Opal-RT Link"));
    }
#endif
    
#ifdef QGC_XBEE_ENABLED
	XbeeLink* xbee = dynamic_cast<XbeeLink*>(link); // new Konrad
	if(xbee != 0)
	{
		QWidget* conf = new XbeeConfigurationWindow(xbee,this); 
		ui.linkScrollArea->setWidget(conf);
		ui.linkGroupBox->setTitle(tr("Xbee Link"));
        ui.linkType->setCurrentIndex(ui.linkType->findData(QGC_LINK_XBEE));
		connect(xbee,SIGNAL(tryConnectBegin(bool)),ui.actionConnect,SLOT(setDisabled(bool)));
		connect(xbee,SIGNAL(tryConnectEnd(bool)),ui.actionConnect,SLOT(setEnabled(bool)));
	}
#endif // QGC_XBEE_ENABLED
    
    if (serial == 0 && udp == 0 && sim == 0 && tcp == 0
#ifdef UNITTEST_BUILD
            && mock == 0
#endif
#ifdef QGC_RTLAB_ENABLED
            && opal == 0
#endif
#ifdef QGC_XBEE_ENABLED
			&& xbee == 0
#endif // QGC_XBEE_ENABLED
       ) {
        qDebug() << "Link is NOT a known link, can't open configuration window";
    }

    connect(ui.linkType,SIGNAL(currentIndexChanged(int)),this,SLOT(linkCurrentIndexChanged(int)));

    // Open details pane for MAVLink if necessary
    MAVLinkProtocol* mavlink = MAVLinkProtocol::instance();
    QWidget* conf = new MAVLinkSettingsWidget(mavlink, this);
    ui.protocolScrollArea->setWidget(conf);
    ui.protocolGroupBox->setTitle(mavlink->getName()+" (Global Settings)");

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

void CommConfigurationWindow::linkCurrentIndexChanged(int currentIndex)
{
    setLinkType(static_cast<qgc_link_t>(ui.linkType->itemData(currentIndex).toInt()));
}

void CommConfigurationWindow::setLinkType(qgc_link_t linktype)
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
#ifdef QGC_XBEE_ENABLED
        case QGC_LINK_XBEE:
			{
				XbeeLink *xbee = new XbeeLink();
				tmpLink = xbee;
				break;
			}
#endif // QGC_XBEE_ENABLED
            
        case QGC_LINK_UDP:
			{
				UDPLink *udp = new UDPLink();
				tmpLink = udp;
				break;
			}
			
        case QGC_LINK_TCP:
            {
            TCPLink *tcp = new TCPLink();
            tmpLink = tcp;
            break;
            }

#ifdef QGC_RTLAB_ENABLED
        case QGC_LINK_OPAL:
			{
				OpalLink* opal = new OpalLink();
				tmpLink = opal;
				break;
			}
#endif // QGC_RTLAB_ENABLED
            
#ifdef UNITTEST_BUILD
        case QGC_LINK_MOCK:
        {
            MockLink* mock = new MockLink;
            tmpLink = mock;
            break;
        }
#endif
            
		default:
        case QGC_LINK_SERIAL:
			{
				SerialLink *serial = new SerialLink();
				tmpLink = serial;
				break;
			}
	}
    
    if (tmpLink) {
        LinkManager::instance()->addLink(tmpLink);
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
        LinkManager::instance()->connectLink(link);
        QGC::SLEEP::msleep(100);
        if (link->isConnected())
            // Auto-close window on connect
            this->window()->close();
    } else {
        LinkManager::instance()->disconnectLink(link);
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
        LinkManager::instance()->disconnectLink(link);  // disconnect connection
        //if(link->_ownedByLinkManager)
        LinkManager::instance()->deleteLink(link);
        //else
        //    link->deleteLater();
    }
    link=NULL;

    this->window()->close();
    this->deleteLater();
}

void CommConfigurationWindow::_linkConnected(void) {
    _connectionState(true);
}

void CommConfigurationWindow::_linkDisconnected(void) {
    _connectionState(false);
}

void CommConfigurationWindow::_connectionState(bool connect)
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

void CommConfigurationWindow::_setAutoConnect(bool state)
{
    link->setAutoConnect(state);
}
