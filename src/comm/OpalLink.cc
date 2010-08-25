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
 *   @brief Implementation of class OpalLink
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#include "OpalLink.h"

OpalLink::OpalLink() :
        connectState(false),
        heartbeatTimer(new QTimer(this)),
        heartbeatRate(MAVLINK_HEARTBEAT_DEFAULT_RATE),
        m_heartbeatsEnabled(true),
        getSignalsTimer(new QTimer(this)),
        getSignalsPeriod(10),
        receiveBuffer(new QQueue<QByteArray>()),
        systemID(1),
        componentID(1)
{
    start(QThread::LowPriority);

    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
    this->name = tr("OpalRT link ") + QString::number(getId());
    LinkManager::instance()->add(this);

    // Start heartbeat timer, emitting a heartbeat at the configured rate
    QObject::connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT(heartbeat()));

    QObject::connect(getSignalsTimer, SIGNAL(timeout()), this, SLOT(getSignals()));
}


/*
 *
  Communication
 *
 */

qint64 OpalLink::bytesAvailable()
{
    return 0;
}

void OpalLink::writeBytes(const char *bytes, qint64 length)
{
    /* decode the message */
    mavlink_message_t msg;
    mavlink_status_t status;        
    int decodeSuccess = 0;
    for (int i=0; (!(decodeSuccess=mavlink_parse_char(this->getId(), bytes[i], &msg, &status))&& i<length); ++i);    

    /* perform the appropriate action */
    if (decodeSuccess)
    {
        switch(msg.msgid)
        {
        case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
            {
                qDebug() << "OpalLink::writeBytes(): request params";
                mavlink_message_t param;
                char paramName[] = "NAV_FILT_INIT";
                mavlink_msg_param_value_pack(systemID, componentID, &param,
                                             (int8_t*)(paramName),
                                             0,
                                             1,
                                             0);
                receiveMessage(param);
            }
        case MAVLINK_MSG_ID_PARAM_SET:
            {
                qDebug() << "OpalLink::writeBytes(): Attempt to set a parameter";
                mavlink_param_set_t param;
                mavlink_msg_param_set_decode(&msg, &param);
                QString paramName((char*)param.param_id);
                if (paramName == "NAV_FILT_INIT")
                {
                    if (param.param_value == 1 || param.param_value == 0)
                    {
                        double values[2] = {};
                        values[0] = param.param_value;
                        setSignals(values);
                    }
                    else
                    {
                        qDebug() << "OpalLink::writeBytes(): Param NAV_FILT_INIT must be 1 or 0";
                    }
                }
            }
            break;
        default:
            {
                qDebug() << "OpalLink::writeBytes(): Unknown mavlink packet";
            }
        }
    }
}


void OpalLink::readBytes()
{
    receiveDataMutex.lock();
//    qDebug() << "OpalLink::readBytes(): Reading a message.  size of buffer: " << receiveBuffer->count();
//    QByteArray message = receiveBuffer->dequeue();


    emit bytesReceived(this, receiveBuffer->dequeue());
    receiveDataMutex.unlock();

}

void OpalLink::receiveMessage(mavlink_message_t message)
{

    // Create buffer
    char buffer[MAVLINK_MAX_PACKET_LEN];
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer((uint8_t*)(buffer), &message);
    // If link is connected
    if (isConnected())
    {
        receiveDataMutex.lock();
        receiveBuffer->enqueue(QByteArray(buffer, len));
        receiveDataMutex.unlock();
        readBytes();
    }

}

void OpalLink::heartbeat()
{

    if (m_heartbeatsEnabled)
    {
//        qDebug() << "OpalLink::heartbeat(): Generate a heartbeat";
        mavlink_message_t beat;
        mavlink_msg_heartbeat_pack(systemID, componentID,&beat, MAV_HELICOPTER, MAV_AUTOPILOT_GENERIC);
        receiveMessage(beat);
    }

}
void OpalLink::setSignals(double *values)
{
    unsigned short numSignals = 9;
    unsigned short logicalId = 1;
    unsigned short signalIndex[] = {0,1};
//    double values[] = {0.5, // ch 1
//                       0.5, // ch2
//                       0.5, // ch3
//                       0.5, // ch4
//                       0.5, // ch5
//                       0.5, // ch6
//                       0.5, // ch7
//                       0.5, // ch8
//                       0.5}; // ch9

    int returnValue;
    returnValue =  OpalSetSignals( numSignals, logicalId, signalIndex, values);
    if (returnValue != EOK)
    {
        setLastErrorMsg();
        displayErrorMsg();
    }
}
void OpalLink::getSignals()
{
//    getSignalsMutex.lock();
//    qDebug() <<  "OpalLink::getSignals(): Attempting to acquire signals";


    unsigned long  timeout = 0;
    unsigned short acqGroup = 0; //this is actually group 1 in the model
    unsigned short allocatedSignals = NUM_OUTPUT_SIGNALS;
    unsigned short *numSignals = new unsigned short(0);
    double *timestep = new double(0);
    double values[NUM_OUTPUT_SIGNALS] = {};
    unsigned short *lastValues = new unsigned short(false);
    unsigned short *decimation = new unsigned short(0);

    while (!(*lastValues))
    {
        int returnVal = OpalGetSignals(timeout, acqGroup, allocatedSignals, numSignals, timestep,
                                       values, lastValues, decimation);

        if (returnVal == EOK )
        {
            //        qDebug() << "OpalLink::getSignals: Timestep=" << *timestep;// << ", Last? " << (bool)(*lastValues);
            /* Send position info to qgroundcontrol */
            mavlink_message_t local_position;
            mavlink_msg_local_position_pack(systemID, componentID, &local_position,
                                            (*timestep)*1000000,
                                            values[OpalRT::X_POS],
                                            values[OpalRT::Y_POS],
                                            values[OpalRT::Z_POS],
                                            values[OpalRT::X_VEL],
                                            values[OpalRT::Y_VEL],
                                            values[OpalRT::Z_VEL]);
            receiveMessage(local_position);

            /* send attitude info to qgroundcontrol */
            mavlink_message_t attitude;
            mavlink_msg_attitude_pack(systemID, componentID, &attitude,
                                      (*timestep)*1000000,
                                      values[OpalRT::ROLL],
                                      values[OpalRT::PITCH],
                                      values[OpalRT::YAW],
                                      0, // rollspeed
                                      0, // pitchspeed
                                      0 // yawspeed
                                      );
            receiveMessage(attitude);

            /* send bias info to qgroundcontrol */
            mavlink_message_t bias;
            mavlink_msg_nav_filter_bias_pack(systemID, componentID, &bias,
                                             (*timestep)*1000000,
                                             values[OpalRT::B_F_0],
                                             values[OpalRT::B_F_1],
                                             values[OpalRT::B_F_2],
                                             values[OpalRT::B_W_0],
                                             values[OpalRT::B_W_1],
                                             values[OpalRT::B_W_2]
                                             );
            receiveMessage(bias);
        }
//        else if (returnVal == EAGAIN)
//        {
//            qDebug() << "OpalLink::getSignals: Data was not ready";
//        }
        // if returnVal == EAGAIN => data just wasn't ready
        else if (returnVal != EAGAIN)
        {
            getSignalsTimer->stop();
            displayErrorMsg();
        }
    }

    /* deallocate used memory */

    delete numSignals;
    delete timestep;
    delete lastValues;
    delete decimation;
//    getSignalsMutex.unlock();

}

/*
 *
  Administrative
 *
 */
void OpalLink::run()
{
//    qDebug() << "OpalLink::run():: Starting the thread";
}

int OpalLink::getId()
{
    return id;
}

QString OpalLink::getName()
{
    return name;
}

void OpalLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}

bool OpalLink::isConnected() {
    //qDebug() << "OpalLink::isConnected:: connectState: " << connectState;
    return connectState;
}




bool OpalLink::connect()
{
    short modelState;

    /// \todo allow configuration of instid in window    
    if ((OpalConnect(101, false, &modelState) == EOK) && (OpalGetSignalControl(0, true) == EOK))
    {
        connectState = true;
        emit connected();
        heartbeatTimer->start(1000/heartbeatRate);
        getSignalsTimer->start(getSignalsPeriod);
    }
    else
    {
        connectState = false;
        displayErrorMsg();
    }

    emit connected(connectState);
    return connectState;
}

bool OpalLink::disconnect()
{
    return false;
}

void OpalLink::displayErrorMsg()
{
    setLastErrorMsg();
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(lastErrorMsg);
    msgBox.exec();
}

void OpalLink::setLastErrorMsg()
{
    char buf[512];
    unsigned short len;
    OpalGetLastErrMsg(buf, sizeof(buf), &len);
    lastErrorMsg.clear();
    lastErrorMsg.append(buf);
}


/*
 *
  Statisctics
 *
 */

qint64 OpalLink::getNominalDataRate()
{
    return 0; //unknown
}

int OpalLink::getLinkQuality()
{
    return -1; //not supported
}

qint64 OpalLink::getTotalUpstream()
{
    statisticsMutex.lock();
    qint64 totalUpstream =  bitsSentTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalUpstream;
}

qint64 OpalLink::getTotalDownstream() {
    statisticsMutex.lock();
    qint64 totalDownstream = bitsReceivedTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalDownstream;
}

qint64 OpalLink::getCurrentUpstream()
{
    return 0; //unknown
}

qint64 OpalLink::getMaxUpstream()
{
    return 0; //unknown
}

qint64 OpalLink::getBitsSent() {
    return bitsSentTotal;
}

qint64 OpalLink::getBitsReceived() {
    return bitsReceivedTotal;
}


bool OpalLink::isFullDuplex()
{
    return false;
}
