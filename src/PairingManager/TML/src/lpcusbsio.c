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

#include <string.h>
#include <stdlib.h>
#include "TML/inc/hidapi.h"
#include "TML/inc/lpcusbsio.h"
#include "TML/inc/lpcusbsio_i2c.h"
#include "TML/inc/framework_Interface.h"
#include "TML/inc/framework_Map.h"
#include "TML/inc/framework_Container.h"

/* ###############
    TODO LIST
    ----------
    - cpu_to _le conversion
    - Reset

 */
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define NUM_LIB_ERR_STRINGS         3
#define NUM_FW_ERR_STRINGS          6
#define NUM_BRIDGE_ERR_STRINGS      4

#define MAX_XFER_TX_LENGTH          (HID_I2C_PACKET_SZ - sizeof(HID_I2C_OUT_REPORT_T) - sizeof(HID_I2C_XFER_PARAMS_T))
#define MAX_XFER_RX_LENGTH          (HID_I2C_PACKET_SZ - sizeof(HID_I2C_IN_REPORT_T))

typedef struct LPCUSBSIO_Request
{
    uint8_t transId;
    int32_t status;
    uint8_t *outPacket;
    uint32_t outPacketLen;
    uint8_t *inPacket;
    uint32_t inPacketLen;
    void* notifier;
}LPCUSBSIO_Request_t;

typedef struct LPCUSBSIO_I2C_Ctrl {
    struct hid_device_info *hidInfo;

    hid_device *hidDev;
    char fwVersion[60];
    uint8_t sesionId;
	void* request_map;
    void* RequestMapLock;
    uint8_t thread_exit;
    void* ResponsethreadHandle;
    void* SendLock;
    void* NotifierContainer;
    void* NotifierContainerLock;
    struct LPCUSBSIO_I2C_Ctrl *next;

} LPCUSBSIO_I2C_Ctrl_t;

struct LPCUSBSIO_Ctrl {
    struct hid_device_info *devInfoList;

    LPCUSBSIO_I2C_Ctrl_t *devList;
};

//static const char *g_LibVersion = "LPCUSBSIO v1.00 (" __DATE__ " " __TIME__ ")";
static const char *g_fwInitVer = "FW Ver Unavailable";

static struct LPCUSBSIO_Ctrl g_Ctrl = {0, };

static const wchar_t *g_LibErrMsgs[NUM_LIB_ERR_STRINGS] = {
    L"No errors are recorded.",
    L"HID library error.",                            /* LPCUSBSIO_ERR_HID_LIB */
    L"Handle passed to the function is invalid.",    /* LPCUSBSIO_ERR_BAD_HANDLE */
};

static const wchar_t *g_fwErrMsgs[NUM_FW_ERR_STRINGS] = {
    L"Firmware error.",                                /* catch-all firmware error */
    L"Fatal error happened",                            /* LPCUSBSIO_ERR_FATAL */
    L"Transfer aborted due to NAK",                    /* LPCUSBSIO_ERR_I2C_NAK */
    L"Transfer aborted due to bus error",            /* LPCUSBSIO_ERR_I2C_BUS */
    L"No acknowledgement received from slave address",    /* LPCUSBSIO_ERR_I2C_SLAVE_NAK */
    L"I2C bus arbitration lost to other master",    /* LPCUSBSIO_ERR_I2C_SLAVE_NAK */
};

static const wchar_t *g_BridgeErrMsgs[NUM_BRIDGE_ERR_STRINGS] = {
    L"Transaction timed out.",                        /* LPCUSBSIO_ERR_TIMEOUT */
    L"Invalid HID_I2C Request or Request not supported in this version.",    /* LPCUSBSIO_ERR_INVALID_CMD */
    L"Invalid parameters are provided for the given Request.",    /* LPCUSBSIO_ERR_INVALID_PARAM */
    L" Partial transfer completed.",                        /* LPCUSBSIO_ERR_PARTIAL_DATA */
};

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

void* getNotifier(LPCUSBSIO_I2C_Ctrl_t * dev)
{
    CONTAINER_STATUS lContainerStatus = CONTAINER_SUCCESS;
    uint32_t size = 0x00;
    void* Notifier = NULL;
    if(NULL != dev)
    {
        framework_LockMutex(dev->NotifierContainerLock);
        if(NULL == dev->NotifierContainer)
        {
            lContainerStatus = container_create(&dev->NotifierContainer, 2);
        }
        if (CONTAINER_SUCCESS == lContainerStatus)
        {
            lContainerStatus = container_size(dev->NotifierContainer, &size);
            if(0x00 == size)
            {
                framework_CreateMutex(&Notifier);
            }
            else
            {
                container_remove(dev->NotifierContainer, 0x00, &Notifier);
            }
        }
        framework_UnlockMutex(dev->NotifierContainerLock);
    }
    return Notifier;
}

void FreeNotifier(LPCUSBSIO_I2C_Ctrl_t * dev, void* Notifier)
{
    if(NULL != dev)
    {
        framework_LockMutex(dev->NotifierContainerLock);
        if(NULL != dev->NotifierContainer && NULL != Notifier)
        {
            container_add(dev->NotifierContainer, Notifier);
        }
        framework_UnlockMutex(dev->NotifierContainerLock);
    }
}

void ReleaseNotifier(LPCUSBSIO_I2C_Ctrl_t * dev)
{
    void* Notifier = NULL;
    if(NULL != dev)
    {
        framework_LockMutex(dev->NotifierContainerLock);
        if(NULL != dev->NotifierContainer)
        {
            do
            {
                container_remove(dev->NotifierContainer, 0x00, &Notifier);
                if(NULL != Notifier)
                {
                    framework_DeleteMutex(Notifier);
                    Notifier = NULL;
                }
            }while(NULL != Notifier);
            container_delete(dev->NotifierContainer);
            dev->NotifierContainer = NULL;
        }
        framework_UnlockMutex(dev->NotifierContainerLock);
    }
}
static struct hid_device_info *GetDevAtIndex(uint32_t index) {
    struct hid_device_info *cur_dev = g_Ctrl.devInfoList;
    uint32_t count = 0;
    
    while (cur_dev) {
        if (count++ == index) {
            break;
        }

        cur_dev = cur_dev->next;
    }
    
    return cur_dev;
}

static int32_t validHandle(LPCUSBSIO_I2C_Ctrl_t *dev)
{
    LPCUSBSIO_I2C_Ctrl_t *curDev = g_Ctrl.devList;
    
    while (dev != curDev) {
        curDev = curDev->next;
    }
    
    return (curDev == NULL) ? 0 : 1;
}

static void freeDevice(LPCUSBSIO_I2C_Ctrl_t *dev)
{
    LPCUSBSIO_I2C_Ctrl_t *curDev = g_Ctrl.devList;
    
    if (curDev == dev) {
        g_Ctrl.devList = dev->next;
    }
    else {
        while (curDev) {
            if (curDev->next == dev) {
                /* update linked list */
                curDev->next = dev->next;
                break;
            }
        }
    }
    free(dev);

    /* unload HID library if all devices are closed. */
    if (g_Ctrl.devList == NULL) {
        hid_free_enumeration(g_Ctrl.devInfoList);
        hid_exit();
    }
    
}

static const wchar_t *GetErrorString(int32_t err)
{
    const wchar_t *retStr = g_LibErrMsgs[0];
    int index;

    index = abs(err);

    if (index < 0x10) {
        retStr = (index < NUM_LIB_ERR_STRINGS) ? g_LibErrMsgs[index] : g_LibErrMsgs[0];
    }
    else if (index < 0x20) {
        index -= 0x10;
        retStr = (index < NUM_FW_ERR_STRINGS) ? g_fwErrMsgs[index] : g_fwErrMsgs[0];
    }
    else if (index < 0x20) {
        index -= 0x30;
        retStr = (index < NUM_BRIDGE_ERR_STRINGS) ? g_BridgeErrMsgs[index] : g_LibErrMsgs[0];
    }

    return retStr;
}

static int32_t ConvertResp(int32_t res)
{
    int ret;
    
    if (res == HID_I2C_RES_OK) {
        ret = LPCUSBSIO_OK;
    }
    else {
        ret = -(res + 0x10);
    }
    
    return ret;
}

static void* WaitRequestResponse(void* pContext)
{
    LPCUSBSIO_I2C_Ctrl_t * dev = pContext;
    HID_I2C_IN_REPORT_T *pIn;
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    int32_t res = 0;
    LPCUSBSIO_Request_t* lRequest;
    STATUS lMapStatus = SUCCESS;
    void* Notifier = NULL;
    uint32_t ReadIndex = 0x00;
    uint32_t lenght = 0x00;
    
    while (0x01 != dev->thread_exit) 
    {
        ReadIndex = 0x00;
        res = hid_read_timeout(dev->hidDev, inPacket, sizeof(inPacket), LPCUSBSIO_READ_TMO);

        if (res > 0) 
        {
            pIn = (HID_I2C_IN_REPORT_T *) &inPacket[0];
            
            framework_LockMutex(dev->RequestMapLock);
            lMapStatus = map_get(dev->request_map, (void*) (intptr_t) pIn->transId, (void**) &lRequest);
            framework_UnlockMutex(dev->RequestMapLock);
            
            if(SUCCESS == lMapStatus)
            {
                ReadIndex += HID_I2C_PACKET_SZ;
                /* check reponse received from LPC */
                lenght = pIn->length;

                memcpy(lRequest->inPacket, inPacket, HID_I2C_PACKET_SZ);

                while (ReadIndex < lenght)
                {
                res = hid_read_timeout(dev->hidDev, inPacket, sizeof(inPacket), LPCUSBSIO_READ_TMO);
                if (res > 0)
                {
                    memcpy(&lRequest->inPacket[ReadIndex], inPacket, HID_I2C_PACKET_SZ);
                    ReadIndex += HID_I2C_PACKET_SZ;
                }

                }

                pIn = (HID_I2C_IN_REPORT_T *)&lRequest->inPacket[0];

                Notifier = lRequest->notifier;
                /* update status */
                res = ConvertResp(pIn->resp);
                lRequest->status = res;
                lRequest->inPacketLen = HID_I2C_PACKET_SZ + 1;
                
                //memcpy(lRequest->inPacket, inPacket, (HID_I2C_PACKET_SZ + 1));
                framework_LockMutex(Notifier);
                framework_NotifyMutex(Notifier, 0);
                framework_UnlockMutex(Notifier);
                Notifier = NULL;
            }
            else
            {
                /*Transition ID received form the chip not recognized !!!!!!   why ??????*/
                //TODO : define the behaviour in this case
                
                res = LPCUSBSIO_ERR_HID_LIB;
            }
        }
        else if (res == 0) 
        {
            res = LPCUSBSIO_ERR_TIMEOUT;
        }
        else
        {
        }
    }
    return NULL;
}

static int32_t I2C_SendRequest(LPCUSBSIO_I2C_Ctrl_t *dev, uint8_t req, uint8_t* dataOut, uint32_t dataOutLen, uint8_t* dataIn, uint32_t dataInLen)
{
    int32_t res = 0;
    static uint32_t ReqIndice = 0x00;
    STATUS lMapStatus = SUCCESS;
    uint8_t outPacket[HID_I2C_PACKET_SZ + 1];
    HID_I2C_OUT_REPORT_T *pOut;
    LPCUSBSIO_Request_t lRequest;
    uint32_t pos = 0x00;
    uint32_t offset = HID_I2C_HEADER_SZ + 1;
    
    lRequest.outPacket = dataOut;
    lRequest.outPacketLen = dataOutLen;
    
    lRequest.inPacket = dataIn;
    lRequest.inPacketLen = dataInLen;
    
    lRequest.status = LPCUSBSIO_OK;
    pOut = (HID_I2C_OUT_REPORT_T *) &outPacket[1];
    pOut->sesId = dev->sesionId;
    pOut->transId = lRequest.transId = ReqIndice++;
    pOut->req = req;
    pOut->length = HID_I2C_HEADER_SZ + dataOutLen;
    
    
    framework_LockMutex(dev->RequestMapLock);
    lMapStatus = map_add(dev->request_map, (void*) (intptr_t) lRequest.transId,	(void*) &lRequest);
    framework_UnlockMutex(dev->RequestMapLock);
    
    if(SUCCESS == lMapStatus)
    {
        //framework_CreateMutex(&lRequest.notifier);
        lRequest.notifier = getNotifier(dev);
        
        framework_LockMutex(lRequest.notifier);
        
        framework_LockMutex(dev->SendLock);
        //do
        //{
            outPacket[0] = 0;

            if(lRequest.outPacket != NULL) memcpy(&outPacket[offset], lRequest.outPacket + pos, HID_I2C_PACKET_SZ + 1 - offset);
        
            res = hid_write(dev->hidDev, outPacket, HID_I2C_PACKET_SZ + 1);
        //    if (HID_I2C_PACKET_SZ + 1 == res)
        //    {
        //        pos += HID_I2C_PACKET_SZ - offset + 1;
        //        offset = 0x01;
        //    }
        //} while (pos < len);
        
        framework_UnlockMutex(dev->SendLock);
        if(0x00 < res)
        {
            framework_WaitMutex(lRequest.notifier, 0);
            framework_UnlockMutex(lRequest.notifier);        
            res = lRequest.status;
        }
        else
        {
            framework_UnlockMutex(lRequest.notifier);
            res = LPCUSBSIO_ERR_HID_LIB;
        }
        
        framework_LockMutex(dev->RequestMapLock);
        map_remove(dev->request_map, (void*) (intptr_t) lRequest.transId);
        framework_UnlockMutex(dev->RequestMapLock);
        FreeNotifier(dev, lRequest.notifier);
        //framework_DeleteMutex(lRequest.notifier);
        lRequest.notifier = NULL;
    }
    else
    {
        res = LPCUSBSIO_ERR_HID_LIB;
    }
    
    return res;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

LPCUSBSIO_API int32_t I2C_GetNumPorts(void)
{
    struct hid_device_info *cur_dev;
    int32_t count = 0;
    
    /* free any HID device structures if we were called previously */
    if (g_Ctrl.devInfoList != NULL) {
        hid_free_enumeration(g_Ctrl.devInfoList);
        g_Ctrl.devInfoList = NULL;
    }

    g_Ctrl.devInfoList = hid_enumerate(LPCUSBSIO_VID, LPCUSBSIO_PID);
    cur_dev = g_Ctrl.devInfoList;
    while (cur_dev) {
        count++;
        cur_dev = cur_dev->next;
    }

    return count;
}

LPCUSBSIO_API LPC_HANDLE I2C_Open(uint32_t index)
{
    hid_device *pHid = NULL;
    LPCUSBSIO_I2C_Ctrl_t *dev = NULL;
    struct hid_device_info *cur_dev;

    cur_dev = GetDevAtIndex(index);

    if (cur_dev) 
    {
        pHid = hid_open_path(cur_dev->path);

        if (pHid) 
        {
            dev = malloc(sizeof(LPCUSBSIO_I2C_Ctrl_t));
            memset(dev, 0, sizeof(LPCUSBSIO_I2C_Ctrl_t));
            dev->hidDev = pHid;
            dev->hidInfo = cur_dev;
            dev->sesionId = rand();
            map_create(&dev->request_map);
            memcpy(&dev->fwVersion[0], g_fwInitVer, strlen(g_fwInitVer));

            /* insert at top */
            dev->next = g_Ctrl.devList;
            g_Ctrl.devList = dev;
            /* Set all calls to this hid device as blocking. */
            // hid_set_nonblocking(dev->hidDev, 0);
            framework_CreateMutex(&dev->SendLock);
            framework_CreateMutex(&dev->RequestMapLock);
            framework_CreateMutex(&dev->NotifierContainerLock);
            dev->thread_exit = 0x00;
            framework_CreateThread(&dev->ResponsethreadHandle , WaitRequestResponse, dev);

        }
    }
    
    return (LPC_HANDLE) dev;
}

LPCUSBSIO_API int32_t I2C_Init(LPC_HANDLE handle, I2C_PORTCONFIG_T *config)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint8_t outPacket[HID_I2C_PACKET_SZ + 1];
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    int32_t res;
    HID_I2C_IN_REPORT_T *pIn;
    
    if ((validHandle(handle) == 0) || (config == NULL)) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    memcpy(outPacket, config, sizeof(I2C_PORTCONFIG_T));
    res = I2C_SendRequest(dev, HID_I2C_REQ_INIT_PORT, outPacket, sizeof(I2C_PORTCONFIG_T), inPacket, sizeof(inPacket));
    if (res == LPCUSBSIO_OK) 
    {
        /* parse response */
        pIn = (HID_I2C_IN_REPORT_T *) &inPacket[0];
        res = pIn->length - HID_I2C_HEADER_SZ;
        /* copy data back to user buffer */
        memcpy(&dev->fwVersion[0], &pIn->data[0], res);
    }
    
    return res;
}

LPCUSBSIO_API int32_t I2C_CancelAllRequest(LPC_HANDLE handle)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint32_t lMapLenght = 0x00, i;
    LPCUSBSIO_Request_t* lRequest = NULL;
    void* Notifier = NULL;

    framework_LockMutex(dev->RequestMapLock);

    //1rst call => we get map lenght
    map_getAll(dev->request_map, NULL, (int*)&lMapLenght);

    lRequest = malloc(lMapLenght * sizeof(LPCUSBSIO_Request_t));
    //2nd call => we get all object in map
    map_getAll(dev->request_map, (void **) &lRequest, (int*) &lMapLenght);

    for (i = 0x00; i < lMapLenght; i++)
    {
        //Cancel request
        if (NULL != lRequest[i].notifier)
        {
        lRequest[i].status = LPCUSBSIO_REQ_CANCELED;
        Notifier = lRequest[i].notifier;
        framework_LockMutex(Notifier);
        framework_NotifyMutex(Notifier, 0);
        framework_UnlockMutex(Notifier);
        Notifier = NULL;
        }
    }
    
    framework_UnlockMutex(dev->RequestMapLock);
    return 0;    
}
LPCUSBSIO_API int32_t I2C_Close(LPC_HANDLE handle)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    
    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    I2C_SendRequest(dev, HID_I2C_REQ_DEINIT_PORT, NULL, 0, inPacket, sizeof(inPacket));
    
    /*Shutdown reader thread*/
    dev->thread_exit = 0x01;
    /*Close remote dev handle : all pending request are canceled*/
    hid_close(dev->hidDev);
    
    /*Wait End of reader thread & clean up*/
    framework_JoinThread(dev->ResponsethreadHandle);
    framework_DeleteThread(dev->ResponsethreadHandle);
    dev->ResponsethreadHandle = NULL;
    /*redaer thread is not running so we notify all pending request*/

    I2C_CancelAllRequest(handle);

    g_Ctrl.devInfoList = NULL;

    ReleaseNotifier(dev);
    
    framework_DeleteMutex(dev->NotifierContainerLock);
    dev->NotifierContainerLock = NULL;

    framework_LockMutex(dev->RequestMapLock);
    map_destroy(dev->request_map);
    dev->request_map = NULL;
    framework_UnlockMutex(dev->RequestMapLock);
    framework_DeleteMutex(dev->RequestMapLock);
    dev->RequestMapLock = NULL;
    framework_DeleteMutex(dev->SendLock);
    dev->SendLock = NULL;
    freeDevice(dev);

    return LPCUSBSIO_OK;
}

LPCUSBSIO_API int32_t I2C_DeviceRead(LPC_HANDLE handle,
                                     uint8_t deviceAddress,
                                     uint8_t *buffer,
                                     uint16_t sizeToTransfer,
                                     uint8_t options)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint8_t outPacket[HID_I2C_PACKET_SZ + 1];
    uint8_t inPacket[0xFFFF];
    int32_t res;
    HID_I2C_RW_PARAMS_T param;
    HID_I2C_IN_REPORT_T *pIn;

    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* do parameter check */
    //if ((sizeToTransfer > MAX_READ_LENGTH) ||
    //    ((sizeToTransfer > 0) && (buffer == NULL)) ||
    //    (deviceAddress > 127)) 
    //{
    //    return LPCUSBSIO_ERR_INVALID_PARAM;
    //}

    param.length = sizeToTransfer;
    param.options = options;
    param.slaveAddr = deviceAddress;
    memcpy(outPacket, &param, sizeof(HID_I2C_RW_PARAMS_T));

    res = I2C_SendRequest(dev, HID_I2C_REQ_DEVICE_READ, outPacket, sizeof(HID_I2C_RW_PARAMS_T), inPacket, sizeof(inPacket));
    if (res == LPCUSBSIO_OK) 
    {
        /* parse response */
        pIn = (HID_I2C_IN_REPORT_T *) &inPacket[0];
        res = pIn->length  - HID_I2C_HEADER_SZ;

        if (pIn->length <= HID_I2C_PACKET_SZ) 
        {
            /* copy data back to user buffer */
            memcpy(buffer, &pIn->data[0], res);
        }
        else
        {
            uint8_t ptr = HID_I2C_HEADER_SZ;
            uint8_t temp, i=0;
            while(res > 0)
            {
                temp = (res > HID_I2C_PACKET_SZ-HID_I2C_HEADER_SZ) ? (HID_I2C_PACKET_SZ-HID_I2C_HEADER_SZ) : res;
                memcpy(&buffer[i], &inPacket[ptr], temp);
                i += temp;
                res -= temp;
                ptr += temp + HID_I2C_HEADER_SZ;
            }
            res = i;
        }
    }

    return res;
}

LPCUSBSIO_API int32_t I2C_DeviceWrite(LPC_HANDLE handle,
                                      uint8_t deviceAddress,
                                      uint8_t *buffer,
                                      uint16_t sizeToTransfer,
                                      uint8_t options)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint8_t outPacket[0xFFFF];
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    int32_t res;
    HID_I2C_RW_PARAMS_T param;
    uint8_t *desBuff;
    
    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* do parameter check */
    if ((sizeToTransfer > MAX_WRITE_LENGTH) ||
        ((sizeToTransfer > 0) && (buffer == NULL)) ||
        (deviceAddress > 127)) 
    {
        return LPCUSBSIO_ERR_INVALID_PARAM;
    }

    param.length = sizeToTransfer;
    param.options = options;
    param.slaveAddr = deviceAddress;
    desBuff = outPacket;
    /* copy params */
    memcpy(desBuff, &param, sizeof(HID_I2C_RW_PARAMS_T));
    desBuff += sizeof(HID_I2C_RW_PARAMS_T);
    /* copy data buffer now */
    memcpy(desBuff, buffer, sizeToTransfer);

    res = I2C_SendRequest(dev, HID_I2C_REQ_DEVICE_WRITE, outPacket, sizeof(HID_I2C_RW_PARAMS_T) + sizeToTransfer, inPacket, sizeof(inPacket));

    if (res == LPCUSBSIO_OK) 
    {
        /* update user on transfered size */
        res = sizeToTransfer;
    }
    
    return res;
}

LPCUSBSIO_API int32_t I2C_FastXfer(LPC_HANDLE handle, I2C_FAST_XFER_T *xfer)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint8_t outPacket[HID_I2C_PACKET_SZ + 1];
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    int32_t res;
    HID_I2C_XFER_PARAMS_T param;
    HID_I2C_IN_REPORT_T *pIn;
    uint8_t *desBuff;
    
    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* do parameter check */
    if ((xfer->txSz > MAX_XFER_TX_LENGTH) || (xfer->rxSz > MAX_XFER_RX_LENGTH) ||
        ((xfer->txSz > 0) && (xfer->txBuff == NULL)) ||
        ((xfer->rxSz > 0) && (xfer->rxBuff == NULL)) ||
        (xfer->slaveAddr > 127) ) 
    {
        return LPCUSBSIO_ERR_INVALID_PARAM;
    }

    param.txLength = xfer->txSz;
    param.rxLength = xfer->rxSz;
    param.options = xfer->options;
    param.slaveAddr = xfer->slaveAddr;
    desBuff = outPacket;
    /* copy params */
    memcpy(desBuff, &param, sizeof(HID_I2C_XFER_PARAMS_T));
    desBuff += sizeof(HID_I2C_XFER_PARAMS_T);
    /* copy data buffer now */
    memcpy(desBuff, &xfer->txBuff[0], xfer->txSz);

    res = I2C_SendRequest(dev, HID_I2C_REQ_DEVICE_XFER, outPacket, sizeof(HID_I2C_XFER_PARAMS_T) + xfer->txSz, inPacket, sizeof(inPacket));

    if (res == LPCUSBSIO_OK)
    {
        /* parse response */
        pIn = (HID_I2C_IN_REPORT_T *) &inPacket[0];
        res = pIn->length - HID_I2C_HEADER_SZ;
        if (res != 0) 
        {
            /* copy data back to user buffer */
            memcpy(&xfer->rxBuff[0], &pIn->data[0], res);
        }
        else 
        {
            /* else it should be Tx only transfer. Update transferred size. */
            res = xfer->txSz;
        }
    }

    return res;
}

LPCUSBSIO_API int32_t I2C_Reset(LPC_HANDLE handle)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    int32_t res;
    
    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    res = I2C_SendRequest(dev, HID_I2C_REQ_RESET, NULL, 0, inPacket, sizeof(inPacket));

    return res;
}

LPCUSBSIO_API const wchar_t *I2C_Error(LPC_HANDLE handle, int32_t status)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    const wchar_t *retStr = NULL;
    
    if (validHandle(handle) != 0) 
    {
        if ((status == LPCUSBSIO_OK) || (LPCUSBSIO_ERR_HID_LIB == status)) 
        {
            retStr = hid_error(dev->hidDev);
        }
        else 
        {
            retStr = GetErrorString(status);
        }
    }
    else 
    {
        retStr = hid_error(NULL);
    }
    
    return retStr;
}


//AL : Useless
//LPCUSBSIO_API const char *I2C_GetVersion(LPC_HANDLE handle)
//{
//    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
//    uint32_t index = 0;
//
//    /* copy library version */
//    memcpy(&g_Version[index], &g_LibVersion[0], strlen(g_LibVersion));
//    index += strlen(g_LibVersion);
//
//    /* if handle is good copy firmware version */
//    if (validHandle(handle) != 0) {
//        g_Version[index] = '/';
//        index++;
//        /* copy firmware version */
//        memcpy(&g_Version[index], &dev->fwVersion[0], strlen(dev->fwVersion));
//        index += strlen(dev->fwVersion);
//    }
//
//    return &g_Version[0];
//}

LPCUSBSIO_API LPCUSBSIO_ERR_T GPIO_SetVENValue(LPC_HANDLE handle, uint8_t newValue)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    LPCUSBSIO_ERR_T res = LPCUSBSIO_OK;
    uint8_t outPacket[HID_I2C_PACKET_SZ + 1];
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];

    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* do parameter check */
    if ( 0 != newValue && 1 != newValue)
    {
        return LPCUSBSIO_ERR_INVALID_PARAM;
    }
    
    outPacket[0] = newValue;

    res = I2C_SendRequest(dev, HID_GPIO_REQ_SET_VEN, outPacket, sizeof(uint8_t), inPacket, sizeof(inPacket));
    
    return res;
}

LPCUSBSIO_API LPCUSBSIO_ERR_T GPIO_SetDWLValue(LPC_HANDLE handle, uint8_t newValue)
{
    LPCUSBSIO_I2C_Ctrl_t *dev = (LPCUSBSIO_I2C_Ctrl_t *) handle;
    LPCUSBSIO_ERR_T res = LPCUSBSIO_OK;
    uint8_t outPacket[HID_I2C_PACKET_SZ + 1];
    uint8_t inPacket[HID_I2C_PACKET_SZ + 1];
    
    if (validHandle(handle) == 0) 
    {
        return LPCUSBSIO_ERR_BAD_HANDLE;
    }

    /* do parameter check */
    if ( 0 != newValue && 1 != newValue)
    {
        return LPCUSBSIO_ERR_INVALID_PARAM;
    }
    
    outPacket[0] = newValue;

    res = I2C_SendRequest(dev, HID_GPIO_REQ_SET_DWL, outPacket, sizeof(uint8_t), inPacket, sizeof(inPacket));

    return res;
}
