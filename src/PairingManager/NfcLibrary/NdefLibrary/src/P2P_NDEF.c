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

#ifdef P2P_SUPPORT
#include <TML/inc/tool.h>
#include <NfcLibrary/NdefLibrary/inc/P2P_NDEF.h>

/* Well-known LLCP SAP Values */
#define SAP_SDP         1
#define SAP_SNEP        4

/* SNEP codes */
#define SNEP_VER10      0x10
#define SNEP_PUT        0x2
#define SNEP_SUCCESS    0x81

/* LLCP PDU Types */
#define SYMM            0x0
#define PAX             0x1
#define AGF             0x2
#define UI              0x3
#define CONNECT         0x4
#define DISC            0x5
#define CC              0x6
#define DM              0x7
#define FRMR            0x8
#define SNL             0x9
#define reservedA       0xA
#define reservedB       0xB
#define I               0xC
#define RR              0xD
#define RNR             0xE
#define reservedF       0xF

/* LLCP parameters */
#define VERSION     1
#define MIUX        2
#define WKS         3
#define LTO         4
#define RW          5
#define SN          6

const unsigned char SNEP_PUT_SUCCESS[] = {SNEP_VER10, SNEP_SUCCESS, 0x00, 0x00, 0x00, 0x00};
const unsigned char LLCP_CONNECT_SNEP[] = {0x11, 0x20};
const unsigned char LLCP_I_SNEP_PUT_HEADER[] = {SNEP_VER10, SNEP_PUT, 0x00, 0x00, 0x00, 0x00};
const unsigned char LLCP_SYMM[] = {0x00, 0x00};

unsigned char *pNdefMessage;
unsigned short NdefMessage_size = 0;

/* Defines the number of symmetry exchanges is expected before initiating the NDEF push (to allow a remote phone to beam an NDEF message first) */
#define NDEF_PUSH_DELAY_COUNT    2

/* Defines at which frequency the symmetry is exchange (in ms) */
#define SYMM_FREQ    500

typedef enum
{
    Idle,
    Initial,
    DelayingPush,
    SnepClientConnecting,
    SnepClientConnected,
    NdefMsgSent
} P2P_SnepClient_state_t;

typedef struct
{
    unsigned char Dsap;
    unsigned char Pdu;
    unsigned char Ssap;
    unsigned char Version;
    unsigned short Miux;
    unsigned short Wks;
    unsigned char Lto;
    unsigned char Rw;
    unsigned char Sn[30];
} P2P_NDEF_LlcpHeader_t;

typedef void P2P_NDEF_Callback_t (unsigned char*, unsigned short);

static P2P_SnepClient_state_t eP2P_SnepClient_State = Initial;
static P2P_NDEF_Callback_t *pP2P_NDEF_PushCb = NULL;
static P2P_NDEF_Callback_t *pP2P_NDEF_PullCb = NULL;
static unsigned short P2P_SnepClient_DelayCount = NDEF_PUSH_DELAY_COUNT;

static void ParseLlcp(unsigned char *pBuf, unsigned short BufSize, P2P_NDEF_LlcpHeader_t *pLlcpHeader)
{
    uint8_t i = 2;

    pLlcpHeader->Dsap = pBuf[0] >> 2;
    pLlcpHeader->Pdu = ((pBuf[0] & 3) << 2) + (pBuf[1] >> 6);
    pLlcpHeader->Ssap = pBuf[1] & 0x3F;

    while(i<BufSize)
    {
        switch (pBuf[i]){
        case VERSION:
            pLlcpHeader->Version = pBuf[i+2];
            break;
        case MIUX:
            pLlcpHeader->Miux = (pBuf[i+2] << 8) + pBuf[i+3];
            break;
        case WKS:
            pLlcpHeader->Wks = (pBuf[i+2] << 8) + pBuf[i+3];
            break;
        case LTO:
            pLlcpHeader->Lto = pBuf[i+2];
            break;
        case RW:
            pLlcpHeader->Rw = pBuf[i+2];
            break;
        case SN:
            memcpy(pLlcpHeader->Sn, &pBuf[i+2], pBuf[i+1] < sizeof(pLlcpHeader->Sn) ? pBuf[i+1] : sizeof(pLlcpHeader->Sn));
            break;
        default:
            break;
        }
        i += pBuf[i+1]+2;
    }
}

static void FillLlcp(P2P_NDEF_LlcpHeader_t LlcpHeader, unsigned char *pBuf)
{
    pBuf[0] = (LlcpHeader.Ssap << 2) + ((LlcpHeader.Pdu >> 2) & 3);
    pBuf[1] = (LlcpHeader.Pdu << 6) + LlcpHeader.Dsap;
}

bool P2P_NDEF_SetMessage(unsigned char *pMessage, unsigned short Message_size, void *pCb)
{
    if (Message_size <= P2P_NDEF_MAX_NDEF_MESSAGE_SIZE)
    {
        pNdefMessage = pMessage;
        NdefMessage_size = Message_size;
        pP2P_NDEF_PushCb = (P2P_NDEF_Callback_t*) pCb;
        return true;
    }
    else
    {
        NdefMessage_size = 0;
        pP2P_NDEF_PushCb = NULL;
        return false;
    }
}

void P2P_NDEF_RegisterPullCallback(void *pCb)
{
    pP2P_NDEF_PullCb = (P2P_NDEF_Callback_t*) pCb;
}

void P2P_NDEF_Reset(void)
{
    if (NdefMessage_size != 0)
    {
        eP2P_SnepClient_State = Initial;
    }
    else
    {
        eP2P_SnepClient_State = Idle;
    }
}

void P2P_NDEF_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *pRsp, unsigned short *pRsp_size)
{
    P2P_NDEF_LlcpHeader_t LlcpHeader;
    
    /* Initialize answer */
    *pRsp_size = 0;

    ParseLlcp(pCmd, Cmd_size, &LlcpHeader);
    
    switch (LlcpHeader.Pdu)
    {
    case CONNECT:
        /* Is connection from SNEP Client ? */
        if ((LlcpHeader.Dsap == SAP_SNEP) || (memcmp(LlcpHeader.Sn, "urn:nfc:sn:snep", 15) == 0))
        {
            /* Only accept the connection is application is registered for NDEF reception */
            if(pP2P_NDEF_PullCb != NULL)
            {
                LlcpHeader.Pdu = CC;
                FillLlcp(LlcpHeader, pRsp);
                *pRsp_size = 2;
            }
        }
        else
        {
            /* Refuse any other connection request */
            LlcpHeader.Pdu = DM;
            FillLlcp(LlcpHeader, pRsp);
            *pRsp_size = 2;
        }
        break;

    case I:
        /* Is SNEP PUT ? */
        if ((pCmd[3] == SNEP_VER10) && (pCmd[4] == SNEP_PUT))
        {
            /* Notify application of the NDEF reception */
            if(pP2P_NDEF_PullCb != NULL) pP2P_NDEF_PullCb(&pCmd[9], pCmd[8]);

            /* Acknowledge the PUT request */
            LlcpHeader.Pdu = I;
            FillLlcp(LlcpHeader, pRsp);
            pRsp[2] = (pCmd[2] >> 4) + 1; // N(R)
            memcpy(&pRsp[3], SNEP_PUT_SUCCESS, sizeof(SNEP_PUT_SUCCESS));
            *pRsp_size = 9;
        }
        break;

    case CC:
        /* Connection to remote SNEP server completed, send NDEF message inside SNEP PUT request */
        eP2P_SnepClient_State = SnepClientConnected;
        break;

    default:
        break;

    }

    /* No answer was set */
    if (*pRsp_size == 0)
    {
        switch(eP2P_SnepClient_State)
        {
        case Initial:
            if((pP2P_NDEF_PullCb == NULL) || (NDEF_PUSH_DELAY_COUNT == 0))
            {
                memcpy(pRsp, LLCP_CONNECT_SNEP, sizeof(LLCP_CONNECT_SNEP));
                *pRsp_size = sizeof(LLCP_CONNECT_SNEP);
                eP2P_SnepClient_State = SnepClientConnecting;
            }
            else
            {
                P2P_SnepClient_DelayCount = 1;
                eP2P_SnepClient_State = DelayingPush;
                /* Wait then send a SYMM */
                Sleep (SYMM_FREQ);
                memcpy(pRsp, LLCP_SYMM, sizeof(LLCP_SYMM));
                *pRsp_size = sizeof(LLCP_SYMM);
            }
            break;

        case DelayingPush:
            if(P2P_SnepClient_DelayCount == NDEF_PUSH_DELAY_COUNT)
            {
                memcpy(pRsp, LLCP_CONNECT_SNEP, sizeof(LLCP_CONNECT_SNEP));
                *pRsp_size = sizeof(LLCP_CONNECT_SNEP);
                eP2P_SnepClient_State = SnepClientConnecting;
            }
            else
            {
                P2P_SnepClient_DelayCount++;
                /* Wait then send a SYMM */
                Sleep (1000);
                memcpy(pRsp, LLCP_SYMM, sizeof(LLCP_SYMM));
                *pRsp_size = sizeof(LLCP_SYMM);
            }
            break;

        case SnepClientConnected:
            LlcpHeader.Pdu = I;
            FillLlcp(LlcpHeader, pRsp);
            pRsp[2] = 0; // N(R)
            pRsp[3] = SNEP_VER10;
            pRsp[4] = SNEP_PUT;
            pRsp[5] = 0;
            pRsp[6] = 0;
            pRsp[7] = 0;
            pRsp[8] = (unsigned char) NdefMessage_size;
            memcpy(&pRsp[9], pNdefMessage, NdefMessage_size);
            *pRsp_size = 9 + NdefMessage_size;
            eP2P_SnepClient_State = NdefMsgSent;
            /* Notify application of the NDEF push */
            if(pP2P_NDEF_PushCb != NULL) pP2P_NDEF_PushCb(pNdefMessage, NdefMessage_size);
            break;

        default:
            /* Wait then send a SYMM */
            Sleep (SYMM_FREQ);
            memcpy(pRsp, LLCP_SYMM, sizeof(LLCP_SYMM));
            *pRsp_size = sizeof(LLCP_SYMM);
            break;
        }
    }
}
#endif
