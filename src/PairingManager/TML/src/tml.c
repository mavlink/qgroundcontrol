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

#include <stdint.h>
#include <TML/inc/tml_hid.h>

void tml_Connect(void) {
	tml_hid_Connect();
}

void tml_Disconnect(void) {
	tml_hid_Disconnect();
}

void tml_Send(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytesSent) {
	tml_hid_Send(pBuffer, BufferLen, pBytesSent);
}

void tml_Receive(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytes,
		uint16_t timeout) {
tml_hid_Receive(pBuffer, BufferLen, pBytes, timeout);
}
