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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <TML/inc/tool.h>
#include <NfcLibrary/inc/Nfc.h>
#include <NfcTask/inc/ndef_helper.h>

//#define RW_NDEF_WRITING
//#define RW_RAW_EXCHANGE
//#define CARDEMU_RAW_EXCHANGE

void print_buf(char *x, char *y, int z)
{
    int loop;
    char b[256];
    char buf[10000];
    strcpy(buf, x);
    for(loop=0;loop<z;loop++) {
        sprintf(b, "%.2x ", y[loop]);
        strcat(buf, b);
    }
    strcat(buf, "\n");
    framework_Log(buf);
}

/* Discovery loop configuration according to the targeted modes of operation */
unsigned char DiscoveryTechnologies[] = {
#if defined P2P_SUPPORT || defined RW_SUPPORT
    MODE_POLL | TECH_PASSIVE_NFCA,
    MODE_POLL | TECH_PASSIVE_NFCF,
#endif // if defined P2P_SUPPORT || defined RW_SUPPORT
#ifdef RW_SUPPORT
    MODE_POLL | TECH_PASSIVE_NFCB,
    MODE_POLL | TECH_PASSIVE_15693,
#endif // ifdef RW_SUPPORT
#ifdef P2P_SUPPORT
    /* Only one POLL ACTIVE mode can be enabled, if both are defined only NFCF applies */
    MODE_POLL | TECH_ACTIVE_NFCA,
    //MODE_POLL | TECH_ACTIVE_NFCF,
#endif // ifdef P2P_SUPPORT
#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
    MODE_LISTEN | TECH_PASSIVE_NFCA,
#endif // if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
#if defined CARDEMU_SUPPORT
    MODE_LISTEN | TECH_PASSIVE_NFCB,
#endif // if defined CARDEMU_SUPPORT
#ifdef P2P_SUPPORT
    MODE_LISTEN | TECH_PASSIVE_NFCF,
    MODE_LISTEN | TECH_ACTIVE_NFCA,
    MODE_LISTEN | TECH_ACTIVE_NFCF,
#endif // ifdef P2P_SUPPORT
};

/* Mode configuration according to the targeted modes of operation */
unsigned mode = 0
#ifdef CARDEMU_SUPPORT
              | NXPNCI_MODE_CARDEMU
#endif // ifdef P2P_SUPPORT
#ifdef P2P_SUPPORT
              | NXPNCI_MODE_P2P
#endif // ifdef CARDEMU_SUPPORT
#ifdef RW_SUPPORT
              | NXPNCI_MODE_RW
#endif // ifdef RW_SUPPORT
;

#if defined P2P_SUPPORT || defined RW_SUPPORT
void NdefPull_Cb(unsigned char *pNdefMessage, unsigned short NdefMessageSize)
{
    unsigned char *pNdefRecord = pNdefMessage;
    NdefRecord_t NdefRecord;
    unsigned char save;

    if (pNdefMessage == NULL)
    {
        framework_Log("--- Issue during NDEF message reception (check provisioned buffer size) \n");
        return;
    }

    while (pNdefRecord != NULL)
    {
        framework_Log("--- NDEF record received:\n");

        NdefRecord = DetectNdefRecordType(pNdefRecord);

        switch(NdefRecord.recordType)
        {
        case MEDIA_VCARD:
            {
                save = NdefRecord.recordPayload[NdefRecord.recordPayloadSize];
                NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = '\0';
                framework_Log("   vCard:\n%s\n", NdefRecord.recordPayload);
                NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = save;
            }
            break;

        case WELL_KNOWN_SIMPLE_TEXT:
            {
                save = NdefRecord.recordPayload[NdefRecord.recordPayloadSize];
                NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = '\0';
                framework_Log("   Text record: %s\n", &NdefRecord.recordPayload[NdefRecord.recordPayload[0]+1]);
                NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = save;
            }
            break;

        case WELL_KNOWN_SIMPLE_URI:
            {
                save = NdefRecord.recordPayload[NdefRecord.recordPayloadSize];
                NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = '\0';
                framework_Log("   URI record: %s%s\n", ndef_helper_UriHead(NdefRecord.recordPayload[0]), &NdefRecord.recordPayload[1]);
                NdefRecord.recordPayload[NdefRecord.recordPayloadSize] = save;
            }
            break;

        case MEDIA_HANDOVER_WIFI:
            {
                unsigned char index = 0, i;

                framework_Log("--- Received WIFI credentials:\n");
                if ((NdefRecord.recordPayload[index] == 0x10) && (NdefRecord.recordPayload[index+1] == 0x0e)) index+= 4;
                while(index < NdefRecord.recordPayloadSize)
                {
                    if (NdefRecord.recordPayload[index] == 0x10)
                    {
                        if (NdefRecord.recordPayload[index+1] == 0x45) {framework_Log("- SSID = "); for(i=0;i<NdefRecord.recordPayload[index+3];i++) framework_Log("%c", NdefRecord.recordPayload[index+4+i]); framework_Log ("\n");}
                        else if (NdefRecord.recordPayload[index+1] == 0x03) framework_Log("- Authenticate Type = %s\n", ndef_helper_WifiAuth(NdefRecord.recordPayload[index+5]));
                        else if (NdefRecord.recordPayload[index+1] == 0x0f) framework_Log("- Encryption Type = %s\n", ndef_helper_WifiEnc(NdefRecord.recordPayload[index+5]));
                        else if (NdefRecord.recordPayload[index+1] == 0x27) {framework_Log("- Network key = "); for(i=0;i<NdefRecord.recordPayload[index+3];i++) framework_Log("#"); framework_Log ("\n");}
                        index += 4 + NdefRecord.recordPayload[index+3];
                    }
                    else continue;
                }
            }
            break;

        case WELL_KNOWN_HANDOVER_SELECT:
            framework_Log("   Handover select version %d.%d\n", NdefRecord.recordPayload[0] >> 4, NdefRecord.recordPayload[0] & 0xF);
            break;

        case WELL_KNOWN_HANDOVER_REQUEST:
            framework_Log("   Handover request version %d.%d\n", NdefRecord.recordPayload[0] >> 4, NdefRecord.recordPayload[0] & 0xF);
            break;

        case MEDIA_HANDOVER_BT:
            print_buf("   BT Handover payload = ", NdefRecord.recordPayload, NdefRecord.recordPayloadSize);
            break;

        case MEDIA_HANDOVER_BLE:
            print_buf("   BLE Handover payload = ", NdefRecord.recordPayload, NdefRecord.recordPayloadSize);
            break;

        case MEDIA_HANDOVER_BLE_SECURE:
            print_buf("   BLE secure Handover payload = ", NdefRecord.recordPayload, NdefRecord.recordPayloadSize);
            break;

        default:
            framework_Log("   Unsupported NDEF record, cannot parse\n");
            break;
        }
        pNdefRecord = GetNextRecord(pNdefRecord);
    }

    framework_Log("\n");
}
#endif // if defined P2P_SUPPORT || defined RW_SUPPORT

#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
const char NDEF_MESSAGE[] = { 0xD1,   // MB/ME/CF/1/IL/TNF
        0x01,   // TYPE LENGTH
        0x07,   // PAYLOAD LENTGH
        'T',    // TYPE
        0x02,   // Status
        'e', 'n', // Language
        'T', 'e', 's', 't' };

void NdefPush_Cb(unsigned char *pNdefRecord, unsigned short NdefRecordSize) {
    framework_Log("--- NDEF Record sent\n\n");
}
#endif // if defined P2P_SUPPORT || defined CARDEMU_SUPPORT

#if defined RW_SUPPORT
#ifdef RW_RAW_EXCHANGE
void PCD_MIFARE_scenario (void)
{
    #define BLK_NB_MFC      4
    #define KEY_MFC         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    #define DATA_WRITE_MFC  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff

    bool status;
    unsigned char Resp[256];
    unsigned char RespSize;
    /* Authenticate sector 1 with generic keys */
    unsigned char Auth[] = {0x40, BLK_NB_MFC/4, 0x10, KEY_MFC};
    /* Read block 4 */
    unsigned char Read[] = {0x10, 0x30, BLK_NB_MFC};
    /* Write block 4 */
    unsigned char WritePart1[] = {0x10, 0xA0, BLK_NB_MFC};
    unsigned char WritePart2[] = {0x10, DATA_WRITE_MFC};

    /* Authenticate */
    status = NxpNci_ReaderTagCmd(Auth, sizeof(Auth), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Authenticate sector %d failed with error 0x%02x\n", Auth[1], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Authenticate sector %d succeed\n", Auth[1]);

    /* Read block */
    status = NxpNci_ReaderTagCmd(Read, sizeof(Read), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Read block %d failed with error 0x%02x\n", Read[2], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Read block %d:", Read[2]); print_buf(" ", (Resp+1), RespSize-2);

    /* Write block */
    status = NxpNci_ReaderTagCmd(WritePart1, sizeof(WritePart1), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Write block %d failed with error 0x%02x\n", WritePart1[2], Resp[RespSize-1]);
        return;
    }
    status = NxpNci_ReaderTagCmd(WritePart2, sizeof(WritePart2), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Write block %d failed with error 0x%02x\n", WritePart1[2], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Block %d written\n", WritePart1[2]);

    /* Read block */
    status = NxpNci_ReaderTagCmd(Read, sizeof(Read), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Read failed with error 0x%02x\n", Resp[RespSize-1]);
        return;
    }
    framework_Log(" Read block %d:", Read[2]); print_buf(" ", (Resp+1), RespSize-2);
}

void PCD_ISO15693_scenario (void)
{
    #define BLK_NB_ISO15693     8
    #define DATA_WRITE_ISO15693 0x11, 0x22, 0x33, 0x44

    bool status;
    unsigned char Resp[256];
    unsigned char RespSize;
    unsigned char ReadBlock[] = {0x02, 0x20, BLK_NB_ISO15693};
    unsigned char WriteBlock[] = {0x02, 0x21, BLK_NB_ISO15693, DATA_WRITE_ISO15693};

    status = NxpNci_ReaderTagCmd(ReadBlock, sizeof(ReadBlock), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0x00))
    {
        framework_Log(" Read block %d failed with error 0x%02x\n", ReadBlock[2], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Read block %d:", ReadBlock[2]); print_buf(" ", (Resp+1), RespSize-2);

    /* Write */
    status = NxpNci_ReaderTagCmd(WriteBlock, sizeof(WriteBlock), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Write block %d failed with error 0x%02x\n", WriteBlock[2], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Block %d written\n", WriteBlock[2]);

    /* Read back */
    status = NxpNci_ReaderTagCmd(ReadBlock, sizeof(ReadBlock), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0x00))
    {
        framework_Log(" Read block %d failed with error 0x%02x\n", ReadBlock[2], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Read block %d:", ReadBlock[2]); print_buf(" ", (Resp+1), RespSize-2);
}

void PCD_ISO14443_3A_scenario (void)
{
    #define BLK_NB_ISO14443_3A      5
    #define DATA_WRITE_ISO14443_3A  0x11, 0x22, 0x33, 0x44

    bool status;
    unsigned char Resp[256];
    unsigned char RespSize;
    /* Read block */
    unsigned char Read[] = {0x30, BLK_NB_ISO14443_3A};
    /* Write block */
    unsigned char Write[] = {0xA2, BLK_NB_ISO14443_3A, DATA_WRITE_ISO14443_3A};
    
    /* Read */
    status = NxpNci_ReaderTagCmd(Read, sizeof(Read), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Read block %d failed with error 0x%02x\n", Read[1], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Read block %d:", Read[1]); print_buf(" ", Resp, 4);
    /* Write */
    status = NxpNci_ReaderTagCmd(Write, sizeof(Write), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Write block %d failed with error 0x%02x\n", Write[1], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Block %d written\n", Write[1]);

    /* Read back */
    status = NxpNci_ReaderTagCmd(Read, sizeof(Read), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
    {
        framework_Log(" Read block %d failed with error 0x%02x\n", Read[1], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Read block %d:", Read[1]); print_buf(" ", Resp, 4);
}

void PCD_ISO14443_4_scenario (void)
{
    bool status;
    unsigned char Resp[256];
    unsigned char RespSize;
    unsigned char SelectPPSE[] = {0x00, 0xA4, 0x04, 0x00, 0x0E, 0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31, 0x00};

    status = NxpNci_ReaderTagCmd(SelectPPSE, sizeof(SelectPPSE), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-2] != 0x90) || (Resp[RespSize-1] != 0x00))
    {
        framework_Log(" Select PPSE failed with error %02x %02x\n", Resp[RespSize-2], Resp[RespSize-1]);
        return;
    }
    framework_Log(" Select NDEF Application succeed\n");
}
#endif // ifdef RW_RAW_EXCHANGE

void displayCardInfo(NxpNci_RfIntf_t RfIntf)
{
    switch(RfIntf.Protocol){
    case PROT_T1T:
    case PROT_T2T:
    case PROT_T3T:
    case PROT_ISODEP:
        framework_Log(" - POLL MODE: Remote T%dT activated\n", RfIntf.Protocol);
        break;
    case PROT_ISO15693:
        framework_Log(" - POLL MODE: Remote ISO15693 card activated\n");
        break;
    case PROT_MIFARE:
        framework_Log(" - POLL MODE: Remote MIFARE card activated\n");
        break;
    default:
        framework_Log(" - POLL MODE: Undetermined target\n");
        return;
    }

    switch(RfIntf.ModeTech) {
    case (MODE_POLL | TECH_PASSIVE_NFCA):
        framework_Log("\tSENS_RES = 0x%.2x 0x%.2x\n", RfIntf.Info.NFC_APP.SensRes[0], RfIntf.Info.NFC_APP.SensRes[1]);
        print_buf("\tNFCID = ", RfIntf.Info.NFC_APP.NfcId, RfIntf.Info.NFC_APP.NfcIdLen);
        if(RfIntf.Info.NFC_APP.SelResLen != 0) framework_Log("\tSEL_RES = 0x%.2x\n", RfIntf.Info.NFC_APP.SelRes[0]);
    break;

    case (MODE_POLL | TECH_PASSIVE_NFCB):
        if(RfIntf.Info.NFC_BPP.SensResLen != 0) print_buf("\tSENS_RES = ", RfIntf.Info.NFC_BPP.SensRes, RfIntf.Info.NFC_BPP.SensResLen);
    break;

    case (MODE_POLL | TECH_PASSIVE_NFCF):
        framework_Log("\tBitrate = %s\n", (RfIntf.Info.NFC_FPP.BitRate==1)?"212":"424");
        if(RfIntf.Info.NFC_FPP.SensResLen != 0) print_buf("\tSENS_RES = ", RfIntf.Info.NFC_FPP.SensRes, RfIntf.Info.NFC_FPP.SensResLen);
    break;

    case (MODE_POLL | TECH_PASSIVE_15693):
        print_buf("\tID = ", RfIntf.Info.NFC_VPP.ID, sizeof(RfIntf.Info.NFC_VPP.ID));
        framework_Log("\tAFI = 0x%.2x\n", RfIntf.Info.NFC_VPP.AFI);
        framework_Log("\tDSFID = 0x%.2x\n", RfIntf.Info.NFC_VPP.DSFID);
    break;

    default:
        break;
    }
}

void task_nfc_reader(NxpNci_RfIntf_t RfIntf)
{
    /* For each discovered cards */
    while(1){
        /* Display detected card information */
        displayCardInfo(RfIntf);

        /* What's the detected card type ? */
        switch(RfIntf.Protocol) {
        case PROT_T1T:
        case PROT_T2T:
        case PROT_T3T:
        case PROT_ISODEP:
#ifndef RW_RAW_EXCHANGE
            /* Process NDEF message read */
            NxpNci_ProcessReaderMode(RfIntf, READ_NDEF);
#ifdef RW_NDEF_WRITING
            RW_NDEF_SetMessage ((unsigned char *) NDEF_MESSAGE, sizeof(NDEF_MESSAGE), *NdefPush_Cb);
            /* Process NDEF message write */
            NxpNci_ReaderReActivate(&RfIntf);
            NxpNci_ProcessReaderMode(RfIntf, WRITE_NDEF);
#endif // RW_NDEF_WRITING
#else // ifndef RW_RAW_EXCHANGE
            if (RfIntf.Protocol == PROT_ISODEP)
            {
                PCD_ISO14443_4_scenario();
            }
            else if (RfIntf.Protocol == PROT_T2T)
            {
                PCD_ISO14443_3A_scenario();
            }
#endif // ifndef RW_RAW_EXCHANGE
            break;

        case PROT_ISO15693:
#ifdef RW_RAW_EXCHANGE
            /* Run dedicated scenario to demonstrate ISO15693 card management */
            PCD_ISO15693_scenario();
#endif // ifdef RW_RAW_EXCHANGE
            break;

        case PROT_MIFARE:
#ifdef RW_RAW_EXCHANGE
            /* Run dedicated scenario to demonstrate MIFARE card management */
            PCD_MIFARE_scenario();
#endif // ifdef RW_RAW_EXCHANGE
            break;

        default:
            break;
        }

        /* If more cards (or multi-protocol card) were discovered (only same technology are supported) select next one */
        if(RfIntf.MoreTags) {
            if(NxpNci_ReaderActivateNext(&RfIntf) == NFC_ERROR) break;
        }
        /* Otherwise leave */
        else break;
    }

    /* Wait for card removal */
    NxpNci_ProcessReaderMode(RfIntf, PRESENCE_CHECK);

    framework_Log("CARD REMOVED\n");

    /* Restart discovery loop */
    NxpNci_StopDiscovery();
    NxpNci_StartDiscovery(DiscoveryTechnologies,sizeof(DiscoveryTechnologies));
}
#endif // if defined RW_SUPPORT

#if defined CARDEMU_SUPPORT
#ifdef CARDEMU_RAW_EXCHANGE
void PICC_ISO14443_4_scenario (void)
{
    unsigned char OK[] = {0x90, 0x00};
    unsigned char Cmd[256];
    unsigned char CmdSize;

    while (1)
    {
        if(NxpNci_CardModeReceive(Cmd, &CmdSize) == NFC_SUCCESS)
        {
            if ((CmdSize >= 2) && (Cmd[0] == 0x00))
            {
            	switch (Cmd[1])
            	{
            	case 0xA4:
                    framework_Log("Select File received\n");
            		break;

            	case 0xB0:
                    framework_Log("Read Binary received\n");
            		break;

            	case 0xD0:
                    framework_Log("Write Binary received\n");
            		break;

            	default:
            		break;
            	}

                NxpNci_CardModeSend(OK, sizeof(OK));
            }
        }
        else
        {
            framework_Log("End of transaction\n");
            return;
        }
    }
}
#endif // if defined CARDEMU_SUPPORT
#endif

void task_nfc(void)
{
    NxpNci_RfIntf_t RfInterface;

#ifdef CARDEMU_SUPPORT
    /* Register NDEF message to be sent to remote reader */
    T4T_NDEF_EMU_SetMessage((unsigned char *) NDEF_MESSAGE, sizeof(NDEF_MESSAGE), *NdefPush_Cb);
#endif // if defined CARDEMU_SUPPORT

#ifdef P2P_SUPPORT
    /* Register NDEF message to be sent to remote peer */
    P2P_NDEF_SetMessage((unsigned char *) NDEF_MESSAGE, sizeof(NDEF_MESSAGE), *NdefPush_Cb);
    /* Register callback for reception of NDEF message from remote peer */
    P2P_NDEF_RegisterPullCallback(*NdefPull_Cb);
#endif // if defined P2P_SUPPORT

#ifdef RW_SUPPORT
    /* Register callback for reception of NDEF message from remote cards */
    RW_NDEF_RegisterPullCallback(*NdefPull_Cb);
#endif // if defined RW_SUPPORT

    /* Open connection to NXPNCI device */
    if (NxpNci_Connect() == NFC_ERROR) {
        framework_Log("Error: cannot connect to NXPNCI device\n");
        return;
    }

    if (NxpNci_ConfigureSettings() == NFC_ERROR) {
        framework_Log("Error: cannot configure NXPNCI settings\n");
        return;
    }

    if (NxpNci_ConfigureMode(mode) == NFC_ERROR)
    {
        framework_Log("Error: cannot configure NXPNCI\n");
        return;
    }

    /* Start Discovery */
    if (NxpNci_StartDiscovery(DiscoveryTechnologies,sizeof(DiscoveryTechnologies)) != NFC_SUCCESS)
    {
        framework_Log("Error: cannot start discovery\n");
        return;
    }

    while(1)
    {
        framework_Log("\nWAITING FOR DEVICE DISCOVERY\n");

        /* Wait until a peer is discovered */
        while(NxpNci_WaitForDiscoveryNotification(&RfInterface) != NFC_SUCCESS);

#ifdef CARDEMU_SUPPORT
        /* Is activated from remote T4T ? */
        if ((RfInterface.Interface == INTF_ISODEP) && ((RfInterface.ModeTech & MODE_MASK) == MODE_LISTEN))
        {
            framework_Log(" - LISTEN MODE: Activated from remote Reader\n");
#ifndef CARDEMU_RAW_EXCHANGE
            NxpNci_ProcessCardMode(RfInterface);
#else
            PICC_ISO14443_4_scenario();
#endif
            framework_Log("READER DISCONNECTED\n");
        }
        else
#endif

#ifdef P2P_SUPPORT
        /* Is activated from remote T4T ? */
        if (RfInterface.Interface == INTF_NFCDEP)
        {
            if ((RfInterface.ModeTech & MODE_LISTEN) == MODE_LISTEN) framework_Log(" - P2P TARGET MODE: Activated from remote Initiator\n");
            else framework_Log(" - P2P INITIATOR MODE: Remote Target activated\n");

            /* Process with SNEP for NDEF exchange */
            NxpNci_ProcessP2pMode(RfInterface);

            framework_Log("PEER LOST\n");
        }
        else
#endif
#ifdef RW_SUPPORT
        if ((RfInterface.ModeTech & MODE_MASK) == MODE_POLL)
        {
            task_nfc_reader(RfInterface);
        }
        else
#endif
        {
            framework_Log("WRONG DISCOVERY\n");
        }
    }
}
