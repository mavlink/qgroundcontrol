#ifdef shell
gcc -o ${0//.c/} $0 -lxbee
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

/* this sample will output the voltage read from analog 0 */

#include <stdio.h>
#include <stdlib.h>
#include <xbee.h>

/* set this to the voltage measured between GND and Vref - 3.3 is a good place to start */
#define Vref 3.3

int main(int argc, char *argv[]) {
  xbee_con *con;
  xbee_pkt *pkt;

  /* setup libxbee */
  if (xbee_setup("/dev/ttyUSB0",57600) == -1) {
    printf("xbee_setup failed...\n");
    return 1;
  }

  /* create an AT connection */
  con = xbee_newcon('I',xbee_64bitRemoteAT,0x13A200,0x403CB26A);

  /* do this forever! */
  for (;;) {
    /* request samples now! */
    xbee_senddata(con,"IS");
    /* get as many packets as we can */
    while ((pkt = xbee_getpacketwait(con)) != NULL) {
      if (pkt) {
	if (pkt->status != 0x00) {
	  /* if the return status was not 0x00 (OK) then the request failed... */
	  printf("Sample A0: -- Request Failed --\n");
	} else {
          if (!xbee_hasanalog(pkt,0,0)) {
            /* there was no data for A0 in the packet */
            printf("Sample A0: -- No Data --\n");
          } else {
            /* it appears that there is sample data for A0! */
	    printf("Sample A0: %.0f (~%.2fv)\n",
		   xbee_getanalog(pkt,0,0,0),
		   xbee_getanalog(pkt,0,0,Vref));
	  }
	}
	/* dont forget to free the packet! */
	free(pkt);
      } else {
	/* couldnt get a packet */
	printf("Sample A0: -- No Packet Returned --\n");
      }
    }
    /* wait a second for the next sample */
    sleep(1);
  }
  
  return 0;
}
