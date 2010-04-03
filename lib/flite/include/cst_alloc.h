/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  July 1999                                        */
/*************************************************************************/
/*                                                                       */
/*    Basic wraparounds for malloc and free                              */
/*                                                                       */
/*************************************************************************/
#ifndef __CST_ALLOC_H__
#define __CST_ALLOC_H__

#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif

/* Global allocation (the only kind on Unix) */
void *cst_safe_alloc(int size);
void *cst_safe_calloc(int size);
void *cst_safe_realloc(void *p,int size);

/* Allocate on local heap (needed on WinCE for various reasons) */
#ifdef UNDER_CE
#include <windows.h>
typedef HANDLE cst_alloc_context;

cst_alloc_context new_alloc_context(int size);
void delete_alloc_context(cst_alloc_context ctx);

void *cst_local_alloc(cst_alloc_context ctx, int size);
void cst_local_free(cst_alloc_context ctx, void *p);
#else /* not UNDER_CE */
typedef void * cst_alloc_context;
#define new_alloc_context(size)   (NULL)
#define delete_alloc_context(ctx)
#define cst_local_alloc(ctx,size) cst_safe_alloc(size)
#define cst_local_free(cst,p)     cst_free(p)
#endif /* UNDER_CE */

/* The public interface to the alloc functions */

/* Note the underlying call is calloc, so everything is zero'd */
#define cst_alloc(TYPE,SIZE) ((TYPE *)cst_safe_alloc(sizeof(TYPE)*(SIZE)))
#define cst_calloc(TYPE,SIZE) ((TYPE *)cst_safe_calloc(sizeof(TYPE)*(SIZE)))
#define cst_realloc(P,TYPE,SIZE) ((TYPE *)cst_safe_realloc((void *)(P),sizeof(TYPE)*(SIZE)))

void cst_free(void *p);

#endif
