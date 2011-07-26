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

/* this sample will return any recieved data, using a callback function */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <xbee.h>

void sighandler(int sig) {
  xbee_pkt *pkt;
  if (sig == SIGINT) {
    xbee_end();
    exit(0);
  }
}

void callback(xbee_con *con, xbee_pkt *pkt) {
  int ret;
  /* print the recieved data */
  printf("Rx: %s\n",pkt->data);
  /* say thank you */
  if ((ret = xbee_senddata(con,"%s",pkt->data)) != 0) {
    printf("xbee_senddata: Error %d... Retrying!\n",ret);
    if ((ret = xbee_senddata(con,"%s",pkt->data)) != 0) {
      printf("xbee_senddata: Error %d... Data lost!\n",ret);
    } else {
      printf("xbee_senddata: Success after retry!\n",ret);
    }
  }
free(pkt);
}

int main(int argc, char *argv[]) {
  xbee_con *con;
  xbee_pkt *pkt, *rpkt;

  /* setup the xbee */
  //if (xbee_setupAPI("/dev/ttyUSB0",57600,'+',250) == -1) {
  //if (xbee_setuplogAPI("/dev/ttyUSB0",57600,2,'+',250) == -1) {
  if (xbee_setuplog("/dev/ttyUSB0",57600,2) == -1) {
    /* oh no... it failed */
    printf("xbee_setup() failed...\n");
    exit(1);
  }

  /* handle ^C */
  signal(SIGINT, sighandler);

  /* setup a connection */
  con = xbee_newcon('I',xbee_64bitData, 0x0013A200, 0x40081826);
  con->waitforACK = 1;
  con->callback = callback;

  printf("Waiting...\n");

  /* do nothing forever! */
  for (;;) {
    sleep(86400); /* 24hrs */
  }

  /* shouldn't ever get here but... */
  return 0;
}

