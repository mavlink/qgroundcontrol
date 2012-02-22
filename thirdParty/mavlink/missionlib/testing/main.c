/*******************************************************************************
 
 Copyright (C) 2011 Lorenz Meier lm ( a t ) inf.ethz.ch
 and Bryan Godbolt godbolt ( a t ) ualberta.ca
 
 adapted from example written by Bryan Godbolt godbolt ( a t ) ualberta.ca
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 ****************************************************************************/
/*
 This program sends some data to qgroundcontrol using the mavlink protocol.  The sent packets
 cause qgroundcontrol to respond with heartbeats.  Any settings or custom commands sent from
 qgroundcontrol are printed by this program along with the heartbeats.
 
 
 I compiled this program sucessfully on Ubuntu 10.04 with the following command
 
 gcc -I ../../pixhawk/mavlink/include -o udp-server udp.c
 
 the rt library is needed for the clock_gettime on linux
 */
/* These headers are for QNX, but should all be standard on unix/linux */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#if (defined __QNX__) | (defined __QNXNTO__)
/* QNX specific headers */
#include <unix.h>
#else
/* Linux / MacOS POSIX timer headers */
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#endif

/* FIRST: MAVLink setup */
//#define MAVLINK_CONFIGURED
//#define MAVLINK_NO_DATA
//#define MAVLINK_WPM_VERBOSE

/* 0: Include MAVLink types */
#include "mavlink_types.h"

/* 1: Define mavlink system storage */
mavlink_system_t mavlink_system;

/* 2: Include actual protocol, REQUIRES mavlink_system */
#include "mavlink.h"

/* 3: Define waypoint helper functions */
void mavlink_wpm_send_message(mavlink_message_t* msg);
void mavlink_wpm_send_gcs_string(const char* string);
uint64_t mavlink_wpm_get_system_timestamp();
void mavlink_missionlib_send_message(mavlink_message_t* msg);
void mavlink_missionlib_send_gcs_string(const char* string);
uint64_t mavlink_missionlib_get_system_timestamp();

/* 4: Include waypoint protocol */
#include "waypoints.h"
mavlink_wpm_storage wpm;


#include "mavlink_missionlib_data.h"
#include "mavlink_parameters.h"

mavlink_pm_storage pm;

/**
 * @brief reset all parameters to default
 * @warning DO NOT USE THIS IN FLIGHT!
 */
void mavlink_pm_reset_params(mavlink_pm_storage* pm)
{
	pm->size = MAVLINK_PM_MAX_PARAM_COUNT;
	// 1) MAVLINK_PM_PARAM_SYSTEM_ID
	pm->param_values[MAVLINK_PM_PARAM_SYSTEM_ID] = 12;
	strcpy(pm->param_names[MAVLINK_PM_PARAM_SYSTEM_ID], "SYS_ID");
	// 2) MAVLINK_PM_PARAM_ATT_K_D
	pm->param_values[MAVLINK_PM_PARAM_ATT_K_D] = 0.3f;
	strcpy(pm->param_names[MAVLINK_PM_PARAM_ATT_K_D], "ATT_K_D");
}


#define BUFFER_LENGTH 2041 // minimum buffer size that can be used with qnx (I don't know why)

char help[] = "--help";


char target_ip[100];

float position[6] = {};
int sock;
struct sockaddr_in gcAddr; 
struct sockaddr_in locAddr;
uint8_t buf[BUFFER_LENGTH];
ssize_t recsize;
socklen_t fromlen;
int bytes_sent;
mavlink_message_t msg;
uint16_t len;
int i = 0;
unsigned int temp = 0;

uint64_t microsSinceEpoch();




/* Provide the interface functions for the waypoint manager */

/*
 *  @brief Sends a MAVLink message over UDP
 */

void mavlink_missionlib_send_message(mavlink_message_t* msg)
{
	uint8_t buf[MAVLINK_MAX_PACKET_LEN];
	uint16_t len = mavlink_msg_to_send_buffer(buf, msg);
	uint16_t bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof (struct sockaddr_in));
	
	printf("SENT %d bytes\n", bytes_sent);
}

void mavlink_missionlib_send_gcs_string(const char* string)
{
	const int len = 50;
	mavlink_statustext_t status;
	int i = 0;
	while (i < len - 1)
	{
		status.text[i] = string[i];
		if (string[i] == '\0')
			break;
		i++;
	}
	status.text[i] = '\0'; // Enforce null termination
	mavlink_message_t msg;
	
	mavlink_msg_statustext_encode(mavlink_system.sysid, mavlink_system.compid, &msg, &status);
	mavlink_missionlib_send_message(&msg);
}

uint64_t mavlink_missionlib_get_system_timestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
}

float roll, pitch, yaw;
uint32_t latitude, longitude, altitude;
uint16_t speedx, speedy, speedz;
float rollspeed, pitchspeed, yawspeed;
bool hilEnabled = false;

int main(int argc, char* argv[])
{	
	// Initialize MAVLink
	mavlink_wpm_init(&wpm);
	mavlink_system.sysid = 5;
	mavlink_system.compid = 20;
	mavlink_pm_reset_params(&pm);
	
	int32_t ground_distance;
	uint32_t time_ms;
	
	
	
	// Create socket
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	// Check if --help flag was used
	if ((argc == 2) && (strcmp(argv[1], help) == 0))
    {
		printf("\n");
		printf("\tUsage:\n\n");
		printf("\t");
		printf("%s", argv[0]);
		printf(" <ip address of QGroundControl>\n");
		printf("\tDefault for localhost: udp-server 127.0.0.1\n\n");
		exit(EXIT_FAILURE);
    }
	
	
	// Change the target ip if parameter was given
	strcpy(target_ip, "127.0.0.1");
	if (argc == 2)
    {
		strcpy(target_ip, argv[1]);
    }
	
	
	memset(&locAddr, 0, sizeof(locAddr));
	locAddr.sin_family = AF_INET;
	locAddr.sin_addr.s_addr = INADDR_ANY;
	locAddr.sin_port = htons(14551);
	
	/* Bind the socket to port 14551 - necessary to receive packets from qgroundcontrol */ 
	if (-1 == bind(sock,(struct sockaddr *)&locAddr, sizeof(struct sockaddr)))
    {
		perror("error bind failed");
		close(sock);
		exit(EXIT_FAILURE);
    } 
	
	/* Attempt to make it non blocking */
	if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
    {
		fprintf(stderr, "error setting nonblocking: %s\n", strerror(errno));
		close(sock);
		exit(EXIT_FAILURE);
    }
	
	
	memset(&gcAddr, 0, sizeof(gcAddr));
	gcAddr.sin_family = AF_INET;
	gcAddr.sin_addr.s_addr = inet_addr(target_ip);
	gcAddr.sin_port = htons(14550);
	
	
	printf("MAVLINK MISSION LIBRARY EXAMPLE PROCESS INITIALIZATION DONE, RUNNING..\n");
	
	
	for (;;) 
    {
		bytes_sent = 0;
		
		/* Send Heartbeat */
		mavlink_msg_heartbeat_pack(mavlink_system.sysid, 200, &msg, MAV_TYPE_HELICOPTER, MAV_AUTOPILOT_GENERIC, 0, 0, 0);
		len = mavlink_msg_to_send_buffer(buf, &msg);
		bytes_sent += sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
		
		/* Send Status */
		mavlink_msg_sys_status_pack(1, 200, &msg, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE, 500, 7500, 0, 0, 0, 0, 0, 0, 0, 0);
		len = mavlink_msg_to_send_buffer(buf, &msg);
		bytes_sent += sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof (struct sockaddr_in));
		
		/* Send Local Position */
		mavlink_msg_local_position_ned_pack(mavlink_system.sysid, 200, &msg, microsSinceEpoch(), 
										position[0], position[1], position[2],
										position[3], position[4], position[5]);
		len = mavlink_msg_to_send_buffer(buf, &msg);
		bytes_sent += sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
		
		/* Send global position */
		if (hilEnabled)
		{
			mavlink_msg_global_position_int_pack(mavlink_system.sysid, 200, &msg, time_ms, latitude, longitude, altitude, ground_distance, speedx, speedy, speedz, (yaw/M_PI)*180*100);
			len = mavlink_msg_to_send_buffer(buf, &msg);
			bytes_sent += sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
		}
		
		/* Send attitude */
		mavlink_msg_attitude_pack(mavlink_system.sysid, 200, &msg, microsSinceEpoch(), roll, pitch, yaw, rollspeed, pitchspeed, yawspeed);
		len = mavlink_msg_to_send_buffer(buf, &msg);
		bytes_sent += sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
		
		/* Send HIL outputs */
		float roll_ailerons = 0;   // -1 .. 1
		float pitch_elevator = 0.2;  // -1 .. 1
		float yaw_rudder = 0.1;      // -1 .. 1
		float throttle = 0.9;      //  0 .. 1
		mavlink_msg_hil_controls_pack_chan(mavlink_system.sysid, mavlink_system.compid, MAVLINK_COMM_0, &msg, microsSinceEpoch(), roll_ailerons, pitch_elevator, yaw_rudder, throttle, mavlink_system.mode, mavlink_system.nav_mode, 0, 0, 0, 0);
		len = mavlink_msg_to_send_buffer(buf, &msg);
		bytes_sent += sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
		
		memset(buf, 0, BUFFER_LENGTH);
		recsize = recvfrom(sock, (void *)buf, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
		if (recsize > 0)
      	{
			// Something received - print out all bytes and parse packet
			mavlink_message_t msg;
			mavlink_status_t status;
			
			printf("Bytes Received: %d\nDatagram: ", (int)recsize);
			for (i = 0; i < recsize; ++i)
			{
				temp = buf[i];
				printf("%02x ", (unsigned char)temp);
				if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status))
				{
					// Packet received
					printf("\nReceived packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
					
					// Handle packet with waypoint component
					mavlink_wpm_message_handler(&msg);
					
					// Handle packet with parameter component
					mavlink_pm_message_handler(MAVLINK_COMM_0, &msg);
					
					// Print HIL values sent to system
					if (msg.msgid == MAVLINK_MSG_ID_HIL_STATE)
					{
						mavlink_hil_state_t hil;
						mavlink_msg_hil_state_decode(&msg, &hil);
						printf("Received HIL state:\n");
						printf("R: %f P: %f Y: %f\n", hil.roll, hil.pitch, hil.yaw);
						roll = hil.roll;
						pitch = hil.pitch;
						yaw = hil.yaw;
						rollspeed = hil.rollspeed;
						pitchspeed = hil.pitchspeed;
						yawspeed = hil.yawspeed;
						speedx = hil.vx;
						speedy = hil.vy;
						speedz = hil.vz;
						latitude = hil.lat;
						longitude = hil.lon;
						altitude = hil.alt;
						hilEnabled = true;
					}
				}
			}
			printf("\n");
		}
		memset(buf, 0, BUFFER_LENGTH);
		usleep(10000); // Sleep 10 ms
		
		
		// Send one parameter
		mavlink_pm_queued_send();
    }
}


/* QNX timer version */
#if (defined __QNX__) | (defined __QNXNTO__)
uint64_t microsSinceEpoch()
{
	
	struct timespec time;
	
	uint64_t micros = 0;
	
	clock_gettime(CLOCK_REALTIME, &time);  
	micros = (uint64_t)time.tv_sec * 100000 + time.tv_nsec/1000;
	
	return micros;
}
#else
uint64_t microsSinceEpoch()
{
	
	struct timeval tv;
	
	uint64_t micros = 0;
	
	gettimeofday(&tv, NULL);  
	micros =  ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
	
	return micros;
}
#endif