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

/* this sample will scan the currently configured channel for all nodes,
   returning the values of a few useful settings */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <xbee.h>

#define MAXNODES 100

int main(int argc, char *argv[]) {
  int i;
  int saidfull = 0;
  int ATNT = 0x19; /* node discover timeout */
  int ATNTc; /* counter */

  int nodes = 0;
  char addrs[MAXNODES][8];

  xbee_con *con;
  xbee_pkt *pkt, *rpkt;

  time_t ttime;
  char stime[32];

  /* setup libxbee */
  if (xbee_setup("/dev/ttyUSB0",57600) == -1) {
    return 1;
  }

  /* grab a local AT connection */
  con = xbee_newcon('I',xbee_localAT);

  /* get the ND timeout */
  xbee_senddata(con,"NT");
  if ((rpkt = xbee_getpacketwait(con)) == NULL) {
    printf("XBee didnt return a result for NT\n");
    return 1;
  }
  ATNT = rpkt->data[0];
  free(rpkt);

  while (1) {
    /* send a ND - Node Discover request */
    xbee_senddata(con,"ND");
    /* only wait for a bit longer than the ND timeout */
    ATNTc = ATNT + 10;
    /* loop until the end packet has been received or timeout reached */
    while (ATNTc--) {
      /* get a packet */
      pkt = xbee_getpacketwait(con);
      /* check a packet was returned, and that its one we are after... */
      if (pkt && !memcmp(pkt->atCmd,"ND",2)) {
        /* is this the end packet? you can tell from the 0 datalen */
        if (pkt->datalen == 0) {
          /* free the packet */
          free(pkt);
          break;
        } else {
          /* check if we know this node already */
          for (i = 0; i < nodes; i++) {
            /* the 64bit address will match one in the list */
            if (!memcmp(&(pkt->data[2]),&(addrs[i]),8)) break;
          }
          ttime = time(NULL);
          strftime(stime,32,"%I:%M:%S %p",gmtime(&ttime));
          /* is there space for another? */
          if ((i == nodes) &&
              (nodes == MAXNODES) &&
              (!saidfull)) {
            printf("MAXNODES reached... Can't add more...\r");
            /* flush so the change is seen! */
            fflush(stdout);
            saidfull = 1;
          } else {
            /* is this a rewrite? */
            if (i != nodes) {
              /* find the line to edit */
              printf("%c[%dA",27,nodes-i);
              /* clear the line */
              printf("%c[2K",27);
            }
            /* write out the info */
            memcpy(&(addrs[nodes]),&(pkt->data[2]),8);
            printf("MY: 0x%02X%02X    ",pkt->data[0],pkt->data[1]);
            printf("SH: 0x%02X%02X%02X%02X    ",pkt->data[2],pkt->data[3],pkt->data[4],pkt->data[5]);
            printf("SL: 0x%02X%02X%02X%02X    ",pkt->data[6],pkt->data[7],pkt->data[8],pkt->data[9]);
            printf("dB: -%2d    ",pkt->data[10]);
            printf("NI: %-20s    ",&(pkt->data[11]));
            printf("@: %s",stime);
            /* is this a rewrite? */
            if (i != nodes) {
              /* go back the the bottom */
              printf("%c[%dB\r",27,nodes-i);
            } else {
              /* new line is only wanted for new nodes */
              printf("\n");
              /* if not, then add 1 to the number of nodes! */
              nodes++;
            }
          }
          /* flush so the change is seen! */
          fflush(stdout);
        }
        /* free the packet */
        free(pkt);
      }
      /* sleep for 100ms (same as NT steps) */
      usleep(100000);
    }
    /* try again! */
    usleep(100000);
  }

  return 0;
}

