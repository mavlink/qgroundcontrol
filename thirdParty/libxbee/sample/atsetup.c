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

/* this sample will setup certain AT parameters of the local XBee unit */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xbee.h>

/* con        = connection to use
   cmd        = 2 character command string, eg NI
   parameter  =  NULL - no parameter
                !NULL - command parameter, either NULL terminated string, or block of memory
   length     =  0 - use parameter as NULL terminated string
                !0 - use 'length' bytes from parameter
   ret        =  NULL - don't return anything
                !NULL - pointer to pointer. doAT will allocate memory, you must free it!
   str        = return data pointer 
   
   returns the length of the data in ret, or -ve for error */
int doAT(xbee_con *con, char *cmd, char *parameter, int length, unsigned char **ret) {
  xbee_pkt *pkt;
  if (con->type != xbee_localAT && con->type != xbee_16bitRemoteAT && con->type != xbee_64bitRemoteAT) {
    printf("Thats not an AT connection!...\n");
    return -1;
  }
  if (strlen(cmd) != 2) {
    printf("Invalid command: \"%s\"\n",cmd);
    return -2;
  }
  if (parameter == NULL) {
    xbee_senddata(con,"%s",cmd);
  } else if (length != 0) {
    char *tmp;
    if ((tmp = malloc(1024)) == NULL) {
      printf("Failed to get memory!\n");
      return -3;
    }
    snprintf(tmp,1024,"%s",cmd);
    memcpy(&(tmp[2]),parameter,(length>1022)?1022:length);
    xbee_nsenddata(con,tmp,length+2);
    free(tmp);
  } else {
    xbee_senddata(con,"%s%s",cmd,parameter);
  }
  pkt = xbee_getpacketwait(con);
  if (pkt == NULL) {
    printf("Failed to set %s!\n",cmd);
    return -4;
  }
  if (pkt->status != 0) {
    printf("An error occured while setting %s!\n",cmd);
    return -5;
  }
  if (ret && pkt->datalen > 0) {
    *ret = realloc(*ret,sizeof(char) * (pkt->datalen + 1));
    memcpy(*ret,pkt->data,pkt->datalen);
    (*ret)[pkt->datalen] = '\0';
    free(pkt);
    return pkt->datalen;
  }
  free(pkt);
  return 0;
}

int main(int argc, char *argv[]) {
  xbee_con *con;
  int ret,i;
  unsigned char *str = NULL;

  if (argc != 2) {
    printf("Usage: %s <newname>\n",argv[0]);
    return 1;
  }

  /* setup libxbee */
  if (xbee_setup("/dev/ttyUSB0",57600) == -1) {
    printf("xbee_setup failed...\n");
    return 1;
  }

  /* create an AT connection */
  con = xbee_newcon('I',xbee_localAT);
  /*con = xbee_newcon('I',xbee_64bitRemoteAT,0x13A200,0x403CB26A);*/

  
  /* get the node's address! */
  if ((ret = doAT(con,"SH",NULL,0,&str)) < 0) return 1;
  if (ret == 4) {
    printf("SH: 0x%02X%02X%02X%02X\n", str[0], str[1], str[2], str[3]);
  }
  if ((ret = doAT(con,"SL",NULL,0,&str)) < 0) return 1;
  if (ret == 4) {
    printf("SL: 0x%02X%02X%02X%02X\n", str[0], str[1], str[2], str[3]);
  }
  
  /* set the power level - 2 methods, i prefer the first but it generates compile warnings :( */
  /*if ((ret = doAT(con,"PL",&((unsigned char[]){4}),1,&str)) < 0) return 1;*/
  /*{
    char t[] = {0};
    if ((ret = doAT(con,"PL",t,1,&str)) < 0) return 1;
  }*/
  
  /* get the power level */
  if ((ret = doAT(con,"PL",NULL,0,&str)) < 0) return 1;
  if (ret == 1) {
    printf("PL: 0x%02X\n", str[0]);
  }
  
  /* get NI */
  if ((ret = doAT(con,"NI",NULL,0,&str)) < 0) return 1;
  if (ret > 0) {
    printf("NI: ");
    for (i = 0; i < ret; i++) {
      printf("%c",(str[i]>=32 && str[i]<=126)?str[i]:'.');
    }
    printf("\n");
  }
  
  printf("Setting NI to '%s': ",(argc!=2)?"MyNode":argv[1]);
  if ((ret = doAT(con,"NI",(argc!=2)?"MyNode":argv[1],0,NULL)) < 0) return 1;
  printf("OK\n");
  
  if ((ret = doAT(con,"NI",NULL,0,&str)) < 0) return 1;
  if (ret > 0) {
    printf("NI: ");
    for (i = 0; i < ret; i++) {
      printf("%c",(str[i]>=32 && str[i]<=126)?str[i]:'.');
    }
    printf("\n");
  }
  
  return 0;
}
