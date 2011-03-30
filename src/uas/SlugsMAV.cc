#include "SlugsMAV.h"

#include <QDebug>

SlugsMAV::SlugsMAV(MAVLinkProtocol* mavlink, int id) :
    UAS(mavlink, id)
{
    widgetTimer = new QTimer (this);
    widgetTimer->setInterval(SLUGS_UPDATE_RATE);

    connect (widgetTimer, SIGNAL(timeout()), this, SLOT(emitSignals()));
    widgetTimer->start();
    memset(&mlRawImuData ,0, sizeof(mavlink_raw_imu_t));// clear all the state structures

#ifdef MAVLINK_ENABLED_SLUGS


    memset(&mlGpsData, 0, sizeof(mavlink_gps_raw_t));
    memset(&mlCpuLoadData, 0, sizeof(mavlink_cpu_load_t));
    memset(&mlAirData, 0, sizeof(mavlink_air_data_t));
    memset(&mlSensorBiasData, 0, sizeof(mavlink_sensor_bias_t));
    memset(&mlDiagnosticData, 0, sizeof(mavlink_diagnostic_t));
    memset(&mlBoot ,0, sizeof(mavlink_boot_t));
    memset(&mlGpsDateTime ,0, sizeof(mavlink_gps_date_time_t));
    memset(&mlApMode ,0, sizeof(mavlink_set_mode_t));
    memset(&mlNavigation ,0, sizeof(mavlink_slugs_navigation_t));
    memset(&mlDataLog ,0, sizeof(mavlink_data_log_t));
    memset(&mlPassthrough ,0, sizeof(mavlink_ctrl_srfc_pt_t));
    memset(&mlActionAck,0, sizeof(mavlink_action_ack_t));
    memset(&mlAction ,0, sizeof(mavlink_slugs_action_t));

    memset(&mlScaled ,0, sizeof(mavlink_scaled_imu_t));
    memset(&mlServo ,0, sizeof(mavlink_servo_output_raw_t));
    memset(&mlChannels ,0, sizeof(mavlink_rc_channels_raw_t));

    updateRoundRobin = 0;
    uasId = id;
#endif
}

/**
 * This function is called by MAVLink once a complete, uncorrupted (CRC check valid)
 * mavlink packet is received.
 *
 * @param link Hardware link the message came from (e.g. /dev/ttyUSB0 or UDP port).
 *             messages can be sent back to the system via this link
 * @param message MAVLink message, as received from the MAVLink protocol stack
 */
void SlugsMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    UAS::receiveMessage(link, message);// Let UAS handle the default message set

    if (message.sysid == uasId) {
#ifdef MAVLINK_ENABLED_SLUGS// Handle your special messages mavlink_message_t* msg = &message;

        switch (message.msgid) {
        case MAVLINK_MSG_ID_RAW_IMU:
            mavlink_msg_raw_imu_decode(&message, &mlRawImuData);
            break;

        case MAVLINK_MSG_ID_BOOT:
            mavlink_msg_boot_decode(&message,&mlBoot);
            emit slugsBootMsg(uasId, mlBoot);
            break;

        case MAVLINK_MSG_ID_ATTITUDE:
            mavlink_msg_attitude_decode(&message, &mlAttitude);
            break;

        case MAVLINK_MSG_ID_GPS_RAW:
            mavlink_msg_gps_raw_decode(&message, &mlGpsData);
            break;

        case MAVLINK_MSG_ID_CPU_LOAD:       //170
            mavlink_msg_cpu_load_decode(&message,&mlCpuLoadData);
            break;

        case MAVLINK_MSG_ID_AIR_DATA:       //171
            mavlink_msg_air_data_decode(&message,&mlAirData);
            break;

        case MAVLINK_MSG_ID_SENSOR_BIAS:    //172
            mavlink_msg_sensor_bias_decode(&message,&mlSensorBiasData);
            break;

        case MAVLINK_MSG_ID_DIAGNOSTIC:     //173
            mavlink_msg_diagnostic_decode(&message,&mlDiagnosticData);
            break;

        case MAVLINK_MSG_ID_SLUGS_NAVIGATION://176
            mavlink_msg_slugs_navigation_decode(&message,&mlNavigation);
            break;

        case MAVLINK_MSG_ID_DATA_LOG:       //177
            mavlink_msg_data_log_decode(&message,&mlDataLog);
            break;

        case MAVLINK_MSG_ID_GPS_DATE_TIME:    //179
            mavlink_msg_gps_date_time_decode(&message,&mlGpsDateTime);
            break;

        case MAVLINK_MSG_ID_MID_LVL_CMDS:     //180
            mavlink_msg_mid_lvl_cmds_decode(&message, &mlMidLevelCommands);
            break;

        case MAVLINK_MSG_ID_CTRL_SRFC_PT:     //181
            mavlink_msg_ctrl_srfc_pt_decode(&message, &mlPassthrough);
            break;

        case MAVLINK_MSG_ID_SLUGS_ACTION:     //183
            mavlink_msg_slugs_action_decode(&message, &mlAction);
            break;

        case MAVLINK_MSG_ID_SCALED_IMU:
            mavlink_msg_scaled_imu_decode(&message, &mlScaled);
            break;

        case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
            mavlink_msg_servo_output_raw_decode(&message, &mlServo);
            break;

        case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
            mavlink_msg_rc_channels_raw_decode(&message, &mlChannels);
            break;

            switch (mlAction.actionId) {
            case SLUGS_ACTION_EEPROM:
                if (mlAction.actionVal == SLUGS_ACTION_FAIL) {
                    emit textMessageReceived(message.sysid, message.compid, 255, "EEPROM Write Fail, Data was not saved in Memory!");
                }
                break;

            case SLUGS_ACTION_PT_CHANGE:
                if (mlAction.actionVal == SLUGS_ACTION_SUCCESS) {
                    emit textMessageReceived(message.sysid, message.compid, 0, "Passthrough Succesfully Changed");
                }
                break;

            case SLUGS_ACTION_MLC_CHANGE:
                if (mlAction.actionVal == SLUGS_ACTION_SUCCESS) {
                    emit textMessageReceived(message.sysid, message.compid, 0, "Mid-level Commands Succesfully Changed");
                }
                break;
            }

            //break;

        default:
            //        qDebug() << "\nSLUGS RECEIVED MESSAGE WITH ID" << message.msgid;
            break;
        }
#endif
    }
}



void SlugsMAV::emitSignals (void)
{
#ifdef MAVLINK_ENABLED_SLUGS
    switch(updateRoundRobin) {
    case 1:
        emit slugsCPULoad(uasId, mlCpuLoadData);
        emit slugsSensorBias(uasId,mlSensorBiasData);
        break;

    case 2:
        emit slugsAirData(uasId, mlAirData);
        emit slugsDiagnostic(uasId,mlDiagnosticData);

        break;

    case 3:
        emit slugsNavegation(uasId, mlNavigation);
        emit slugsDataLog(uasId, mlDataLog);
        break;

    case 4:
        emit slugsGPSDateTime(uasId, mlGpsDateTime);

        break;

    case 5:
        emit slugsActionAck(uasId,mlActionAck);
        emit emitGpsSignals();
        break;

    case 6:
        emit slugsChannels(uasId, mlChannels);
        emit slugsServo(uasId, mlServo);
        emit slugsScaled(uasId, mlScaled);

        break;
    }

    emit slugsAttitude(uasId, mlAttitude);
    emit attitudeChanged(this,
                         mlAttitude.roll,
                         mlAttitude.pitch,
                         mlAttitude.yaw,
                         0.0);
#endif

    emit slugsRawImu(uasId, mlRawImuData);


    // wrap around
    updateRoundRobin = updateRoundRobin > 10? 1: updateRoundRobin + 1;


}

#ifdef MAVLINK_ENABLED_SLUGS
void SlugsMAV::emitGpsSignals (void)
{

    // qDebug()<<"After Emit GPS Signal"<<mlGpsData.fix_type;


    //ToDo Uncomment if. it was comment only to test

// if (mlGpsData.fix_type > 0){
    emit globalPositionChanged(this,
                               mlGpsData.lon,
                               mlGpsData.lat,
                               mlGpsData.alt,
                               0.0);

    emit slugsGPSCogSog(uasId,mlGpsData.hdg, mlGpsData.v);

}



#endif // MAVLINK_ENABLED_SLUGS
