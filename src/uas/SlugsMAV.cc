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

#ifdef MAVLINK_ENABLED_SLUGS_MESSAGES

    case MAVLINK_MSG_ID_CPU_LOAD:
        {
            mavlink_cpu_load_t cpu_load;
            quint64 time = getUnixTime(0);
            mavlink_msg_cpu_load_decode(&message,&cpu_load);

                emit valueChanged(uasId, "CPU Load", cpu_load.target, time);
                emit valueChanged(uasId, "SensorDSC Load", cpu_load.sensLoad, time);
                emit valueChanged(uasId, "ControlDSC Load", cpu_load.ctrlLoad, time);
                emit valueChanged(uasId, "Battery Volt", cpu_load.batVolt, time);

            break;
        }
    case MAVLINK_MSG_ID_AIR_DATA:
        {
            mavlink_air_data_t air_data;
            quint64 time = getUnixTime(0);
            mavlink_msg_air_data_decode(&message,&air_data);
                emit valueChanged(uasId, "Air Data",air_data.target,time);
                emit valueChanged(uasId, "Presion Dinamica", air_data.dynamicPressure,time);
                emit valueChanged(uasId, "Presion Estatica",air_data.staticPressure, time);
                emit valueChanged(uasId, "Temperatura",air_data.temperature,time);

            break;
        }
    case MAVLINK_MSG_ID_SENSOR_BIAS:
        {
            mavlink_sensor_bias_t sensor_bias;
            quint64 time = getUnixTime(0);
            mavlink_msg_sensor_bias_decode(&message,&sensor_bias);
                emit valueChanged(uasId,"Sensor Bias",sensor_bias.target, time);
                emit valueChanged(uasId,"Acelerometro X",sensor_bias.axBias, time);
                emit valueChanged(uasId,"Acelerometro y",sensor_bias.ayBias,time);
                emit valueChanged(uasId,"Acelerometro Z",sensor_bias.azBias,time);
                emit valueChanged(uasId,"Gyro X",sensor_bias.gxBias,time);
                emit valueChanged(uasId,"Gyro Y",sensor_bias.gyBias,time);
                emit valueChanged(uasId,"Gyro Z",sensor_bias.gzBias,time);

            break;
        }
    case MAVLINK_MSG_ID_DIAGNOSTIC:
        {
            mavlink_diagnostic_t diagnostic;
            quint64 time = getUnixTime(0);
            mavlink_msg_diagnostic_decode(&message,&diagnostic);
                emit valueChanged(uasId,"Diagnostico",diagnostic.target,time);
                emit valueChanged(uasId,"Diagnostico F1",diagnostic.diagFl1,time);
                emit valueChanged(uasId,"Diagnostico F2",diagnostic.diagFl2,time);
                emit valueChanged(uasId,"Diagnostico F3",diagnostic.diagFl3,time);
                emit valueChanged(uasId,"Diagnostico S1",diagnostic.diagSh1,time);
                emit valueChanged(uasId,"Diagnostico S2",diagnostic.diagSh2,time);
                emit valueChanged(uasId,"Diagnostico S3",diagnostic.diagSh3,time);

            break;
        }
    case MAVLINK_MSG_ID_PILOT_CONSOLE:
        {
            mavlink_pilot_console_t pilot;
            quint64 time = getUnixTime(0);
            mavlink_msg_pilot_console_decode(&message,&pilot);
                emit valueChanged(uasId,"Mensajes PWM",pilot.target,time);
                emit valueChanged(uasId,"Aceleracion Consola",pilot.dt,time);
                emit valueChanged(uasId,"Aleron Izq Consola",pilot.dla,time);
                emit valueChanged(uasId,"Aleron Der Consola",pilot.dra,time);
                emit valueChanged(uasId,"Timon Consola",pilot.dr,time);
                emit valueChanged(uasId,"Elevador Consola",pilot.de,time);

            break;
        }
    case MAVLINK_MSG_ID_PWM_COMMANDS:
        {
            mavlink_pwm_commands_t pwm;
            quint64 time = getUnixTime(0);
            mavlink_msg_pwm_commands_decode(&message,&pwm);
                emit valueChanged(uasId,"Superficies de Control",pwm.target,time);
                emit valueChanged(uasId,"Comando Aceleracion PA",pwm.dt_c,time);
                emit valueChanged(uasId,"Comando Aleron Izq PA",pwm.dla_c,time);
                emit valueChanged(uasId,"Comando Aleron Der PA",pwm.dra_c,time);
                emit valueChanged(uasId,"Comando Timon PA",pwm.dr_c,time);
                emit valueChanged(uasId,"Comando elevador Izq PA",pwm.dle_c,time);
                emit valueChanged(uasId,"Comando Elevador Der PA",pwm.dre_c,time);
                emit valueChanged(uasId,"Comando Flap Izq PA",pwm.dlf_c,time);
                emit valueChanged(uasId,"Comando Flap Der PA",pwm.drf_c,time);
                emit valueChanged(uasId,"Comando Aux1 PA",pwm.aux1,time);
                emit valueChanged(uasId,"Comando Aux2 PA",pwm.aux2,time);


            break;
        }
#endif

    default:
        qDebug() << "\nSLUGS RECEIVED MESSAGE WITH ID" << message.msgid;
        break;
    }
}
