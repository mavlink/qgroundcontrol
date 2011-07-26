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

/* this sample will make use of multiple instances of libxbee and send messages between the attached XBees */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <xbee.h>

int mode;
char bitmask;
char outputs;

xbee_hnd xbee1;
xbee_hnd xbee2;

void sighandler(int sig) {
  if (sig == SIGINT) {
    _xbee_end(xbee1);
    _xbee_end(xbee2);
    exit(0);
  }
}

void xbee1CB(xbee_con *con, xbee_pkt *pkt) {
  char data[128];
  snprintf(data,pkt->datalen+1,"%s",pkt->data);
  printf("XBee1: Rx[%3d]: %s\n",pkt->datalen,data);
}

void xbee2CB(xbee_con *con, xbee_pkt *pkt) {
  char data[128];
  snprintf(data,pkt->datalen+1,"%s",pkt->data);
  printf("XBee2: Rx[%3d]: %s\n",pkt->datalen,data);
}

int main(int argc, char *argv[]) {
  xbee_con *con1;
  xbee_con *con2;

  if (!(xbee1 = _xbee_setuplogAPI("/dev/ttyUSB0",57600,3,'+',250))) {
  //if (!(xbee1 = _xbee_setupAPI("/dev/ttyUSB0",57600,'+',250))) {
    printf("xbee1: setup error...\n");
    return 1;
  }
  if (!(xbee2 = _xbee_setuplogAPI("/dev/ttyUSB1",57600,4,'+',250))) {
  //if (!(xbee2 = _xbee_setupAPI("/dev/ttyUSB1",57600,'+',250))) {
    printf("xbee2: setup error...\n");
    return 1;
  }

  /* handle ^C */
  signal(SIGINT, sighandler);

  con1 = _xbee_newcon(xbee1,'1',xbee_64bitData,   0x0013A200, 0x40081826);
  con1->waitforACK = 1;
  con1->callback = xbee1CB;
  
  con2 = _xbee_newcon(xbee2,'2',xbee_64bitData,   0x0013A200, 0x404B75DE);
  con2->waitforACK = 1;
  con2->callback = xbee2CB;

  while (1) {
    printf("xbee1: Tx\n");
    _xbee_logit(xbee1,"xbee1: Tx");
    _xbee_logit(xbee2,"xbee1: Tx");
    _xbee_senddata(xbee1,con1,"Hello");
    usleep(1000000);
    printf("xbee2: Tx\n");
    _xbee_logit(xbee1,"xbee2: Tx");
    _xbee_logit(xbee2,"xbee2: Tx");
    _xbee_senddata(xbee2,con2,"Hi There!");
    usleep(1000000);
  }
  
  return 0;
}
