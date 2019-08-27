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

#include <NfcLibrary/inc/Nfc.h>

#ifdef CARDEMU_SUPPORT
#include <NfcLibrary/NdefLibrary/inc/T4T_NDEF_emu.h>
#endif

#ifdef P2P_SUPPORT
#include <NfcLibrary/NdefLibrary/inc/P2P_NDEF.h>
#endif

#ifdef RW_SUPPORT
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF.h>
#include <NfcLibrary/NdefLibrary/inc/RW_NDEF_T3T.h>
#endif

#define NXPNCI_SUCCESS      NFC_SUCCESS
#define NXPNCI_ERROR        NFC_ERROR

#ifdef NCI_DEBUG
#include <stdio.h>
#define NCI_PRINT(...)        {printf(__VA_ARGS__);}
unsigned short debug_loop;
#define NCI_PRINT_LOOP(x,y)   {for(debug_loop=0; debug_loop<y; debug_loop++) printf("%.2x ", x[debug_loop]);}
#define NCI_PRINT_BUF(x,y,z)  {char tmp[200]; int loop; sprintf(tmp, x); \
                               for(loop=0;loop<(z<30?z:30);loop++) sprintf(tmp+7+(loop*3), "%.2x ", y[loop]); \
                               if(loop==30) sprintf(tmp+7+(loop*3), "...\n"); \
                               else sprintf(tmp+7+(loop*3), "\n"); \
                               printf(tmp);}
#else
#define NCI_PRINT(...)
#define NCI_PRINT_LOOP(x,y)
#define NCI_PRINT_BUF(x,y,z)
#endif

