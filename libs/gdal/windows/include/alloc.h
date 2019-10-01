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
#ifndef _ALLOC_H_
#define _ALLOC_H_

#ifndef NO_STDLIB
#include <stdlib.h>
#else
extern char *malloc();
extern char *realloc();
#ifndef NULL
#define NULL  0
#endif /* !NULL */
#endif /* !NO_STDLIB */


#ifdef HDF
#define Alloc(theNum, theType) \
	(theType *)HDmalloc(sizeof(theType) * (theNum))
#else
#define Alloc(theNum, theType) \
	(theType *)malloc(sizeof(theType) * (theNum))
#endif


#ifndef NO_STDLIB
#ifdef HDF
#define Free(ptr)		HDfree((VOIDP)ptr)
#else
#define Free(ptr)		free(ptr)
#define HDfree(ptr)     free(ptr)
#endif
#else
/* old style free */
#ifdef HDF
#define Free(ptr)		(void)HDfree((char *)ptr)
#else
#define Free(ptr)		(void)free((char *)ptr)
#define HDfree(ptr)     (void)free((char *)ptr)
#endif
#endif /* !NO_STDLIB */


/* We need to define these to standard ones when HDF is not defined */
#ifndef HDF
#define HDcalloc(nelem, elsize)   calloc(nelem,elsize)
#define HDmemset(dst,c,n)         memset(dst,c,n)
#define HDrealloc(p,s)            realloc(p,s)
#define HDmalloc(s)               malloc(s)
#endif /* HDF */

#define ARRAYLEN(arr) (sizeof(arr)/sizeof(arr[0]))

#endif /* !_ALLOC_H_ */
