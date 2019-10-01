/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF/releases/.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id$ */

/*-----------------------------------------------------------------------------
 * File:    dfi.h
 * Purpose: HDF internal header file
 * Invokes: stdio.h, sys/file.h
 * Contents:
 *  Compilation parameters
 *  Machine-dependent definitions
 *  Flexibility definitions: i/o buffering, dynamic memory, structure i/o
 *  Size parameters
 * Remarks: To port to a new system, only dfi.h and Makefile need be modified.
 *          This file is included with user programs, but users do not see it.
 *---------------------------------------------------------------------------*/

#ifndef DFI_H
#define DFI_H

/*--------------------------------------------------------------------------*/
/*          Compilation Parameters for Flexibility and Portability          */

/* modify this line for buffered/unbuffered i/o */
#define DF_BUFFIO

/* modify this line for dynamic/static memory allocation */
#define DF_DYNAMIC

/* modify this line if structures cannot be read/written as is */
#undef  DF_STRUCTOK     /* leave it this way - hdfsh expects it */

/*--------------------------------------------------------------------------*/
/*                      Machine dependencies                                */
/*--------------------------------------------------------------------------*/

#ifdef IRIX
#undef DF_STRUCTOK
#include <sys/types.h>
#include <sys/file.h>   /* for unbuffered i/o stuff */
#ifndef DFmovmem
#define DFmovmem(from, to, len) bcopy(from, to, len)
#endif /* DFmovmem */
#ifndef DF_STRUCTOK
#define UINT16READ(p, x)    { x = ((*p++) & 255)<<8; x |= (*p++) & 255; }
#define INT16READ(p, x)     { x = (*p++)<<8; x |= (*p++) & 255; }
#define INT32READ(p, x)     { x = (*p++)<<24; x|=((*p++) & 255)<<16;    \
                                x|=((*p++) & 255)<<8; x|=(*p++) & 255; }
#define UINT16WRITE(p, x)   { *p++ = (x>>8) & 255; *p++ = x & 255; }
#define INT16WRITE(p, x)    { *p++ = (x>>8) & 255; *p++ = x & 255; }
#define INT32WRITE(p, x)    { *p++ = (x>>24) & 255; *p++ = (x>>16) & 255;   \
                                *p++ = (x>>8) & 255; *p++ = x & 255; }
#endif /*DF_STRUCTOK */
#define DF_CREAT(name, prot) creat(name, prot)
#ifndef DF_MT
#define DF_MT   DFMT_IRIX
#endif /* DF_MT  */
#endif /*IRIX */

#ifdef IBM6000  /* NOTE: IBM6000 defines are same as for SUN */
#if ! defined mc68010 && ! defined mc68020 && ! defined mc68030
#undef DF_STRUCTOK
#endif
#include <sys/file.h>   /* for unbuffered i/o stuff */
#define DFmovmem(from, to, len) memcpy(to, from, len)
#ifndef DF_STRUCTOK
#define UINT16READ(p, x) { x = ((*p++) & 255)<<8; x |= (*p++) & 255; }
#define INT16READ(p, x) { x = (*p++)<<8; x |= (*p++) & 255; }
#define INT32READ(p, x) { x = (*p++)<<24; x|=((*p++) & 255)<<16;    \
            x|=((*p++) & 255)<<8; x|=(*p++) & 255; }
#define UINT16WRITE(p, x) { *p++ = (x>>8) & 255; *p++ = x & 255; }
#define INT16WRITE(p, x) { *p++ = (x>>8) & 255; *p++ = x & 255; }
#define INT32WRITE(p, x) { *p++ = (x>>24) & 255; *p++ = (x>>16) & 255;  \
            *p++ = (x>>8) & 255; *p++ = x & 255; }
#endif /*DF_STRUCTOK */
#define DF_CREAT(name, prot) creat(name, prot)
#define DF_MT   DFMT_IBM6000
#endif /*IBM6000 */

#ifdef APOLLO
#if ! defined mc68010 && ! defined mc68020 && ! defined mc68030
#undef DF_STRUCTOK
#endif
#include <sys/file.h>   /* for unbuffered i/o stuff */
#define int8 char
#define uint8 unsigned char
#define int16 short int
#define uint16 unsigned short int
#define int32 long int
#define uint32 unsigned long int
#define float32 float
#define DFmovmem(from, to, len) memcpy(to, from, len)
#ifndef DF_STRUCTOK
#define UINT16READ(p, x) { x = ((*p++) & 255)<<8; x |= (*p++) & 255; }
#define INT16READ(p, x) { x = (*p++)<<8; x |= (*p++) & 255; }
#define INT32READ(p, x) { x = (*p++)<<24; x|=((*p++) & 255)<<16;    \
            x|=((*p++) & 255)<<8; x|=(*p++) & 255; }
#define UINT16WRITE(p, x) { *p++ = (x>>8) & 255; *p++ = x & 255; }
#define INT16WRITE(p, x) { *p++ = (x>>8) & 255; *p++ = x & 255; }
#define INT32WRITE(p, x) { *p++ = (x>>24) & 255; *p++ = (x>>16) & 255;  \
            *p++ = (x>>8) & 255; *p++ = x & 255; }
#endif /*DF_STRUCTOK */
#define DF_CREAT(name, prot) creat(name, prot)
#define DF_MT   DFMT_APOLLO
#endif /*APOLLO */

/*--------------------------------------------------------------------------*/
/*                      Flexibility parameters                              */
#ifdef DF_BUFFIO    /* set all calls to do buffered I/O */
#define DF_OPEN(x,y) fopen(x,y)
#define DF_CLOSE(x) fclose(x)
#define DF_SEEK(x,y,z) fseek(x,y,z)
#define DF_SKEND(x,y,z) fseek(x,y,z)
#define DF_TELL(x) ftell(x)
#define DF_READ(a,b,c,d) fread(a,b,c,d)
#define DF_WRITE(a,b,c,d) fwrite(a,b,c,d)
#define DF_FLUSH(a) fflush(a)
#define DF_OPENERR(f)   (!(f))
#define DF_RDACCESS "rb"
#define DF_WRACCESS "rb+"

#else  /*DF_BUFFIO         unbuffered i/o */
#define DF_OPEN(x,y) open(x,y)
#define DF_CLOSE(x) close(x)
#define DF_SEEK(x,y,z) lseek(x,y,z)
#define DF_SKEND(x,y,z) lseek(x,-1*y,z)
#define DF_TELL(x) lseek(x,0L,1)
#define DF_READ(a,b,c,d) read(d,a,b*c)
#define DF_WRITE(a,b,c,d) write(d,a,b*c)
#define DF_OPENERR(f)   ((f) == -1)
#define DF_FLUSH(a)     /* no need to flush */
#define DF_RDACCESS O_RDONLY
#define DF_WRACCESS O_RDWR
#endif /* DF_BUFFIO */

#ifndef FILE
#include <stdio.h>
#endif /*FILE */

#endif /* DFI_H */
