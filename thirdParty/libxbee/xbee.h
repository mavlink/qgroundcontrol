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
#ifndef XBEE_H
#define XBEE_H

#if !defined(__GNUC__) && !defined(_WIN32)
#error "This library is only currently compatible with Linux and Win32"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __LIBXBEE_API_H
typedef void* xbee_hnd;
#endif

#include <stdarg.h>

#ifdef __GNUC__ /* ---- */
#include <semaphore.h>
typedef pthread_mutex_t    xbee_mutex_t;
typedef pthread_cond_t     xbee_cond_t;
typedef pthread_t          xbee_thread_t;
typedef sem_t              xbee_sem_t;
typedef FILE*              xbee_file_t;
#elif (defined(WIN32) || defined(_WIN32)) /* -------------- */
#include <Windows.h>
#define CALLTYPE   __stdcall
#define CALLTYPEVA __cdecl
typedef HANDLE             xbee_mutex_t;
typedef CONDITION_VARIABLE xbee_cond_t;
typedef HANDLE             xbee_thread_t;
typedef HANDLE             xbee_sem_t;
typedef HANDLE             xbee_file_t;
#else
#error "Unknown operating system or compiler"
#endif /* ------------- */

#ifndef CALLTYPE
#define CALLTYPE
#endif

#ifndef CALLTYPEVA
#define CALLTYPEVA
#endif

enum xbee_types {
  xbee_unknown,

  xbee_localAT,       /* frame ID */
  xbee_remoteAT,
  xbee_modemStatus,
  xbee_txStatus,
  
  /* XBee Series 1 stuff */
  xbee_16bitRemoteAT, /* frame ID */
  xbee_64bitRemoteAT, /* frame ID */

  xbee_16bitData,     /* frame ID for ACKs */
  xbee_64bitData,     /* frame ID for ACKs */

  xbee_16bitIO,
  xbee_64bitIO,
  
  /* XBee Series 2 stuff */
  xbee2_data,
  xbee2_txStatus
};
typedef enum xbee_types xbee_types;

typedef struct xbee_sample xbee_sample;
struct xbee_sample {
  /* X  A5 A4 A3 A2 A1 A0 D8    D7 D6 D5 D4 D3 D2 D1 D0  */
  unsigned short IOmask;          /*                  IO */
  /* X  X  X  X  X  X  X  D8    D7 D6 D5 D4 D3 D2 D1 D0  */
  unsigned short IOdigital;       /*                  IO */
  /* X  X  X  X  X  D  D  D     D  D  D  D  D  D  D  D   */
  unsigned short IOanalog[6];     /*                  IO */
};

typedef struct xbee_pkt xbee_pkt;
struct xbee_pkt {
  unsigned int sAddr64        : 1; /* yes / no */
  unsigned int dataPkt        : 1; /* if no - AT packet */
  unsigned int txStatusPkt    : 1;
  unsigned int modemStatusPkt : 1;
  unsigned int remoteATPkt    : 1;
  unsigned int IOPkt          : 1;
  unsigned int isBroadcastADR : 1;
  unsigned int isBroadcastPAN : 1;

  unsigned char frameID;          /* AT        Status    */
  unsigned char atCmd[2];         /* AT                  */

  unsigned char status;           /* AT  Data  Status    */ /* status / options */
  unsigned char samples;
  unsigned char RSSI;             /*     Data            */

  unsigned char Addr16[2];        /* AT  Data            */

  unsigned char Addr64[8];        /* AT  Data            */

  unsigned char data[128];        /* AT  Data            */

  unsigned int datalen;
  xbee_types type;

  xbee_pkt *next;

  xbee_sample IOdata[1];  /* this array can be extended by using a this trick:
                             p = calloc(sizeof(xbee_pkt) + (sizeof(xbee_sample) * (samples - 1))) */
};

typedef struct xbee_con xbee_con;
struct xbee_con {
  unsigned int tAddr64       : 1;
  unsigned int atQueue       : 1; /* queues AT commands until AC is sent */
  unsigned int txDisableACK  : 1;
  unsigned int txBroadcastPAN: 1; /* broadcasts to PAN */
  unsigned int destroySelf   : 1; /* if set, the callback thread will destroy the connection
                                     after all of the packets have been processed */
  unsigned int waitforACK    : 1; /* waits for the ACK or NAK after transmission */
  unsigned int noFreeAfterCB : 1; /* prevents libxbee from free'ing the packet after the callback has completed */
  unsigned int __spare__     : 1;
  xbee_types type;
  unsigned char frameID;
  unsigned char tAddr[8];         /* 64-bit 0-7   16-bit 0-1 */
  void *customData;               /* can be used to store data related to this connection */
  void (CALLTYPE *callback)(xbee_con*,xbee_pkt*); /* call back function */
  void *callbackList;
  xbee_mutex_t callbackmutex;
  xbee_mutex_t callbackListmutex;
  xbee_mutex_t Txmutex;
  xbee_sem_t waitforACKsem;
  volatile unsigned char ACKstatus; /* 255 = waiting, 0 = success, 1 = no ack, 2 = cca fail, 3 = purged */
  xbee_con *next;
};

int CALLTYPE xbee_setup(char *path, int baudrate);
int CALLTYPE xbee_setuplog(char *path, int baudrate, int logfd);
int CALLTYPE xbee_setupAPI(char *path, int baudrate, char cmdSeq, int cmdTime);
int CALLTYPE xbee_setuplogAPI(char *path, int baudrate, int logfd, char cmdSeq, int cmdTime);
xbee_hnd CALLTYPE _xbee_setup(char *path, int baudrate);
xbee_hnd CALLTYPE _xbee_setuplog(char *path, int baudrate, int logfd);
xbee_hnd CALLTYPE _xbee_setupAPI(char *path, int baudrate, char cmdSeq, int cmdTime);
xbee_hnd CALLTYPE _xbee_setuplogAPI(char *path, int baudrate, int logfd, char cmdSeq, int cmdTime);

int CALLTYPE xbee_end(void);
int CALLTYPE _xbee_end(xbee_hnd xbee);

void CALLTYPE xbee_logitf(char *format, ...);
void CALLTYPE _xbee_logitf(xbee_hnd xbee, char *format, ...);
void CALLTYPE xbee_logit(char *str);
void CALLTYPE _xbee_logit(xbee_hnd xbee, char *str);

xbee_con * CALLTYPEVA xbee_newcon(unsigned char frameID, xbee_types type, ...);
xbee_con * CALLTYPEVA _xbee_newcon(xbee_hnd xbee, unsigned char frameID, xbee_types type, ...);
xbee_con * CALLTYPE _xbee_vnewcon(xbee_hnd xbee, unsigned char frameID, xbee_types type, va_list ap);

void CALLTYPE xbee_purgecon(xbee_con *con);
void CALLTYPE _xbee_purgecon(xbee_hnd xbee, xbee_con *con);

void CALLTYPE xbee_endcon2(xbee_con **con, int alreadyUnlinked);
void CALLTYPE _xbee_endcon2(xbee_hnd xbee, xbee_con **con, int alreadyUnlinked);
#define xbee_endcon(x) xbee_endcon2(&(x),0)
#define _xbee_endcon(xbee,x) _xbee_endcon2((xbee),&(x),0)

int CALLTYPE xbee_nsenddata(xbee_con *con, char *data, int length);
int CALLTYPE _xbee_nsenddata(xbee_hnd xbee, xbee_con *con, char *data, int length);
int CALLTYPEVA xbee_senddata(xbee_con *con, char *format, ...);
int CALLTYPEVA _xbee_senddata(xbee_hnd xbee, xbee_con *con, char *format, ...);
int CALLTYPE xbee_vsenddata(xbee_con *con, char *format, va_list ap);
int CALLTYPE _xbee_vsenddata(xbee_hnd xbee, xbee_con *con, char *format, va_list ap);

#if defined(WIN32)
/* oh and just 'cos windows has rubbish memory management rules... this too */
void CALLTYPE xbee_free(void *ptr);
#endif /* ------------- */

xbee_pkt * CALLTYPE xbee_getpacket(xbee_con *con);
xbee_pkt * CALLTYPE _xbee_getpacket(xbee_hnd xbee, xbee_con *con);
xbee_pkt * CALLTYPE xbee_getpacketwait(xbee_con *con);
xbee_pkt * CALLTYPE _xbee_getpacketwait(xbee_hnd xbee, xbee_con *con);

int CALLTYPE xbee_hasdigital(xbee_pkt *pkt, int sample, int input);
int CALLTYPE xbee_getdigital(xbee_pkt *pkt, int sample, int input);

int CALLTYPE xbee_hasanalog(xbee_pkt *pkt, int sample, int input);
double CALLTYPE xbee_getanalog(xbee_pkt *pkt, int sample, int input, double Vref);

const char * CALLTYPE xbee_svn_version(void);
const char * CALLTYPE xbee_build_info(void);

void CALLTYPE xbee_listen_stop(xbee_hnd xbee);

#ifdef __cplusplus
} /* cplusplus */
#endif

#endif
