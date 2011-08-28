/** @file
 *	@brief MAVLink comm protocol testsuite generated from common.xml
 *	@see http://qgroundcontrol.org/mavlink/
 *	Generated on Wed Aug 24 17:14:16 2011
 */
#ifndef COMMON_TESTSUITE_H
#define COMMON_TESTSUITE_H

#ifdef __cplusplus
extern "C" {
#endif

static void mavtest_generate_outputs(mavlink_channel_t chan)
{
	mavlink_msg_heartbeat_send(chan , 5 , 72 , 139 , 206 , 17 , 84 , 151 );
	mavlink_msg_sys_status_send(chan , 17235 , 17339 , 17443 , 17547 , 17651 , 17755 , 17859 , 199 , 17963 , 18067 , 18171 , 18275 );
	mavlink_msg_system_time_send(chan , 9223372036854775807 , 963497880 );
	mavlink_msg_system_time_utc_send(chan , 963497464 , 963497672 );
	mavlink_msg_ping_send(chan , 963497880 , 41 , 108 , 9223372036854775807 );
	mavlink_msg_change_operator_control_send(chan , 5 , 72 , 139 , "AQGWMCSIYOEUKAQGWMCSIYOEU" );
	mavlink_msg_change_operator_control_ack_send(chan , 5 , 72 , 139 );
	mavlink_msg_auth_key_send(chan , "ARIZQHYPGXOFWNEVMDULCTKBSJARIZQH" );
	mavlink_msg_set_mode_send(chan , 5 , 72 );
	mavlink_msg_set_flight_mode_send(chan , 5 , 72 );
	mavlink_msg_set_safety_mode_send(chan , 5 , 72 );
	mavlink_msg_param_request_read_send(chan , 139 , 206 , "AHOVCJQXELSZGNUB" , 17235 );
	mavlink_msg_param_request_list_send(chan , 5 , 72 );
	mavlink_msg_param_value_send(chan , "AXUROLIFCZWTQNKH" , 17.0 , 77 , 17443 , 17547 );
	mavlink_msg_param_set_send(chan , 17 , 84 , "APETIXMBQFUJYNCR" , 17.0 , 199 );
	mavlink_msg_gps_raw_int_send(chan , 9223372036854775807 , 89 , 963497880 , 963498088 , 963498296 , 18275 , 18379 , 18483 , 18587 , 156 );
	mavlink_msg_scaled_imu_send(chan , 9223372036854775807 , 17651 , 17755 , 17859 , 17963 , 18067 , 18171 , 18275 , 18379 , 18483 );
	mavlink_msg_gps_status_send(chan , 5 , "72" , "132" , "192" , "252" , "56" );
	mavlink_msg_raw_imu_send(chan , 9223372036854775807 , 17651 , 17755 , 17859 , 17963 , 18067 , 18171 , 18275 , 18379 , 18483 );
	mavlink_msg_raw_pressure_send(chan , 9223372036854775807 , 17651 , 17755 , 17859 , 17963 );
	mavlink_msg_scaled_pressure_send(chan , 9223372036854775807 , 73.0 , 101.0 , 18067 );
	mavlink_msg_attitude_send(chan , 9223372036854775807 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 );
	mavlink_msg_local_position_send(chan , 9223372036854775807 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 );
	mavlink_msg_global_position_send(chan , 9223372036854775807 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 );
	mavlink_msg_gps_raw_send(chan , 9223372036854775807 , 113 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 , 241.0 , 180 );
	mavlink_msg_rc_channels_raw_send(chan , 17235 , 17339 , 17443 , 17547 , 17651 , 17755 , 17859 , 17963 , 53 );
	mavlink_msg_rc_channels_scaled_send(chan , 17235 , 17339 , 17443 , 17547 , 17651 , 17755 , 17859 , 17963 , 53 );
	mavlink_msg_servo_output_raw_send(chan , 17235 , 17339 , 17443 , 17547 , 17651 , 17755 , 17859 , 17963 );
	mavlink_msg_waypoint_send(chan , 223 , 34 , 18691 , 101 , 168 , 235 , 46 , 17.0 , 45.0 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 );
	mavlink_msg_waypoint_request_send(chan , 139 , 206 , 17235 );
	mavlink_msg_waypoint_set_current_send(chan , 139 , 206 , 17235 );
	mavlink_msg_waypoint_current_send(chan , 17235 );
	mavlink_msg_waypoint_request_list_send(chan , 5 , 72 );
	mavlink_msg_waypoint_count_send(chan , 139 , 206 , 17235 );
	mavlink_msg_waypoint_clear_all_send(chan , 5 , 72 );
	mavlink_msg_waypoint_reached_send(chan , 17235 );
	mavlink_msg_waypoint_ack_send(chan , 5 , 72 , 139 );
	mavlink_msg_gps_set_global_origin_send(chan , 41 , 108 , 963497464 , 963497672 , 963497880 );
	mavlink_msg_gps_local_origin_set_send(chan , 963497464 , 963497672 , 963497880 );
	mavlink_msg_local_position_setpoint_set_send(chan , 53 , 120 , 17.0 , 45.0 , 73.0 , 101.0 );
	mavlink_msg_local_position_setpoint_send(chan , 17.0 , 45.0 , 73.0 , 101.0 );
	mavlink_msg_global_position_setpoint_int_send(chan , 963497464 , 963497672 , 963497880 , 17859 );
	mavlink_msg_safety_set_allowed_area_send(chan , 77 , 144 , 211 , 17.0 , 45.0 , 73.0 , 101.0 , 129.0 , 157.0 );
	mavlink_msg_safety_allowed_area_send(chan , 77 , 17.0 , 45.0 , 73.0 , 101.0 , 129.0 , 157.0 );
	mavlink_msg_set_roll_pitch_yaw_thrust_send(chan , 53 , 120 , 17.0 , 45.0 , 73.0 , 101.0 );
	mavlink_msg_set_roll_pitch_yaw_speed_thrust_send(chan , 53 , 120 , 17.0 , 45.0 , 73.0 , 101.0 );
	mavlink_msg_roll_pitch_yaw_thrust_setpoint_send(chan , 963497464 , 45.0 , 73.0 , 101.0 , 129.0 );
	mavlink_msg_roll_pitch_yaw_speed_thrust_setpoint_send(chan , 963497464 , 45.0 , 73.0 , 101.0 , 129.0 );
	mavlink_msg_nav_controller_output_send(chan , 17.0 , 45.0 , 18275 , 18379 , 18483 , 73.0 , 101.0 , 129.0 );
	mavlink_msg_position_target_send(chan , 17.0 , 45.0 , 73.0 , 101.0 );
	mavlink_msg_state_correction_send(chan , 17.0 , 45.0 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 , 241.0 );
	mavlink_msg_request_data_stream_send(chan , 139 , 206 , 17 , 17235 , 84 );
	mavlink_msg_data_stream_send(chan , 139 , 17235 , 206 );
	mavlink_msg_manual_control_send(chan , 53 , 17.0 , 45.0 , 73.0 , 101.0 , 120 , 187 , 254 , 65 );
	mavlink_msg_rc_channels_override_send(chan , 53 , 120 , 17235 , 17339 , 17443 , 17547 , 17651 , 17755 , 17859 , 17963 );
	mavlink_msg_global_position_int_send(chan , 963497464 , 963497672 , 963497880 , 17859 , 17963 , 18067 , 18171 );
	mavlink_msg_vfr_hud_send(chan , 17.0 , 45.0 , 18067 , 18171 , 73.0 , 101.0 );
	mavlink_msg_command_short_send(chan , 53 , 120 , 187 , 254 , 17.0 , 45.0 , 73.0 , 101.0 );
	mavlink_msg_command_long_send(chan , 89 , 156 , 223 , 34 , 17.0 , 45.0 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 );
	mavlink_msg_command_ack_send(chan , 17.0 , 45.0 );
	mavlink_msg_hil_state_send(chan , 9223372036854775807 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 , 963499128 , 963499336 , 963499544 , 19523 , 19627 , 19731 , 19835 , 19939 , 20043 );
	mavlink_msg_hil_controls_send(chan , 9223372036854775807 , 73.0 , 101.0 , 129.0 , 157.0 , 185.0 , 213.0 , 241.0 , 269.0 , 125 , 192 );
	mavlink_msg_hil_rc_inputs_raw_send(chan , 9223372036854775807 , 17651 , 17755 , 17859 , 17963 , 18067 , 18171 , 18275 , 18379 , 18483 , 18587 , 18691 , 18795 , 101 );
	mavlink_msg_optical_flow_send(chan , 9223372036854775807 , 53 , 17859 , 17963 , 120 , 73.0 );
	mavlink_msg_memory_vect_send(chan , 17235 , 139 , 206 , "17" );
	mavlink_msg_debug_vect_send(chan , "ATMFYRKDWP" , 9223372036854775807 , 73.0 , 101.0 , 129.0 );
	mavlink_msg_named_value_float_send(chan , "AHOVCJQXEL" , 17.0 );
	mavlink_msg_named_value_int_send(chan , "AHOVCJQXEL" , 963497464 );
	mavlink_msg_statustext_send(chan , 5 , "AIQYGOWEMUCKSAIQYGOWEMUCKSAIQYGOWEMUCKSAIQYGOWEMUC" );
	mavlink_msg_debug_send(chan , 17 , 17.0 );
}

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // COMMON_TESTSUITE_H
