/*
 * @brief LPC USB serial I/O interface definition
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#ifndef __LPCUSBSIO_H
#define __LPCUSBSIO_H

#include <stdint.h>

#if defined(LPCUSBSIO_EXPORTS)
#define LPCUSBSIO_API __declspec(dllexport)
#elif defined(LPCUSBSIO_IMPORTS)
#define LPCUSBSIO_API __declspec(dllimport)
#else
#define LPCUSBSIO_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup LPCUSBSIO_I2C_API LPC USB serial I/O (LPCUSBSIO) I2C API interface
 * <b>API description</b><br>
 * The LPCUSBSIO-I2C APIs can be divided into two broad sets. The first set consists of
 * five control APIs and the second set consists of two data transferring APIs. On error
 * most APIs return an LPCUSBSIO_ERR_T code. Application code can call I2C_Error() routine
 * to get user presentable uni-code string corresponding to the last error.
 * <br>
 * The current version of LPCUSBSIO allows communicating with I2C slave devices only.
 *
 * @{
 */
#define MAX_WRITE_LENGTH            52 /* Shall be a multiple of 4 */
#define MAX_READ_LENGTH             (HID_I2C_PACKET_SZ - sizeof(HID_I2C_IN_REPORT_T))

/** NXP USB-IF vendor ID. */
#define LPCUSBSIO_VID                       0x1FC9
/** USB-IF product ID for LPCUSBSIO devices. */
#define LPCUSBSIO_PID                       0x0088
/** Read time-out value in milliseconds used by the library. If a response is not received
 *
 */
//AL : change default timeout 500 ms to INIFNITE timeout
#define LPCUSBSIO_READ_TMO                  -1

/** I2C_CFG_OPTIONS Configuration options flags used by I2C_PORTCONFIG_T
 * @{
 */
/** Generate start condition before transmitting */
#define I2C_CONFIG_OPTIONS_FORCE_INIT      0x00000001

/** @} */

/** I2C_IO_OPTIONS Options to I2C_DeviceWrite & I2C_DeviceRead routines
 * @{
 */
/** Generate start condition before transmitting */
#define I2C_TRANSFER_OPTIONS_START_BIT      0x0001

/** Generate stop condition at the end of transfer */
#define I2C_TRANSFER_OPTIONS_STOP_BIT       0x0002

/** Continue transmitting data in bulk without caring about Ack or nAck from device if this bit is
 *  not set. If this bit is set then stop transmitting the data in the buffer when the device nAcks
 */
#define I2C_TRANSFER_OPTIONS_BREAK_ON_NACK  0x0004

/** lpcusbsio-I2C generates an ACKs for every byte read. Some I2C slaves require the I2C
   master to generate a nACK for the last data byte read. Setting this bit enables working with such
   I2C slaves */
#define I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE 0x0008

/* Setting this bit would mean that the address field should be ignored.
 * The address is either a part of the data or this is a special I2C
 * frame that doesn't require an address. For example when transferring a
 * frame greater than the USB_HID packet this option can be used.
 */
#define I2C_TRANSFER_OPTIONS_NO_ADDRESS     0x00000040

/** @} */

/** I2C_FAST_TRANSFER_OPTIONS I2C master faster transfer options
 * @{
 */

/** Ignore NACK during data transfer. By default transfer is aborted. */
#define I2C_FAST_XFER_OPTION_IGNORE_NACK     0x01
/** ACK last byte received. By default we NACK last byte we receive per I2C specification. */
#define I2C_FAST_XFER_OPTION_LAST_RX_ACK     0x02

/**
 * @}
 */

/******************************************************************************
*                                Type defines
******************************************************************************/
/** @brief Handle type */
typedef void *LPC_HANDLE;

/** @brief Error types returned by LPCUSBSIO APIs */
typedef enum LPCUSBSIO_ERR_t {
    /** All API return positive number for success */
    LPCUSBSIO_OK = 0,
    /** HID library error. */
    LPCUSBSIO_ERR_HID_LIB = -1,
    /** Handle passed to the function is invalid. */
    LPCUSBSIO_ERR_BAD_HANDLE = -2,

    /* Errors from hardware I2C interface*/
    /** Fatal error occurred */
    LPCUSBSIO_ERR_FATAL = -0x11,
    /** Transfer aborted due to NACK  */
    LPCUSBSIO_ERR_I2C_NAK = -0x12,
    /** Transfer aborted due to bus error  */
    LPCUSBSIO_ERR_I2C_BUS = -0x13,
    /** NAK received after SLA+W or SLA+R  */
    LPCUSBSIO_ERR_I2C_SLAVE_NAK = -0x14,
    /** I2C bus arbitration lost to other master  */
    LPCUSBSIO_ERR_I2C_ARBLOST = -0x15,

    /* Errors from firmware's HID-I2C bridge module */
    /** Transaction timed out */
    LPCUSBSIO_ERR_TIMEOUT = -0x20,
    /** Invalid HID_I2C Request or Request not supported in this version. */
    LPCUSBSIO_ERR_INVALID_CMD = -0x21,
    /** Invalid parameters are provided for the given Request. */
    LPCUSBSIO_ERR_INVALID_PARAM = -0x22,
    /** Partial transfer completed. */
    LPCUSBSIO_ERR_PARTIAL_DATA = -0x23,
    /** Request has been canceled*/
    LPCUSBSIO_REQ_CANCELED = -0x24,
} LPCUSBSIO_ERR_T;

/** @brief I2C clock rates */
typedef enum I2C_ClockRate_t {
    I2C_CLOCK_25KHZ_MODE     =  25000,        /*!< 25kb/sec */
    I2C_CLOCK_50KHZ_MODE     =  50000,        /*!< 50kb/sec */
    I2C_CLOCK_STANDARD_MODE  = 100000,        /*!< 100kb/sec */
    I2C_CLOCK_200KHZ_MODE    = 200000,        /*!< 200kb/sec */
    I2C_CLOCK_FAST_MODE      = 400000,        /*!< 400kb/sec */
    I2C_CLOCK_FAST_MODE_PLUS = 1000000,        /*!< 1000kb/sec */
} I2C_CLOCKRATE_T;

/** @brief Port configuration information */
typedef struct PortConfig_t {
    I2C_CLOCKRATE_T ClockRate;                                    /*!< I2C Clock speed */
    uint32_t          Options;                                    /*!< Configuration options */
} I2C_PORTCONFIG_T;

/** @brief Fast transfer parameter structure */
typedef struct FastXferParam_t {
    uint8_t txSz;                /*!< Number of bytes in transmit array,
                                   if 0 only receive transfer will be carried on */
    uint8_t rxSz;                /*!< Number of bytes to received,
                                   if 0 only transmission we be carried on */
    uint8_t options;            /*!< Fast transfer options */
    uint8_t slaveAddr;            /*!< 7-bit I2C Slave address */
    const uint8_t *txBuff;        /*!< Pointer to array of bytes to be transmitted */
    uint8_t *rxBuff;            /*!< Pointer memory where bytes received from I2C be stored */
} I2C_FAST_XFER_T;

//AL : Useless
/** @brief Get version string of the library.
 *
 * @param handle    : A device handle returned from I2C_OpenPort().
 *
 * @returns
 * This function returns a string containing the version of the library.
 * If the device handle passed is not NULL then the firmware version of
 * the connected device is appended to the string.
 */
//LPCUSBSIO_API const char *I2C_GetVersion(LPC_HANDLE handle);

/** @brief Get number I2C ports available on the LPC controller.
 *
 * This function gets the number of I2C ports that are available on the LPC controller.
 * The number of ports available in each of these chips is different.
 *
 * @returns
 * The number of ports available on the LPC controller.
 *
 */
LPCUSBSIO_API int I2C_GetNumPorts(void);

/** @brief Opens the indexed port.
 *
 * This function opens the indexed port and provides a handle to it. Valid values for
 * the index of port can be from 0 to the value obtained using I2C_GetNumPorts
 * – 1).
 *
 * @param index        : Index of the port to be opened.
 *
 * @returns
 * This function returns a handle to I2C port object on
 * success or NULL on failure.
 */
LPCUSBSIO_API LPC_HANDLE I2C_Open(uint32_t index);

/** @brief Initialize the port.
 *
 * This function initializes the port and the communication parameters associated
 * with it.
 *
 * @param handle    : Handle of the port.
 * @param config    : Pointer to I2C_PORTCONFIG_T structure. Members of
 * I2C_PORTCONFIG_T structure contains the values for I2C
 * master clock, latency timer and Options
 *
 * @returns
 * This function returns LPCUSBSIO_OK on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 */
LPCUSBSIO_API int32_t I2C_Init(LPC_HANDLE handle, I2C_PORTCONFIG_T *config);

/** @brief Closes a port.
 *
 * Closes a port and frees all resources that were used by it.
 *
 * @param handle    : Handle of the port.
 *
 * @returns
 * This function returns LPCUSBSIO_OK on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 *
 */
LPCUSBSIO_API int32_t I2C_Close(LPC_HANDLE handle);

/** @brief Reset I2C Controller.
 *
 *  @param handle    : A device handle returned from I2C_OpenPort().
 *
 *  @returns
 *  This function returns LPCUSBSIO_OK on success and negative error code on failure.
 *  Check @ref LPCUSBSIO_ERR_T for more details on error code.
 *
 */
LPCUSBSIO_API int32_t I2C_Reset(LPC_HANDLE handle);

/** @brief Get a string describing the last error which occurred.
 *
 * @param handle    : A device handle returned from I2C_OpenPort().
 *
 * @returns
 * This function returns a string containing the last error
 * which occurred or NULL if none has occurred.
 *
 */
LPCUSBSIO_API const wchar_t *I2C_Error(LPC_HANDLE handle, int32_t status);

/** @brief Read from an addressed I2C slave.
 *
 * This function reads the specified number of bytes from an addressed I2C slave.
 * The @a options parameter effects the transfers. Some example transfers are shown below :
 * - When I2C_TRANSFER_OPTIONS_START_BIT, I2C_TRANSFER_OPTIONS_STOP_BIT and
 * I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE are set.
 *
 *  <b> S Addr Rd [A] [rxBuff0] A [rxBuff1] A ...[rxBuffN] NA P </b>
 *
 * - If I2C_TRANSFER_OPTIONS_NO_ADDRESS is also set.
 *
 *    <b> S [rxBuff0] A [rxBuff1] A ...[rxBuffN] NA P </b>
 *
 * - if I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE is not set
 *
 *  <b> S Addr Rd [A] [rxBuff0] A [rxBuff1] A ...[rxBuffN] A P </b>
 *
 * - If I2C_TRANSFER_OPTIONS_STOP_BIT is not set.
 *
 *  <b> S Addr Rd [A] [rxBuff0] A [rxBuff1] A ...[rxBuffN] NA </b>
 *
 * @param handle        : Handle of the port.
 * @param deviceAddress    : Address of the I2C slave. This is a 7bit value and
 * it should not contain the data direction bit, i.e. the decimal
 * value passed should be always less than 128
 * @param buffer        : Pointer to the buffer where data is to be read
 * @param sizeToTransfer: Number of bytes to be read
 * @param options: This parameter specifies data transfer options. Check I2C_TRANSFER_OPTIONS_ macros.
 * @returns
 * This function returns number of bytes read on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 */
LPCUSBSIO_API int32_t I2C_DeviceRead(LPC_HANDLE handle,
                                     uint8_t deviceAddress,
                                     uint8_t *buffer,
                                     uint16_t sizeToTransfer,
                                     uint8_t options);

/** @brief Writes to the addressed I2C slave.
 *
 * This function writes the specified number of bytes to an addressed I2C slave.
 * The @a options parameter effects the transfers. Some example transfers are shown below :
 * - When I2C_TRANSFER_OPTIONS_START_BIT, I2C_TRANSFER_OPTIONS_STOP_BIT and
 * I2C_TRANSFER_OPTIONS_BREAK_ON_NACK are set.
 *
 *  <b> S Addr Wr[A] txBuff0[A] txBuff1[A] ... txBuffN[A] P </b>
 *
 *  - If I2C_TRANSFER_OPTIONS_NO_ADDRESS is also set.
 *
 *        <b> S txBuff0[A ] ... txBuffN[A] P </b>
 *
 *  - if I2C_TRANSFER_OPTIONS_BREAK_ON_NACK is not set
 *
 *      <b> S Addr Wr[A] txBuff0[A or NA] ... txBuffN[A or NA] P </b>
 *
 *  - If I2C_TRANSFER_OPTIONS_STOP_BIT is not set.
 *
 *      <b> S Addr Wr[A] txBuff0[A] txBuff1[A] ... txBuffN[A] </b>
 *
 * @param handle        : Handle of the port.
 * @param deviceAddress    : Address of the I2C slave. This is a 7bit value and
 * it should not contain the data direction bit, i.e. the decimal
 * value passed should be always less than 128
 * @param sizeToTransfer: Number of bytes to be written
 * @param buffer        : Pointer to the buffer where data is to be read
 * @param options        : This parameter specifies data transfer options. Check I2C_TRANSFER_OPTIONS_ macros.
 * @returns
 * This function returns number of bytes written on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 */
LPCUSBSIO_API int32_t I2C_DeviceWrite(LPC_HANDLE handle,
                                      uint8_t deviceAddress,
                                      uint8_t *buffer,
                                      uint16_t sizeToTransfer,
                                      uint8_t options);

/**@brief    Transmit and Receive data in master mode
 *
 * The parameter @a xfer should have its member @a slaveAddr initialized
 * to the 7 - Bit slave address to which the master will do the xfer, Bit0
 * to bit6 should have the address and Bit8 is ignored.During the transfer
 * no code(like event handler) must change the content of the memory
 * pointed to by @a xfer.The member of @a xfer, @a txBuff and @a txSz be
 * initialized to the memory from which the I2C must pick the data to be
 * transferred to slave and the number of bytes to send respectively, similarly
 * @a rxBuff and @a rxSz must have pointer to memory where data received
 * from slave be stored and the number of data to get from slave respectively.
 *
 * Following types of transfers are possible :
 * - Write-only transfer : When @a rxSz member of @a xfer is set to 0.
 *
 *     <b> S Addr Wr[A] txBuff0[A] txBuff1[A] ... txBuffN[A] P </b>
 *
 *  - If I2C_FAST_XFER_OPTION_IGNORE_NACK is set in @a options member
 *
 *     <b> S Addr Wr[A] txBuff0[A or NA] ... txBuffN[A or NA] P </b>
 *
 * - Read-only transfer : When @a txSz member of @a xfer is set to 0.
 *
 *      <b> S Addr Rd[A][rxBuff0] A[rxBuff1] A ...[rxBuffN] NA P </b>
 *
 *  - If I2C_FAST_XFER_OPTION_LAST_RX_ACK is set in @a options member
 *
 *      <b> S Addr Rd[A][rxBuff0] A[rxBuff1] A ...[rxBuffN] A P </b>
 *
 * - Read-Write transfer : When @a rxSz and @ txSz members of @a xfer are non - zero.
 *
 *      <b> S Addr Wr[A] txBuff0[A] txBuff1[A] ... txBuffN[A] <br>
 *          S Addr Rd[A][rxBuff0] A[rxBuff1] A ...[rxBuffN] NA P </b>
 *
 * @param handle    : Handle of the port.
 * @param    xfer : Pointer to a I2C_FAST_XFER_T structure.
 * @returns
 * This function returns number of bytes read on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 */
LPCUSBSIO_API int32_t I2C_FastXfer(LPC_HANDLE handle, I2C_FAST_XFER_T *xfer);



/** @brief Change the state of VEN GPIO
 *
 * This function change VEN GPIO State
 *
 * @param handle        : Handle of the port.
 * @param newValure        : New GPIO State
 * @returns
 * This function returns number of bytes written on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 */
LPCUSBSIO_API LPCUSBSIO_ERR_T GPIO_SetVENValue(LPC_HANDLE handle, uint8_t newValue);

/** @brief Change the state of Download GPIO
 *
 * This function change Download GPIO State
 *
 * @param handle        : Handle of the port.
 * @param newValure        : New GPIO State
 * @returns
 * This function returns number of bytes written on success and negative error code on failure.
 * Check @ref LPCUSBSIO_ERR_T for more details on error code.
 */
LPCUSBSIO_API LPCUSBSIO_ERR_T GPIO_SetDWLValue(LPC_HANDLE handle, uint8_t newValue);

LPCUSBSIO_API LPCUSBSIO_ERR_T I2C_CancelAllRequest(LPC_HANDLE handle);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*__LPCUSBSIO_H*/
