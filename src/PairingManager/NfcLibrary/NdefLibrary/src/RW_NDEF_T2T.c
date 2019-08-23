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

#ifdef RW_SUPPORT
#ifndef NO_NDEF_SUPPORT
#include <TML/inc/tool.h>
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF.h>

/* TODO: No support for tag larger than 1024 bytes (requiring SECTOR_SELECT command use) */

#define T2T_MAGIC_NUMBER    0xE1
#define T2T_NDEF_TLV        0x03

typedef enum
{
    Initial,
    Reading_CC,
    Reading_Data,
    Reading_NDEF,
    Writing_Data
} RW_NDEF_T2T_state_t;

typedef struct
{
    unsigned char BlkNb;
    unsigned short MessagePtr;
    unsigned short MessageSize;
    unsigned char *pMessage;
} RW_NDEF_T2T_Ndef_t;

static RW_NDEF_T2T_state_t eRW_NDEF_T2T_State = Initial;
static RW_NDEF_T2T_Ndef_t RW_NDEF_T2T_Ndef;

void RW_NDEF_T2T_Reset(void)
{
    eRW_NDEF_T2T_State = Initial;
    RW_NDEF_T2T_Ndef.pMessage = NdefBuffer;
}

void RW_NDEF_T2T_Read_Next(unsigned char *pRsp, unsigned short Rsp_size, unsigned char *pCmd, unsigned short *pCmd_size)
{
    /* By default no further command to be sent */
    *pCmd_size = 0;

    switch(eRW_NDEF_T2T_State)
    {
    case Initial:
        /* Read CC */
        pCmd[0] = 0x30;
        pCmd[1] = 0x03;
        *pCmd_size = 2;
        eRW_NDEF_T2T_State = Reading_CC;
        break;

    case Reading_CC:
        /* Is CC Read and Is Ndef ?*/
        if ((Rsp_size == 17) && (pRsp[Rsp_size-1] == 0x00) && (pRsp[0] == T2T_MAGIC_NUMBER))
        {
            /* Read First data */
            pCmd[0] = 0x30;
            pCmd[1] = 0x04;
            *pCmd_size = 2;

            eRW_NDEF_T2T_State = Reading_Data;
        }
        break;

    case Reading_Data:
        /* Is Read success ?*/
        if ((Rsp_size == 17) && (pRsp[Rsp_size-1] == 0x00))
        {
            unsigned char Tmp = 0;
            /* If not NDEF Type skip TLV */
            while (pRsp[Tmp] != T2T_NDEF_TLV)
            {
                Tmp += 2 + pRsp[Tmp+1];
                if (Tmp > Rsp_size) return;
            }

            if(pRsp[Tmp+1] == 0xFF)
            {
                RW_NDEF_T2T_Ndef.MessageSize = (pRsp[Tmp+2] << 8) + pRsp[Tmp+3];
                Tmp += 2;
            }
            else RW_NDEF_T2T_Ndef.MessageSize = pRsp[Tmp+1];

            /* If provisioned buffer is not large enough, notify the application and stop reading */
            if (RW_NDEF_T2T_Ndef.MessageSize > RW_MAX_NDEF_FILE_SIZE)
            {
                if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(NULL, 0);
                break;
            }

            /* Is NDEF read already completed ? */
            if (RW_NDEF_T2T_Ndef.MessageSize <= ((Rsp_size-1) - Tmp - 2))
            {
                memcpy (RW_NDEF_T2T_Ndef.pMessage, &pRsp[Tmp+2], RW_NDEF_T2T_Ndef.MessageSize);

                /* Notify application of the NDEF reception */
                if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(RW_NDEF_T2T_Ndef.pMessage, RW_NDEF_T2T_Ndef.MessageSize);
            }
            else
            {
                RW_NDEF_T2T_Ndef.MessagePtr = (Rsp_size-1) - Tmp - 2;
                memcpy (RW_NDEF_T2T_Ndef.pMessage, &pRsp[Tmp+2], RW_NDEF_T2T_Ndef.MessagePtr);
                RW_NDEF_T2T_Ndef.BlkNb = 8;

                /* Read NDEF content */
                pCmd[0] = 0x30;
                pCmd[1] = RW_NDEF_T2T_Ndef.BlkNb;
                *pCmd_size = 2;
                eRW_NDEF_T2T_State = Reading_NDEF;
            }
        }
        break;

        case Reading_NDEF:
        /* Is Read success ?*/
        if ((Rsp_size == 17) && (pRsp[Rsp_size-1] == 0x00))
        {
            /* Is NDEF read already completed ? */
            if ((RW_NDEF_T2T_Ndef.MessageSize - RW_NDEF_T2T_Ndef.MessagePtr) < 16)
            {
                memcpy (&RW_NDEF_T2T_Ndef.pMessage[RW_NDEF_T2T_Ndef.MessagePtr], pRsp, RW_NDEF_T2T_Ndef.MessageSize - RW_NDEF_T2T_Ndef.MessagePtr);

                /* Notify application of the NDEF reception */
                if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(RW_NDEF_T2T_Ndef.pMessage, RW_NDEF_T2T_Ndef.MessageSize);
            }
            else
            {
                memcpy (&RW_NDEF_T2T_Ndef.pMessage[RW_NDEF_T2T_Ndef.MessagePtr], pRsp, 16);
                RW_NDEF_T2T_Ndef.MessagePtr += 16;
                RW_NDEF_T2T_Ndef.BlkNb += 4;

                /* Read NDEF content */
                pCmd[0] = 0x30;
                pCmd[1] = RW_NDEF_T2T_Ndef.BlkNb;
                *pCmd_size = 2;
            }
        }
        break;

    default:
        break;
    }
}

void RW_NDEF_T2T_Write_Next(unsigned char *pRsp, unsigned short Rsp_size, unsigned char *pCmd, unsigned short *pCmd_size)
{
    /* By default no further command to be sent */
    *pCmd_size = 0;

    switch(eRW_NDEF_T2T_State)
    {
    case Initial:
        /* Read CC */
        pCmd[0] = 0x30;
        pCmd[1] = 0x03;
        *pCmd_size = 2;
        eRW_NDEF_T2T_State = Reading_CC;
        break;

    case Reading_CC:
        /* Is CC Read, Is Ndef and is R/W ?*/
        if ((Rsp_size == 17) && (pRsp[Rsp_size-1] == 0x00) && (pRsp[0] == T2T_MAGIC_NUMBER) && (pRsp[3] == 0x00))
        {
            /* Is size enough ? */
            if (pRsp[2]*8 >= RW_NdefMessage_size)
            {
                /* Write First data */
                pCmd[0] = 0xA2;
                pCmd[1] = 0x04;
                pCmd[2] = 0x03;
                if (RW_NdefMessage_size > 0xFF)
                {
                    pCmd[3] = 0xFF;
                    pCmd[4] = (RW_NdefMessage_size & 0xFF00) >> 8;
                    pCmd[5] = RW_NdefMessage_size & 0xFF;
                    RW_NDEF_T2T_Ndef.MessagePtr = 0;
                }
                else
                {
                    pCmd[3] = (unsigned char) RW_NdefMessage_size;
                    memcpy(&pCmd[4], pRW_NdefMessage, 2);
                    RW_NDEF_T2T_Ndef.MessagePtr = 2;
                }
                RW_NDEF_T2T_Ndef.BlkNb = 5;
                *pCmd_size = 6;
                eRW_NDEF_T2T_State = Writing_Data;
            }
        }
        break;

    case Writing_Data:
        /* Is Write success ?*/
        if ((Rsp_size == 2) && (pRsp[Rsp_size-1] == 0x00))
        {
            /* Is NDEF write already completed ? */
            if (RW_NdefMessage_size <= RW_NDEF_T2T_Ndef.MessagePtr)
            {
                /* Notify application of the NDEF reception */
                if(pRW_NDEF_PushCb != NULL) pRW_NDEF_PushCb(pRW_NdefMessage, RW_NdefMessage_size);
            }
            else
            {
                /* Write NDEF content */
                pCmd[0] = 0xA2;
                pCmd[1] = RW_NDEF_T2T_Ndef.BlkNb;
                memcpy(&pCmd[2], pRW_NdefMessage+RW_NDEF_T2T_Ndef.MessagePtr, 4);
                *pCmd_size = 6;

                RW_NDEF_T2T_Ndef.MessagePtr+=4;
                RW_NDEF_T2T_Ndef.BlkNb++;
            }
        }
        break;

    default:
        break;
    }
}
#endif
#endif
