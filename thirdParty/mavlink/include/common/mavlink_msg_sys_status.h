// MESSAGE SYS_STATUS PACKING

#define MAVLINK_MSG_ID_SYS_STATUS 1

typedef struct __mavlink_sys_status_t
{
 uint16_t onboard_control_sensors_present; ///< Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 uint16_t onboard_control_sensors_enabled; ///< Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 uint16_t onboard_control_sensors_health; ///< Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 uint16_t load; ///< Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 uint16_t voltage_battery; ///< Battery voltage, in millivolts (1 = 1 millivolt)
 uint16_t current_battery; ///< Battery current, in milliamperes (1 = 1 milliampere)
 uint16_t watt; ///< Watts consumed from this battery since startup
 uint16_t errors_uart; ///< Dropped packets on all links (packets that were corrupted on reception on the MAV)
 uint16_t errors_i2c; ///< Dropped packets on all links (packets that were corrupted on reception)
 uint16_t errors_spi; ///< Dropped packets on all links (packets that were corrupted on reception)
 uint16_t errors_can; ///< Dropped packets on all links (packets that were corrupted on reception)
 uint8_t battery_percent; ///< Remaining battery energy: (0%: 0, 100%: 255)
} mavlink_sys_status_t;

#define MAVLINK_MSG_ID_SYS_STATUS_LEN 23
#define MAVLINK_MSG_ID_1_LEN 23



#define MAVLINK_MESSAGE_INFO_SYS_STATUS { \
	"SYS_STATUS", \
	12, \
	{  { "onboard_control_sensors_present", MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_sys_status_t, onboard_control_sensors_present) }, \
         { "onboard_control_sensors_enabled", MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_sys_status_t, onboard_control_sensors_enabled) }, \
         { "onboard_control_sensors_health", MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_sys_status_t, onboard_control_sensors_health) }, \
         { "load", MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_sys_status_t, load) }, \
         { "voltage_battery", MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_sys_status_t, voltage_battery) }, \
         { "current_battery", MAVLINK_TYPE_UINT16_T, 0, 10, offsetof(mavlink_sys_status_t, current_battery) }, \
         { "watt", MAVLINK_TYPE_UINT16_T, 0, 12, offsetof(mavlink_sys_status_t, watt) }, \
         { "errors_uart", MAVLINK_TYPE_UINT16_T, 0, 14, offsetof(mavlink_sys_status_t, errors_uart) }, \
         { "errors_i2c", MAVLINK_TYPE_UINT16_T, 0, 16, offsetof(mavlink_sys_status_t, errors_i2c) }, \
         { "errors_spi", MAVLINK_TYPE_UINT16_T, 0, 18, offsetof(mavlink_sys_status_t, errors_spi) }, \
         { "errors_can", MAVLINK_TYPE_UINT16_T, 0, 20, offsetof(mavlink_sys_status_t, errors_can) }, \
         { "battery_percent", MAVLINK_TYPE_UINT8_T, 0, 22, offsetof(mavlink_sys_status_t, battery_percent) }, \
         } \
}


/**
 * @brief Pack a sys_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param onboard_control_sensors_present Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_enabled Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_health Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param voltage_battery Battery voltage, in millivolts (1 = 1 millivolt)
 * @param current_battery Battery current, in milliamperes (1 = 1 milliampere)
 * @param watt Watts consumed from this battery since startup
 * @param battery_percent Remaining battery energy: (0%: 0, 100%: 255)
 * @param errors_uart Dropped packets on all links (packets that were corrupted on reception on the MAV)
 * @param errors_i2c Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_spi Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_can Dropped packets on all links (packets that were corrupted on reception)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint16_t onboard_control_sensors_present, uint16_t onboard_control_sensors_enabled, uint16_t onboard_control_sensors_health, uint16_t load, uint16_t voltage_battery, uint16_t current_battery, uint16_t watt, uint8_t battery_percent, uint16_t errors_uart, uint16_t errors_i2c, uint16_t errors_spi, uint16_t errors_can)
{
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	put_uint16_t_by_index(msg, 0, onboard_control_sensors_present); // Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 2, onboard_control_sensors_enabled); // Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 4, onboard_control_sensors_health); // Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 6, load); // Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	put_uint16_t_by_index(msg, 8, voltage_battery); // Battery voltage, in millivolts (1 = 1 millivolt)
	put_uint16_t_by_index(msg, 10, current_battery); // Battery current, in milliamperes (1 = 1 milliampere)
	put_uint16_t_by_index(msg, 12, watt); // Watts consumed from this battery since startup
	put_uint16_t_by_index(msg, 14, errors_uart); // Dropped packets on all links (packets that were corrupted on reception on the MAV)
	put_uint16_t_by_index(msg, 16, errors_i2c); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint16_t_by_index(msg, 18, errors_spi); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint16_t_by_index(msg, 20, errors_can); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint8_t_by_index(msg, 22, battery_percent); // Remaining battery energy: (0%: 0, 100%: 255)

	return mavlink_finalize_message(msg, system_id, component_id, 23, 114);
}

/**
 * @brief Pack a sys_status message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param onboard_control_sensors_present Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_enabled Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_health Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param voltage_battery Battery voltage, in millivolts (1 = 1 millivolt)
 * @param current_battery Battery current, in milliamperes (1 = 1 milliampere)
 * @param watt Watts consumed from this battery since startup
 * @param battery_percent Remaining battery energy: (0%: 0, 100%: 255)
 * @param errors_uart Dropped packets on all links (packets that were corrupted on reception on the MAV)
 * @param errors_i2c Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_spi Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_can Dropped packets on all links (packets that were corrupted on reception)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint16_t onboard_control_sensors_present,uint16_t onboard_control_sensors_enabled,uint16_t onboard_control_sensors_health,uint16_t load,uint16_t voltage_battery,uint16_t current_battery,uint16_t watt,uint8_t battery_percent,uint16_t errors_uart,uint16_t errors_i2c,uint16_t errors_spi,uint16_t errors_can)
{
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	put_uint16_t_by_index(msg, 0, onboard_control_sensors_present); // Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 2, onboard_control_sensors_enabled); // Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 4, onboard_control_sensors_health); // Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 6, load); // Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	put_uint16_t_by_index(msg, 8, voltage_battery); // Battery voltage, in millivolts (1 = 1 millivolt)
	put_uint16_t_by_index(msg, 10, current_battery); // Battery current, in milliamperes (1 = 1 milliampere)
	put_uint16_t_by_index(msg, 12, watt); // Watts consumed from this battery since startup
	put_uint16_t_by_index(msg, 14, errors_uart); // Dropped packets on all links (packets that were corrupted on reception on the MAV)
	put_uint16_t_by_index(msg, 16, errors_i2c); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint16_t_by_index(msg, 18, errors_spi); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint16_t_by_index(msg, 20, errors_can); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint8_t_by_index(msg, 22, battery_percent); // Remaining battery energy: (0%: 0, 100%: 255)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 23, 114);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a sys_status message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param onboard_control_sensors_present Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_enabled Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_health Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param voltage_battery Battery voltage, in millivolts (1 = 1 millivolt)
 * @param current_battery Battery current, in milliamperes (1 = 1 milliampere)
 * @param watt Watts consumed from this battery since startup
 * @param battery_percent Remaining battery energy: (0%: 0, 100%: 255)
 * @param errors_uart Dropped packets on all links (packets that were corrupted on reception on the MAV)
 * @param errors_i2c Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_spi Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_can Dropped packets on all links (packets that were corrupted on reception)
 */
static inline void mavlink_msg_sys_status_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint16_t onboard_control_sensors_present,uint16_t onboard_control_sensors_enabled,uint16_t onboard_control_sensors_health,uint16_t load,uint16_t voltage_battery,uint16_t current_battery,uint16_t watt,uint8_t battery_percent,uint16_t errors_uart,uint16_t errors_i2c,uint16_t errors_spi,uint16_t errors_can)
{
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	put_uint16_t_by_index(msg, 0, onboard_control_sensors_present); // Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 2, onboard_control_sensors_enabled); // Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 4, onboard_control_sensors_health); // Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	put_uint16_t_by_index(msg, 6, load); // Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	put_uint16_t_by_index(msg, 8, voltage_battery); // Battery voltage, in millivolts (1 = 1 millivolt)
	put_uint16_t_by_index(msg, 10, current_battery); // Battery current, in milliamperes (1 = 1 milliampere)
	put_uint16_t_by_index(msg, 12, watt); // Watts consumed from this battery since startup
	put_uint16_t_by_index(msg, 14, errors_uart); // Dropped packets on all links (packets that were corrupted on reception on the MAV)
	put_uint16_t_by_index(msg, 16, errors_i2c); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint16_t_by_index(msg, 18, errors_spi); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint16_t_by_index(msg, 20, errors_can); // Dropped packets on all links (packets that were corrupted on reception)
	put_uint8_t_by_index(msg, 22, battery_percent); // Remaining battery energy: (0%: 0, 100%: 255)

	mavlink_finalize_message_chan_send(msg, chan, 23, 114);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a sys_status struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sys_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sys_status_t* sys_status)
{
	return mavlink_msg_sys_status_pack(system_id, component_id, msg, sys_status->onboard_control_sensors_present, sys_status->onboard_control_sensors_enabled, sys_status->onboard_control_sensors_health, sys_status->load, sys_status->voltage_battery, sys_status->current_battery, sys_status->watt, sys_status->battery_percent, sys_status->errors_uart, sys_status->errors_i2c, sys_status->errors_spi, sys_status->errors_can);
}

/**
 * @brief Send a sys_status message
 * @param chan MAVLink channel to send the message
 *
 * @param onboard_control_sensors_present Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_enabled Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param onboard_control_sensors_health Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 * @param load Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 * @param voltage_battery Battery voltage, in millivolts (1 = 1 millivolt)
 * @param current_battery Battery current, in milliamperes (1 = 1 milliampere)
 * @param watt Watts consumed from this battery since startup
 * @param battery_percent Remaining battery energy: (0%: 0, 100%: 255)
 * @param errors_uart Dropped packets on all links (packets that were corrupted on reception on the MAV)
 * @param errors_i2c Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_spi Dropped packets on all links (packets that were corrupted on reception)
 * @param errors_can Dropped packets on all links (packets that were corrupted on reception)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sys_status_send(mavlink_channel_t chan, uint16_t onboard_control_sensors_present, uint16_t onboard_control_sensors_enabled, uint16_t onboard_control_sensors_health, uint16_t load, uint16_t voltage_battery, uint16_t current_battery, uint16_t watt, uint8_t battery_percent, uint16_t errors_uart, uint16_t errors_i2c, uint16_t errors_spi, uint16_t errors_can)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 23);
	mavlink_msg_sys_status_pack_chan_send(chan, msg, onboard_control_sensors_present, onboard_control_sensors_enabled, onboard_control_sensors_health, load, voltage_battery, current_battery, watt, battery_percent, errors_uart, errors_i2c, errors_spi, errors_can);
}

#endif

// MESSAGE SYS_STATUS UNPACKING


/**
 * @brief Get field onboard_control_sensors_present from sys_status message
 *
 * @return Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 */
static inline uint16_t mavlink_msg_sys_status_get_onboard_control_sensors_present(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field onboard_control_sensors_enabled from sys_status message
 *
 * @return Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 */
static inline uint16_t mavlink_msg_sys_status_get_onboard_control_sensors_enabled(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  2);
}

/**
 * @brief Get field onboard_control_sensors_health from sys_status message
 *
 * @return Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 */
static inline uint16_t mavlink_msg_sys_status_get_onboard_control_sensors_health(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field load from sys_status message
 *
 * @return Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 */
static inline uint16_t mavlink_msg_sys_status_get_load(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Get field voltage_battery from sys_status message
 *
 * @return Battery voltage, in millivolts (1 = 1 millivolt)
 */
static inline uint16_t mavlink_msg_sys_status_get_voltage_battery(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Get field current_battery from sys_status message
 *
 * @return Battery current, in milliamperes (1 = 1 milliampere)
 */
static inline uint16_t mavlink_msg_sys_status_get_current_battery(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  10);
}

/**
 * @brief Get field watt from sys_status message
 *
 * @return Watts consumed from this battery since startup
 */
static inline uint16_t mavlink_msg_sys_status_get_watt(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  12);
}

/**
 * @brief Get field battery_percent from sys_status message
 *
 * @return Remaining battery energy: (0%: 0, 100%: 255)
 */
static inline uint8_t mavlink_msg_sys_status_get_battery_percent(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  22);
}

/**
 * @brief Get field errors_uart from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception on the MAV)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_uart(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  14);
}

/**
 * @brief Get field errors_i2c from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_i2c(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  16);
}

/**
 * @brief Get field errors_spi from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_spi(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  18);
}

/**
 * @brief Get field errors_can from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_can(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  20);
}

/**
 * @brief Decode a sys_status message into a struct
 *
 * @param msg The message to decode
 * @param sys_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_sys_status_decode(const mavlink_message_t* msg, mavlink_sys_status_t* sys_status)
{
#if MAVLINK_NEED_BYTE_SWAP
	sys_status->onboard_control_sensors_present = mavlink_msg_sys_status_get_onboard_control_sensors_present(msg);
	sys_status->onboard_control_sensors_enabled = mavlink_msg_sys_status_get_onboard_control_sensors_enabled(msg);
	sys_status->onboard_control_sensors_health = mavlink_msg_sys_status_get_onboard_control_sensors_health(msg);
	sys_status->load = mavlink_msg_sys_status_get_load(msg);
	sys_status->voltage_battery = mavlink_msg_sys_status_get_voltage_battery(msg);
	sys_status->current_battery = mavlink_msg_sys_status_get_current_battery(msg);
	sys_status->watt = mavlink_msg_sys_status_get_watt(msg);
	sys_status->errors_uart = mavlink_msg_sys_status_get_errors_uart(msg);
	sys_status->errors_i2c = mavlink_msg_sys_status_get_errors_i2c(msg);
	sys_status->errors_spi = mavlink_msg_sys_status_get_errors_spi(msg);
	sys_status->errors_can = mavlink_msg_sys_status_get_errors_can(msg);
	sys_status->battery_percent = mavlink_msg_sys_status_get_battery_percent(msg);
#else
	memcpy(sys_status, MAVLINK_PAYLOAD(msg), 23);
#endif
}
