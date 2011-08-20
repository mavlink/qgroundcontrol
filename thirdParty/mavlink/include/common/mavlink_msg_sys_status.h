// MESSAGE SYS_STATUS PACKING

#define MAVLINK_MSG_ID_SYS_STATUS 1
#define MAVLINK_MSG_ID_SYS_STATUS_LEN 23
#define MAVLINK_MSG_1_LEN 23
#define MAVLINK_MSG_ID_SYS_STATUS_KEY 0xDA
#define MAVLINK_MSG_1_KEY 0xDA

typedef struct __mavlink_sys_status_t 
{
	uint16_t onboard_control_sensors_present;	///< Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	uint16_t onboard_control_sensors_enabled;	///< Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	uint16_t onboard_control_sensors_health;	///< Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	uint16_t load;	///< Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	uint16_t voltage_battery;	///< Battery voltage, in millivolts (1 = 1 millivolt)
	uint16_t current_battery;	///< Battery current, in milliamperes (1 = 1 milliampere)
	uint16_t watt;	///< Watts consumed from this battery since startup
	uint16_t errors_uart;	///< Dropped packets on all links (packets that were corrupted on reception on the MAV)
	uint16_t errors_i2c;	///< Dropped packets on all links (packets that were corrupted on reception)
	uint16_t errors_spi;	///< Dropped packets on all links (packets that were corrupted on reception)
	uint16_t errors_can;	///< Dropped packets on all links (packets that were corrupted on reception)
	uint8_t battery_percent;	///< Remaining battery energy: (0%: 0, 100%: 255)

} mavlink_sys_status_t;

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
static inline uint16_t mavlink_msg_sys_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t onboard_control_sensors_present, uint16_t onboard_control_sensors_enabled, uint16_t onboard_control_sensors_health, uint16_t load, uint16_t voltage_battery, uint16_t current_battery, uint16_t watt, uint8_t battery_percent, uint16_t errors_uart, uint16_t errors_i2c, uint16_t errors_spi, uint16_t errors_can)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	p->onboard_control_sensors_present = onboard_control_sensors_present;	// uint16_t:Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	p->onboard_control_sensors_enabled = onboard_control_sensors_enabled;	// uint16_t:Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	p->onboard_control_sensors_health = onboard_control_sensors_health;	// uint16_t:Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	p->load = load;	// uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->voltage_battery = voltage_battery;	// uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	p->current_battery = current_battery;	// uint16_t:Battery current, in milliamperes (1 = 1 milliampere)
	p->watt = watt;	// uint16_t:Watts consumed from this battery since startup
	p->battery_percent = battery_percent;	// uint8_t:Remaining battery energy: (0%: 0, 100%: 255)
	p->errors_uart = errors_uart;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception on the MAV)
	p->errors_i2c = errors_i2c;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)
	p->errors_spi = errors_spi;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)
	p->errors_can = errors_can;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SYS_STATUS_LEN);
}

/**
 * @brief Pack a sys_status message
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
static inline uint16_t mavlink_msg_sys_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t onboard_control_sensors_present, uint16_t onboard_control_sensors_enabled, uint16_t onboard_control_sensors_health, uint16_t load, uint16_t voltage_battery, uint16_t current_battery, uint16_t watt, uint8_t battery_percent, uint16_t errors_uart, uint16_t errors_i2c, uint16_t errors_spi, uint16_t errors_can)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SYS_STATUS;

	p->onboard_control_sensors_present = onboard_control_sensors_present;	// uint16_t:Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	p->onboard_control_sensors_enabled = onboard_control_sensors_enabled;	// uint16_t:Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	p->onboard_control_sensors_health = onboard_control_sensors_health;	// uint16_t:Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	p->load = load;	// uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	p->voltage_battery = voltage_battery;	// uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	p->current_battery = current_battery;	// uint16_t:Battery current, in milliamperes (1 = 1 milliampere)
	p->watt = watt;	// uint16_t:Watts consumed from this battery since startup
	p->battery_percent = battery_percent;	// uint8_t:Remaining battery energy: (0%: 0, 100%: 255)
	p->errors_uart = errors_uart;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception on the MAV)
	p->errors_i2c = errors_i2c;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)
	p->errors_spi = errors_spi;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)
	p->errors_can = errors_can;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SYS_STATUS_LEN);
}

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


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
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
static inline void mavlink_msg_sys_status_send(mavlink_channel_t chan, uint16_t onboard_control_sensors_present, uint16_t onboard_control_sensors_enabled, uint16_t onboard_control_sensors_health, uint16_t load, uint16_t voltage_battery, uint16_t current_battery, uint16_t watt, uint8_t battery_percent, uint16_t errors_uart, uint16_t errors_i2c, uint16_t errors_spi, uint16_t errors_can)
{
	mavlink_header_t hdr;
	mavlink_sys_status_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SYS_STATUS_LEN )
	payload.onboard_control_sensors_present = onboard_control_sensors_present;	// uint16_t:Bitmask showing which onboard controllers and sensors are present. Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	payload.onboard_control_sensors_enabled = onboard_control_sensors_enabled;	// uint16_t:Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	payload.onboard_control_sensors_health = onboard_control_sensors_health;	// uint16_t:Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
	payload.load = load;	// uint16_t:Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
	payload.voltage_battery = voltage_battery;	// uint16_t:Battery voltage, in millivolts (1 = 1 millivolt)
	payload.current_battery = current_battery;	// uint16_t:Battery current, in milliamperes (1 = 1 milliampere)
	payload.watt = watt;	// uint16_t:Watts consumed from this battery since startup
	payload.battery_percent = battery_percent;	// uint8_t:Remaining battery energy: (0%: 0, 100%: 255)
	payload.errors_uart = errors_uart;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception on the MAV)
	payload.errors_i2c = errors_i2c;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)
	payload.errors_spi = errors_spi;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)
	payload.errors_can = errors_can;	// uint16_t:Dropped packets on all links (packets that were corrupted on reception)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SYS_STATUS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SYS_STATUS;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xDA, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
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
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->onboard_control_sensors_present);
}

/**
 * @brief Get field onboard_control_sensors_enabled from sys_status message
 *
 * @return Bitmask showing which onboard controllers and sensors are enabled:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 */
static inline uint16_t mavlink_msg_sys_status_get_onboard_control_sensors_enabled(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->onboard_control_sensors_enabled);
}

/**
 * @brief Get field onboard_control_sensors_health from sys_status message
 *
 * @return Bitmask showing which onboard controllers and sensors are operational or have an error:  Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control
 */
static inline uint16_t mavlink_msg_sys_status_get_onboard_control_sensors_health(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->onboard_control_sensors_health);
}

/**
 * @brief Get field load from sys_status message
 *
 * @return Maximum usage in percent of the mainloop time, (0%: 0, 100%: 1000) should be always below 1000
 */
static inline uint16_t mavlink_msg_sys_status_get_load(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->load);
}

/**
 * @brief Get field voltage_battery from sys_status message
 *
 * @return Battery voltage, in millivolts (1 = 1 millivolt)
 */
static inline uint16_t mavlink_msg_sys_status_get_voltage_battery(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->voltage_battery);
}

/**
 * @brief Get field current_battery from sys_status message
 *
 * @return Battery current, in milliamperes (1 = 1 milliampere)
 */
static inline uint16_t mavlink_msg_sys_status_get_current_battery(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->current_battery);
}

/**
 * @brief Get field watt from sys_status message
 *
 * @return Watts consumed from this battery since startup
 */
static inline uint16_t mavlink_msg_sys_status_get_watt(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->watt);
}

/**
 * @brief Get field battery_percent from sys_status message
 *
 * @return Remaining battery energy: (0%: 0, 100%: 255)
 */
static inline uint8_t mavlink_msg_sys_status_get_battery_percent(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint8_t)(p->battery_percent);
}

/**
 * @brief Get field errors_uart from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception on the MAV)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_uart(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->errors_uart);
}

/**
 * @brief Get field errors_i2c from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_i2c(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->errors_i2c);
}

/**
 * @brief Get field errors_spi from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_spi(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->errors_spi);
}

/**
 * @brief Get field errors_can from sys_status message
 *
 * @return Dropped packets on all links (packets that were corrupted on reception)
 */
static inline uint16_t mavlink_msg_sys_status_get_errors_can(const mavlink_message_t* msg)
{
	mavlink_sys_status_t *p = (mavlink_sys_status_t *)&msg->payload[0];
	return (uint16_t)(p->errors_can);
}

/**
 * @brief Decode a sys_status message into a struct
 *
 * @param msg The message to decode
 * @param sys_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_sys_status_decode(const mavlink_message_t* msg, mavlink_sys_status_t* sys_status)
{
	memcpy( sys_status, msg->payload, sizeof(mavlink_sys_status_t));
}
