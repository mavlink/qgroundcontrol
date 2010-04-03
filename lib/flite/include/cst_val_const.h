/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
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
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  Const Vals, and macros to define them                                */
/*                                                                       */
/*  Before you give up in disgust bear with me on this.  Every single    */
/*  line in this file has been *very* carefully decided on after much    */
/*  thought, experimentation and reading more specs of the C language    */
/*  than most people even thought existed.  However inspite of that, the */
/*  result is still unsatisfying from an elegance point of view but the  */
/*  given all the constraints this is probably the best compromise.      */
/*                                                                       */
/*  This file offers macros for defining const cst_vals.  I know many    */
/*  are already laughing at me for wanting runtime types on objects and  */
/*  will use this code as exemplars of why this should be done in C++, I */
/*  say good luck to them with their 4M footprint while I go for my      */
/*  50K footprint.  But I *will* do cst_vals in 8 bytes and I *will*     */
/*  have them defined const so they are in the text segment              */
/*                                                                       */
/*  The problem here is that there is not yet a standard way to do       */
/*  initialization of unions.  There is one in the C99 standard and GCC  */
/*  already supports it, but other compilers do not so I can't use that  */
/*                                                                       */
/*  So I need a way to make an object that will have the right 8 bytes   */
/*  for ints, floats, strings and cons cells that will work on any C     */
/*  compiler and will be of type const cst_val.  That unfortunately      */
/*  isn't trivial.                                                       */
/*                                                                       */
/*  For the time being ignoring byte order, and address size, which      */
/*  will be ignored in the particular discuss (though dealt with below)  */
/*  I'd like to do something like                                        */
/*                                                                       */
/*  const cst_val fredi = { CST_VAL_TYPE_INT,-1, 42 };                   */
/*  const cst_val fredf = { CST_VAL_TYPE_FLOAT,-1, 4.2 };                */
/*  const cst_val freds = { CST_VAL_TYPE_STRING,-1, "42" };              */
/*                                                                       */
/*  Even if you accept warnings this isn't going to work, if you add     */
/*  extra {} you can get rid of some warnings but the fval/ival/ *vval   */
/*  part isn't going to work, the compiler *may* try to take the value   */
/*  and assign it using the type of the first field defined in the       */
/*  union.  This could be made to work for ints and void* as pointers    */
/*  can be used as ints (even of 64 bit architectures) but the float     */
/*  isn't going to work.  Casting will cause the float to be changed to  */
/*  an int by CPP which isn't what you want, what you want is that the   */
/*  four byte region gets filled with the four bytes that represent the  */
/*  float itself.  Now you could get the four byte represention of the   */
/*  float and pretend that is an int (0xbfff9a4 for 4.2 on intel), that  */
/*  would work but that doesn't seem satifying and I'd need to have a    */
/*  preprocessor that could convert that.  You could make atoms always   */
/*  have a pointer to another piece of memory, but that would take up    */
/*  another 4 bytes not just for these constants but all other cst_vals  */
/*  create                                                               */
/*                                                                       */
/*  So you could do                                                      */
/*                                                                       */
/*  const cst_val_int fredi = { CST_VAL_TYPE_INT,-1, 42 };               */
/*  const cst_val_float fredf = { CST_VAL_TYPE_FLOAT,-1, 4.2 };          */
/*  const cst_val_string freds = { CST_VAL_TYPE_STRING,-1, "42" };       */
/*                                                                       */
/*  Though that's a slippery slope I don't want to have these explicit   */
/*  new types, the first short defines the type perfectly adequately     */
/*  and the whole point of runtime types is that there is one object     */
/*  type.                                                                */
/*                                                                       */
/*  Well just initialize them at runtime, but, that isn't thread safe,   */
/*  slows down startup, requires a instance implicitly in the code and   */
/*  on the heap. As these are const, they should go in ROM.              */
/*                                                                       */
/*  At this moment, I think the second version is the least problematic  */
/*  though it makes defining val_consts more unpleasant than they should */
/*  be and forces changes elsewhere in the code even when the compiler   */
/*  does support initialization of unions                                */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_VAL_CONSTS_H__
#define _CST_VAL_CONSTS_H__

#include "cst_val_defs.h"

/* There is built-in int to string conversions here for numbers   */
/* up to 20, note if you make this bigger you have to hand change */
/* other things too                                               */
#define CST_CONST_INT_MAX 19

#ifndef NO_UNION_INITIALIZATION

/* This is the simple way when initialization of unions is supported */

#define DEF_CONST_VAL_INT(N,V) const cst_val N = {{.a={.type=CST_VAL_TYPE_INT,.ref_count=-1,.v={.ival=V}}}}
#define DEF_CONST_VAL_STRING(N,S) const cst_val N = {{.a={.type=CST_VAL_TYPE_STRING,.ref_count=-1,.v={.vval= (void *)S}}}}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val N = {{.a={.type=CST_VAL_TYPE_FLOAT,.ref_count=-1,.v={.fval=F}}}}
#define DEF_CONST_VAL_CONS(N,A,D) const cst_val N = {{.cc={.car=A,.cdr=D }}}

extern const cst_val val_int_0; 
extern const cst_val val_int_1; 
extern const cst_val val_int_2;
extern const cst_val val_int_3;
extern const cst_val val_int_4;
extern const cst_val val_int_5;
extern const cst_val val_int_6;
extern const cst_val val_int_7;
extern const cst_val val_int_8;
extern const cst_val val_int_9;
extern const cst_val val_int_10; 
extern const cst_val val_int_11; 
extern const cst_val val_int_12;
extern const cst_val val_int_13;
extern const cst_val val_int_14;
extern const cst_val val_int_15;
extern const cst_val val_int_16;
extern const cst_val val_int_17;
extern const cst_val val_int_18;
extern const cst_val val_int_19;

extern const cst_val val_string_0; 
extern const cst_val val_string_1; 
extern const cst_val val_string_2;
extern const cst_val val_string_3;
extern const cst_val val_string_4;
extern const cst_val val_string_5;
extern const cst_val val_string_6;
extern const cst_val val_string_7;
extern const cst_val val_string_8;
extern const cst_val val_string_9;
extern const cst_val val_string_10; 
extern const cst_val val_string_11; 
extern const cst_val val_string_12;
extern const cst_val val_string_13;
extern const cst_val val_string_14;
extern const cst_val val_string_15;
extern const cst_val val_string_16;
extern const cst_val val_string_17;
extern const cst_val val_string_18;
extern const cst_val val_string_19;

#else
/* Only GCC seems to currently support the C99 standard for giving      */
/* explicit names for fields for initializing unions, because we want   */
/* things to be const, and to be small structures this is really useful */
/* thus for compilers not supporting no_union_initization we use other  */
/* structure that we know (hope ?) are the same size and use agressive  */
/* casting. The goal here is wholly justified by the method here isn't  */
/* pretty                                                               */

/* These structures are defined *solely* to get round initialization    */
/* problems if you need to use these in any code you are using your are */
/* unquestionably doing the wrong thing                                 */
typedef struct cst_val_atom_struct_float {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important */
#else
    short type;  /* order is here important */
    short ref_count;
#endif
    float fval;
} cst_val_float;

typedef struct cst_val_atom_struct_int {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important (and unintuitive) */
#else
    short type;  /* order is here important */
    short ref_count;
#endif
    int ival;
} cst_val_int;

typedef struct cst_val_atom_struct_void {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important */
#else
    short type;  /* order is here important */
    short ref_count;
#endif
    void *vval;
} cst_val_void;

#ifdef WORDS_BIGENDIAN
#define DEF_CONST_VAL_INT(N,V) const cst_val_int N={-1, CST_VAL_TYPE_INT, V}
#define DEF_CONST_VAL_STRING(N,S) const cst_val_void N={-1,CST_VAL_TYPE_STRING,(void *)S}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val_float N={-1,CST_VAL_TYPE_FLOAT,(float)F}
#else
#define DEF_CONST_VAL_INT(N,V) const cst_val_int N={CST_VAL_TYPE_INT,-1,V}
#define DEF_CONST_VAL_STRING(N,S) const cst_val_void N={CST_VAL_TYPE_STRING,-1,(void *)S}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val_float N={CST_VAL_TYPE_FLOAT,-1,(float)F}
#endif
#define DEF_CONST_VAL_CONS(N,A,D) const cst_val_cons N={A,D}

/* in the non-union intialization version we these consts have to be */
/* more typed than need, we'll cast the back later                   */
extern const cst_val_int val_int_0; 
extern const cst_val_int val_int_1; 
extern const cst_val_int val_int_2;
extern const cst_val_int val_int_3;
extern const cst_val_int val_int_4;
extern const cst_val_int val_int_5;
extern const cst_val_int val_int_6;
extern const cst_val_int val_int_7;
extern const cst_val_int val_int_8;
extern const cst_val_int val_int_9;
extern const cst_val_int val_int_10; 
extern const cst_val_int val_int_11; 
extern const cst_val_int val_int_12;
extern const cst_val_int val_int_13;
extern const cst_val_int val_int_14;
extern const cst_val_int val_int_15;
extern const cst_val_int val_int_16;
extern const cst_val_int val_int_17;
extern const cst_val_int val_int_18;
extern const cst_val_int val_int_19;

extern const cst_val_void val_string_0; 
extern const cst_val_void val_string_1; 
extern const cst_val_void val_string_2;
extern const cst_val_void val_string_3;
extern const cst_val_void val_string_4;
extern const cst_val_void val_string_5;
extern const cst_val_void val_string_6;
extern const cst_val_void val_string_7;
extern const cst_val_void val_string_8;
extern const cst_val_void val_string_9;
extern const cst_val_void val_string_10; 
extern const cst_val_void val_string_11; 
extern const cst_val_void val_string_12;
extern const cst_val_void val_string_13;
extern const cst_val_void val_string_14;
extern const cst_val_void val_string_15;
extern const cst_val_void val_string_16;
extern const cst_val_void val_string_17;
extern const cst_val_void val_string_18;
extern const cst_val_void val_string_19;

#endif

#define DEF_STATIC_CONST_VAL_INT(N,V) static DEF_CONST_VAL_INT(N,V)
#define DEF_STATIC_CONST_VAL_STRING(N,S) static DEF_CONST_VAL_STRING(N,S)
#define DEF_STATIC_CONST_VAL_FLOAT(N,F) static DEF_CONST_VAL_FLOAT(N,F)
#define DEF_STATIC_CONST_VAL_CONS(N,A,D) static DEF_CONST_VAL_CONS(N,A,D)

/* Some actual val consts */
/* The have casts as in the non-union intialize case the casts are necessary */
/* but in the union initial case these casts are harmless                    */

#define VAL_INT_0 (cst_val *)&val_int_0
#define VAL_INT_1 (cst_val *)&val_int_1
#define VAL_INT_2 (cst_val *)&val_int_2
#define VAL_INT_3 (cst_val *)&val_int_3
#define VAL_INT_4 (cst_val *)&val_int_4
#define VAL_INT_5 (cst_val *)&val_int_5
#define VAL_INT_6 (cst_val *)&val_int_6
#define VAL_INT_7 (cst_val *)&val_int_7
#define VAL_INT_8 (cst_val *)&val_int_8
#define VAL_INT_9 (cst_val *)&val_int_9
#define VAL_INT_10 (cst_val *)&val_int_10
#define VAL_INT_11 (cst_val *)&val_int_11
#define VAL_INT_12 (cst_val *)&val_int_12
#define VAL_INT_13 (cst_val *)&val_int_13
#define VAL_INT_14 (cst_val *)&val_int_14
#define VAL_INT_15 (cst_val *)&val_int_15
#define VAL_INT_16 (cst_val *)&val_int_16
#define VAL_INT_17 (cst_val *)&val_int_17
#define VAL_INT_18 (cst_val *)&val_int_18
#define VAL_INT_19 (cst_val *)&val_int_19

const cst_val *val_int_n(int n);

#define VAL_STRING_0 (cst_val *)&val_string_0
#define VAL_STRING_1 (cst_val *)&val_string_1
#define VAL_STRING_2 (cst_val *)&val_string_2
#define VAL_STRING_3 (cst_val *)&val_string_3
#define VAL_STRING_4 (cst_val *)&val_string_4
#define VAL_STRING_5 (cst_val *)&val_string_5
#define VAL_STRING_6 (cst_val *)&val_string_6
#define VAL_STRING_7 (cst_val *)&val_string_7
#define VAL_STRING_8 (cst_val *)&val_string_8
#define VAL_STRING_9 (cst_val *)&val_string_9
#define VAL_STRING_10 (cst_val *)&val_string_10
#define VAL_STRING_11 (cst_val *)&val_string_11
#define VAL_STRING_12 (cst_val *)&val_string_12
#define VAL_STRING_13 (cst_val *)&val_string_13
#define VAL_STRING_14 (cst_val *)&val_string_14
#define VAL_STRING_15 (cst_val *)&val_string_15
#define VAL_STRING_16 (cst_val *)&val_string_16
#define VAL_STRING_17 (cst_val *)&val_string_17
#define VAL_STRING_18 (cst_val *)&val_string_18
#define VAL_STRING_19 (cst_val *)&val_string_19

const cst_val *val_string_n(int n);


#endif
