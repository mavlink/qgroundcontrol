#pragma once

// Host-only platform contract for driver tests. These are application data
// structures, not UBX wire layouts; no Qt or PX4 runtime is required.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>

using gps_abstime = uint64_t;
inline gps_abstime gps_test_time = 0;
inline gps_abstime gps_absolute_time() { return gps_test_time; }
inline void gps_usleep(unsigned long usecs) { gps_test_time += usecs; }

#define GPS_INFO(...) do {} while (0)
inline std::vector<std::string> gps_test_warnings;
inline void gps_test_warn(const char *format, ...)
{
	char message[1024]{};
	va_list args;
	va_start(args, format);
	std::vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	gps_test_warnings.emplace_back(message);
}

#define GPS_WARN(...) gps_test_warn(__VA_ARGS__)
#define GPS_ERR(...) do {} while (0)
#define M_DEG_TO_RAD_F 0.01745329251994329577f
#define M_RAD_TO_DEG 57.2957795130823208768


struct sensor_gps_s
{
	uint64_t timestamp;
	uint64_t timestamp_sample;

	uint32_t device_id;

	double latitude_deg;
	double longitude_deg;
	double altitude_msl_m;
	double altitude_ellipsoid_m;

	float s_variance_m_s;
	float c_variance_rad;

	static constexpr uint8_t FIX_TYPE_NONE = 1;
	static constexpr uint8_t FIX_TYPE_2D = 2;
	static constexpr uint8_t FIX_TYPE_3D = 3;
	static constexpr uint8_t FIX_TYPE_RTCM_CODE_DIFFERENTIAL = 4;
	static constexpr uint8_t FIX_TYPE_RTK_FLOAT = 5;
	static constexpr uint8_t FIX_TYPE_RTK_FIXED = 6;
	static constexpr uint8_t FIX_TYPE_EXTRAPOLATED = 8;
	uint8_t fix_type;

	float eph;
	float epv;

	float hdop;
	float vdop;

	int32_t noise_per_ms;
	uint16_t automatic_gain_control;

	static constexpr uint8_t JAMMING_STATE_UNKNOWN = 0;
	static constexpr uint8_t JAMMING_STATE_OK = 1;
	static constexpr uint8_t JAMMING_STATE_MITIGATED = 2;
	static constexpr uint8_t JAMMING_STATE_DETECTED = 3;
	uint8_t jamming_state;
	int32_t jamming_indicator;

	static constexpr uint8_t SPOOFING_STATE_UNKNOWN = 0;
	static constexpr uint8_t SPOOFING_STATE_OK = 1;
	static constexpr uint8_t SPOOFING_STATE_MITIGATED = 2;
	static constexpr uint8_t SPOOFING_STATE_DETECTED = 3;
	uint8_t spoofing_state;

	static constexpr uint8_t AUTHENTICATION_STATE_UNKNOWN = 0;
	static constexpr uint8_t AUTHENTICATION_STATE_INITIALIZING = 1;
	static constexpr uint8_t AUTHENTICATION_STATE_ERROR = 2;
	static constexpr uint8_t AUTHENTICATION_STATE_OK = 3;
	static constexpr uint8_t AUTHENTICATION_STATE_DISABLED = 4;
	uint8_t authentication_state;

	float vel_m_s;
	float vel_n_m_s;
	float vel_e_m_s;
	float vel_d_m_s;
	float cog_rad;
	bool vel_ned_valid;

	int32_t timestamp_time_relative;
	uint64_t time_utc_usec;

	uint8_t satellites_used;

	static constexpr uint32_t SYSTEM_ERROR_OK = 0;
	static constexpr uint32_t SYSTEM_ERROR_INCOMING_CORRECTIONS = 1;
	static constexpr uint32_t SYSTEM_ERROR_CONFIGURATION = 2;
	static constexpr uint32_t SYSTEM_ERROR_SOFTWARE = 4;
	static constexpr uint32_t SYSTEM_ERROR_ANTENNA = 8;
	static constexpr uint32_t SYSTEM_ERROR_EVENT_CONGESTION = 16;
	static constexpr uint32_t SYSTEM_ERROR_CPU_OVERLOAD = 32;
	static constexpr uint32_t SYSTEM_ERROR_OUTPUT_CONGESTION = 64;
	uint32_t system_error;

	float heading;
	float heading_offset;
	float heading_accuracy;

	float rtcm_injection_rate;
	uint8_t selected_rtcm_instance;

	static constexpr uint8_t CORRECTIONS_PROTOCOL_UNKNOWN = 0;
	static constexpr uint8_t CORRECTIONS_PROTOCOL_RTCM3 = 1;
	static constexpr uint8_t CORRECTIONS_PROTOCOL_SPARTN = 2;
	static constexpr uint8_t CORRECTIONS_PROTOCOL_HAS = 3;
	static constexpr uint8_t CORRECTIONS_PROTOCOL_PMP = 4;
	static constexpr uint8_t CORRECTIONS_PROTOCOL_QZSS_L6 = 5;
	uint8_t corrections_protocol;

	bool corrections_crc_failed;

	static constexpr uint8_t CORRECTIONS_MSG_USED_UNKNOWN = 0;
	static constexpr uint8_t CORRECTIONS_MSG_USED_NOT_USED = 1;
	static constexpr uint8_t CORRECTIONS_MSG_USED_USED = 2;
	uint8_t corrections_msg_used;

	float antenna_offset_x;
	float antenna_offset_y;
	float antenna_offset_z;
};

struct sensor_gnss_relative_s
{
	uint64_t timestamp;
	uint64_t timestamp_sample;

	uint32_t device_id;

	uint64_t time_utc_usec;

	uint16_t reference_station_id;

	float position[3];
	float position_accuracy[3];

	float heading;
	float heading_accuracy;

	float position_length;
	float accuracy_length;

	bool gnss_fix_ok;
	bool differential_solution;
	bool relative_position_valid;
	bool carrier_solution_floating;
	bool carrier_solution_fixed;
	bool moving_base_mode;
	bool reference_position_miss;
	bool reference_observations_miss;
	bool heading_valid;
	bool relative_position_normalized;
};

struct satellite_info_s
{
	uint64_t timestamp;
	static constexpr uint8_t SAT_INFO_MAX_SATELLITES = 40;

	uint8_t count;
	uint8_t svid[SAT_INFO_MAX_SATELLITES];
	uint8_t used[SAT_INFO_MAX_SATELLITES];
	uint8_t elevation[SAT_INFO_MAX_SATELLITES];
	uint8_t azimuth[SAT_INFO_MAX_SATELLITES];
	uint8_t snr[SAT_INFO_MAX_SATELLITES];
	uint8_t prn[SAT_INFO_MAX_SATELLITES];
};
