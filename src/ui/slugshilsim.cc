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
    rxSocket->disconnectFromHost();
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
      rxSocket->disconnectFromHost();
      rxSocket->bind(QHostAddress::Any, ui->ed_rxPort->text().toInt());
      //txSocket->bind(QHostAddress::Broadcast, ui->ed_txPort->text().toInt());

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

    rxSocket->disconnectFromHost();
  }
}

void SlugsHilSim::readDatagram(void){
  static int count = 0;
  while (rxSocket->hasPendingDatagrams()) {
           QByteArray datagram;
           datagram.resize(rxSocket->pendingDatagramSize());
           QHostAddress sender;
           quint16 senderPort;

           rxSocket->readDatagram(datagram.data(), datagram.size(),
                                   &sender, &senderPort);

           if (datagram.size() == 113) {
             processHilDatagram(&datagram);
           }

           ui->ed_count->setText(QString::number(count++));
       }
}


void SlugsHilSim::activeUasSet(UASInterface* uas){

  if (uas != NULL) {
    activeUas = static_cast <UAS *>(uas);
  }
}


void SlugsHilSim::processHilDatagram(const QByteArray* datagram){
  unsigned char i = 0;

  mavlink_message_t msg;

  // GPS
  mavlink_gps_raw_t tmpGpsRaw;
  mavlink_gps_date_time_t tmpGpsTime;

  tmpGpsTime.year  = datagram->at(i++);
  tmpGpsTime.month = datagram->at(i++);
  tmpGpsTime.day   = datagram->at(i++);
  tmpGpsTime.hour  = datagram->at(i++);
  tmpGpsTime.min   = datagram->at(i++);
  tmpGpsTime.sec   = datagram->at(i++);

  tmpGpsRaw.lat = getFloatFromDatagram(datagram, &i);
  tmpGpsRaw.lon = getFloatFromDatagram(datagram, &i);
  tmpGpsRaw.alt = getFloatFromDatagram(datagram, &i);

  tmpGpsRaw.hdg = getUint16FromDatagram(datagram, &i);
  tmpGpsRaw.v   = getUint16FromDatagram(datagram, &i);
  tmpGpsRaw.eph = getUint16FromDatagram(datagram, &i);

  tmpGpsRaw.fix_type = datagram->at(i++);
  tmpGpsTime.visSat  = datagram->at(i++);

  //mavlink_msg_gps_date_time_pack();

  //activeUas->sendMessage();

  // TODO: this is legacy of old HIL datagram. Need to remove from Simulink model
  i++;

  ui->ed_1->setText(QString::number(tmpGpsRaw.hdg));
  ui->ed_2->setText(QString::number(tmpGpsRaw.v));
  ui->ed_3->setText(QString::number(tmpGpsRaw.eph));
}

float SlugsHilSim::getFloatFromDatagram (const QByteArray* datagram, unsigned char * i){
  tFloatToChar tmpF2C;

  tmpF2C.chData[0] = datagram->at((*i)++);
  tmpF2C.chData[1] = datagram->at((*i)++);
  tmpF2C.chData[2] = datagram->at((*i)++);
  tmpF2C.chData[3] = datagram->at((*i)++);


//  if (uas != NULL) {
//    //activeUas = uas;
//  }

  return tmpF2C.flData;
}

uint16_t SlugsHilSim::getUint16FromDatagram (const QByteArray* datagram, unsigned char * i){
  tUint16ToChar tmpU2C;

  tmpU2C.chData[0] = datagram->at((*i)++);
  tmpU2C.chData[1] = datagram->at((*i)++);

  return tmpU2C.uiData;

}
