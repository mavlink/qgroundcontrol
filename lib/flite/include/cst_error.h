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
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Error handler                                                        */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_ERROR_H__
#define _CST_ERROR_H__

#include <stdlib.h>

#ifdef DIE_ON_ERROR
# ifdef UNDER_CE
#  define cst_error() *(int *)0=0
# else
#  define cst_error() abort()
# endif
#elif __palmos__
#ifdef __ARM_ARCH_4T__

typedef long *jmp_buf[10]; /* V1-V8, SP, LR (see po_setjmp.c) */
extern jmp_buf *cst_errjmp;
extern char cst_error_msg[];
int setjmp(register jmp_buf env);
void longjmp(register jmp_buf env, register int value);

# define cst_error() (cst_errjmp ? longjmp(*cst_errjmp,1) : 0)
#else  /* m68K */
/* I've never tested this or even compiled it (Flite is ARM compiled) */
#  define cst_error() ErrFatalDisplayIf(-1, "cst_error")
#endif
#else /* not palmos */
#include <setjmp.h>
extern jmp_buf *cst_errjmp;
# define cst_error() (cst_errjmp ? longjmp(*cst_errjmp,1) : exit(-1))
#endif

/* WinCE sometimes doesn't have stdio, so this is a wrapper for
   fprintf(stderr, ...) */
int cst_errmsg(const char *fmt, ...);
#define cst_dbgmsg cst_errmsg

/* Need macros to help set catches */

#endif
