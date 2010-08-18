/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
Please see our website at <http://pixhawk.ethz.ch>

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

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
 *   @brief Implementation of configuration window for serial links
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
#include "SerialLinkInterface.h"
#include "UDPLink.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"

CommConfigurationWindow::CommConfigurationWindow(LinkInterface* link, ProtocolInterface* protocol, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
    this->link = link;

    // Setup the user interface according to link type
    ui.setupUi(this);

    // Create action to open this menu
    // Create configuration action for this link
    // Connect the current UAS
    action = new QAction(QIcon(":/images/devices/network-wireless.svg"), "", link);
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

    if(this->link->isConnected())
    {
        ui.connectionStatusLabel->setText(tr("Connected"));

        // TODO Deactivate all settings to force user to manually disconnect first
    }
    else
    {
        ui.connectionStatusLabel->setText(tr("Disconnected"));
    }

    // TODO Move these calls to each link so that dynamic casts vanish

    // Open details pane for serial link if necessary
    SerialLinkInterface* serial = dynamic_cast<SerialLinkInterface*>(link);
    if(serial != 0)
    {
        QWidget* conf = new SerialConfigurationWindow(serial, this);
        QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, ui.linkGroupBox);
        layout->addWidget(conf);
        ui.linkGroupBox->setLayout(layout);
        ui.linkGroupBox->setTitle(tr("serial link"));
        //ui.linkGroupBox->setTitle(link->getName());
        //connect(link, SIGNAL(nameChanged(QString)), ui.linkGroupBox, SLOT(setTitle(QString)));
    }
    else if (dynamic_cast<UDPLink*>(link) != 0)
    {
        ui.linkGroupBox->setTitle(tr("UDP Link"));
    }
    else
    {
        qDebug() << "Link is NOT a known link, can't open configuration window";
    }

    // Open details pane for MAVLink if necessary
    MAVLinkProtocol* mavlink = dynamic_cast<MAVLinkProtocol*>(protocol);
    if(mavlink != 0)
    {
        QWidget* conf = new MAVLinkSettingsWidget(mavlink, this);
        QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, ui.protocolGroupBox);
        layout->addWidget(conf);
        ui.protocolGroupBox->setLayout(layout);
        ui.protocolGroupBox->setTitle(protocol->getName());
    }
    else
    {
        qDebug() << "Protocol is NOT MAVLink, can't open configuration window";
    }

    // Open details for UDP link if necessary
    // TODO

    // Display the widget
    this->window()->setWindowTitle(tr("Settings for ") + this->link->getName());
    this->hide();
}

CommConfigurationWindow::~CommConfigurationWindow() {

}

QAction* CommConfigurationWindow::getAction()
{
    return action;
}

void CommConfigurationWindow::setLinkType(int linktype)
{
    // Adjust the form layout per link type
    Q_UNUSED(linktype);
}

void CommConfigurationWindow::setConnection()
{
    if(!link->isConnected())
    {
        link->connect();
    }
    else
    {
        link->disconnect();
    }
}

void CommConfigurationWindow::setLinkName(QString name)
{
    Q_UNUSED(name); // FIXME
    action->setText(tr("Configure ") + link->getName());
    action->setStatusTip(tr("Configure ") + link->getName());
    this->window()->setWindowTitle(tr("Settings for ") + this->link->getName());
}

void CommConfigurationWindow::remove()
{
    link->disconnect();
    //delete link;
    //delete action;
    this->window()->close();
    qDebug() << "TODO: Link cannot be deleted: CommConfigurationWindow::remove() NOT IMPLEMENTED!";
}

void CommConfigurationWindow::connectionState(bool connect)
{
    ui.connectButton->setChecked(connect);
    if(connect)
    {
        ui.connectionStatusLabel->setText(tr("Connected"));
        ui.connectButton->setText(tr("Disconnect"));
    }
    else
    {
        ui.connectionStatusLabel->setText(tr("Disconnected"));
        ui.connectButton->setText(tr("Connect"));
    }
}
