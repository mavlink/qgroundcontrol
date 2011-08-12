/** @file
 *	@brief MAVLink comm protocol enumerations / structures / constants.
 *	@see http://qgroundcontrol.org/mavlink/
 *	Edited on Monday, August 8 2011
 */

#ifndef MAVLINK_TYPES_H_
#define MAVLINK_TYPES_H_

#include "inttypes.h"

#define MAVLINK_STX 0xD5 ///< Packet start sign
#define MAVLINK_STX_LEN 1 ///< Length of start sign
#define MAVLINK_MAX_PAYLOAD_LEN 255 ///< Maximum payload length

#define MAVLINK_CORE_HEADER_LEN 5 ///< Length of core header (of the comm. layer): message length (1 byte) + message sequence (1 byte) + message system id (1 byte) + message component id (1 byte) + message type id (1 byte)
#define MAVLINK_NUM_HEADER_BYTES (MAVLINK_CORE_HEADER_LEN + MAVLINK_STX_LEN) ///< Length of all header bytes, including core and checksum
#define MAVLINK_NUM_CHECKSUM_BYTES 2
#define MAVLINK_NUM_NON_PAYLOAD_BYTES (MAVLINK_NUM_HEADER_BYTES + MAVLINK_NUM_CHECKSUM_BYTES)
#define MAVLINK_NUM_NON_STX_PAYLOAD_BYTES (MAVLINK_NUM_NON_PAYLOAD_BYTES - MAVLINK_STX_LEN)

#define MAVLINK_MAX_PACKET_LEN (MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_NON_PAYLOAD_BYTES) ///< Maximum packet length
//#define MAVLINK_MAX_DATA_LEN MAVLINK_MAX_PACKET_LEN - MAVLINK_STX_LEN

typedef struct __mavlink_system
{
    uint8_t sysid;   ///< Used by the MAVLink message_xx_send() convenience function
    uint8_t compid;  ///< Used by the MAVLink message_xx_send() convenience function
    uint8_t type;    ///< Unused, can be used by user to store the system's type
    uint8_t state;   ///< Unused, can be used by user to store the system's state
    uint8_t mode;    ///< Unused, can be used by user to store the system's mode
    uint8_t nav_mode; ///< Unused, can be used by user to store the system's navigation mode
} mavlink_system_t;

typedef struct __mavlink_message
{
	union
	{
		uint16_t ck;     ///< Checksum word
		struct
		{
			uint8_t ck_a;    ///< Checksum low byte
			uint8_t ck_b;    ///< Checksum high byte
                };
	};
    uint8_t STX;     ///< start of packet marker
    uint8_t len;     ///< Length of payload
    uint8_t seq;     ///< Sequence of packet
    uint8_t sysid;   ///< ID of message sender system/aircraft
    uint8_t compid;  ///< ID of the message sender component
    uint8_t msgid;   ///< ID of message in payload
    uint8_t payload[MAVLINK_MAX_PAYLOAD_LEN]; ///< Payload data, ALIGNMENT IMPORTANT ON MCU
} mavlink_message_t;

typedef struct __mavlink_header
{
	union
	{
		uint16_t ck;     ///< Checksum word
		struct
		{
			uint8_t ck_a;    ///< Checksum low byte
			uint8_t ck_b;    ///< Checksum high byte
                };
	};
    uint8_t STX;     ///< start of packet marker
    uint8_t len;     ///< Length of payload
    uint8_t seq;     ///< Sequence of packet
    uint8_t sysid;   ///< ID of message sender system/aircraft
    uint8_t compid;  ///< ID of the message sender component
    uint8_t msgid;   ///< ID of message in payload
} mavlink_header_t;

typedef enum
{
    MAVLINK_COMM_0,
    MAVLINK_COMM_1,
    MAVLINK_COMM_2,
    MAVLINK_COMM_3,
    MAVLINK_COMM_NB,
    MAVLINK_COMM_NB_HIGH = 16
} mavlink_channel_t;

typedef enum
{
    MAVLINK_PARSE_STATE_UNINIT=0,
    MAVLINK_PARSE_STATE_IDLE,
    MAVLINK_PARSE_STATE_GOT_STX,
    MAVLINK_PARSE_STATE_GOT_SEQ,
    MAVLINK_PARSE_STATE_GOT_LENGTH,
    MAVLINK_PARSE_STATE_GOT_SYSID,
    MAVLINK_PARSE_STATE_GOT_COMPID,
    MAVLINK_PARSE_STATE_GOT_MSGID,
    MAVLINK_PARSE_STATE_GOT_PAYLOAD,
    MAVLINK_PARSE_STATE_GOT_CRC1
} mavlink_parse_state_t; ///< The state machine for the comm parser

typedef struct __mavlink_status
{
	union
	{
		uint16_t ck;     ///< Checksum word
		struct
		{
			uint8_t ck_a;    ///< Checksum low byte
			uint8_t ck_b;    ///< Checksum high byte
                };
	};
    uint8_t msg_received;               ///< Number of received messages
    uint8_t buffer_overrun;             ///< Number of buffer overruns
    uint8_t parse_error;                ///< Number of parse errors
    mavlink_parse_state_t parse_state;  ///< Parsing state machine
    uint8_t packet_idx;                 ///< Index in current packet
    uint8_t current_rx_seq;             ///< Sequence number of last packet received
    uint8_t current_tx_seq;             ///< Sequence number of last packet sent
    uint16_t packet_rx_success_count;   ///< Received packets
    uint16_t packet_rx_drop_count;      ///< Number of packet drops
} mavlink_status_t;

#endif /* MAVLINK_TYPES_H_ */
