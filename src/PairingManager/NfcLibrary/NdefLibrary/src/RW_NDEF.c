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
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF_T1T.h>
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF_T2T.h>
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF_T3T.h>
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF_T4T.h>

/* Allocate buffer for NDEF operations */
unsigned char NdefBuffer[RW_MAX_NDEF_FILE_SIZE];

typedef void RW_NDEF_Fct_t (unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size);

unsigned char *pRW_NdefMessage;
unsigned short RW_NdefMessage_size;

RW_NDEF_Callback_t *pRW_NDEF_PullCb;
RW_NDEF_Callback_t *pRW_NDEF_PushCb;

static RW_NDEF_Fct_t *pReadFct = NULL;
static RW_NDEF_Fct_t *pWriteFct = NULL;

bool RW_NDEF_SetMessage(unsigned char *pMessage, unsigned short Message_size, void *pCb)
{
    if (Message_size <= RW_MAX_NDEF_FILE_SIZE)
    {
        pRW_NdefMessage = pMessage;
        RW_NdefMessage_size = Message_size;
        pRW_NDEF_PushCb = (RW_NDEF_Callback_t*) pCb;
        return true;
    }
    else
    {
        RW_NdefMessage_size = 0;
        pRW_NDEF_PushCb = NULL;
        return false;
    }
}

void RW_NDEF_RegisterPullCallback(void *pCb)
{
    pRW_NDEF_PullCb = (RW_NDEF_Callback_t *) pCb;
}

void RW_NDEF_Reset(unsigned char type)
{
    pReadFct = NULL;
    pWriteFct = NULL;

    switch (type)
    {
    case RW_NDEF_TYPE_T1T:
        RW_NDEF_T1T_Reset();
        pReadFct = RW_NDEF_T1T_Read_Next;
        break;
    case RW_NDEF_TYPE_T2T:
        RW_NDEF_T2T_Reset();
        pReadFct = RW_NDEF_T2T_Read_Next;
        pWriteFct = RW_NDEF_T2T_Write_Next;
        break;
    case RW_NDEF_TYPE_T3T:
        RW_NDEF_T3T_Reset();
        pReadFct = RW_NDEF_T3T_Read_Next;
        break;
    case RW_NDEF_TYPE_T4T:
        RW_NDEF_T4T_Reset();
        pReadFct = RW_NDEF_T4T_Read_Next;
        pWriteFct = RW_NDEF_T4T_Write_Next;
        break;
    default:
        break;
    }
}

void RW_NDEF_Read_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size)
{
    if (pReadFct != NULL) pReadFct(pCmd, Cmd_size, Rsp, pRsp_size);
}

void RW_NDEF_Write_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size)
{
    if (pWriteFct != NULL) pWriteFct(pCmd, Cmd_size, Rsp, pRsp_size);
}
#endif
#endif
