#pragma once

#include <cstddef>
#include <cstdint>

#define SBF_CONFIG_FORCE_INPUT "SSSSSSSSSS\n"

#define SBF_CONFIG_BAUDRATE "setCOMSettings, %s, baud%d\n"

#define SBF_CONFIG_RESET "setSBFOutput, Stream1, %s, none, off\n"

#define SBF_TX_CFG_PRT_BAUDRATE 115200

#define SBF_DATA_IO "setDataInOut, %s, Auto, SBF\n"

#define SBF_CONFIG_DISABLE_OUTPUT "setDataInOut,%s%d,,-RTCMv3-RTCMv2-CMRv2\n"

/* RTK Protocol */
#define SBF_CONFIG_OUTPUT_RTCM3 "setDataInOut, %s, Auto, RTCMv3+SBF\n"

/* RTK Fixed */
#define SBF_CONFIG_RTCM_STATIC_COORDINATES "setStaticPosGeodetic, Geodetic1, %.9f, %.9f, %.4f, WGS84\n"

#define SBF_CONFIG_RTCM_STATIC_OFFSET "setAntennaOffset, Main, %f, %f, %f\n"

#define SBF_CONFIG_RTCM_STATIC1 "setReceiverDynamics, Low, Static\n"

#define SBF_CONFIG_RTCM_STATIC2 "setPVTMode, Static, , Geodetic1\n"

/* RTK Auto */
#define SBF_CONFIG_RTCM_SURVEY_IN "setPVTMode, Static, All, auto\n"

/* Status */
#define SBF_CONFIG_RTCM_STATUS "setSBFOutput, Stream1, %s, +PVTGeodetic, msec500\n"

#define SBF_SYNC1 0x24
#define SBF_SYNC2 0x40

/* Block IDs */
#define SBF_ID_PVTGeodetic 4007

/*** SBF protocol binary message and payload definitions ***/

typedef struct
{
    uint8_t mode_type
        : 4;                     /**< Bit field indicating the PVT mode type, as follows:
                                      0: No PVT available (the Error field indicates the cause of the absence of the PVT solution)
                                      1: Stand-Alone PVT
                                      2: Differential PVT
                                      3: Fixed location
                                      4: RTK with fixed ambiguities
                                      5: RTK with float ambiguities
                                      6: SBAS aided PVT
                                      7: moving-base RTK with fixed ambiguities
                                      8: moving-base RTK with float ambiguities
                                      10:Precise Point Positioning (PPP) */
    uint8_t mode_reserved : 2;   /**< Reserved */
    uint8_t mode_base_fixed : 1; /**< Automatic static base position determination is still in progress (Mode bit 6). */
    uint8_t mode_2d : 1;         /**< 2D/3D flag: set in 2D mode(height assumed constant and not computed). */
    uint8_t error;               /**< PVT error code. The following values are defined:
                                       0: No Error
                                       1: Not enough measurements
                                       2: Not enough ephemerides available
                                       3: DOP too large (larger than 15)
                                       4: Sum of squared residuals too large
                                       5: No convergence
                                       6: Not enough measurements after outlier rejection
                                       7: Position output prohibited due to export laws
                                       8: Not enough differential corrections available
                                       9: Base station coordinates unavailable
                                       10:Ambiguities not fixed and user requested to only output RTK-fixed positions
                                       Note: if this field has a non-zero value, all following fields are set to their Do-Not-Use
                                    value. */
    double latitude;             /**< Marker latitude, from -PI/2 to +PI/2, positive North of Equator */
    double longitude;            /**< Marker longitude, from -PI to +PI, positive East of Greenwich */
    double height;               /**< Marker ellipsoidal height (with respect to the ellipsoid specified by Datum) */
    float undulation;            /**< Geoid undulation computed from the global geoid model defined in
                                         the document 'Technical Characteristics of the NAVSTAR GPS, NATO, June 1991' */
    float vn;                    /**< Velocity in the North direction */
    float ve;                    /**< Velocity in the East direction */
    float vu;                    /**< Velocity in the Up direction */
    float cog;                   /**< Course over ground: this is defined as the angle of the vehicle with respect
                                         to the local level North, ranging from 0 to 360, and increasing towards east.
                                         Set to the do-not-use value when the speed is lower than 0.1m/s. */
    double rx_clk_bias;          /**< Receiver clock bias relative to system time reported in the Time System field.
                                        To transfer the receiver time to the system time, use: tGPS/GST=trx-RxClkBias */
    float RxClkDrift;            /**< Receiver clock drift relative to system time (relative frequency error) */
    uint8_t time_system;         /**< Time system of which the offset is provided in this sub-block:
                                       0:GPStime
                                       1:Galileotime
                                       3:GLONASStime */
    uint8_t datum;               /**< This field defines in which datum the coordinates are expressed:
                                       0: WGS84/ITRS
                                       19: Datum equal to that used by the DGNSS/RTK basestation
                                       30: ETRS89(ETRF2000 realization)
                                       31: NAD83(2011), North American Datum(2011)
                                       32: NAD83(PA11), North American Datum, Pacificplate (2011)
                                       33: NAD83(MA11), North American Datum, Marianas plate(2011)
                                       34: GDA94(2010), Geocentric Datum of Australia (2010)
                                       250:First user-defined datum
                                       251:Second user-defined datum */
    uint8_t nr_sv;               /**< Total number of satellites used in the PVT computation. */
    uint8_t wa_corr_info;   /**< Bit field providing information about which wide area corrections have been applied:
                                  Bit 0: set if orbit and satellite clock correction information is used
                                  Bit 1: set if range correction information is used
                                  Bit 2: set if ionospheric information is used
                                  Bit 3: set if orbit accuracy information is used(UERE/SISA)
                                  Bit 4: set if DO229 Precision Approach mode is active
                                  Bits 5-7: Reserved */
    uint16_t reference_id;  /**< In case of DGPS or RTK operation, this field is to be interpreted as the base station
                               identifier.  In SBAS operation, this field is to be interpreted as the PRN of the
                               geostationary satellite  used (from 120 to 158). If multiple base stations or multiple
                               geostationary satellites are used  the value is set to 65534.*/
    uint16_t mean_corr_age; /**< In case of DGPS or RTK, this field is the mean age of the differential corrections.
                                 In case of SBAS operation, this field is the mean age of the 'fast corrections'
                                 provided by the SBAS satellites */
    uint32_t signal_info;   /**< Bit field indicating the type of GNSS signals having been used in the PVT computations.
                                 If a bit i is set, the signal type having index i has been used. */
    uint8_t alert_flag;     /**< Bit field indicating integrity related information */

    // Revision 1
    uint8_t nr_bases;
    uint16_t ppp_info;
    // Revision 2
    uint16_t latency;
    uint16_t h_accuracy;
    uint16_t v_accuracy;
} sbf_payload_pvt_geodetic_t;

/* General message and payload buffer */

typedef struct
{
    uint16_t sync;  /** The Sync field is a 2-byte array always set to 0x24, 0x40. The first byte of every SBF block has
                        hexadecimal value 24 (decimal 36, ASCII '$'). The second byte of every SBF block has hexadecimal
                        value 40 (decimal 64, ASCII '@'). */
    uint16_t crc16; /** The CRC field is the 16-bit CRC of all the bytes in an SBF block from and including the ID field
                     to the last byte of the block. The generator polynomial for this CRC is the so-called CRC-CCITT
                     polynomial: x 16 + x 12 + x 5 + x 0 . The CRC is computed in the forward direction using a seed of
                     0, no reverse and no final XOR. */
    uint16_t msg_id
        : 13;       /** The ID field is a 2-byte block ID, which uniquely identifies the block type and its contents */
    uint8_t msg_revision : 3; /** block revision number, starting from 0 at the initial block definition, and
                                 incrementing each time backwards - compatible changes are performed to the block  */
    uint16_t length;          /** The Length field is a 2-byte unsigned integer containing the size of the SBF block.
                                  It is the total number of bytes in the SBF block including the header.
                                  It is always a multiple of 4. */
    uint32_t TOW;             /**< Time-Of-Week: Time-tag, expressed in whole milliseconds from
                                   the beginning of the current Galileo/GPSweek. */
    uint16_t WNc;             /**< The GPS week number associated with the TOW. WNc is a continuous
                                   weekcount (hence the "c"). It is not affected by GPS week roll overs,
                                   which occur every 1024 weeks. By definition of the Galileo system time,
                                   WNc is also the Galileo week number + 1024. */

    sbf_payload_pvt_geodetic_t payload_pvt_geodetic;

    uint8_t padding[16];
} sbf_buf_t;

/*** END OF SBF protocol binary message and payload definitions ***/

/* Decoder state */
typedef enum
{
    SBF_DECODE_SYNC1 = 0,
    SBF_DECODE_SYNC2,
    SBF_DECODE_PAYLOAD
} sbf_decode_state_t;
