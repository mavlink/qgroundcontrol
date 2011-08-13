/** @file
 *	@brief Main MAVLink comm protocol routines.
 *	@see http://qgroundcontrol.org/mavlink/
 *	Edited on Monday, August 8 2011
 */

#ifndef  _MAVLINK_PROTOCOL_H_
#define  _MAVLINK_PROTOCOL_H_

#include "mavlink_types.h"

#include "mavlink_checksum.h"

#ifdef MAVLINK_CHECK_LENGTH
extern const uint8_t MAVLINK_CONST mavlink_msg_lengths[256];
#endif

extern const uint8_t MAVLINK_CONST mavlink_msg_keys[256];

extern mavlink_status_t m_mavlink_status[MAVLINK_COMM_NB];
extern mavlink_message_t m_mavlink_message[MAVLINK_COMM_NB];
extern mavlink_system_t mavlink_system;


/**
 * @brief Initialize the communication stack
 *
 * This function has to be called before using commParseBuffer() to initialize the different status registers.
 *
 * @return Will initialize the different buffers and status registers.
 */
static void mavlink_parse_state_initialize(mavlink_status_t* initStatus)
{
	if ((initStatus->parse_state <= MAVLINK_PARSE_STATE_UNINIT) || (initStatus->parse_state > MAVLINK_PARSE_STATE_GOT_CRC1))
	{
		initStatus->ck_a = 0;
		initStatus->ck_b = 0;
		initStatus->msg_received = 0;
		initStatus->buffer_overrun = 0;
		initStatus->parse_error = 0;
		initStatus->parse_state = MAVLINK_PARSE_STATE_UNINIT;
		initStatus->packet_idx = 0;
		initStatus->packet_rx_drop_count = 0;
		initStatus->packet_rx_success_count = 0;
		initStatus->current_rx_seq = 0;
		initStatus->current_tx_seq = 0;
	}
}

static inline mavlink_status_t* mavlink_get_channel_status(uint8_t chan)
{

	return &m_mavlink_status[chan];
}

/**
 * @brief Finalize a MAVLink message with MAVLINK_COMM_0 as default channel
 *
 * This function calculates the checksum and sets length and aircraft id correctly.
 * It assumes that the message id and the payload are already correctly set. 
 *
 * @warning This function implicitely assumes the message is sent over channel zero.
 *          if the message is sent over a different channel it will reach the receiver
 *          without error, BUT the sequence number might be wrong due to the wrong
 *          channel sequence counter. This will result is wrongly reported excessive
 *          packet loss. Please use @see mavlink_{pack|encode}_headerless and then
 *          @see mavlink_finalize_message_chan before sending for a correct channel
 *          assignment. Please note that the mavlink_msg_xxx_pack and encode functions
 *          assign channel zero as default and thus induce possible loss counter errors.\
 *          They have been left to ensure code compatibility.
 *
 * @see mavlink_finalize_message_chan
 * @param msg Message to finalize
 * @param system_id Id of the sending (this) system, 1-127
 * @param length Message length, usually just the counter incremented while packing the message
 */
static inline uint16_t mavlink_finalize_message(mavlink_message_t* msg, uint8_t system_id, uint8_t component_id, uint16_t length)
{
	// This code part is the same for all messages;
	uint8_t key;
	msg->len = length;
	msg->sysid = system_id;
	msg->compid = component_id;
	// One sequence number per component
	msg->seq = mavlink_get_channel_status(MAVLINK_COMM_0)->current_tx_seq;
        mavlink_get_channel_status(MAVLINK_COMM_0)->current_tx_seq = mavlink_get_channel_status(MAVLINK_COMM_0)->current_tx_seq+1;
	msg->ck = crc_calculate_msg(msg, length + MAVLINK_CORE_HEADER_LEN);
	key = MAVLINK_CONST_READ( mavlink_msg_keys[msg->msgid] );
	crc_accumulate( key, &msg->ck ); /// include key in X25 checksum

	return length + MAVLINK_NUM_NON_STX_PAYLOAD_BYTES;
}

/**
 * @brief Finalize a MAVLink message with channel assignment
 *
 * This function calculates the checksum and sets length and aircraft id correctly.
 * It assumes that the message id and the payload are already correctly set. This function
 * can also be used if the message header has already been written before (as in mavlink_msg_xxx_pack
 * instead of mavlink_msg_xxx_pack_headerless), it just introduces little extra overhead.
 *
 * @param msg Message to finalize
 * @param system_id Id of the sending (this) system, 1-127
 * @param length Message length, usually just the counter incremented while packing the message
 */
static inline uint16_t mavlink_finalize_message_chan(mavlink_message_t* msg, uint8_t system_id, uint8_t component_id, uint8_t chan, uint16_t length)
{
	// This code part is the same for all messages;
	uint8_t key;
	msg->len = length;
	msg->sysid = system_id;
	msg->compid = component_id;
	// One sequence number per component
	msg->seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = mavlink_get_channel_status(chan)->current_tx_seq+1;
	msg->ck = crc_calculate_msg(msg, length + MAVLINK_CORE_HEADER_LEN);
	key = MAVLINK_CONST_READ( mavlink_msg_keys[msg->msgid] );
	crc_accumulate( key, &msg->ck ); /// include key in X25 checksum

	return length + MAVLINK_NUM_NON_STX_PAYLOAD_BYTES;
}

/**
 * @brief Pack a message to send it over a serial byte stream
 */
static inline uint16_t mavlink_msg_to_send_buffer(uint8_t* buffer, const mavlink_message_t* msg)
{
	*(buffer+0) = MAVLINK_STX; ///< Start transmit
//	memcpy((buffer+1), msg, msg->len + MAVLINK_CORE_HEADER_LEN); ///< Core header plus payload
    memcpy((buffer+1), &msg->len, MAVLINK_CORE_HEADER_LEN); ///< Core header
    memcpy((buffer+1+MAVLINK_CORE_HEADER_LEN), &msg->payload[0], msg->len); ///< payload
    *(buffer + msg->len + MAVLINK_CORE_HEADER_LEN + 1) = msg->ck_a;
	*(buffer + msg->len + MAVLINK_CORE_HEADER_LEN + 2) = msg->ck_b;
	return msg->len + MAVLINK_NUM_NON_PAYLOAD_BYTES;
//	return 0;
}

/**
 * @brief Get the required buffer size for this message
 */
static inline uint16_t mavlink_msg_get_send_buffer_length(const mavlink_message_t* msg)
{
	return msg->len + MAVLINK_NUM_NON_PAYLOAD_BYTES;
}

union checksum_ {
	uint16_t s;
	uint8_t c[2];
};

static inline void mavlink_start_checksum(mavlink_message_t* msg)
{
	crc_init(&msg->ck);
}

static inline void mavlink_update_checksum(mavlink_message_t* msg, uint8_t c)
{
	crc_accumulate(c, &msg->ck);
}

/**
 * This is a convenience function which handles the complete MAVLink parsing.
 * the function will parse one byte at a time and return the complete packet once
 * it could be successfully decoded. Checksum and other failures will be silently
 * ignored.
 *
 * @param chan     ID of the current channel. This allows to parse different channels with this function.
 *                 a channel is not a physical message channel like a serial port, but a logic partition of
 *                 the communication streams in this case. COMM_NB is the limit for the number of channels
 *                 on MCU (e.g. ARM7), while COMM_NB_HIGH is the limit for the number of channels in Linux/Windows
 * @param c        The char to barse
 *
 * @param returnMsg NULL if no message could be decoded, the message data else
 * @return 0 if no message could be decoded, 1 else
 *
 * A typical use scenario of this function call is:
 *
 * @code
 * #include <inttypes.h> // For fixed-width uint8_t type
 *
 * mavlink_message_t msg;
 * int chan = 0;
 *
 *
 * while(serial.bytesAvailable > 0)
 * {
 *   uint8_t byte = serial.getNextByte();
 *   if (mavlink_parse_char(chan, byte, &msg))
 *     {
 *     printf("Received message with ID %d, sequence: %d from component %d of system %d", msg.msgid, msg.seq, msg.compid, msg.sysid);
 *     }
 * }
 *
 *
 * @endcode
 */
#ifdef MAVLINK_STATIC_BUFFER
static inline mavlink_message_t* mavlink_parse_char(uint8_t chan, uint8_t c, mavlink_message_t* r_message, mavlink_status_t* r_mavlink_status)
#else
static inline int16_t mavlink_parse_char(uint8_t chan, uint8_t c, mavlink_message_t* r_message, mavlink_status_t* r_mavlink_status)
#endif
{
	// Initializes only once, values keep unchanged after first initialization
	mavlink_parse_state_initialize(mavlink_get_channel_status(chan));

	mavlink_message_t* rxmsg = &m_mavlink_message[chan]; ///< The currently decoded message
	mavlink_status_t* status = mavlink_get_channel_status(chan); ///< The current decode status
	int bufferIndex = 0;

	status->msg_received = 0;

	switch (status->parse_state)
	{
	case MAVLINK_PARSE_STATE_UNINIT:
	case MAVLINK_PARSE_STATE_IDLE:
		if (c == MAVLINK_STX)
		{
			status->parse_state = MAVLINK_PARSE_STATE_GOT_STX;
			mavlink_start_checksum(rxmsg);
		}
		break;

	case MAVLINK_PARSE_STATE_GOT_STX:
		if (status->msg_received)
		{
			status->buffer_overrun++;
			status->parse_error++;
			status->msg_received = 0;
			status->parse_state = MAVLINK_PARSE_STATE_IDLE;
		}
		else
		{
			// NOT counting STX, LENGTH, SEQ, SYSID, COMPID, MSGID, CRC1 and CRC2
			rxmsg->len = c;
			status->packet_idx = 0;
			mavlink_update_checksum(rxmsg, c);
			status->parse_state = MAVLINK_PARSE_STATE_GOT_LENGTH;
		}
		break;

	case MAVLINK_PARSE_STATE_GOT_LENGTH:
		rxmsg->seq = c;
		mavlink_update_checksum(rxmsg, c);
		status->parse_state = MAVLINK_PARSE_STATE_GOT_SEQ;
		break;

	case MAVLINK_PARSE_STATE_GOT_SEQ:
		rxmsg->sysid = c;
		mavlink_update_checksum(rxmsg, c);
		status->parse_state = MAVLINK_PARSE_STATE_GOT_SYSID;
		break;

	case MAVLINK_PARSE_STATE_GOT_SYSID:
		rxmsg->compid = c;
		mavlink_update_checksum(rxmsg, c);
		status->parse_state = MAVLINK_PARSE_STATE_GOT_COMPID;
		break;

	case MAVLINK_PARSE_STATE_GOT_COMPID:
		rxmsg->msgid = c;
		mavlink_update_checksum(rxmsg, c);
#ifdef MAVLINK_CHECK_LENGTH
		if (rxmsg->len != MAVLINK_CONST_READ( mavlink_msg_lengths[c] ) )
		{
			status->parse_state = MAVLINK_PARSE_STATE_IDLE; // abort, not going to understand it anyway
			break;
		} else ;
#endif
		if (rxmsg->len == 0)
		{
			status->parse_state = MAVLINK_PARSE_STATE_GOT_PAYLOAD;
		}
		else
		{
			status->parse_state = MAVLINK_PARSE_STATE_GOT_MSGID;
		}
		break;

	case MAVLINK_PARSE_STATE_GOT_MSGID:
		rxmsg->payload[status->packet_idx++] = c;
		mavlink_update_checksum(rxmsg, c);
		if (status->packet_idx == rxmsg->len)
		{
			status->parse_state = MAVLINK_PARSE_STATE_GOT_PAYLOAD;
			mavlink_update_checksum(rxmsg, 
				MAVLINK_CONST_READ( mavlink_msg_keys[rxmsg->msgid] ));
		}
		break;

	case MAVLINK_PARSE_STATE_GOT_PAYLOAD:
		if (c != rxmsg->ck_a)
		{
			// Check first checksum byte
			status->parse_error++;
			status->msg_received = 0;
			status->parse_state = MAVLINK_PARSE_STATE_IDLE;
			if (c == MAVLINK_STX)
			{
				status->parse_state = MAVLINK_PARSE_STATE_GOT_STX;
				mavlink_start_checksum(rxmsg);
			}
		}
		else
		{
			status->parse_state = MAVLINK_PARSE_STATE_GOT_CRC1;
		}
		break;

	case MAVLINK_PARSE_STATE_GOT_CRC1:
		if (c != rxmsg->ck_b)
		{// Check second checksum byte
			status->parse_error++;
			status->msg_received = 0;
			status->parse_state = MAVLINK_PARSE_STATE_IDLE;
			if (c == MAVLINK_STX)
			{
				status->parse_state = MAVLINK_PARSE_STATE_GOT_STX;
				mavlink_start_checksum(rxmsg);
			}
		}
		else
		{
			// Successfully got message
			status->msg_received = 1;
			status->parse_state = MAVLINK_PARSE_STATE_IDLE;
			if ( r_message != NULL )
				memcpy(r_message, rxmsg, sizeof(mavlink_message_t));
			else ;
		}
		break;
	}

	bufferIndex++;
	// If a message has been sucessfully decoded, check index
	if (status->msg_received == 1)
	{
		//while(status->current_seq != rxmsg->seq)
		//{
		//	status->packet_rx_drop_count++;
		//               status->current_seq++;
		//}
		status->current_rx_seq = rxmsg->seq;
		// Initial condition: If no packet has been received so far, drop count is undefined
		if (status->packet_rx_success_count == 0) status->packet_rx_drop_count = 0;
		// Count this packet as received
		status->packet_rx_success_count++;
	}

	r_mavlink_status->current_rx_seq = status->current_rx_seq+1;
	r_mavlink_status->packet_rx_success_count = status->packet_rx_success_count;
	r_mavlink_status->packet_rx_drop_count = status->parse_error;
	status->parse_error = 0;
#ifdef MAVLINK_STATIC_BUFFER
	if (status->msg_received == 1)
	{
		if ( r_message != NULL )
			return r_message;
		else return rxmsg;
	} else return NULL;
#else
	if (status->msg_received == 1)
		return 1;
	else return 0;
#endif
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

// To make MAVLink work on your MCU, define a similar function

/*

#include "mavlink_types.h"

void comm_send_ch(mavlink_channel_t chan, uint8_t ch)
{
    if (chan == MAVLINK_COMM_0)
    {
        uart0_transmit(ch);
    }
    if (chan == MAVLINK_COMM_1)
    {
    	uart1_transmit(ch);
    }
}


static inline void mavlink_send_msg(mavlink_channel_t chan, mavlink_message_t* msg)
{
	// ARM7 MCU board implementation
	// Create pointer on message struct
	// Send STX
	comm_send_ch(chan, MAVLINK_STX);
	comm_send_ch(chan, msg->len);
	comm_send_ch(chan, msg->seq);
	comm_send_ch(chan, msg->sysid);
	comm_send_ch(chan, msg->compid);
	comm_send_ch(chan, msg->msgid);
	for(uint16_t i = 0; i < msg->len; i++)
	{
		comm_send_ch(chan, msg->payload[i]);
	}
	comm_send_ch(chan, msg->ck_a);
	comm_send_ch(chan, msg->ck_b);
}

static inline void mavlink_send_mem(mavlink_channel_t chan, (uint8_t *)mem, uint8_t num)
{
	// ARM7 MCU board implementation
	// Create pointer on message struct
	// Send STX
	for(uint16_t i = 0; i < num; i++)
	{
		comm_send_ch( chan, mem[i] );
	}
}
 */
static inline void mavlink_send_uart(mavlink_channel_t chan, mavlink_message_t* msg);
static inline void mavlink_send_mem(mavlink_channel_t chan, uint8_t *mem, uint16_t num);
#define mavlink_send_msg( a, b ) mavlink_send_uart( a, b )
#endif

#endif /* _MAVLINK_PROTOCOL_H_ */
