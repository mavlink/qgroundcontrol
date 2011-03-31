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
 *   @author Alejandro Molina <am.alex09@gmail.com>
 */


#include "SlugsHilSim.h"
#include "ui_SlugsHilSim.h"


SlugsHilSim::SlugsHilSim(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsHilSim)
{
    ui->setupUi(this);

    rxSocket = new QUdpSocket(this);
    txSocket = new QUdpSocket(this);

    hilLink = NULL;

    connect(LinkManager::instance(), SIGNAL(newLink(LinkInterface*)), this, SLOT(addToCombo(LinkInterface*)));
    connect(ui->cb_mavlinkLinks, SIGNAL(currentIndexChanged(int)), this, SLOT(linkSelected(int)));
    connect(ui->bt_startHil, SIGNAL(clicked()), this, SLOT(putInHilMode()));
    connect(rxSocket, SIGNAL(readyRead()), this, SLOT(readDatagram()));

    linksAvailable.clear();

#ifdef MAVLINK_ENABLED_SLUGS
    memset(&tmpAirData, 0, sizeof(mavlink_air_data_t));
    memset(&tmpAttitudeData, 0, sizeof(mavlink_attitude_t));
    memset(&tmpGpsData, 0, sizeof(mavlink_gps_raw_t));
    memset(&tmpGpsTime, 0, sizeof(mavlink_gps_date_time_t));
    memset(&tmpLocalPositionData, 0, sizeof(mavlink_sensor_bias_t));
    memset(&tmpRawImuData, 0, sizeof(mavlink_raw_imu_t));
#endif

    foreach (LinkInterface* link, LinkManager::instance()->getLinks()) {
        addToCombo(link);
    }
}

SlugsHilSim::~SlugsHilSim()
{
    rxSocket->disconnectFromHost();
    delete ui;
}

void SlugsHilSim::addToCombo(LinkInterface* theLink)
{

    linksAvailable.insert(ui->cb_mavlinkLinks->count(),theLink);
    ui->cb_mavlinkLinks->addItem(theLink->getName());

    if (hilLink == NULL) {
        hilLink = theLink;
    }

}

void SlugsHilSim::putInHilMode(void)
{

    bool sw_enableControls = !(ui->bt_startHil->isChecked());
    QString  buttonCaption= ui->bt_startHil->isChecked()? "Stop Slugs HIL Mode": "Set Slugs in HIL Mode";

    if (ui->bt_startHil->isChecked()) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText("You are about to put SLUGS in HIL Mode.");
        msgBox.setInformativeText("It will stop reading the actual sensor readings. Do you wish to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        if(msgBox.exec() == QMessageBox::Yes) {
            rxSocket->disconnectFromHost();
            rxSocket->bind(QHostAddress::Any, ui->ed_rxPort->text().toInt());
            //txSocket->bind(QHostAddress::Broadcast, ui->ed_txPort->text().toInt());

            ui->ed_ipAdress->setEnabled(sw_enableControls);
            ui->ed_rxPort->setEnabled(sw_enableControls);
            ui->ed_txPort->setEnabled(sw_enableControls);
            ui->cb_mavlinkLinks->setEnabled(sw_enableControls);

            ui->bt_startHil->setText(buttonCaption);

            activeUas->startHil();

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
        activeUas->stopHil();
    }
}

void SlugsHilSim::readDatagram(void)
{
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

            sendMessageToSlugs();

            commandDatagramToSimulink();
        }

        ui->ed_count->setText(QString::number(count++));
    }
}


void SlugsHilSim::activeUasSet(UASInterface* uas)
{

    if (uas != NULL) {
        activeUas = static_cast <UAS *>(uas);
    }
}


void SlugsHilSim::processHilDatagram(const QByteArray* datagram)
{
#ifdef MAVLINK_ENABLED_SLUGS
    unsigned char i = 0;


    tmpGpsTime.year  = datagram->at(i++);
    tmpGpsTime.month = datagram->at(i++);
    tmpGpsTime.day   = datagram->at(i++);
    tmpGpsTime.hour  = datagram->at(i++);
    tmpGpsTime.min   = datagram->at(i++);
    tmpGpsTime.sec   = datagram->at(i++);

    tmpGpsData.lat = getFloatFromDatagram(datagram, &i);
    tmpGpsData.lon = getFloatFromDatagram(datagram, &i);
    tmpGpsData.alt = getFloatFromDatagram(datagram, &i);

    tmpGpsData.hdg = getUint16FromDatagram(datagram, &i);
    tmpGpsData.v   = getUint16FromDatagram(datagram, &i);

    tmpGpsData.eph = getUint16FromDatagram(datagram, &i);
    tmpGpsData.fix_type = datagram->at(i++);
    tmpGpsTime.visSat  = datagram->at(i++);
    i++;

    tmpAirData.dynamicPressure= getFloatFromDatagram(datagram, &i);
    tmpAirData.staticPressure= getFloatFromDatagram(datagram, &i);
    tmpAirData.temperature= getUint16FromDatagram(datagram, &i);

    // TODO Salto en el Datagrama
    i=i+8;

    tmpRawImuData.xgyro = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.ygyro = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.zgyro = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.xacc = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.yacc = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.zacc = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.xmag = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.ymag = getUint16FromDatagram(datagram, &i);
    tmpRawImuData.zmag = getUint16FromDatagram(datagram, &i);

    tmpAttitudeData.roll = getFloatFromDatagram(datagram, &i);
    tmpAttitudeData.pitch = getFloatFromDatagram(datagram, &i);
    tmpAttitudeData.yaw = getFloatFromDatagram(datagram, &i);

    tmpAttitudeData.rollspeed = getFloatFromDatagram(datagram, &i);
    tmpAttitudeData.pitchspeed = getFloatFromDatagram(datagram, &i);
    tmpAttitudeData.yawspeed = getFloatFromDatagram(datagram, &i);

    // TODO Crear Paquete SYNC TIME
    i=i+2;

    tmpLocalPositionData.x = getFloatFromDatagram(datagram, &i);
    tmpLocalPositionData.y = getFloatFromDatagram(datagram, &i);
    tmpLocalPositionData.z = getFloatFromDatagram(datagram, &i);
    tmpLocalPositionData.vx = getFloatFromDatagram(datagram, &i);
    tmpLocalPositionData.vy = getFloatFromDatagram(datagram, &i);
    tmpLocalPositionData.vz = getFloatFromDatagram(datagram, &i);

    // TODO: this is legacy of old HIL datagram. Need to remove from Simulink model
    i++;

    ui->ed_1->setText(QString::number(tmpRawImuData.xacc));
    ui->ed_2->setText(QString::number(tmpRawImuData.yacc));
    ui->ed_3->setText(QString::number(tmpRawImuData.zacc));

    ui->tbA->setText(QString::number(tmpRawImuData.xgyro));
    ui->tbB->setText(QString::number(tmpRawImuData.ygyro));
    ui->tbC->setText(QString::number(tmpRawImuData.zgyro));

#else
    Q_UNUSED(datagram);
#endif
}

float SlugsHilSim::getFloatFromDatagram (const QByteArray* datagram, unsigned char * i)
{
    tFloatToChar tmpF2C;

    tmpF2C.chData[0] = datagram->at((*i)++);
    tmpF2C.chData[1] = datagram->at((*i)++);
    tmpF2C.chData[2] = datagram->at((*i)++);
    tmpF2C.chData[3] = datagram->at((*i)++);

    return tmpF2C.flData;
}

uint16_t SlugsHilSim::getUint16FromDatagram (const QByteArray* datagram, unsigned char * i)
{
    tUint16ToChar tmpU2C;

    tmpU2C.chData[0] = datagram->at((*i)++);
    tmpU2C.chData[1] = datagram->at((*i)++);

    return tmpU2C.uiData;

}

void SlugsHilSim::linkSelected(int cbIndex)
{
#ifdef MAVLINK_ENABLED_SLUGS
    // HIL code to go here...
    //hilLink = linksAvailable
    // FIXME Mariano

    hilLink =linksAvailable.value(cbIndex);

#else
    Q_UNUSED(cbIndex)
#endif
}

void SlugsHilSim::sendMessageToSlugs()
{
#ifdef MAVLINK_ENABLED_SLUGS
    mavlink_message_t msg;

    mavlink_msg_local_position_encode(MG::SYSTEM::ID,
                                      MG::SYSTEM::COMPID,
                                      &msg,
                                      &tmpLocalPositionData);
    activeUas->sendMessage(hilLink, msg);
    memset(&msg, 0, sizeof(mavlink_message_t));

    mavlink_msg_attitude_encode(MG::SYSTEM::ID,
                                MG::SYSTEM::COMPID,
                                &msg,
                                &tmpAttitudeData);
    activeUas->sendMessage(hilLink, msg);
    memset(&msg, 0, sizeof(mavlink_message_t));

    mavlink_msg_raw_imu_encode(MG::SYSTEM::ID,
                               MG::SYSTEM::COMPID,
                               &msg,
                               &tmpRawImuData);
    activeUas->sendMessage(hilLink, msg);
    memset(&msg, 0, sizeof(mavlink_message_t));

    mavlink_msg_air_data_encode(MG::SYSTEM::ID,
                                MG::SYSTEM::COMPID,
                                &msg,
                                &tmpAirData);
    activeUas->sendMessage(hilLink, msg);
    memset(&msg, 0, sizeof(mavlink_message_t));

    mavlink_msg_gps_raw_encode(MG::SYSTEM::ID,
                               MG::SYSTEM::COMPID,
                               &msg,
                               &tmpGpsData);
    activeUas->sendMessage(hilLink, msg);
    memset(&msg, 0, sizeof(mavlink_message_t));

    mavlink_msg_gps_date_time_encode(MG::SYSTEM::ID,
                                     MG::SYSTEM::COMPID,
                                     &msg,
                                     &tmpGpsTime);
    activeUas->sendMessage(hilLink, msg);
    memset(&msg, 0, sizeof(mavlink_message_t));
#endif
}


void SlugsHilSim::commandDatagramToSimulink()
{
#ifdef MAVLINK_ENABLED_SLUGS
    //mavlink_pwm_commands_t* pwdC = (static_cast<SlugsMAV*>(activeUas))->getPwmCommands();

    //mavlink_pwm_commands_t* pwdC;

//    if(pwdC != NULL){
//    }

    QByteArray data;
    data.resize(22);

    unsigned char i=0;
    setUInt16ToDatagram(data, &i, 1);//pwdC->dt_c);
    setUInt16ToDatagram(data, &i, 2);//pwdC->dla_c);
    setUInt16ToDatagram(data, &i, 3);//pwdC->dra_c);
    setUInt16ToDatagram(data, &i, 4);//pwdC->dr_c);
    setUInt16ToDatagram(data, &i, 5);//pwdC->dle_c);
    setUInt16ToDatagram(data, &i, 6);//pwdC->dre_c);
    setUInt16ToDatagram(data, &i, 7);//pwdC->dlf_c);
    setUInt16ToDatagram(data, &i, 8);//pwdC->drf_c);
    setUInt16ToDatagram(data, &i, 9);//pwdC->aux1);
    setUInt16ToDatagram(data, &i, 10);//pwdC->aux2);
    setUInt16ToDatagram(data, &i, 11);//value default

    txSocket->writeDatagram(data, QHostAddress::Broadcast, ui->ed_txPort->text().toInt());
#endif
}

void SlugsHilSim::setUInt16ToDatagram(QByteArray& datagram, unsigned char* pos, uint16_t value)
{
    tUint16ToChar tmpUnion;
    tmpUnion.uiData= value;

    datagram[(*pos)++]= tmpUnion.chData[0];
    datagram[(*pos)++]= tmpUnion.chData[1];
}
