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
#include <string.h>
#include <NfcTask/inc/ndef_helper.h>

const char* ndef_helper_WifiAuth(unsigned char auth)
{
    switch (auth)
    {
    case 0x01: return "Open";
    case 0x02: return "WPA-Personal";
    case 0x04: return "Shared";
    case 0x08: return "WPA-Enterprise";
    case 0x10: return "WPA2-Enterprise";
    case 0x20: return "WPA2-Personal";
    default: return "unknown";
    }
}

const char* ndef_helper_WifiEnc(unsigned char enc)
{
    switch (enc)
    {
    case 0x01: return "None";
    case 0x02: return "WEP";
    case 0x04: return "TKIP";
    case 0x08: return "AES";
    case 0x10: return "AES/TKIP";
    default: return "unknown";
    }
}

const char* ndef_helper_UriHead(unsigned char head)
{
    switch (head) {
    case 0:    return ("");
    case 1:    return ("http://www.");
    case 2:    return ("https://www.");
    case 3:    return ("http://");
    case 4:    return ("https://");
    case 5:    return ("tel:");
    case 6:    return ("mailto:");
    default:return ("!!!unknown!!!");
    }
}

NdefRecord_t DetectNdefRecordType(unsigned char *pNdefRecord)
{
    NdefRecord_t record;

    uint8_t typeField;

    /* Short or normal record ?*/
    if (pNdefRecord[0] & NDEF_RECORD_SR_MASK)
    {
        record.recordPayloadSize = pNdefRecord[2];
        typeField = 3;
    }
    else
    {
        record.recordPayloadSize = (pNdefRecord[2]<<24) + (pNdefRecord[3]<<16) + (pNdefRecord[4]<<8) + pNdefRecord[5];
        typeField = 6;
    }

    /* ID present ?*/
    if(pNdefRecord[0] & NDEF_RECORD_IL_MASK)
    {
        record.recordPayload = pNdefRecord + typeField + pNdefRecord[1] + 1 + pNdefRecord[typeField];
        typeField++;
    }
    else
    {
        record.recordPayload = pNdefRecord + typeField + pNdefRecord[1];
    }

    /* Well known Record Type ?*/
    if ((pNdefRecord[0] & NDEF_RECORD_TNF_MASK) == NDEF_WELL_KNOWN)
    {
        if (pNdefRecord[1] == 0x1)
        {
            switch (pNdefRecord[typeField])
            {
            case 'T':
                record.recordType = WELL_KNOWN_SIMPLE_TEXT;
                break;
            case 'U':
                record.recordType = WELL_KNOWN_SIMPLE_URI;
                break;
            default:
                record.recordType = UNSUPPORTED_NDEF_RECORD;
                break;
            }
        } else if (pNdefRecord[1] == 0x2)
        {
            if (memcmp(&pNdefRecord[typeField], "Sp", pNdefRecord[1]) == 0x0)
            {
                record.recordType = WELL_KNOWN_SMART_POSTER;
            }
            else if (memcmp(&pNdefRecord[typeField], "Hs", pNdefRecord[1]) == 0x0)
            {
                record.recordType = WELL_KNOWN_HANDOVER_SELECT;
            }
            else if (memcmp(&pNdefRecord[typeField], "Hr", pNdefRecord[1]) == 0x0)
            {
                record.recordType = WELL_KNOWN_HANDOVER_REQUEST;
            }
            else if (memcmp(&pNdefRecord[typeField], "ac", pNdefRecord[1]) == 0x0)
            {
                record.recordType = WELL_KNOWN_ALTERNATIVE_CARRIER;
            }
            else if (memcmp(&pNdefRecord[typeField], "cr", pNdefRecord[1]) == 0x0)
            {
                record.recordType = WELL_KNOWN_COLLISION_RESOLUTION;
            }
            else
            {
                record.recordType = UNSUPPORTED_NDEF_RECORD;
            }
        }
    }
    /* Media Record Type ?*/
    else if ((pNdefRecord[0] & NDEF_RECORD_TNF_MASK) == NDEF_MEDIA)
    {
        if ((memcmp(&pNdefRecord[typeField], "text/x-vCard", pNdefRecord[1]) == 0x0) ||
            (memcmp(&pNdefRecord[typeField], "text/vcard", pNdefRecord[1]) == 0x0))
        {
            record.recordType = MEDIA_VCARD;
        }
        else if (memcmp(&pNdefRecord[typeField], "application/vnd.wfa.wsc", pNdefRecord[1]) == 0x0)
        {
            record.recordType = MEDIA_HANDOVER_WIFI;
        }
        else if (memcmp(&pNdefRecord[typeField], "application/vnd.bluetooth.ep.oob", pNdefRecord[1]) == 0x0)
        {
            record.recordType = MEDIA_HANDOVER_BT;
        }
        else if (memcmp(&pNdefRecord[typeField], "application/vnd.bluetooth.le.oob", pNdefRecord[1]) == 0x0)
        {
            record.recordType = MEDIA_HANDOVER_BLE;
        }
        else if (memcmp(&pNdefRecord[typeField], "application/vnd.bluetooth.secure.le.oob", pNdefRecord[1]) == 0x0)
        {
            record.recordType = MEDIA_HANDOVER_BLE_SECURE;
        }
        else
        {
            record.recordType = UNSUPPORTED_NDEF_RECORD;
        }
    }
    /* Absolute URI Record Type ?*/
    else if ((pNdefRecord[0] & NDEF_RECORD_TNF_MASK) == NDEF_ABSOLUTE_URI)
    {
        record.recordType = ABSOLUTE_URI;
    }
    else
    {
        record.recordType = UNSUPPORTED_NDEF_RECORD;
    }

    return record;
}

unsigned char* GetNextRecord(unsigned char *pNdefRecord)
{
    unsigned char *temp = NULL;

    /* Message End ?*/
    if (!(pNdefRecord[0] & NDEF_RECORD_ME_MASK)) {
        /* Short or normal record ?*/
        if (pNdefRecord[0] & NDEF_RECORD_SR_MASK)
        {
            /* ID present ?*/
            if(pNdefRecord[0] & NDEF_RECORD_IL_MASK)
                temp = (pNdefRecord + 4 + pNdefRecord[1] + pNdefRecord[2] + pNdefRecord[3]);
            else
                temp = (pNdefRecord + 3 + pNdefRecord[1] + pNdefRecord[2]);
        }
        else
        {
            /* ID present ?*/
            if(pNdefRecord[0] & NDEF_RECORD_IL_MASK)
                temp = (pNdefRecord + 7 + pNdefRecord[1] + (pNdefRecord[2]<<24) + (pNdefRecord[3]<<16) + (pNdefRecord[4]<<8) + pNdefRecord[5] + pNdefRecord[6]);
            else
                temp = (pNdefRecord + 6 + pNdefRecord[1] + (pNdefRecord[2]<<24) + (pNdefRecord[3]<<16) + (pNdefRecord[4]<<8) + pNdefRecord[5]);
        }
    }
    return temp;
}
