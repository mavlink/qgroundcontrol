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

/* this sample will control digital output pins of a chosen node */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <xbee.h>

int mode;
char bitmask;
char outputs;

void sighandler(int sig) {
  if (sig == SIGINT) {
    xbee_end();
    exit(0);
  }
}

void doneCB(xbee_con *con, xbee_pkt *pkt) {
  /* this packet should be confirmation... */
  xbee_end();
  exit(0);
}
void doCB(xbee_con *con, xbee_pkt *pkt) {
  int i,m;
  outputs = pkt->IOdata[0].IOdigital;
  printf("\n                       7 6 5 4 3 2 1 0\n");
  printf("Current output state: ");
  for (i = 0; i < 8; i++) {
    if (xbee_hasdigital(pkt,0,7-i)) {
      if (xbee_getdigital(pkt,0,7-i)) {
        printf(" 1");
      } else {
        printf(" 0");
      }
    } else {
      printf(" x");
    }
  }
  printf("\n");
  switch (mode) {
  case 0: outputs |=  bitmask; break;
  case 1: outputs &= ~bitmask; break;
  case 2: default:
    xbee_end();
    exit(0);
  }
  m = outputs;
  printf("New output state:     ");
  for (i = 0; i < 8; i++) {
    if (xbee_hasdigital(pkt,0,7-i)) {
      if (m & 0x80) {
        printf(" 1");
      } else {
        printf(" 0");
      }
    } else {
      printf(" x");
    }
    m <<= 1;
  }
  printf("\n\n");
  con->callback = doneCB;
  xbee_senddata(con,"IO%c",outputs);
}

void usage(char *argv0) {
  printf("Usage: %s <query>\n",argv0);
  printf("Usage: %s <on|off> <port[0-7]>[port[0-7]]...\n",argv0);
  exit(1);
}
int main(int argc, char *argv[]) {
  xbee_con *con;

  if (argc < 2) usage(argv[0]);

  if (!strcasecmp(argv[1],"on")) {
    mode = 0;
  } else if (!strcasecmp(argv[1],"off")) {
    mode = 1;
  } else if (!strcasecmp(argv[1],"query")) {
    mode = 2;
  } else usage(argv[0]);

  if (mode != 2) {
    int i;
    char *c;
    bitmask = 0;
    c = argv[2];
    while (*c != '\0') {
      *c -= '0';
      if ((*c >= 0) && (*c <= 7)) {
        bitmask |= 0x01 << *c;
      } else {
        usage(argv[0]);
      }
      c++;
    }
  }

  /* setup libxbee */
  if (xbee_setupAPI("/dev/ttyUSB0",57600,'+',250) == -1) {
    return 1;
  }

  /* handle ^C */
  signal(SIGINT, sighandler);

  /* get a connection to the remote XBee */
  con = xbee_newcon('I',xbee_64bitRemoteAT,   0x0013A200, 0x403CB26B);
  con->waitforACK = 1;
  con->callback = doCB;

  xbee_senddata(con,"IS");

  /* timeout after 1 second... */
  sleep(1);

  xbee_end();
  return 0;
}
