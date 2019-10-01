/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*****************************************************************************
 * File: mcache.h
 *
 * This is a modfied version of the original Berkley code for
 * manipulating a memory pool. This version however is not 
 * compatible with the original Berkley version.
 *
 * This version uses HDF number types.
 *
 * AUTHOR - George V.- 1996/08/22
 *****************************************************************************/ 

/* $Id$ */

/*
 *  NOTE:
 *    Here pagesize is the same thing as chunk size and pages refer to chunks.
 *    I just didn't bother to change all references from pages to chunks.
 *
 *    -georgev
 */

#ifndef _MCACHE_H
#define _MCACHE_H

/* Required include */
#include "hqueue.h"    /* Circluar queue functions(Macros) */

/* Set return/succeed values */
#ifdef SUCCEED
#define RET_SUCCESS  SUCCEED
#define RET_ERROR    FAIL
#else
#define RET_SUCCESS  0
#define RET_ERROR    -1
#endif

/*
 * The memory pool scheme is a simple one.  Each in-memory page is referenced
 * by a bucket which is threaded in up to two (three?) ways.  All active pages
 * are threaded on a hash chain (hashed by page number) and an lru chain.
 * (Inactive pages are threaded on a free chain?).  Each reference to a memory
 * pool is handed an opaque MPOOL cookie which stores all of this information.
 */

/* Current Hash table size. Page numbers start with 1 
* (i.e 0 will denote invalid page number) */
#define	HASHSIZE	    128
#define	HASHKEY(pgno)  ((pgno -1) % HASHSIZE)

/* Default pagesize and max # of pages to cache */
#define DEF_PAGESIZE   8192
#define DEF_MAXCACHE   1

#define MAX_PAGE_NUMBER 0xffffffff  /* >= # of pages in a object */

/* The BKT structures are the elements of the queues. */
typedef struct _bkt 
{
  CIRCLEQ_ENTRY(_bkt) hq;	/* hash queue */
  CIRCLEQ_ENTRY(_bkt) q;	/* lru queue */
  VOID    *page;            /* page */
  int32   pgno;             /* page number */
#define	MCACHE_DIRTY  0x01  /* page needs to be written */
#define	MCACHE_PINNED 0x02  /* page is pinned into memory */
  uint8   flags;            /* flags */
} BKT;

/* The element structure for every page referenced(read/written) in object */
typedef struct _lelem
{
  CIRCLEQ_ENTRY(_lelem) hl;	    /* hash list */
  int32        pgno;            /* page number */
#ifdef STATISTICS
  int32	      elemhit;          /* # of hits on page */
#endif
#define ELEM_READ       0x01
#define ELEM_WRITTEN    0x02
#define ELEM_SYNC       0x03
  uint8      eflags;            /* 1= read, 2=written, 3=synced */
} L_ELEM;

#define	MCACHE_EXTEND    0x10	/* increase number of pages 
                                   i.e extend object */

/* Memory pool cache */
typedef struct MCACHE
{
  CIRCLEQ_HEAD(_lqh, _bkt)    lqh;	      /* lru queue head */
  CIRCLEQ_HEAD(_hqh, _bkt)    hqh[HASHSIZE];  /* hash queue array */
  CIRCLEQ_HEAD(_lhqh, _lelem) lhqh[HASHSIZE]; /* hash of all elements */
  int32	curcache;		      /* current num of cached pages */
  int32	maxcache;		      /* max number of cached pages */
  int32	npages;			      /* number of pages in the object */
  int32	pagesize;		      /* cache page size */
  int32 object_id;            /* access ID of object this cache is for */
  int32 object_size;          /* size of object to cache 
                                 must be multiple of pagesize for now */
  int32 (*pgin) (VOID *cookie, int32 pgno, VOID *page); /* page in conversion routine */
  int32 (*pgout) (VOID *cookie, int32 pgno, const VOID *page);/* page out conversion routine*/
  VOID	*pgcookie;                         /* cookie for page in/out routines */
#ifdef STATISTICS
  int32	listhit;                /* # of list hits */
  int32	listalloc;              /* # of list elems allocated */
  int32	cachehit;               /* # of cache hits */
  int32	cachemiss;              /* # of cache misses */
  int32	pagealloc;              /* # of pages allocated */
  int32	pageflush;              /* # of pages flushed */
  int32	pageget;                /* # of pages requested from pool */
  int32	pagenew;                /* # of new pages */
  int32	pageput;                /* # of pages put back into pool */
  int32	pageread;               /* # of pages read from object */
  int32	pagewrite;              /* # of pages written to object */
#endif /* STATISTICS */
} MCACHE;

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

extern MCACHE *mcache_open (
    VOID *key,          /* IN:byte string used as handle to share buffers */
    int32 object_id,    /* IN: object handle */
    int32 pagesize,     /* IN: chunk size in bytes */
    int32 maxcache,     /* IN: maximum number of pages to cache at any time */
    int32 npages,       /* IN: number of chunks currently in object */
    int32 flags         /* IN: 0= object exists, 1= does not exist */);

extern VOID	 mcache_filter (
    MCACHE *mp,             /* IN: MCACHE cookie */
    int32 (*pgin)(VOID *cookie, int32 pgno, VOID *page) ,/* IN: page in filter */
    int32 (*pgout)(VOID *cookie, int32 pgno, const VOID *page) , /* IN: page out filter */
    VOID *pgcookie          /* IN: filter cookie */);

extern VOID	*mcache_new (
    MCACHE *mp,      /* IN: MCACHE cookie */
    int32 *pgnoaddr, /* IN/OUT: address of newly create page */
    int32 flags      /* IN:MCACHE_EXTEND or 0 */);


extern VOID	*mcache_get (
    MCACHE *mp, /* IN: MCACHE cookie */
    int32 pgno, /* IN: page number */
    int32 flags /* IN: XXX not used? */);

extern intn	 mcache_put (
    MCACHE *mp, /* IN: MCACHE cookie */
    VOID *page, /* IN: page to put */ 
    int32 flags /* IN: flags = 0, MCACHE_DIRTY */);

extern intn	 mcache_sync (
    MCACHE *mp /* IN: MCACHE cookie */);

extern intn	 mcache_close (
    MCACHE *mp /* IN: MCACHE cookie */);

extern int32  mcache_get_pagesize (
    MCACHE *mp /* IN: MCACHE cookie */);

extern int32  mcache_get_maxcache (
    MCACHE *mp /* IN: MCACHE cookie */);

extern int32  mcache_set_maxcache (
    MCACHE *mp,     /* IN: MCACHE cookie */
    int32  maxcache /* IN: max pages to cache */);

extern int32  mcache_get_npages (
    MCACHE *mp /* IN: MCACHE cookie */);

#ifdef STATISTICS
extern VOID	 mcache_stat(
    MCACHE *mp /* IN: MCACHE cookie */);
#endif /* STATISTICS */
#if 0 /* NOT USED */
extern intn	 mcache_page_sync (
    MCACHE *mp, /* IN: MCACHE cookie */
    int32 pgno, /* IN: page to sync */
    int32 flags /* IN: flags */);
#endif

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#endif /* _MCACHE_H */
