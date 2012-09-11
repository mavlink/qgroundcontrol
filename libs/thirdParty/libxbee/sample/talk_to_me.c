#ifdef shell
gcc -o ${0//.c/} $0 -lxbee -g
exit
}
#endif
/*
  libxbee - a C library to aid the use of Digi's Series 1 XBee modules
            running in API mode (AP=2).

  Copyright (C) 2009  Attie Grande (attie@attie.co.uk)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* this sample will make the remote XBee talk to us! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xbee.h>

int main(int argc, char *argv[]) {
  union {
    unsigned char as8[8];
    unsigned int as32[2];
  } addr;
  xbee_con *atCon, *rCon;
  xbee_pkt *pkt;

  /* the extra arguments are the CC ('+' by default) and GT (1000) by default AT values */
  xbee_setuplogAPI("/dev/ttyUSB0",57600,2,'+',1000);

  atCon = xbee_newcon('@', xbee_localAT);

  xbee_senddata(atCon, "SH");
  pkt = xbee_getpacketwait(atCon);
  if (!pkt || pkt->status || pkt->atCmd[0] != 'S' || pkt->atCmd[1] != 'H') {
    printf("Missing SH Packet!\n");
    return 1;
  }
  addr.as8[3] = pkt->data[0];
  addr.as8[2] = pkt->data[1];
  addr.as8[1] = pkt->data[2];
  addr.as8[0] = pkt->data[3];
  free(pkt);

  xbee_senddata(atCon, "SL");
  pkt = xbee_getpacketwait(atCon);
  if (!pkt || pkt->status || pkt->atCmd[0] != 'S' || pkt->atCmd[1] != 'L') {
    printf("Missing SL Packet!\n");
    return 1;
  }
  addr.as8[7] = pkt->data[0];
  addr.as8[6] = pkt->data[1];
  addr.as8[5] = pkt->data[2];
  addr.as8[4] = pkt->data[3];
  free(pkt);

  printf("Local XBee address is: 0x%08X %08X\n", addr.as32[0], addr.as32[1]);
    
  rCon = xbee_newcon('#', xbee_64bitRemoteAT, 0x13A200, 0x403CB26A);
  
  xbee_senddata(rCon, "DH%c%c%c%c", addr.as8[3], addr.as8[2], addr.as8[1], addr.as8[0]);
  usleep(250000);
  xbee_senddata(rCon, "DL%c%c%c%c", addr.as8[7], addr.as8[6], addr.as8[5], addr.as8[4]);
  usleep(250000);
  
  /* calling xbee_end() will return the xbee to its previous API mode */
  xbee_end();
  return 0;
}
