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

/* this sample will control the digital 0 output from the keyboard. Type:
     0, <RETURN> - off
     1, <RETURN> - on
     q, <RETURN> - quit */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <xbee.h>

int main(int argc,char *argv[]) {
  xbee_con *con;
  xbee_pkt *pkt;

  printf("Hello\n");

  if (xbee_setup("/dev/ttyUSB0",57600) == -1) {
    printf("failed to setup xbee\n");
    return 1;
  }

  con = xbee_newcon('R',xbee_64bitRemoteAT,0x0013a200,0x403af247);
  if (!con) {
    printf("no connection returned\n");
    return 1;
  }

  for (;;) {

    xbee_senddata(con,"D0");
    if ((pkt = xbee_getpacketwait(con)) == NULL) {
      printf("no packet returned from state probe\n");
      return 1;
    }

    if (pkt->status != 0) {
      printf("state probe failed (ret=0x%02X - ",pkt->status);
      switch (pkt->status) {
      case 0x1: printf("Error");             break;
      case 0x2: printf("Invalid Command");   break;
      case 0x3: printf("Invalid Parameter"); break;
      case 0x4: printf("No Response");       break;
      default:  printf("Unknown");           break;
      }
      printf(")\n");
      return 1;
    }

    if  (pkt->datalen != 1) {
      printf("unexpected datalen from state probe\n");
      return 1;
    }

    if (pkt->data[0] == 0x05) {
      printf("this port is currently ON\n");
    } else if (pkt->data[0] == 0x04) {
      printf("this port is currently OFF\n");
    } else {
      printf("this port is currently in an unknown state\n");
      return 1;
    }
    free(pkt);
    pkt = NULL;

  recharprompt:
    printf("--> ");
  rechar:
    switch(getchar()) {
    case 'q': case 'Q':
      printf("byebye\n");
      return 0;
    case '0':
      printf("turning off...\n");
      xbee_senddata(con,"D0%c",0x04);
      break;
    case '1':
      printf("turning on...\n");
      xbee_senddata(con,"D0%c",0x05);
      break;
    case '\n': goto rechar;
    default: goto recharprompt;
    }

    if ((pkt = xbee_getpacketwait(con)) != NULL) {
      if (pkt->status != 0) {
	printf("state set failed (ret=0x%02X - ",pkt->status);
	switch (pkt->status) {
	case 0x1: printf("Error");             break;
	case 0x2: printf("Invalid Command");   break;
	case 0x3: printf("Invalid Parameter"); break;
	case 0x4: printf("No Response");       break;
	default:  printf("Unknown");           break;
	}
	printf(")\n");
	return 1;
      }
    }

  }

  return 0;
}
