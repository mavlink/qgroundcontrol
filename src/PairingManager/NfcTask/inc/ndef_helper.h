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

#define NDEF_EMPTY            0x00
#define NDEF_WELL_KNOWN       0x01
#define NDEF_MEDIA            0x02
#define NDEF_ABSOLUTE_URI     0x03
#define NDEF_EXTERNAL         0x04
#define NDEF_UNKNOWN          0x05
#define NDEF_UNCHANGED        0x06
#define NDEF_RESERVED         0x07

#define NDEF_RECORD_MB_MASK   0x80
#define NDEF_RECORD_ME_MASK   0x40
#define NDEF_RECORD_CF_MASK   0x20
#define NDEF_RECORD_SR_MASK   0x10
#define NDEF_RECORD_IL_MASK   0x08
#define NDEF_RECORD_TNF_MASK  0x07

typedef enum
{
    WELL_KNOWN_SIMPLE_TEXT,
    WELL_KNOWN_SIMPLE_URI,
    WELL_KNOWN_SMART_POSTER,
    WELL_KNOWN_HANDOVER_SELECT,
    WELL_KNOWN_HANDOVER_REQUEST,
    WELL_KNOWN_ALTERNATIVE_CARRIER,
    WELL_KNOWN_COLLISION_RESOLUTION,
    MEDIA_VCARD,
    MEDIA_HANDOVER_WIFI,
    MEDIA_HANDOVER_BT,
    MEDIA_HANDOVER_BLE,
    MEDIA_HANDOVER_BLE_SECURE,
    ABSOLUTE_URI,
    UNSUPPORTED_NDEF_RECORD = 0xFF
}NdefRecordType_e;

typedef struct
{
    NdefRecordType_e recordType;
    unsigned char *  recordPayload;
    unsigned int     recordPayloadSize;
} NdefRecord_t;

const char* ndef_helper_WifiAuth(unsigned char auth);
const char* ndef_helper_WifiEnc(unsigned char enc);
const char* ndef_helper_UriHead(unsigned char head);
NdefRecord_t DetectNdefRecordType(unsigned char *pNdefRecord);
unsigned char* GetNextRecord(unsigned char *pNdefRecord);


