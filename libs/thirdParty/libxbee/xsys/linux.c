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

/* ################################################################# */
/* ### Linux Code ################################################## */
/* ################################################################# */

/*  this file contains code that is used by Linux ONLY */
#ifndef __GNUC__
#error "This file should only be used on a Linux system"
#endif

#include "linux.h"

int init_serial(xbee_hnd xbee, int baudrate) {
  struct flock fl;
  struct termios tc;
  speed_t chosenbaud;

  /* select the baud rate */
  switch (baudrate) {
  case 1200:  chosenbaud = B1200;   break;
  case 2400:  chosenbaud = B2400;   break;
  case 4800:  chosenbaud = B4800;   break;
  case 9600:  chosenbaud = B9600;   break;
  case 19200: chosenbaud = B19200;  break;
  case 38400: chosenbaud = B38400;  break;
  case 57600: chosenbaud = B57600;  break;
  case 115200:chosenbaud = B115200; break;
  default:
    fprintf(stderr,"%s(): Unknown or incompatiable baud rate specified... (%d)\n",__FUNCTION__,baudrate);
    return -1;
  };

  /* open the serial port as a file descriptor */
  if ((xbee->ttyfd = open(xbee->path,O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
    xbee_perror("xbee_setup():open()");
    xbee_mutex_destroy(xbee->conmutex);
    xbee_mutex_destroy(xbee->pktmutex);
    xbee_mutex_destroy(xbee->sendmutex);
    Xfree(xbee->path);
    return -1;
  }

  /* lock the file */
  fl.l_type = F_WRLCK | F_RDLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  fl.l_pid = getpid();
  if (fcntl(xbee->ttyfd, F_SETLK, &fl) == -1) {
    xbee_perror("xbee_setup():fcntl()");
    xbee_mutex_destroy(xbee->conmutex);
    xbee_mutex_destroy(xbee->pktmutex);
    xbee_mutex_destroy(xbee->sendmutex);
    Xfree(xbee->path);
    close(xbee->ttyfd);
    return -1;
  }

  /* open the serial port as a FILE* */
  if ((xbee->tty = fdopen(xbee->ttyfd,"r+")) == NULL) {
    xbee_perror("xbee_setup():fdopen()");
    xbee_mutex_destroy(xbee->conmutex);
    xbee_mutex_destroy(xbee->pktmutex);
    xbee_mutex_destroy(xbee->sendmutex);
    Xfree(xbee->path);
    close(xbee->ttyfd);
    return -1;
  }

  /* flush the serial port */
  fflush(xbee->tty);

  /* disable buffering */
  setvbuf(xbee->tty,NULL,_IONBF,BUFSIZ);

  /* setup the baud rate and other io attributes */
  tcgetattr(xbee->ttyfd, &tc);
  /* input flags */
  tc.c_iflag &= ~ IGNBRK;           /* enable ignoring break */
  tc.c_iflag &= ~(IGNPAR | PARMRK); /* disable parity checks */
  tc.c_iflag &= ~ INPCK;            /* disable parity checking */
  tc.c_iflag &= ~ ISTRIP;           /* disable stripping 8th bit */
  tc.c_iflag &= ~(INLCR | ICRNL);   /* disable translating NL <-> CR */
  tc.c_iflag &= ~ IGNCR;            /* disable ignoring CR */
  tc.c_iflag &= ~(IXON | IXOFF);    /* disable XON/XOFF flow control */
  /* output flags */
  tc.c_oflag &= ~ OPOST;            /* disable output processing */
  tc.c_oflag &= ~(ONLCR | OCRNL);   /* disable translating NL <-> CR */
  tc.c_oflag &= ~ OFILL;            /* disable fill characters */
  /* control flags */
  tc.c_cflag |=   CREAD;            /* enable reciever */
  tc.c_cflag &= ~ PARENB;           /* disable parity */
  tc.c_cflag &= ~ CSTOPB;           /* disable 2 stop bits */
  tc.c_cflag &= ~ CSIZE;            /* remove size flag... */
  tc.c_cflag |=   CS8;              /* ...enable 8 bit characters */
  tc.c_cflag |=   HUPCL;            /* enable lower control lines on close - hang up */
  /* local flags */
  tc.c_lflag &= ~ ISIG;             /* disable generating signals */
  tc.c_lflag &= ~ ICANON;           /* disable canonical mode - line by line */
  tc.c_lflag &= ~ ECHO;             /* disable echoing characters */
  tc.c_lflag &= ~ ECHONL;           /* ??? */
  tc.c_lflag &= ~ NOFLSH;           /* disable flushing on SIGINT */
  tc.c_lflag &= ~ IEXTEN;           /* disable input processing */
  /* control characters */
  memset(tc.c_cc,0,sizeof(tc.c_cc));
  /* i/o rates */
  cfsetspeed(&tc, chosenbaud);     /* set i/o baud rate */
  tcsetattr(xbee->ttyfd, TCSANOW, &tc);
  tcflow(xbee->ttyfd, TCOON|TCION); /* enable input & output transmission */

  return 0;
}

static int xbee_select(xbee_hnd xbee, struct timeval *timeout) {
  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(xbee->ttyfd, &fds);

  return select(xbee->ttyfd+1, &fds, NULL, NULL, timeout);
}

#define xbee_sem_wait1sec(a) xbee_sem_wait1sec2(&(a))
static inline int xbee_sem_wait1sec2(xbee_sem_t *sem) {
  struct timespec to;
  clock_gettime(CLOCK_REALTIME,&to);
  to.tv_sec++;
  return sem_timedwait(sem,&to);
}
