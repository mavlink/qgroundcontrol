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
/*               Date:  January 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  cst front-end to Henry Spencer's regex code                          */
/*                                                                       */
/*************************************************************************/

/* Includes portions or regexp.h, copyright follows: */
/*
 * Copyright (c) 1986 by University of Toronto.
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Henry Spencer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)regexp.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _CST_REGEX_H__
#define _CST_REGEX_H__

#include "cst_file.h"
#include "cst_string.h"

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define	CST_REGMAGIC	0234

typedef struct cst_regex_struct {
    char regstart;		/* Internal use only. */
    char reganch;		/* Internal use only. */
    char *regmust;		/* Internal use only. */
    int regmlen;		/* Internal use only. */
    int regsize;
    char *program;
} cst_regex;

#define CST_NSUBEXP  10
typedef struct cst_regstate_struct {
	const char *startp[CST_NSUBEXP];
	const char *endp[CST_NSUBEXP];
	const char *input;
	const char *bol;
} cst_regstate;

cst_regex *new_cst_regex(const char *str);
void delete_cst_regex(cst_regex *r);

int cst_regex_match(const cst_regex *r, const char *str);
cst_regstate *cst_regex_match_return(const cst_regex *r, const char *str);

/* Internal functions from original HS code */
cst_regex *hs_regcomp(const char *);
cst_regstate *hs_regexec(const cst_regex *, const char *);
void hs_regdelete(cst_regex *);

/* Works similarly to snprintf(3), in that at most max characters are
   written to out, including the trailing NUL, and the return value is
   the number of characters written, *excluding* the trailing NUL.
   Also works similarly to wcstombs(3) in that passing NULL as out
   will count the number of characters that would be written without
   doing any actual conversion, and ignoring max.  So, you could use
   it like this:

   rx = new_cst_regex("\\(.*\\)_\\(.*\\)");
   if ((rs = cst_regex_match_return(rx, "foo_bar")) != NULL) {
   	size_t n;

	n = cst_regsub(rs, "\\1_\\2_quux", NULL, 0) + 1;
	out = cst_alloc(char, n);
	cst_regsub(rs, "\\1_\\2_quux", out, n);
   } */
size_t cst_regsub(const cst_regstate *r, const char *in, char *out, size_t max);

/* Initialize the regex engine and global regex constants */
void cst_regex_init();

/* Regexps used in text processing (these are latin-alphabet specific
   and to some extent US English-specific) */
extern const cst_regex * const cst_rx_white;
extern const cst_regex * const cst_rx_alpha;
extern const cst_regex * const cst_rx_uppercase;
extern const cst_regex * const cst_rx_lowercase;
extern const cst_regex * const cst_rx_alphanum;
extern const cst_regex * const cst_rx_identifier;
extern const cst_regex * const cst_rx_int;
extern const cst_regex * const cst_rx_double;
extern const cst_regex * const cst_rx_commaint;
extern const cst_regex * const cst_rx_digits;
extern const cst_regex * const cst_rx_dotted_abbrev;

/* Table of regexps used in CART trees (only one so far) */
extern const cst_regex * const cst_regex_table[];
#define CST_RX_dotted_abbrev_NUM 0

#endif
