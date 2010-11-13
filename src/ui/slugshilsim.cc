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
 *   @brief Configuration Window for Slugs' HIL Simulator
 *   @author Mariano Lizarraga <malife@gmail.com>
 */


#include "slugshilsim.h"
#include "ui_slugshilsim.h"
#include "LinkManager.h"

SlugsHilSim::SlugsHilSim(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsHilSim)
{
    ui->setupUi(this);

    rxSocket = new QUdpSocket(this);
    txSocket = new QUdpSocket(this);

    connect(LinkManager::instance(), SIGNAL(newLink(LinkInterface*)), this, SLOT(addToCombo(LinkInterface*)));
    connect(ui->bt_startHil, SIGNAL(clicked()), this, SLOT(putInHilMode()));
    connect(rxSocket, SIGNAL(readyRead()), this, SLOT(readDatagram()));

    linksAvailable.clear();
}

SlugsHilSim::~SlugsHilSim()
{
    delete ui;
}

void SlugsHilSim::linkAdded(void){

//  ui->cb_mavlinkLinks->clear();

//  QList<LinkInterface *> linkList;
//  linkList.append(LinkManager::instance()->getLinks()) ;

//  for (int i = 0; i< linkList.size(); i++){
//    ui->cb_mavlinkLinks->addItem((linkList.takeFirst())->getName());
//  }

}

void SlugsHilSim::addToCombo(LinkInterface* theLink){

  ui->cb_mavlinkLinks->addItem(theLink->getName());
  linksAvailable.insert(ui->cb_mavlinkLinks->count(),theLink);
}

void SlugsHilSim::putInHilMode(void){

  bool sw_enableControls = !(ui->bt_startHil->isChecked());
  QString  buttonCaption= ui->bt_startHil->isChecked()? "Stop Slugs HIL Mode": "Set Slugs in HIL Mode";

  if (ui->bt_startHil->isChecked()){
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("You are about to put SLUGS in HIL Mode.");
    msgBox.setInformativeText("It will stop reading the actual sensor readings. Do you wish to continue?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if(msgBox.exec() == QMessageBox::Yes)
    {
      rxSocket->bind(QHostAddress::Any, ui->ed_rxPort->text().toInt());
      txSocket->bind(QHostAddress::Broadcast, ui->ed_txPort->text().toInt());

      ui->ed_ipAdress->setEnabled(sw_enableControls);
      ui->ed_rxPort->setEnabled(sw_enableControls);
      ui->ed_txPort->setEnabled(sw_enableControls);
      ui->cb_mavlinkLinks->setEnabled(sw_enableControls);

      ui->bt_startHil->setText(buttonCaption);


    } else {
      ui->bt_startHil->setChecked(false);
    }
  } else {
    ui->ed_ipAdress->setEnabled(sw_enableControls);
    ui->ed_rxPort->setEnabled(sw_enableControls);
    ui->ed_txPort->setEnabled(sw_enableControls);
    ui->cb_mavlinkLinks->setEnabled(sw_enableControls);

    ui->bt_startHil->setText(buttonCaption);
  }




}

void SlugsHilSim::readDatagram(void){

}


void SlugsHilSim::activeUasSet(UASInterface* uas){

  if (uas != NULL) {
    //activeUas = uas;
  }
}
