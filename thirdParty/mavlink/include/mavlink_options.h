/** @file
 *	@brief MAVLink comm protocol option constants.
 *	@see http://qgroundcontrol.org/mavlink/
 *	Edited on Monday, August 8 2011
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ML_OPTIONS_H_
#define _ML_OPTIONS_H_


/**
 *
 *  Receive message length check option. On receive verify the length field
 *  as soon as the message ID field is received. Requires a 256 byte const
 *  table. Comment out the define to leave out the table and code to check it.
 *
 */
//#define MAVLINK_CHECK_LENGTH

/**
 *
 *  Receive message buffer option. This option should be used only when the
 *  side effects are understood but allows the underlying program access to
 *  the internal recieve buffer - eliminating the usual double buffering. It
 *  also REQUIRES changes in the return type of mavlink_parse_char so only
 *  enable if you make the changes required. Default DISABLED.
 *
 */
//#define MAVLINK_STATIC_BUFFER

/**
 *
 *  Receive message buffers option. This option defines how many msg buffers
 *  mavlink will define, and thereby how many links it can support. A default
 *  will be supplied if the symbol is not pre-defined, dependant on the make
 *  envionment. The default is 16 for a recognised OS envionment and 1
 *  otherwise.
 *
 */
#if !((defined MAVLINK_COMM_NB) | (MAVLINK_COMM_NB < 1))
#undef MAVLINK_COMM_NB
  #if (defined linux) | (defined __linux) | (defined  __MACH__) | (defined _WIN32) | (defined __APPLE__)
  #define MAVLINK_COMM_NB 16
  #else
  #define MAVLINK_COMM_NB 1
  #endif
#endif


/**
 *
 *  Data relization option. This option controls inclusion of the file 
 *  mavlink_data.h in the current compile unit - thus defining mavlink's 
 *  variables. Default is ON (not defined) because typically mavlink.h is only
 *  included once in a system but if it was used in two files there would
 *  be duplicate variables at link time. Normal practice would be to define
 *  this symbol outside of this file as defining it here will cause missing
 *  symbols at link time. In other words in the first file to include mavlink.h
 *  do not define this sybol, then define this symbol in all other files before
 *  including mavlink.h
 *
 */
//#define MAVLINK_NO_DATA
#ifdef MAVLINK_NO_DATA
  #undef MAVLINK_DATA
#else
  #define MAVLINK_DATA
#endif

/**
 *
 *  Custom data const data relization and access options. 
 *  This define is placed in the form 
 *  const uint8_t MAVLINK_CONST name[] = { ... }; 
 *  for the keys table and (if applicable) lengths table to tell the compiler 
 *  were to put the data. The access option is placed in the form
 *  variable = MAVLINK_CONST_READ( name[i] );
 *  in order to allow custom read function's or accessors.
 *  By default MAVLINK_CONST is defined as nothing and MAVLINK_CONST_READ as
 *  MAVLINK_CONST_READ( a ) a
 *  These symbols are only defined if not already defined allowing this file
 *  to remain unchanged while the actual definitions are maintained in external
 *  files.
 *
 */
#ifndef MAVLINK_CONST
#define MAVLINK_CONST
#endif
#ifndef MAVLINK_CONST_READ
#define MAVLINK_CONST_READ( a ) a
#endif


/**
 *
 *  Convience functions. These are all in one send functions that are very
 *  easy to use. Just define the symbol MAVLINK_USE_CONVENIENCE_FUNCTIONS.
 *  These functions also support a buffer check, to ensure there is enough
 *  space in your comm buffer that the function would not block - it could
 *  also be used as the basis of a MUTEX. This is implemented in the send
 *  function as a macro with two arguments, first the comm chan number and
 *  the message length in the form 
 *  MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_LEN )
 *  followed by the function code and then
 *  MAVLINK_BUFFER_CHECK_START
 *  Note that there are no terminators on these statements to allow for
 *  code nesting or other constructs. Default value for both is empty.
 *  A sugested implementation is shown below and the symbols will be defined
 *  only if they are not allready.
 *
 *  if ( serial_space( chan ) > len ) { // serial_space returns available space
 *  ..... code that creates message
 *  } 
 *
 *  #define MAVLINK_BUFFER_CHECK_START( c, l ) if ( serial_space( c ) > l ) {
 *  #define MAVLINK_BUFFER_CHECK_END }
 *
 */
//#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#ifndef MAVLINK_BUFFER_CHECK_START
#define MAVLINK_BUFFER_CHECK_START( c, l ) ;
#endif
#ifndef MAVLINK_BUFFER_CHECK_END
#define MAVLINK_BUFFER_CHECK_END ;
#endif

#endif /* _ML_OPTIONS_H_ */

#ifdef __cplusplus
}
#endif
