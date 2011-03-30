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
    componentID(1),
    params(NULL),
    opalInstID(101),
    sendRCValues(false),
    sendRawController(false),
    sendPosition(false)
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
    if (decodeSuccess) {
        switch(msg.msgid) {
        case MAVLINK_MSG_ID_PARAM_REQUEST_LIST: {
            qDebug() << "OpalLink::writeBytes(): request params";

            mavlink_message_t param;


            OpalRT::ParameterList::const_iterator paramIter;
            for (paramIter = params->begin(); paramIter != params->end(); ++paramIter) {
                mavlink_msg_param_value_pack(systemID,
                                             (*paramIter).getComponentID(),
                                             &param,
                                             (*paramIter).getParamID().toInt8_t(),
                                             (static_cast<OpalRT::Parameter>(*paramIter)).getValue(),
                                             params->count(),
                                             params->indexOf(*paramIter));
                receiveMessage(param);
            }


        }
        case MAVLINK_MSG_ID_PARAM_SET: {

//                qDebug() << "OpalLink::writeBytes(): Attempt to set a parameter";

            mavlink_param_set_t param;
            mavlink_msg_param_set_decode(&msg, &param);
            OpalRT::QGCParamID paramName((char*)param.param_id);

//                qDebug() << "OpalLink::writeBytes():paramName: " << paramName;

            if ((*params).contains(param.target_component, paramName)) {
                OpalRT::Parameter p = (*params)(param.target_component, paramName);
//                    qDebug() << __FILE__ << ":" << __LINE__ << ": "  << p;
                // Set the param value in Opal-RT
                p.setValue(param.param_value);

                // Get the param value from Opal-RT to make sure it was set properly
                mavlink_message_t paramMsg;
                mavlink_msg_param_value_pack(systemID,
                                             p.getComponentID(),
                                             &paramMsg,
                                             p.getParamID().toInt8_t(),
                                             p.getValue(),
                                             params->count(),
                                             params->indexOf(p));
                receiveMessage(paramMsg);
            }
        }
        break;
//        case MAVLINK_MSG_ID_REQUEST_RC_CHANNELS:
//        {
//        	mavlink_request_rc_channels_t rc;
//        	mavlink_msg_request_rc_channels_decode(&msg, &rc);
//        	this->sendRCValues = static_cast<bool>(rc.enabled);
//        }
//        break;
#ifdef MAVLINK_ENABLED_UALBERTA_MESSAGES
        case MAVLINK_MSG_ID_RADIO_CALIBRATION: {
            mavlink_radio_calibration_t radio;
            mavlink_msg_radio_calibration_decode(&msg, &radio);
//            qDebug() << "RADIO CALIBRATION RECEIVED";
//            qDebug() << "AILERON: " << radio.aileron[0] << " " << radio.aileron[1] << " " << radio.aileron[2];
//            qDebug() << "ELEVATOR: " << radio.elevator[0] << " " << radio.elevator[1] << " " << radio.elevator[2];
//            qDebug() << "RUDDER: " << radio.rudder[0] << " " << radio.rudder[1] << " " << radio.rudder[2];
//            qDebug() << "GYRO: " << radio.gyro[0] << " " << radio.gyro[1];
//            qDebug() << "PITCH: " << radio.pitch[0] << radio.pitch[1] << radio.pitch[2] << radio.pitch[3] << radio.pitch[4];
//            qDebug() << "THROTTLE: " << radio.throttle[0] << radio.throttle[1] << radio.throttle[2] << radio.throttle[3] << radio.throttle[4];

            /* AILERON SERVO */
            if (params->contains(OpalRT::SERVO_INPUTS, "AIL_RIGHT_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "AIL_RIGHT_IN").setValue(((radio.aileron[0]>900 /*in us?*/)?radio.aileron[0]/1000:radio.aileron[0]));
            if (params->contains(OpalRT::SERVO_OUTPUTS, "AIL_RIGHT_OUT"))
                params->getParameter(OpalRT::SERVO_OUTPUTS, "AIL_RIGHT_OUT").setValue(((radio.aileron[0]>900 /*in us?*/)?radio.aileron[0]/1000:radio.aileron[0]));
            if (params->contains(OpalRT::SERVO_INPUTS, "AIL_CENTER_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "AIL_CENTER_IN").setValue(((radio.aileron[1]>900 /*in us?*/)?radio.aileron[1]/1000:radio.aileron[1]));
            if (params->contains(OpalRT::SERVO_OUTPUTS, "AIL_CENTER_OUT"))
                params->getParameter(OpalRT::SERVO_OUTPUTS, "AIL_CENTER_OUT").setValue(((radio.aileron[1]>900 /*in us?*/)?radio.aileron[1]/1000:radio.aileron[1]));
            if (params->contains(OpalRT::SERVO_INPUTS, "AIL_LEFT_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "AIL_LEFT_IN").setValue(((radio.aileron[2]>900 /*in us?*/)?radio.aileron[2]/1000:radio.aileron[2]));
            if (params->contains(OpalRT::SERVO_OUTPUTS, "AIL_LEFT_OUT"))
                params->getParameter(OpalRT::SERVO_OUTPUTS, "AIL_LEFT_OUT").setValue(((radio.aileron[2]>900 /*in us?*/)?radio.aileron[2]/1000:radio.aileron[2]));
            /* ELEVATOR SERVO */
            if (params->contains(OpalRT::SERVO_INPUTS, "ELE_DOWN_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "ELE_DOWN_IN").setValue(((radio.elevator[0]>900 /*in us?*/)?radio.elevator[0]/1000:radio.elevator[0]));
            if (params->contains(OpalRT::SERVO_OUTPUTS, "ELE_DOWN_OUT"))
                params->getParameter(OpalRT::SERVO_OUTPUTS, "ELE_DOWN_OUT").setValue(((radio.elevator[0]>900 /*in us?*/)?radio.elevator[0]/1000:radio.elevator[0]));
            if (params->contains(OpalRT::SERVO_INPUTS, "ELE_CENTER_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "ELE_CENTER_IN").setValue(((radio.elevator[1]>900 /*in us?*/)?radio.elevator[1]/1000:radio.elevator[1]));
            if (params->contains(OpalRT::SERVO_OUTPUTS, "ELE_CENTER_OUT"))
                params->getParameter(OpalRT::SERVO_OUTPUTS, "ELE_CENTER_OUT").setValue(((radio.elevator[1]>900 /*in us?*/)?radio.elevator[1]/1000:radio.elevator[1]));
            if (params->contains(OpalRT::SERVO_INPUTS, "ELE_UP_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "ELE_UP_IN").setValue(((radio.elevator[2]>900 /*in us?*/)?radio.elevator[2]/1000:radio.elevator[2]));
            if (params->contains(OpalRT::SERVO_OUTPUTS, "ELE_UP_OUT"))
                params->getParameter(OpalRT::SERVO_OUTPUTS, "ELE_UP_OUT").setValue(((radio.elevator[2]>900 /*in us?*/)?radio.elevator[2]/1000:radio.elevator[2]));
            /* THROTTLE SERVO */
            if (params->contains(OpalRT::SERVO_INPUTS, "THR_SET0_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "THR_SET0_IN").setValue(((radio.throttle[0]>900 /*in us?*/)?radio.throttle[0]/1000:radio.throttle[0]));
            if (params->contains(OpalRT::SERVO_INPUTS, "THR_SET1_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "THR_SET1_IN").setValue(((radio.throttle[1]>900 /*in us?*/)?radio.throttle[1]/1000:radio.throttle[1]));
            if (params->contains(OpalRT::SERVO_INPUTS, "THR_SET2_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "THR_SET2_IN").setValue(((radio.throttle[2]>900 /*in us?*/)?radio.throttle[2]/1000:radio.throttle[2]));
            if (params->contains(OpalRT::SERVO_INPUTS, "THR_SET3_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "THR_SET3_IN").setValue(((radio.throttle[3]>900 /*in us?*/)?radio.throttle[3]/1000:radio.throttle[3]));
            if (params->contains(OpalRT::SERVO_INPUTS, "THR_SET4_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "THR_SET4_IN").setValue(((radio.throttle[4]>900 /*in us?*/)?radio.throttle[4]/1000:radio.throttle[4]));
            /* RUDDER SERVO */
            if (params->contains(OpalRT::SERVO_INPUTS, "RUD_LEFT_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "RUD_LEFT_IN").setValue(((radio.rudder[0]>900 /*in us?*/)?radio.rudder[0]/1000:radio.rudder[0]));
            if (params->contains(OpalRT::SERVO_INPUTS, "RUD_CENTER_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "RUD_CENTER_IN").setValue(((radio.rudder[1]>900 /*in us?*/)?radio.rudder[1]/1000:radio.rudder[1]));
            if (params->contains(OpalRT::SERVO_INPUTS, "RUD_RIGHT_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "RUD_RIGHT_IN").setValue(((radio.rudder[2]>900 /*in us?*/)?radio.rudder[2]/1000:radio.rudder[2]));
            /* GYRO MODE/GAIN SWITCH */
            if (params->contains(OpalRT::SERVO_INPUTS, "GYRO_DEF_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "GYRO_DEF_IN").setValue(((radio.gyro[0]>900 /*in us?*/)?radio.gyro[0]/1000:radio.gyro[0]));
            if (params->contains(OpalRT::SERVO_INPUTS, "GYRO_TOG_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "GYRO_TOG_IN").setValue(((radio.gyro[1]>900 /*in us?*/)?radio.gyro[1]/1000:radio.gyro[1]));
            /* PITCH SERVO */
            if (params->contains(OpalRT::SERVO_INPUTS, "PIT_SET0_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "PIT_SET0_IN").setValue(((radio.pitch[0]>900 /*in us?*/)?radio.pitch[0]/1000:radio.pitch[0]));
            if (params->contains(OpalRT::SERVO_INPUTS, "PIT_SET1_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "PIT_SET1_IN").setValue(((radio.pitch[1]>900 /*in us?*/)?radio.pitch[1]/1000:radio.pitch[1]));
            if (params->contains(OpalRT::SERVO_INPUTS, "PIT_SET2_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "PIT_SET2_IN").setValue(((radio.pitch[2]>900 /*in us?*/)?radio.pitch[2]/1000:radio.pitch[2]));
            if (params->contains(OpalRT::SERVO_INPUTS, "PIT_SET3_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "PIT_SET3_IN").setValue(((radio.pitch[3]>900 /*in us?*/)?radio.pitch[3]/1000:radio.pitch[3]));
            if (params->contains(OpalRT::SERVO_INPUTS, "PIT_SET4_IN"))
                params->getParameter(OpalRT::SERVO_INPUTS, "PIT_SET4_IN").setValue(((radio.pitch[4]>900 /*in us?*/)?radio.pitch[4]/1000:radio.pitch[4]));
        }
        break;
#endif
#ifdef MAVLINK_ENABLED_PIXHAWK
        case MAVLINK_MSG_ID_REQUEST_DATA_STREAM: {
            mavlink_request_data_stream_t stream;
            mavlink_msg_request_data_stream_decode(&msg, &stream);
            switch (stream.req_stream_id) {
            case 0: // All data types
                break;
            case 1: // Raw Sensor Data
                break;
            case 2: // extended system status
                break;
            case 3: // rc channel data
                sendRCValues = (stream.start_stop == 1?true:false);
                break;
            case 4: // raw controller
                if (stream.start_stop == 1)
                    sendRawController = true;
                else
                    sendRawController = false;
                break;
            case 5: // raw sensor fusion
                break;
            case 6: // position
                sendPosition = (stream.start_stop == 1?true:false);
                break;
            case 7: // extra 1
                break;
            case 8: // extra 2
                break;
            case 9: // extra 3
                break;
            default:
                qDebug() << __FILE__ << __LINE__ << "Received Unknown Data Strem Request with ID" << stream.req_stream_id;
            }
        }
        break;
        default: {
            qDebug() << "OpalLink::writeBytes(): Unknown mavlink packet";
        }
        }
#endif
    }
}


void OpalLink::readBytes()
{
    receiveDataMutex.lock();
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
    if (isConnected()) {
        receiveDataMutex.lock();
        receiveBuffer->enqueue(QByteArray(buffer, len));
        receiveDataMutex.unlock();
        readBytes();
    }

}

void OpalLink::heartbeat()
{

    if (m_heartbeatsEnabled) {
        mavlink_message_t beat;
        mavlink_msg_heartbeat_pack(systemID, componentID,&beat, MAV_HELICOPTER, MAV_AUTOPILOT_GENERIC);
        receiveMessage(beat);
    }

}
void OpalLink::setSignals(double *values)
{
    unsigned short numSignals = 2;
    unsigned short logicalId = 1;
    unsigned short signalIndex[] = {0,1};

    int returnValue;
    returnValue =  OpalSetSignals( numSignals, logicalId, signalIndex, values);
    if (returnValue != EOK) {
        OpalRT::OpalErrorMsg::displayLastErrorMsg();
    }
}
void OpalLink::getSignals()
{
    unsigned long  timeout = 0;
    unsigned short acqGroup = 0; //this is actually group 1 in the model
    unsigned short *numSignals = new unsigned short(0);
    double *timestep = new double(0);
    double values[OpalRT::NUM_OUTPUT_SIGNALS] = {};
    unsigned short *lastValues = new unsigned short(false);
    unsigned short *decimation = new unsigned short(0);

    while (!(*lastValues)) {
        int returnVal = OpalGetSignals(timeout, acqGroup, OpalRT::NUM_OUTPUT_SIGNALS, numSignals, timestep,
                                       values, lastValues, decimation);

        if (returnVal == EOK ) {
            /* Send position info to qgroundcontrol */
            if (sendPosition) {
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
            }
            /* send attitude info to qgroundcontrol */
            mavlink_message_t attitude;
            mavlink_msg_attitude_pack(systemID, componentID, &attitude,
                                      (*timestep)*1000000,
                                      values[OpalRT::ROLL],
                                      values[OpalRT::PITCH],
                                      values[OpalRT::YAW],
                                      values[OpalRT::ROLL_SPEED],
                                      values[OpalRT::PITCH_SPEED],
                                      values[OpalRT::YAW_SPEED]
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

            /* send radio outputs */
            if (sendRCValues) {
                mavlink_message_t rc;
                mavlink_msg_rc_channels_raw_pack(systemID, componentID, &rc,
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_1]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_2]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_3]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_4]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_5]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_6]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_7]),
                                                 duty2PulseMicros(values[OpalRT::RAW_CHANNEL_8]),
                                                 0 //rssi unused
                                                );
                receiveMessage(rc);
            }
            if (sendRawController) {
                mavlink_message_t rawController;
                mavlink_msg_attitude_controller_output_pack(systemID, componentID, &rawController,
                        1,
                        rescaleControllerOutput(values[OpalRT::CONTROLLER_AILERON]),
                        rescaleControllerOutput(values[OpalRT::CONTROLLER_ELEVATOR]),
                        0, // yaw not used
                        0 // thrust not used
                                                           );
                receiveMessage(rawController);
            }
        } else if (returnVal != EAGAIN) { // if returnVal == EAGAIN => data just wasn't ready
            getSignalsTimer->stop();
            OpalRT::OpalErrorMsg::displayLastErrorMsg();
        }
    }

    /* deallocate used memory */

    delete numSignals;
    delete timestep;
    delete lastValues;
    delete decimation;

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

bool OpalLink::isConnected()
{
    return connectState;
}

uint16_t OpalLink::duty2PulseMicros(double duty)
{
    /* duty cycle assumed to be of a signal at 70 Hz */
    return static_cast<uint16_t>(duty/70*1000000);
}

uint8_t OpalLink::rescaleNorm(double norm, int ch)
{
    switch(ch) {
    case OpalRT::NORM_CHANNEL_1:
    case OpalRT::NORM_CHANNEL_2:
    case OpalRT::NORM_CHANNEL_4:
    default:
        // three setpoints
        return static_cast<uint8_t>((norm+1)/2*255);
        break;
    case OpalRT::NORM_CHANNEL_5:
        //two setpoints
    case OpalRT::NORM_CHANNEL_3:
    case OpalRT::NORM_CHANNEL_6:
        return static_cast<uint8_t>(norm*255);
        break;
    }
}

int8_t OpalLink::rescaleControllerOutput(double raw)
{
    return static_cast<int8_t>((raw>=0?raw*127:raw*128));
}

bool OpalLink::connect()
{
    short modelState;

    if ((OpalConnect(opalInstID, false, &modelState) == EOK)
            && (OpalGetSignalControl(0, true) == EOK)
            && (OpalGetParameterControl(true) == EOK)) {
        connectState = true;
        if (params)
            delete params;
        params = new OpalRT::ParameterList();
        emit connected();
        heartbeatTimer->start(1000/heartbeatRate);
        getSignalsTimer->start(getSignalsPeriod);
    } else {
        connectState = false;
        OpalRT::OpalErrorMsg::displayLastErrorMsg();
    }

    emit connected(connectState);
    return connectState;
}

bool OpalLink::disconnect()
{
    // OpalDisconnect returns void so its success or failure cannot be tested
    OpalDisconnect();
    heartbeatTimer->stop();
    getSignalsTimer->stop();
    connectState = false;
    emit connected(connectState);
    return true;
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

qint64 OpalLink::getTotalDownstream()
{
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

qint64 OpalLink::getBitsSent()
{
    return bitsSentTotal;
}

qint64 OpalLink::getBitsReceived()
{
    return bitsReceivedTotal;
}


bool OpalLink::isFullDuplex()
{
    return false;
}
