#include "senseSoarMAV.h"
#include <qmath.h>

senseSoarMAV::senseSoarMAV(MAVLinkProtocol* mavlink, int id)
	: UAS(mavlink, id), senseSoarState(0)
{
}


senseSoarMAV::~senseSoarMAV(void)
{
}

void senseSoarMAV::receiveMessage(LinkInterface *link, mavlink_message_t message)
{
#ifdef MAVLINK_ENABLED_SENSESOAR
	if (message.sysid == uasId)  // make sure the message is for the right UAV
	{ 
		if (!link) return;
		switch (message.msgid)
		{
		case MAVLINK_MSG_ID_CMD_AIRSPEED_ACK: // TO DO: check for acknowledgement after sended commands
			{
				mavlink_cmd_airspeed_ack_t airSpeedMsg;
				mavlink_msg_cmd_airspeed_ack_decode(&message,&airSpeedMsg);
				break;
			}
		/*case MAVLINK_MSG_ID_CMD_AIRSPEED_CHNG: only sent to UAV
			{
				break;
			}*/
		case MAVLINK_MSG_ID_FILT_ROT_VEL: // rotational velocities
			{
				mavlink_filt_rot_vel_t rotVelMsg;
				mavlink_msg_filt_rot_vel_decode(&message,&rotVelMsg);
				quint64 time = getUnixTime();
				for(unsigned char i=0;i<3;i++)
				{
					this->m_rotVel[i]=rotVelMsg.rotVel[i];
				}
				emit valueChanged(uasId, "rollspeed", "rad/s", this->m_rotVel[0], time);
                emit valueChanged(uasId, "pitchspeed", "rad/s", this->m_rotVel[1], time);
                emit valueChanged(uasId, "yawspeed", "rad/s", this->m_rotVel[2], time);
				emit attitudeSpeedChanged(uasId, this->m_rotVel[0], this->m_rotVel[1], this->m_rotVel[2], time);
				break;
			}
		case MAVLINK_MSG_ID_LLC_OUT: // low level controller output
			{
				mavlink_llc_out_t llcMsg;
				mavlink_msg_llc_out_decode(&message,&llcMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "Servo. 1", "rad", llcMsg.servoOut[0], time);
                emit valueChanged(uasId, "Servo. 2", "rad", llcMsg.servoOut[1], time);
				emit valueChanged(uasId, "Servo. 3", "rad", llcMsg.servoOut[2], time);
				emit valueChanged(uasId, "Servo. 4", "rad", llcMsg.servoOut[3], time);
				emit valueChanged(uasId, "Motor. 1", "raw", llcMsg.MotorOut[0]  , time);
				emit valueChanged(uasId, "Motor. 2", "raw", llcMsg.MotorOut[1], time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_AIR_TEMP:
			{
				mavlink_obs_air_temp_t airTMsg;
				mavlink_msg_obs_air_temp_decode(&message,&airTMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "Air Temp", "°", airTMsg.airT, time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_AIR_VELOCITY:
			{
				mavlink_obs_air_velocity_t airVMsg;
				mavlink_msg_obs_air_velocity_decode(&message,&airVMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "AirVel. mag", "m/s", airVMsg.magnitude, time);
				emit valueChanged(uasId, "AirVel. AoA", "rad", airVMsg.aoa, time);
				emit valueChanged(uasId, "AirVel. Slip", "rad", airVMsg.slip, time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_ATTITUDE:
			{
				mavlink_obs_attitude_t quatMsg;
				mavlink_msg_obs_attitude_decode(&message,&quatMsg);
				quint64 time = getUnixTime();
				this->quat2euler(quatMsg.quat,this->roll,this->pitch,this->yaw);
				emit valueChanged(uasId, "roll", "rad", roll, time);
                emit valueChanged(uasId, "pitch", "rad", pitch, time);
                emit valueChanged(uasId, "yaw", "rad", yaw, time);
				emit valueChanged(uasId, "roll deg", "deg", (roll/M_PI)*180.0, time);
                emit valueChanged(uasId, "pitch deg", "deg", (pitch/M_PI)*180.0, time);
                emit valueChanged(uasId, "heading deg", "deg", (yaw/M_PI)*180.0, time);
				emit attitudeChanged(this, roll, pitch, yaw, time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_BIAS:
			{
				mavlink_obs_bias_t biasMsg;
				mavlink_msg_obs_bias_decode(&message, &biasMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "acc. biasX", "m/s^2", biasMsg.accBias[0], time);
				emit valueChanged(uasId, "acc. biasY", "m/s^2", biasMsg.accBias[1], time);
				emit valueChanged(uasId, "acc. biasZ", "m/s^2", biasMsg.accBias[2], time);
				emit valueChanged(uasId, "gyro. biasX", "rad/s", biasMsg.gyroBias[0], time);
				emit valueChanged(uasId, "gyro. biasY", "rad/s", biasMsg.gyroBias[1], time);
				emit valueChanged(uasId, "gyro. biasZ", "rad/s", biasMsg.gyroBias[2], time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_POSITION:
			{
				mavlink_obs_position_t posMsg;
				mavlink_msg_obs_position_decode(&message, &posMsg);
				quint64 time = getUnixTime();
				this->longitude = posMsg.lon/(double)1E7;
				this->latitude = posMsg.lat/(double)1E7;
				this->altitude = posMsg.alt/1000.0;
				emit valueChanged(uasId, "latitude", "deg", this->latitude, time);
                emit valueChanged(uasId, "longitude", "deg", this->longitude, time);
                emit valueChanged(uasId, "altitude", "m", this->altitude, time);
				emit globalPositionChanged(this, this->latitude, this->longitude, this->altitude, time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_QFF:
			{
				mavlink_obs_qff_t qffMsg;
				mavlink_msg_obs_qff_decode(&message,&qffMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "QFF", "Pa", qffMsg.qff, time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_VELOCITY:
			{
				mavlink_obs_velocity_t velMsg;
				mavlink_msg_obs_velocity_decode(&message, &velMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "x speed", "m/s", velMsg.vel[0], time);
                emit valueChanged(uasId, "y speed", "m/s", velMsg.vel[1], time);
                emit valueChanged(uasId, "z speed", "m/s", velMsg.vel[2], time);
				emit speedChanged(this, velMsg.vel[0], velMsg.vel[1], velMsg.vel[2], time);
				break;
			}
		case MAVLINK_MSG_ID_OBS_WIND:
			{
				mavlink_obs_wind_t windMsg;
				mavlink_msg_obs_wind_decode(&message, &windMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "Wind speed x", "m/s", windMsg.wind[0], time);
				emit valueChanged(uasId, "Wind speed y", "m/s", windMsg.wind[1], time);
				emit valueChanged(uasId, "Wind speed z", "m/s", windMsg.wind[2], time);
				break;
			}
		case MAVLINK_MSG_ID_PM_ELEC:
			{
				mavlink_pm_elec_t pmMsg;
				mavlink_msg_pm_elec_decode(&message, &pmMsg);
				quint64 time = getUnixTime();
				emit valueChanged(uasId, "Battery status", "%", pmMsg.BatStat, time);
				emit valueChanged(uasId, "Power consuming", "W", pmMsg.PwCons, time);
				emit valueChanged(uasId, "Power generating sys1", "W", pmMsg.PwGen[0], time);
				emit valueChanged(uasId, "Power generating sys2", "W", pmMsg.PwGen[1], time);
				emit valueChanged(uasId, "Power generating sys3", "W", pmMsg.PwGen[2], time);
				break;
			}
		case MAVLINK_MSG_ID_SYS_STAT:
			{
#define STATE_WAKING_UP            0x0  // TO DO: not important here, only for the visualisation needed
#define STATE_ON_GROUND            0x1
#define STATE_MANUAL_FLIGHT        0x2
#define STATE_AUTONOMOUS_FLIGHT    0x3
#define STATE_AUTONOMOUS_LAUNCH    0x4
				mavlink_sys_stat_t statMsg;
				mavlink_msg_sys_stat_decode(&message,&statMsg);
				quint64 time = getUnixTime();
				// check actuator states
				emit valueChanged(uasId, "Motor1 status", "on/off", (statMsg.act & 0x01), time);
				emit valueChanged(uasId, "Motor2 status", "on/off", (statMsg.act & 0x02)>>1, time);
				emit valueChanged(uasId, "Servo1 status", "on/off", (statMsg.act & 0x04)>>2, time);
				emit valueChanged(uasId, "Servo2 status", "on/off", (statMsg.act & 0x08)>>3, time);
				emit valueChanged(uasId, "Servo3 status", "on/off", (statMsg.act & 0x10)>>4, time);
				emit valueChanged(uasId, "Servo4 status", "on/off", (statMsg.act & 0x20)>>5, time);
				// check the current state of the sensesoar
				this->senseSoarState = statMsg.mod;
				emit valueChanged(uasId,"senseSoar status","-",this->senseSoarState,time);
				// check the gps fixes
				emit valueChanged(uasId,"Lat Long fix","true/false", (statMsg.gps & 0x01), time);
				emit valueChanged(uasId,"Altitude fix","true/false", (statMsg.gps & 0x02), time);
				emit valueChanged(uasId,"GPS horizontal accuracy","m",((statMsg.gps & 0x1C)>>2), time);
				emit valueChanged(uasId,"GPS vertiacl accuracy","m",((statMsg.gps & 0xE0)>>5),time);
				// Xbee RSSI
				emit valueChanged(uasId, "Xbee strength", "%", statMsg.commRssi, time);
				//emit valueChanged(uasId, "Xbee strength", "%", statMsg.gps, time);  // TO DO: define gps bits

				break;
			}
		default:
			{
				// Let UAS handle the default message set
				UAS::receiveMessage(link, message);
				break;
			}
		}
	}
#else
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);
    Q_UNUSED(link);
    Q_UNUSED(message);
#endif // MAVLINK_ENABLED_SENSESOAR
}

void senseSoarMAV::quat2euler(const double *quat, double &roll, double &pitch, double &yaw)
{ 
	roll = std::atan2(2*(quat[0]*quat[1] + quat[2]*quat[3]),quat[0]*quat[0] - quat[1]*quat[1] - quat[2]*quat[2] + quat[3]*quat[3]);
	pitch = std::asin(qMax(-1.0,qMin(1.0,2*(quat[0]*quat[2] - quat[1]*quat[3]))));
	yaw = std::atan2(2*(quat[1]*quat[2] + quat[0]*quat[3]),quat[0]*quat[0] + quat[1]*quat[1] - quat[2]*quat[2] - quat[3]*quat[3]);
	return;
}