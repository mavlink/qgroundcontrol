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
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*    endianness                                                         */
/*                                                                       */
/*************************************************************************/
#ifndef __CST_ENDIAN_H__
#define __CST_ENDIAN_H__

extern const int cst_endian_loc;
/* Sun, HP, SGI Mips, M68000 */
#define CST_BIG_ENDIAN (((char *)&cst_endian_loc)[0] == 0)
/* Intel, Alpha, DEC Mips, Vax, Arm, Other MIPS (Casio etc) */
#define CST_LITTLE_ENDIAN (((char *)&cst_endian_loc)[0] != 0)

/* EST byte order strings */
#define BYTE_ORDER_BIG "10"
#define BYTE_ORDER_LITTLE "01"

#define SWAPINT(x) ((((unsigned int)x) & 0xff) << 24 | \
        (((unsigned int)x) & 0xff00) << 8 | \
	(((unsigned int)x) & 0xff0000) >> 8 | \
        (((unsigned int)x) & 0xff000000) >> 24)
/* For m68k we want to be a little more explicit */
#define SWAPLONG(x) ((((unsigned long)x) & 0xff) << 24 | \
        (((unsigned long)x) & 0xff00) << 8 | \
	(((unsigned long)x) & 0xff0000) >> 8 | \
        (((unsigned long)x) & 0xff000000) >> 24)
#define SWAPSHORT(x) ((((unsigned short)x) & 0xff) << 8 | \
        (((unsigned short)x) & 0xff00) >> 8)

void swap_bytes_short(short * b,int n);

void swapdouble(double *d);
void swapfloat(float *f);

#endif
