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

#define T1T_MAGIC_NUMBER    0xE1
#define T1T_NDEF_TLV        0x03

const unsigned char T1T_RID[] = {0x78,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char T1T_RALL[] = {0x00,0x00,0x00};
const unsigned char T1T_READ8[] = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

typedef enum
{
    Initial,
    Getting_ID,
    Reading_CardContent,
    Reading_NDEF
} RW_NDEF_T1T_state_t;

typedef struct
{
    unsigned char HR0;
    unsigned char HR1;
    unsigned char UID[4];
    unsigned char BlkNb;
    unsigned short MessagePtr;
    unsigned short MessageSize;
    unsigned char *pMessage;
} RW_NDEF_T1T_Ndef_t;

static RW_NDEF_T1T_state_t eRW_NDEF_T1T_State = Initial;
static RW_NDEF_T1T_Ndef_t RW_NDEF_T1T_Ndef;

void RW_NDEF_T1T_Reset(void)
{
    eRW_NDEF_T1T_State = Initial;
    RW_NDEF_T1T_Ndef.pMessage = NdefBuffer;
}

void RW_NDEF_T1T_Read_Next(unsigned char *pRsp, unsigned short Rsp_size, unsigned char *pCmd, unsigned short *pCmd_size)
{
    /* By default no further command to be sent */
    *pCmd_size = 0;

    switch(eRW_NDEF_T1T_State)
    {
    case Initial:
        /* Send T1T_RID */
        memcpy (pCmd, T1T_RID, sizeof(T1T_RID));
        *pCmd_size = 7;
        eRW_NDEF_T1T_State = Getting_ID;
        break;

    case Getting_ID:
        /* Is CC Read and Is Ndef ?*/
        if ((Rsp_size == 7) && (pRsp[Rsp_size-1] == 0x00))
        {
            /* Fill File structure */
            RW_NDEF_T1T_Ndef.HR0 = pRsp[0];
            RW_NDEF_T1T_Ndef.HR1 = pRsp[1];
            memcpy (RW_NDEF_T1T_Ndef.UID, &pRsp[2], sizeof(RW_NDEF_T1T_Ndef.UID));
            
            /* Read full card content */
            memcpy (pCmd, T1T_RALL, sizeof(T1T_RALL));
            memcpy (&pCmd[3], RW_NDEF_T1T_Ndef.UID, sizeof(RW_NDEF_T1T_Ndef.UID));
            *pCmd_size = sizeof(T1T_RALL) + sizeof(RW_NDEF_T1T_Ndef.UID);
            eRW_NDEF_T1T_State = Reading_CardContent;
        }
        break;

    case Reading_CardContent:
        /* Is Read success ?*/
        if ((Rsp_size == 123) && (pRsp[Rsp_size-1] == 0x00))
        {
            /* Check CC */ 
            if (pRsp[10] == T1T_MAGIC_NUMBER)
            {
                unsigned char Tmp = 14;
                unsigned char data_size;

                /* If not NDEF Type skip TLV */
                while (pRsp[Tmp] != T1T_NDEF_TLV)
                {
                    Tmp += 2 + pRsp[Tmp+1];
                    if (Tmp > Rsp_size) return;
                }

                RW_NDEF_T1T_Ndef.MessageSize = pRsp[Tmp+1];
                data_size = (Rsp_size - 1) - 16 - Tmp - 2;

                /* If provisioned buffer is not large enough, notify the application and stop reading */
                if (RW_NDEF_T1T_Ndef.MessageSize > RW_MAX_NDEF_FILE_SIZE)
                {
                    if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(NULL, 0);
                    break;
                }

                /* Is NDEF read already completed ? */
                if(RW_NDEF_T1T_Ndef.MessageSize <= data_size)
                {
                    memcpy(RW_NDEF_T1T_Ndef.pMessage, &pRsp[Tmp+2], RW_NDEF_T1T_Ndef.MessageSize);

                    /* Notify application of the NDEF reception */
                    if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(RW_NDEF_T1T_Ndef.pMessage, RW_NDEF_T1T_Ndef.MessageSize);
                }
                else
                {
                    RW_NDEF_T1T_Ndef.MessagePtr = data_size;
                    memcpy (RW_NDEF_T1T_Ndef.pMessage, &pRsp[Tmp+2], RW_NDEF_T1T_Ndef.MessagePtr);
                    RW_NDEF_T1T_Ndef.BlkNb = 0x10;

                    /* Read NDEF content */
                    memcpy (pCmd, T1T_READ8, sizeof(T1T_READ8));
                    pCmd[1] = RW_NDEF_T1T_Ndef.BlkNb;
                    memcpy (&pCmd[10], RW_NDEF_T1T_Ndef.UID, sizeof(RW_NDEF_T1T_Ndef.UID));
                    *pCmd_size = sizeof(T1T_READ8) + sizeof(RW_NDEF_T1T_Ndef.UID);

                    eRW_NDEF_T1T_State = Reading_NDEF;
                }
            }
        }
        break;

        case Reading_NDEF:
            /* Is Read success ?*/
            if ((Rsp_size == 10) && (pRsp[Rsp_size-1] == 0x00))
            {
                /* Is NDEF read already completed ? */
                if ((RW_NDEF_T1T_Ndef.MessageSize - RW_NDEF_T1T_Ndef.MessagePtr) < 8)
                {
                    memcpy (&RW_NDEF_T1T_Ndef.pMessage[RW_NDEF_T1T_Ndef.MessagePtr], &pRsp[1], RW_NDEF_T1T_Ndef.MessageSize - RW_NDEF_T1T_Ndef.MessagePtr);

                    /* Notify application of the NDEF reception */
                    if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(RW_NDEF_T1T_Ndef.pMessage, RW_NDEF_T1T_Ndef.MessageSize);
                }
                else
                {
                    memcpy (&RW_NDEF_T1T_Ndef.pMessage[RW_NDEF_T1T_Ndef.MessagePtr], &pRsp[1], 8);
                    RW_NDEF_T1T_Ndef.MessagePtr += 8;
                    RW_NDEF_T1T_Ndef.BlkNb++;

                    /* Read NDEF content */
                    memcpy (pCmd, T1T_READ8, sizeof(T1T_READ8));
                    pCmd[1] = RW_NDEF_T1T_Ndef.BlkNb;
                    memcpy (&pCmd[10], RW_NDEF_T1T_Ndef.UID, sizeof(RW_NDEF_T1T_Ndef.UID));
                    *pCmd_size = sizeof(T1T_READ8) + sizeof(RW_NDEF_T1T_Ndef.UID);
                }
            }
        break;

    default:
        break;
    }
}
#endif
#endif
