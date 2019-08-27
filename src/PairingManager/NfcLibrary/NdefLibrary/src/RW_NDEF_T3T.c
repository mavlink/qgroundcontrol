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

#define T3T_MAGIC_NUMBER    0xE1
#define T3T_NDEF_TLV        0x03

unsigned char T3T_Check[] = {0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0B,0x00,0x1,0x80,0x00};

typedef enum
{
    Initial,
    Getting_AttributeInfo,
    Reading_CardContent
} RW_NDEF_T3T_state_t;

typedef struct
{
    unsigned char IDm[8];
    unsigned char BlkNb;
    unsigned short Ptr;
    unsigned short Size;
    unsigned char *p;
} RW_NDEF_T3T_Ndef_t;

static RW_NDEF_T3T_state_t eRW_NDEF_T3T_State = Initial;
static RW_NDEF_T3T_Ndef_t RW_NDEF_T3T_Ndef;

void RW_NDEF_T3T_Reset(void)
{
    eRW_NDEF_T3T_State = Initial;
    RW_NDEF_T3T_Ndef.p = NdefBuffer;
}

void RW_NDEF_T3T_SetIDm(unsigned char *pIDm)
{
    memcpy(RW_NDEF_T3T_Ndef.IDm, pIDm, sizeof(RW_NDEF_T3T_Ndef.IDm));
    memcpy(&T3T_Check[2], pIDm, sizeof(RW_NDEF_T3T_Ndef.IDm));
}

void RW_NDEF_T3T_Read_Next(unsigned char *pRsp, unsigned short Rsp_size, unsigned char *pCmd, unsigned short *pCmd_size)
{
    /* By default no further command to be sent */
    *pCmd_size = 0;

    switch(eRW_NDEF_T3T_State)
    {
    case Initial:
        /* Get AttributeInfo */
        memcpy (pCmd, T3T_Check, sizeof(T3T_Check));
        *pCmd_size = sizeof(T3T_Check);
        eRW_NDEF_T3T_State = Getting_AttributeInfo;
        break;

    case Getting_AttributeInfo:
        /* Is Check success ?*/
        if ((pRsp[Rsp_size-1] == 0x00) && (pRsp[1] == 0x07) && (pRsp[10] == 0x00) && (pRsp[11] == 0x00))
        {
            /* Fill File structure */
            RW_NDEF_T3T_Ndef.Size = (pRsp[24] << 16) + (pRsp[25] << 16) + pRsp[26];

            /* If provisioned buffer is not large enough, notify the application and stop reading */
            if (RW_NDEF_T3T_Ndef.Size > RW_MAX_NDEF_FILE_SIZE)
            {
                if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(NULL, 0);
                break;
            }

            RW_NDEF_T3T_Ndef.Ptr = 0;
            RW_NDEF_T3T_Ndef.BlkNb = 1;

            /* Read first NDEF block */
            memcpy (pCmd, T3T_Check, sizeof(T3T_Check));
            pCmd[15] = 0x01;
            *pCmd_size = sizeof(T3T_Check);
            eRW_NDEF_T3T_State = Reading_CardContent;
        }
        break;

    case Reading_CardContent:
        /* Is Check success ?*/
        if ((pRsp[Rsp_size-1] == 0x00) && (pRsp[1] == 0x07) && (pRsp[10] == 0x00) && (pRsp[11] == 0x00))
        {
            /* Is NDEF message read completed ?*/
            if ((RW_NDEF_T3T_Ndef.Size - RW_NDEF_T3T_Ndef.Ptr) <= 16)
            {
                memcpy(&RW_NDEF_T3T_Ndef.p[RW_NDEF_T3T_Ndef.Ptr], &pRsp[13], (RW_NDEF_T3T_Ndef.Size - RW_NDEF_T3T_Ndef.Ptr));
                /* Notify application of the NDEF reception */
                if(pRW_NDEF_PullCb != NULL) pRW_NDEF_PullCb(RW_NDEF_T3T_Ndef.p, RW_NDEF_T3T_Ndef.Size);
            }
            else
            {
                memcpy(&RW_NDEF_T3T_Ndef.p[RW_NDEF_T3T_Ndef.Ptr], &pRsp[13], 16);
                RW_NDEF_T3T_Ndef.Ptr += 16;
                RW_NDEF_T3T_Ndef.BlkNb++;

                /* Read next NDEF block */
                memcpy (pCmd, T3T_Check, sizeof(T3T_Check));
                pCmd[15] = RW_NDEF_T3T_Ndef.BlkNb;
                *pCmd_size = sizeof(T3T_Check);
            }
        }
        break;

    default:
        break;
    }
}
#endif
#endif
