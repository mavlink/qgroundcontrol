#include "SlugsMAV.h"

#include <QDebug>

SlugsMAV::SlugsMAV(MAVLinkProtocol* mavlink, int id) :
        UAS(mavlink, id)//,
        // Place other initializers here
{
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
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);

    // Handle your special messages mavlink_message_t* msg = &message;
    switch (message.msgid)
    {
    case MAVLINK_MSG_ID_HEARTBEAT:
        {
            qDebug() << "SLUGS RECEIVED HEARTBEAT";
            break;
        }

#ifdef MAVLINK_ENABLED_SLUGS

    case MAVLINK_MSG_ID_CPU_LOAD://170
        {
            mavlink_cpu_load_t cpu_load;
            quint64 time = getUnixTime(0);
            mavlink_msg_cpu_load_decode(&message,&cpu_load);

                emit valueChanged(uasId, tr("SensorDSC Load"), cpu_load.sensLoad, time);
                emit valueChanged(uasId, tr("ControlDSC Load"), cpu_load.ctrlLoad, time);
                emit valueChanged(uasId, tr("Battery Volt"), cpu_load.batVolt, time);

                emit slugsCPULoad(uasId,
                                  cpu_load.sensLoad,
                                  cpu_load.ctrlLoad,
                                  cpu_load.batVolt,
                                  0);
//qDebug()<<"--------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_CPU_LOAD 170";
            break;
        }
    case MAVLINK_MSG_ID_AIR_DATA://171
        {
            mavlink_air_data_t air_data;
            quint64 time = getUnixTime(0);
            mavlink_msg_air_data_decode(&message,&air_data);
                emit valueChanged(uasId, tr("Dynamic Pressure"), air_data.dynamicPressure,time);
                emit valueChanged(uasId, tr("Static Pressure"),air_data.staticPressure, time);
                emit valueChanged(uasId, tr("Temp"),air_data.temperature,time);

                emit slugsAirData(uasId,
                                  air_data.dynamicPressure,
                                  air_data.staticPressure,
                                  air_data.temperature,
                                  time);
//qDebug()<<"--------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_AIR_DATA 171";
            break;
        }
    case MAVLINK_MSG_ID_SENSOR_BIAS://172
        {
            mavlink_sensor_bias_t sensor_bias;
            quint64 time = getUnixTime(0);
            mavlink_msg_sensor_bias_decode(&message,&sensor_bias);
                emit valueChanged(uasId,tr("Ax Bias"),sensor_bias.axBias, time);
                emit valueChanged(uasId,tr("Ay Bias"),sensor_bias.ayBias,time);
                emit valueChanged(uasId,tr("Az Bias"),sensor_bias.azBias,time);
                emit valueChanged(uasId,tr("Gx Bias"),sensor_bias.gxBias,time);
                emit valueChanged(uasId,tr("Gy Bias"),sensor_bias.gyBias,time);
                emit valueChanged(uasId,tr("Gz Bias"),sensor_bias.gzBias,time);

                emit slugsSensorBias(uasId,
                                     sensor_bias.axBias,
                                     sensor_bias.ayBias,
                                     sensor_bias.azBias,
                                     sensor_bias.gxBias,
                                     sensor_bias.gyBias,
                                     sensor_bias.gzBias,
                                     0);
          //     qDebug()<<"------------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_SENSOR_BIAS 172";


            break;
        }
    case MAVLINK_MSG_ID_DIAGNOSTIC://173
        {
            mavlink_diagnostic_t diagnostic;
            quint64 time = getUnixTime(0);
            mavlink_msg_diagnostic_decode(&message,&diagnostic);
                emit valueChanged(uasId,tr("Diag F1"),diagnostic.diagFl1,time);
                emit valueChanged(uasId,tr("Diag F2"),diagnostic.diagFl2,time);
                emit valueChanged(uasId,tr("Diag F3"),diagnostic.diagFl3,time);
                emit valueChanged(uasId,tr("Diag S1"),diagnostic.diagSh1,time);
                emit valueChanged(uasId,tr("Diag S2"),diagnostic.diagSh2,time);
                emit valueChanged(uasId,tr("Diag S3"),diagnostic.diagSh3,time);

                emit slugsDiagnostic(uasId,
                                     diagnostic.diagFl1,
                                     diagnostic.diagFl2,
                                     diagnostic.diagFl3,
                                     diagnostic.diagSh1,
                                     diagnostic.diagSh2,
                                     diagnostic.diagSh3,
                                     0);
//qDebug()<<"------->>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_DIAGNOSTIC 173";
            break;
        }
    case MAVLINK_MSG_ID_PILOT_CONSOLE://174
        {
            mavlink_pilot_console_t pilot;
            quint64 time = getUnixTime(0);
            mavlink_msg_pilot_console_decode(&message,&pilot);
                emit valueChanged(uasId,"dt",pilot.dt,time);
                emit valueChanged(uasId,"dla",pilot.dla,time);
                emit valueChanged(uasId,"dra",pilot.dra,time);
                emit valueChanged(uasId,"dr",pilot.dr,time);
                emit valueChanged(uasId,"de",pilot.de,time);

                emit slugsPilotConsolePWM(uasId,
                                          pilot.dt,
                                          pilot.dla,
                                          pilot.dra,
                                          pilot.dr,
                                          pilot.de,
                                          0);
             //   qDebug()<<"---------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_PILOT_CONSOLE 174";


            break;
        }
    case MAVLINK_MSG_ID_PWM_COMMANDS://175
        {
            mavlink_pwm_commands_t pwm;
            quint64 time = getUnixTime(0);
            mavlink_msg_pwm_commands_decode(&message,&pwm);
                emit valueChanged(uasId,"dt_c",pwm.dt_c,time);
                emit valueChanged(uasId,"dla_c",pwm.dla_c,time);
                emit valueChanged(uasId,"dra_c",pwm.dra_c,time);
                emit valueChanged(uasId,"dr_c",pwm.dr_c,time);
                emit valueChanged(uasId,"dle_c",pwm.dle_c,time);
                emit valueChanged(uasId,"dre_c",pwm.dre_c,time);
                emit valueChanged(uasId,"dlf_c",pwm.dlf_c,time);
                emit valueChanged(uasId,"drf_c",pwm.drf_c,time);
                emit valueChanged(uasId,"da1_c",pwm.aux1,time);
                emit valueChanged(uasId,"da2_c",pwm.aux2,time);

                emit slugsPWM(uasId,
                              pwm.dt_c,
                              pwm.dla_c,
                              pwm.dra_c,
                              pwm.dr_c,
                              pwm.dle_c,
                              pwm.dre_c,
                              pwm.dlf_c,
                              pwm.drf_c,
                              pwm.aux1,
                              pwm.aux2,
                              0);
//qDebug()<<"----------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_PWM_COMMANDS 175";

            break;
        }
    case MAVLINK_MSG_ID_SLUGS_NAVIGATION://176
        {
            mavlink_slugs_navigation_t nav;
            quint64 time = getUnixTime(0);
            mavlink_msg_slugs_navigation_decode(&message,&nav);
                emit valueChanged(uasId,"u_m",nav.u_m,time);
                emit valueChanged(uasId,"phi_c",nav.phi_c,time);
                emit valueChanged(uasId,"theta_c",nav.theta_c,time);
                emit valueChanged(uasId,"psiDot_c",nav.psiDot_c,time);
                emit valueChanged(uasId,"ay_body",nav.ay_body,time);
                emit valueChanged(uasId,"totalDist",nav.totalDist,time);
                emit valueChanged(uasId,"dist2Go",nav.dist2Go,time);
                emit valueChanged(uasId,"fromWP",nav.fromWP,time);
                emit valueChanged(uasId,"toWP",nav.toWP,time);

                emit slugsNavegation(uasId,
                                     nav.u_m,
                                     nav.phi_c,
                                     nav.theta_c,
                                     nav.psiDot_c,
                                     nav.ay_body,
                                     nav.totalDist,
                                     nav.dist2Go,
                                     nav.fromWP,
                                     nav.toWP,
                                     time);

           // qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_SLUGS_NAVIGATION 176";

            break;
        }
    case MAVLINK_MSG_ID_DATA_LOG://177
        {
            mavlink_data_log_t dataLog;
            quint64 time = getUnixTime(0);
            mavlink_msg_data_log_decode(&message,&dataLog);
                emit valueChanged(uasId,"fl_1",dataLog.fl_1,time);
                emit valueChanged(uasId,"fl_2",dataLog.fl_2,time);
                emit valueChanged(uasId,"fl_3",dataLog.fl_3,time);
                emit valueChanged(uasId,"fl_4",dataLog.fl_4,time);
                emit valueChanged(uasId,"fl_5",dataLog.fl_5,time);
                emit valueChanged(uasId,"fl_6",dataLog.fl_6,time);

                emit slugsDataLog(uasId,
                                  dataLog.fl_1,
                                  dataLog.fl_2,
                                  dataLog.fl_3,
                                  dataLog.fl_4,
                                  dataLog.fl_5,
                                  dataLog.fl_6,
                                  time);

             //   qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO LOG DATA 177";



            break;
        }

    case MAVLINK_MSG_ID_FILTERED_DATA://178
        {

            mavlink_filtered_data_t fil;
            quint64 time = getUnixTime(0);
            mavlink_msg_filtered_data_decode(&message,&fil);
                emit valueChanged(uasId,"aX",fil.aX,time);
                emit valueChanged(uasId,"aY",fil.aY,time);
                emit valueChanged(uasId,"aZ",fil.aZ,time);
                emit valueChanged(uasId,"gX",fil.gX,time);
                emit valueChanged(uasId,"gY",fil.gY,time);
                emit valueChanged(uasId,"gZ",fil.gZ,time);
                emit valueChanged(uasId,"mX",fil.mX,time);
                emit valueChanged(uasId,"mY",fil.mY,time);
                emit valueChanged(uasId,"mZ",fil.mZ,time);

                emit slugsFilteredData(uasId,
                                       fil.aX,
                                       fil.aY,
                                       fil.aZ,
                                       fil.gX,
                                       fil.gY,
                                       fil.gZ,
                                       fil.mX,
                                       fil.mY,
                                       fil.mZ,
                                       time);



              //  qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO filtering data 178";



            break;
        }
    case MAVLINK_MSG_ID_GPS_DATE_TIME://179
        {
            mavlink_gps_date_time_t gps;
            quint64 time = getUnixTime(0);
            mavlink_msg_gps_date_time_decode(&message,&gps);
                emit valueChanged(uasId,"year",gps.year,time);
                emit valueChanged(uasId,"month",gps.month,time);
                emit valueChanged(uasId,"day",gps.day,time);
                emit valueChanged(uasId,"hour",gps.hour,time);
                emit valueChanged(uasId,"min",gps.min,time);
                emit valueChanged(uasId,"sec",gps.sec,time);
                emit valueChanged(uasId,"visSat",gps.visSat,time);


                emit slugsGPSDateTime(uasId,
                                       gps.year,
                                       gps.month,
                                       gps.day,
                                       gps.hour,
                                       gps.min,
                                       gps.sec,
                                       gps.visSat,
                                       time);



              //  qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_GPS_DATE_TIME 179";



            break;
        }
    case MAVLINK_MSG_ID_ACTION_ACK:
        {
            qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_ACTION_ACK";
            mavlink_action_ack_t ack;

            mavlink_msg_action_ack_decode(&message,&ack);
            emit slugsActionAck(uasId,ack.action,ack.result);
        }
//    case MAVLINK_MSG_ID_MID_LVL_CMDS://180
//        {


//             //   qDebug()<<"------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_MID_LVL_CMDS 180";



//            break;
//        }
//    case MAVLINK_MSG_ID_CTRL_SRFC_PT://181
//        {


//               // qDebug()<<"--------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_CTRL_SRFC_PT 181";



//            break;
//        }
//    case MAVLINK_MSG_ID_PID://182
//        {


//               // qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_PID 182";



//            break;
//        }
//    case MAVLINK_MSG_ID_SLUGS_ACTION://183
//        {


//               // qDebug()<<"-------->>>>>>>>>>>>>>> EMITIENDO MAVLINK_MSG_ID_SLUGS_ACTION 183";



//            break;
//        }
#endif

    default:
        qDebug() << "\nSLUGS RECEIVED MESSAGE WITH ID" << message.msgid;
        break;
    }
}
