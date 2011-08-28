#ifndef  _MAVLINK_PROTOCOL_H_
#define  _MAVLINK_PROTOCOL_H_

#include "string.h"
#include "mavlink_types.h"

/* 
   If you want MAVLink on a system that is native big-endian,
   you need to define NATIVE_BIG_ENDIAN
*/
#ifdef NATIVE_BIG_ENDIAN
# define MAVLINK_NEED_BYTE_SWAP (MAVLINK_ENDIAN == MAVLINK_LITTLE_ENDIAN)
#else
# define MAVLINK_NEED_BYTE_SWAP (MAVLINK_ENDIAN != MAVLINK_LITTLE_ENDIAN)
#endif

#ifndef MAVLINK_STACK_BUFFER
#define MAVLINK_STACK_BUFFER 0
#endif

#ifndef MAVLINK_ALIGNED_FIELDS
#define MAVLINK_ALIGNED_FIELDS (MAVLINK_ENDIAN == MAVLINK_LITTLE_ENDIAN)
#endif

#ifndef MAVLINK_ASSERT
#define MAVLINK_ASSERT(x)
#endif

#ifdef MAVLINK_SEPARATE_HELPERS
#define MAVLINK_HELPER
#else
#define MAVLINK_HELPER static inline
#include "mavlink_helpers.h"
#endif // MAVLINK_SEPARATE_HELPERS

/* always include the prototypes to ensure we don't get out of sync */
MAVLINK_HELPER mavlink_status_t* mavlink_get_channel_status(uint8_t chan);
MAVLINK_HELPER uint16_t mavlink_finalize_message_chan(mavlink_message_t* msg, uint8_t system_id, uint8_t component_id, 
						      uint8_t chan, uint16_t length, uint8_t crc_extra);
MAVLINK_HELPER uint16_t mavlink_finalize_message(mavlink_message_t* msg, uint8_t system_id, uint8_t component_id, 
						 uint16_t length, uint8_t crc_extra);
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
MAVLINK_HELPER void mavlink_finalize_message_chan_send(mavlink_message_t* msg,
						       mavlink_channel_t chan, uint16_t length, uint8_t crc_extra);
#endif
MAVLINK_HELPER uint16_t mavlink_msg_to_send_buffer(uint8_t *buffer, const mavlink_message_t *msg);
MAVLINK_HELPER void mavlink_start_checksum(mavlink_message_t* msg);
MAVLINK_HELPER void mavlink_update_checksum(mavlink_message_t* msg, uint8_t c);
MAVLINK_HELPER uint8_t mavlink_parse_char(uint8_t chan, uint8_t c, mavlink_message_t* r_message, mavlink_status_t* r_mavlink_status);
MAVLINK_HELPER uint8_t put_bitfield_n_by_index(int32_t b, uint8_t bits, uint8_t packet_index, uint8_t bit_index, 
					       uint8_t* r_bit_index, uint8_t* buffer);
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
MAVLINK_HELPER void mavlink_send_uart(mavlink_channel_t chan, mavlink_message_t* msg);
#endif

/**
 * @brief Get the required buffer size for this message
 */
static inline uint16_t mavlink_msg_get_send_buffer_length(const mavlink_message_t* msg)
{
	return msg->len + MAVLINK_NUM_NON_PAYLOAD_BYTES;
}

typedef union {
	uint8_t b[2];
	uint16_t i;
} generic_16bit;

typedef union {
	uint8_t b[4];
	uint32_t i;
	float f;
} generic_32bit;

typedef union {
	uint8_t b[8];
	uint64_t i;
	double d;
} generic_64bit;

/**
 * @brief Place an unsigned byte into the buffer
 *
 * @param b the byte to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_uint8_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, uint8_t b)
{
	msg->payload.u8[wire_offset] = b;
}

/**
 * @brief Place a signed byte into the buffer
 *
 * @param b the byte to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_int8_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, int8_t b)
{
	msg->payload.i8[wire_offset] = b;
}

/**
 * @brief Place a char into the buffer
 *
 * @param b the byte to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_char_by_index(mavlink_message_t *msg, uint8_t wire_offset, char b)
{
	msg->payload.c[wire_offset] = b;
}

/**
 * @brief Place two unsigned bytes into the buffer
 *
 * @param b the bytes to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_uint16_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, uint16_t b)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_16bit g;
	g.i = b;
	msg->payload.u8[wire_offset+0] = g.b[1];
	msg->payload.u8[wire_offset+1] = g.b[0];
#else
	msg->payload.u16[wire_offset/2] = b;
#endif
}

/**
 * @brief Place two signed bytes into the buffer
 *
 * @param b the bytes to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_int16_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, int16_t b)
{
#if MAVLINK_NEED_BYTE_SWAP
	put_uint16_t_by_index(msg, wire_offset, b);
#else
	msg->payload.i16[wire_offset/2] = b;
#endif
}

/**
 * @brief Place four unsigned bytes into the buffer
 *
 * @param b the bytes to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_uint32_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, uint32_t b)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_32bit g;
	g.i = b;
	msg->payload.u8[wire_offset+0] = g.b[3];
	msg->payload.u8[wire_offset+1] = g.b[2];
	msg->payload.u8[wire_offset+2] = g.b[1];
	msg->payload.u8[wire_offset+3] = g.b[0];
#else
	msg->payload.u32[wire_offset/4] = b;
#endif
}

/**
 * @brief Place four signed bytes into the buffer
 *
 * @param b the bytes to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_int32_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, int32_t b)
{
#if MAVLINK_NEED_BYTE_SWAP
	put_uint32_t_by_index(msg, wire_offset, b);
#else
	msg->payload.i32[wire_offset/4] = b;
#endif
}

/**
 * @brief Place four unsigned bytes into the buffer
 *
 * @param b the bytes to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_uint64_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, uint64_t b)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_64bit r;
	r.i = b;
	msg->payload.u8[wire_offset+0] = r.b[7];
	msg->payload.u8[wire_offset+1] = r.b[6];
	msg->payload.u8[wire_offset+2] = r.b[5];
	msg->payload.u8[wire_offset+3] = r.b[4];
	msg->payload.u8[wire_offset+4] = r.b[3];
	msg->payload.u8[wire_offset+5] = r.b[2];
	msg->payload.u8[wire_offset+6] = r.b[1];
	msg->payload.u8[wire_offset+7] = r.b[0];
#else
	msg->payload.u64[wire_offset/8] = b;
#endif
}

/**
 * @brief Place four signed bytes into the buffer
 *
 * @param b the bytes to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_int64_t_by_index(mavlink_message_t *msg, uint8_t wire_offset, int64_t b)
{
#if MAVLINK_NEED_BYTE_SWAP
	put_uint64_t_by_index(msg, wire_offset, b);
#else
	msg->payload.i64[wire_offset/8] = b;
#endif
}

/**
 * @brief Place a float into the buffer
 *
 * @param b the float to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_float_by_index(mavlink_message_t *msg, uint8_t wire_offset, float b)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_32bit g;
	g.f = b;
	put_uint32_t_by_index(msg, wire_offset, g.i);
#else
	msg->payload.f[wire_offset/4] = b;
#endif
}

/**
 * @brief Place a double into the buffer
 *
 * @param b the double to add
 * @param wire_offset the position in the packet
 * @param buffer the packet buffer
 */
static inline void put_double_by_index(mavlink_message_t *msg, uint8_t wire_offset, double b)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_64bit g;
	g.d = b;
	put_uint64_t_by_index(msg, wire_offset, g.i);
#else
	msg->payload.d[wire_offset/8] = b;
#endif
}

/*
 * Place a char array into a buffer
 */
static inline void put_char_array_by_index(mavlink_message_t *msg, uint8_t wire_offset, const char *b, uint8_t array_length)
{
	memcpy(&msg->payload.c[wire_offset], b, array_length);
}

/*
 * Place a uint8_t array into a buffer
 */
static inline void put_uint8_t_array_by_index(mavlink_message_t *msg, uint8_t wire_offset, const uint8_t *b, uint8_t array_length)
{
	memcpy(&msg->payload.u8[wire_offset], b, array_length);
}

/*
 * Place a int8_t array into a buffer
 */
static inline void put_int8_t_array_by_index(mavlink_message_t *msg, uint8_t wire_offset, const int8_t *b, uint8_t array_length)
{
	memcpy(&msg->payload.i8[wire_offset], b, array_length);
}

#if MAVLINK_NEED_BYTE_SWAP
#define PUT_ARRAY_BY_INDEX(TYPE, V) \
static inline void put_ ## TYPE ##_array_by_index(mavlink_message_t *msg, uint8_t wire_offset, const TYPE *b, uint8_t array_length) \
{ \
	uint16_t i; \
	for (i=0; i<array_length; i++) { \
		put_## TYPE ##_by_index(msg, wire_offset+(i*sizeof(b[0])), b[i]); \
	} \
}
#else
#define PUT_ARRAY_BY_INDEX(TYPE, V)					\
static inline void put_ ## TYPE ##_array_by_index(mavlink_message_t *msg, uint8_t wire_offset, const TYPE *b, uint8_t array_length) \
{ \
	memcpy(&msg->payload.V[wire_offset/sizeof(TYPE)], b, array_length*sizeof(TYPE)); \
}
#endif

PUT_ARRAY_BY_INDEX(uint16_t, u16)
PUT_ARRAY_BY_INDEX(uint32_t, u32)
PUT_ARRAY_BY_INDEX(uint64_t, u64)
PUT_ARRAY_BY_INDEX(int16_t,  i16)
PUT_ARRAY_BY_INDEX(int32_t,  i32)
PUT_ARRAY_BY_INDEX(int64_t,  i64)
PUT_ARRAY_BY_INDEX(float,    f)
PUT_ARRAY_BY_INDEX(double,   d)

#define MAVLINK_MSG_RETURN_char(msg, wire_offset) msg->payload.c[wire_offset]
#define MAVLINK_MSG_RETURN_int8_t(msg, wire_offset) msg->payload.i8[wire_offset]
#define MAVLINK_MSG_RETURN_uint8_t(msg, wire_offset) msg->payload.u8[wire_offset]

static inline uint16_t MAVLINK_MSG_RETURN_uint16_t(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_16bit r;
	r.b[1] = msg->payload.u8[wire_offset+0];
	r.b[0] = msg->payload.u8[wire_offset+1];
	return r.i;
#else
	return msg->payload.u16[wire_offset/2];
#endif
}

static inline int16_t MAVLINK_MSG_RETURN_int16_t(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	return (int16_t)MAVLINK_MSG_RETURN_uint16_t(msg, wire_offset);
#else
	return msg->payload.i16[wire_offset/2];
#endif
}

static inline uint32_t MAVLINK_MSG_RETURN_uint32_t(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_32bit r;
	r.b[3] = msg->payload.u8[wire_offset+0];
	r.b[2] = msg->payload.u8[wire_offset+1];
	r.b[1] = msg->payload.u8[wire_offset+2];
	r.b[0] = msg->payload.u8[wire_offset+3];
	return r.i;
#else
	return msg->payload.u32[wire_offset/4];
#endif
}

static inline int32_t MAVLINK_MSG_RETURN_int32_t(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	return (int32_t)MAVLINK_MSG_RETURN_uint32_t(msg, wire_offset);
#else
	return msg->payload.i32[wire_offset/4];
#endif
}

static inline uint64_t MAVLINK_MSG_RETURN_uint64_t(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_64bit r;
	r.b[7] = msg->payload.u8[wire_offset+0];
	r.b[6] = msg->payload.u8[wire_offset+1];
	r.b[5] = msg->payload.u8[wire_offset+2];
	r.b[4] = msg->payload.u8[wire_offset+3];
	r.b[3] = msg->payload.u8[wire_offset+4];
	r.b[2] = msg->payload.u8[wire_offset+5];
	r.b[1] = msg->payload.u8[wire_offset+6];
	r.b[0] = msg->payload.u8[wire_offset+7];
	return r.i;
#else
	return msg->payload.u64[wire_offset/8];
#endif
}

static inline int64_t MAVLINK_MSG_RETURN_int64_t(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	return (int64_t)MAVLINK_MSG_RETURN_uint64_t(msg, wire_offset);
#else
	return msg->payload.i64[wire_offset/8];
#endif
}

static inline float MAVLINK_MSG_RETURN_float(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_32bit r;
	r.b[3] = msg->payload.u8[wire_offset+0];
	r.b[2] = msg->payload.u8[wire_offset+1];
	r.b[1] = msg->payload.u8[wire_offset+2];
	r.b[0] = msg->payload.u8[wire_offset+3];
	return r.f;
#else
	return msg->payload.f[wire_offset/4];
#endif
}

static inline double MAVLINK_MSG_RETURN_double(const mavlink_message_t *msg, uint8_t wire_offset)
{
#if MAVLINK_NEED_BYTE_SWAP
	generic_64bit r;
	r.b[7] = msg->payload.u8[wire_offset+0];
	r.b[6] = msg->payload.u8[wire_offset+1];
	r.b[5] = msg->payload.u8[wire_offset+2];
	r.b[4] = msg->payload.u8[wire_offset+3];
	r.b[3] = msg->payload.u8[wire_offset+4];
	r.b[2] = msg->payload.u8[wire_offset+5];
	r.b[1] = msg->payload.u8[wire_offset+6];
	r.b[0] = msg->payload.u8[wire_offset+7];
	return r.d;
#else
	return msg->payload.d[wire_offset/8];
#endif
}


static inline uint16_t MAVLINK_MSG_RETURN_char_array(const mavlink_message_t *msg, char *value, 
						     uint8_t array_length, uint8_t wire_offset)
{
	memcpy(value, &msg->payload.c[wire_offset], array_length);
	return array_length;
}

static inline uint16_t MAVLINK_MSG_RETURN_uint8_t_array(const mavlink_message_t *msg, uint8_t *value, 
							uint8_t array_length, uint8_t wire_offset)
{
	memcpy(value, &msg->payload.u8[wire_offset], array_length);
	return array_length;
}

static inline uint16_t MAVLINK_MSG_RETURN_int8_t_array(const mavlink_message_t *msg, int8_t *value, 
						       uint8_t array_length, uint8_t wire_offset)
{
	memcpy(value, &msg->payload.i8[wire_offset], array_length);
	return array_length;
}

#if MAVLINK_NEED_BYTE_SWAP
#define RETURN_ARRAY_BY_INDEX(TYPE, V) \
static inline uint16_t MAVLINK_MSG_RETURN_## TYPE ##_array(const mavlink_message_t *msg, TYPE *value, \
							 uint8_t array_length, uint8_t wire_offset) \
{ \
	uint16_t i; \
	for (i=0; i<array_length; i++) { \
		value[i] = MAVLINK_MSG_RETURN_## TYPE (msg, wire_offset+(i*sizeof(value[0]))); \
	} \
	return array_length*sizeof(value[0]); \
}
#else
#define RETURN_ARRAY_BY_INDEX(TYPE, V)					\
static inline uint16_t MAVLINK_MSG_RETURN_## TYPE ##_array(const mavlink_message_t *msg, TYPE *value, \
							 uint8_t array_length, uint8_t wire_offset) \
{ \
	memcpy(value, &msg->payload.V[wire_offset/sizeof(TYPE)], array_length*sizeof(TYPE)); \
	return array_length*sizeof(value[0]); \
}
#endif

RETURN_ARRAY_BY_INDEX(uint16_t, u16)
RETURN_ARRAY_BY_INDEX(uint32_t, u32)
RETURN_ARRAY_BY_INDEX(uint64_t, u64)
RETURN_ARRAY_BY_INDEX(int16_t,  i16)
RETURN_ARRAY_BY_INDEX(int32_t,  i32)
RETURN_ARRAY_BY_INDEX(int64_t,  i64)
RETURN_ARRAY_BY_INDEX(float,    f)
RETURN_ARRAY_BY_INDEX(double,   d)

#if MAVLINK_STACK_BUFFER == 1
/*
  we need to be able to declare a mavlink_message_t on the stack for
  _send() calls. This can either be done by declaring a whole message,
  or by declaring a buffer of just the right size for this specific
  message type, and casting a mavlink_message_t structure to it. Using
  the cast is a violation of the strict aliasing rules for C, so only
  use it if you know its OK for your architecture  
 */
#define MAVLINK_ALIGNED_MESSAGE(msg, length) \
uint64_t  _buf[(MAVLINK_NUM_CHECKSUM_BYTES+MAVLINK_NUM_NON_PAYLOAD_BYTES+(length)+7)/8]; \
mavlink_message_t *msg = (mavlink_message_t *)&_buf[0]
#else
#define MAVLINK_ALIGNED_MESSAGE(msg, length) \
	mavlink_message_t _msg;	\
	mavlink_message_t *msg = &_msg
#endif // MAVLINK_STACK_BUFFER

#endif // _MAVLINK_PROTOCOL_H_
