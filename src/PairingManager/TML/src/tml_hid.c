/*
*         Copyright (c), NXP Semiconductors Caen / France
*
*                     (C)NXP Semiconductors
*       All rights are reserved. Reproduction in whole or in part is
*      prohibited without the written consent of the copyright owner.
*  NXP reserves the right to make changes without notice at any time.
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
*particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
*                          arising from its use.
*/

#include <TML/inc/tool.h>
#include <TML/inc/tml.h>
#include "TML/inc/lpcusbsio.h"
#include "TML/inc/lpcusbsio_i2c.h"
#include "TML/inc/framework_Interface.h"
#include "TML/inc/framework_Timer.h"

#define I2C_ADD 0x28

static I2C_PORTCONFIG_T  lCfgParam;
static LPC_HANDLE mDriverHandle = NULL;
static bool isAborted;

static uint8_t tml_Tx(uint8_t *pBuff, uint16_t buffLen) {
    int lRes = 0x00;
    int j=0;

    do {
       lRes = I2C_DeviceWrite(mDriverHandle, I2C_ADD, pBuff+j, (buffLen-j>=MAX_WRITE_LENGTH)?MAX_WRITE_LENGTH:buffLen-j,
           I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT);
       if(lRes < 0) return 0;
       j += MAX_WRITE_LENGTH;
    } while (j <= buffLen);

    return 1;
}

static void tml_Rx(uint8_t *pBuff, uint16_t buffLen, uint16_t *pBytesRead) {
    int lRes = 0x00;
    lRes = I2C_DeviceRead( mDriverHandle, I2C_ADD, pBuff, 3, I2C_TRANSFER_OPTIONS_START_BIT | I2C_TRANSFER_OPTIONS_STOP_BIT | I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE);
    if (lRes > 0) *pBytesRead = pBuff[2] + 3;
    else *pBytesRead = 0;
}

static bool tml_Init(void) {
    int lRes = 0x00;

    if(NULL != mDriverHandle) return true;

    lRes = I2C_GetNumPorts();
    if (lRes < 0) return false;

    mDriverHandle = I2C_Open(0); 
    if(NULL == mDriverHandle) return false;

    memset(&lCfgParam, 0x00, sizeof(I2C_PORTCONFIG_T));
    lCfgParam.ClockRate = I2C_CLOCK_FAST_MODE;
    lRes = I2C_Init(mDriverHandle, &lCfgParam);
    if (lRes < 0){
        I2C_Close(mDriverHandle);
        mDriverHandle = NULL;
        return false;
    }
    return true;
}

static void tml_Reset(void) {
    uint8_t buffer[30];
    uint16_t nbBytes;

    if (mDriverHandle == NULL) return;

    I2C_Reset(mDriverHandle);

    GPIO_SetVENValue(mDriverHandle, 0x00);
    GPIO_SetDWLValue(mDriverHandle, 0x00);
    Sleep(10);    
    GPIO_SetVENValue(mDriverHandle, 0x01);

    /* Insure I2C fragmentation is enabled */
    tml_Tx((uint8_t*)"\x20\x00\x01\x01", 4);
    tml_Tx((uint8_t*)"\x20\x00\x01\x01", 4);
    tml_Rx(buffer, sizeof(buffer), &nbBytes);
    tml_Tx((uint8_t*)"\x20\x01\x00", 3);
    tml_Rx(buffer, sizeof(buffer), &nbBytes);
    tml_Tx((uint8_t*)"\x20\x02\x05\x01\xA0\x05\x01\x10", 8);
    tml_Rx(buffer, sizeof(buffer), &nbBytes);

    GPIO_SetVENValue(mDriverHandle, 0x00);
    Sleep(10);    
    GPIO_SetVENValue(mDriverHandle, 0x01);
}

int tml_hid_Connect(void) {
    if(!tml_Init()) return 1;
    tml_Reset();
    return 0;
}

void tml_hid_Disconnect(void){
    /*    I2C_Reset(mDriverHandle);
    I2C_CancelAllRequest(mDriverHandle);
    I2C_Close(mDriverHandle);
    Sleep(2000);*/
}

void tml_hid_Send(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytesSent){
    if(tml_Tx(pBuffer, BufferLen) == 0)
    {
        *pBytesSent = 0;
    }
    else
    {
        *pBytesSent = BufferLen;
    }
}

static void timeoutCb(void* pContext)
{
    uint8_t dummy[] = {0x20, 0x00, 0x00};
    I2C_CancelAllRequest(mDriverHandle);
    /* Send dummy frame to re-align reception */
    tml_Tx(dummy, sizeof(dummy));
    isAborted = true;
}

void tml_hid_Receive(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytes, uint16_t timeout){
    if (timeout == TIMEOUT_INFINITE) tml_Rx(pBuffer, BufferLen, pBytes);
    else {
        void *timer;
        uint32_t timerId;
        framework_TimerCreate(&timer);
        timerId = *(uint32_t*)timer;
        *pBytes = 0;
        isAborted = false;
        framework_TimerStart(&timerId, timeout, timeoutCb, NULL);
        tml_Rx(pBuffer, BufferLen, pBytes);
        if(isAborted) *pBytes = 0;
        framework_TimerStop(&timerId);
        framework_TimerDelete(&timerId);
    }
}

void tml_hid_Cancel(void)
{
    I2C_CancelAllRequest(mDriverHandle);
}
