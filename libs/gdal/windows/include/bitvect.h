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
 * File:    bitvect.h
 * Purpose: header file for bit-vector API
 * Dependencies: 
 * Invokes:
 * Contents:
 * Structure definitions: 
 * Constant definitions: 
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __BITVECT_H
#define __BITVECT_H

#include "H4api_adpt.h"

#include "hdf.h"

/* Boolean values used */
typedef enum {BV_FALSE=0, BV_TRUE=1} bv_bool;

/* Flags for the bit-vector */
#define BV_INIT_TO_ONE  0x00000001  /* to indicate whether to create the bit-vector with one's instead of zero's */
#define BV_EXTENDABLE   0x00000002  /* to indicate that the bit-vector can be extended */

/* Default size of a bit-vector */
#define BV_DEFAULT_BITS 128

/* Define the size of the chunks bits are allocated in */
#define BV_CHUNK_SIZE 64

/* Create the external interface data structures needed */
typedef struct bv_struct_tag *bv_ptr;

#if defined BV_MASTER | defined BV_TESTER

/* Base type of the array used to store the bits */
typedef unsigned char bv_base;

/* # of bits in the base type of the array used to store the bits */
#define BV_BASE_BITS    (sizeof(bv_base)*8)

/* bit-vector structure used */
typedef struct bv_struct_tag {
    uint32 bits_used;       /* The actual number of bits current in use */
    uint32 array_size;      /* The number of bv_base elements in the bit-vector */
    uint32 flags;           /* The flags used to create this bit-vector */
    int32 last_zero;        /* The last location we know had a zero bit */
    bv_base *buffer;        /* Pointer to the buffer used to store the bits */
  }bv_struct;

/* Table of bits for each bit position */
/*  (This will need to be changed/expanded if another base type is used) */
static const uint8 bv_bit_value[8]={
    1,  /* bit 0's value is 1 */
    2,  /* bit 1's value is 2 */
    4,  /* bit 2's value is 4 */
    8,  /* bit 3's value is 8 */
    16,  /* bit 4's value is 16 */
    32,  /* bit 5's value is 32 */
    64,  /* bit 6's value is 64 */
    128   /* bit 7's value is 128 */
};

/* Table of bit-masks for each number of bits in a byte */
/*  (This will need to be changed/expanded if another base type is used) */
static const uint8 bv_bit_mask[9]={
    0x00,  /* 0 bits is a mask of 0x00 */
    0x01,  /* 1 bits is a mask of 0x01 */
    0x03,  /* 2 bits is a mask of 0x03 */
    0x07,  /* 3 bits is a mask of 0x07 */
    0x0F,  /* 4 bits is a mask of 0x0F */
    0x1F,  /* 5 bits is a mask of 0x1F */
    0x3F,  /* 6 bits is a mask of 0x3F */
    0x7F,  /* 7 bits is a mask of 0x7F */
    0xFF   /* 8 bits is a mask of 0xFF */
};

/* Table of "first zero bit" for each byte value */
/*  (This will need to be changed/expanded if another base type is used) */
static const int8 bv_first_zero[256]={
    0,  /* "0" - bit 0 is lowest zero */ 1,  /* "1" - bit 1 is lowest zero */
    0,  /* "2" - bit 0 is lowest zero */ 2,  /* "3" - bit 2 is lowest zero */
    0,  /* "4" - bit 0 is lowest zero */ 1,  /* "5" - bit 1 is lowest zero */
    0,  /* "6" - bit 0 is lowest zero */ 3,  /* "7" - bit 3 is lowest zero */
    0,  /* "8" - bit 0 is lowest zero */ 1,  /* "9" - bit 1 is lowest zero */
    0,  /* "10" - bit 0 is lowest zero */ 2,  /* "11" - bit 2 is lowest zero */
    0,  /* "12" - bit 0 is lowest zero */ 1,  /* "13" - bit 1 is lowest zero */
    0,  /* "14" - bit 0 is lowest zero */ 4,  /* "15" - bit 4 is lowest zero */
    0,  /* "16" - bit 0 is lowest zero */ 1,  /* "17" - bit 1 is lowest zero */
    0,  /* "18" - bit 0 is lowest zero */ 2,  /* "19" - bit 2 is lowest zero */
    0,  /* "20" - bit 0 is lowest zero */ 1,  /* "21" - bit 1 is lowest zero */
    0,  /* "22" - bit 0 is lowest zero */ 3,  /* "23" - bit 3 is lowest zero */
    0,  /* "24" - bit 0 is lowest zero */ 1,  /* "25" - bit 1 is lowest zero */
    0,  /* "26" - bit 0 is lowest zero */ 2,  /* "27" - bit 2 is lowest zero */
    0,  /* "28" - bit 0 is lowest zero */ 1,  /* "29" - bit 1 is lowest zero */
    0,  /* "30" - bit 0 is lowest zero */ 5,  /* "31" - bit 5 is lowest zero */
    0,  /* "32" - bit 0 is lowest zero */ 1,  /* "33" - bit 1 is lowest zero */
    0,  /* "34" - bit 0 is lowest zero */ 2,  /* "35" - bit 2 is lowest zero */
    0,  /* "36" - bit 0 is lowest zero */ 1,  /* "37" - bit 1 is lowest zero */
    0,  /* "38" - bit 0 is lowest zero */ 3,  /* "39" - bit 3 is lowest zero */
    0,  /* "40" - bit 0 is lowest zero */ 1,  /* "41" - bit 1 is lowest zero */
    0,  /* "42" - bit 0 is lowest zero */ 2,  /* "43" - bit 2 is lowest zero */
    0,  /* "44" - bit 0 is lowest zero */ 1,  /* "45" - bit 1 is lowest zero */
    0,  /* "46" - bit 0 is lowest zero */ 4,  /* "47" - bit 4 is lowest zero */
    0,  /* "48" - bit 0 is lowest zero */ 1,  /* "49" - bit 1 is lowest zero */
    0,  /* "50" - bit 0 is lowest zero */ 2,  /* "51" - bit 2 is lowest zero */
    0,  /* "52" - bit 0 is lowest zero */ 1,  /* "53" - bit 1 is lowest zero */
    0,  /* "54" - bit 0 is lowest zero */ 3,  /* "55" - bit 3 is lowest zero */
    0,  /* "56" - bit 0 is lowest zero */ 1,  /* "57" - bit 1 is lowest zero */
    0,  /* "58" - bit 0 is lowest zero */ 2,  /* "59" - bit 2 is lowest zero */
    0,  /* "60" - bit 0 is lowest zero */ 1,  /* "61" - bit 1 is lowest zero */
    0,  /* "62" - bit 0 is lowest zero */ 6,  /* "63" - bit 6 is lowest zero */
    0,  /* "64" - bit 0 is lowest zero */ 1,  /* "65" - bit 1 is lowest zero */
    0,  /* "66" - bit 0 is lowest zero */ 2,  /* "67" - bit 2 is lowest zero */
    0,  /* "68" - bit 0 is lowest zero */ 1,  /* "69" - bit 1 is lowest zero */
    0,  /* "70" - bit 0 is lowest zero */ 3,  /* "71" - bit 3 is lowest zero */
    0,  /* "72" - bit 0 is lowest zero */ 1,  /* "73" - bit 1 is lowest zero */
    0,  /* "74" - bit 0 is lowest zero */ 2,  /* "75" - bit 2 is lowest zero */
    0,  /* "76" - bit 0 is lowest zero */ 1,  /* "77" - bit 1 is lowest zero */
    0,  /* "78" - bit 0 is lowest zero */ 4,  /* "79" - bit 4 is lowest zero */
    0,  /* "80" - bit 0 is lowest zero */ 1,  /* "81" - bit 1 is lowest zero */
    0,  /* "82" - bit 0 is lowest zero */ 2,  /* "83" - bit 2 is lowest zero */
    0,  /* "84" - bit 0 is lowest zero */ 1,  /* "85" - bit 1 is lowest zero */
    0,  /* "86" - bit 0 is lowest zero */ 3,  /* "87" - bit 3 is lowest zero */
    0,  /* "88" - bit 0 is lowest zero */ 1,  /* "89" - bit 1 is lowest zero */
    0,  /* "90" - bit 0 is lowest zero */ 2,  /* "91" - bit 2 is lowest zero */
    0,  /* "92" - bit 0 is lowest zero */ 1,  /* "93" - bit 1 is lowest zero */
    0,  /* "94" - bit 0 is lowest zero */ 5,  /* "95" - bit 5 is lowest zero */
    0,  /* "96" - bit 0 is lowest zero */ 1,  /* "97" - bit 1 is lowest zero */
    0,  /* "98" - bit 0 is lowest zero */ 2,  /* "99" - bit 2 is lowest zero */
    0,  /* "100" - bit 0 is lowest zero */ 1,  /* "101" - bit 1 is lowest zero */
    0,  /* "102" - bit 0 is lowest zero */ 3,  /* "103" - bit 3 is lowest zero */
    0,  /* "104" - bit 0 is lowest zero */ 1,  /* "105" - bit 1 is lowest zero */
    0,  /* "106" - bit 0 is lowest zero */ 2,  /* "107" - bit 2 is lowest zero */
    0,  /* "108" - bit 0 is lowest zero */ 1,  /* "109" - bit 1 is lowest zero */
    0,  /* "110" - bit 0 is lowest zero */ 4,  /* "111" - bit 4 is lowest zero */
    0,  /* "112" - bit 0 is lowest zero */ 1,  /* "113" - bit 1 is lowest zero */
    0,  /* "114" - bit 0 is lowest zero */ 2,  /* "115" - bit 2 is lowest zero */
    0,  /* "116" - bit 0 is lowest zero */ 1,  /* "117" - bit 1 is lowest zero */
    0,  /* "118" - bit 0 is lowest zero */ 3,  /* "119" - bit 3 is lowest zero */
    0,  /* "120" - bit 0 is lowest zero */ 1,  /* "121" - bit 1 is lowest zero */
    0,  /* "122" - bit 0 is lowest zero */ 2,  /* "123" - bit 2 is lowest zero */
    0,  /* "124" - bit 0 is lowest zero */ 1,  /* "125" - bit 1 is lowest zero */
    0,  /* "126" - bit 0 is lowest zero */ 7,  /* "127" - bit 7 is lowest zero */
    0,  /* "128" - bit 0 is lowest zero */ 1,  /* "129" - bit 1 is lowest zero */
    0,  /* "130" - bit 0 is lowest zero */ 2,  /* "131" - bit 2 is lowest zero */
    0,  /* "132" - bit 0 is lowest zero */ 1,  /* "133" - bit 1 is lowest zero */
    0,  /* "134" - bit 0 is lowest zero */ 3,  /* "135" - bit 3 is lowest zero */
    0,  /* "136" - bit 0 is lowest zero */ 1,  /* "137" - bit 1 is lowest zero */
    0,  /* "138" - bit 0 is lowest zero */ 2,  /* "139" - bit 2 is lowest zero */
    0,  /* "140" - bit 0 is lowest zero */ 1,  /* "141" - bit 1 is lowest zero */
    0,  /* "142" - bit 0 is lowest zero */ 4,  /* "143" - bit 4 is lowest zero */
    0,  /* "144" - bit 0 is lowest zero */ 1,  /* "145" - bit 1 is lowest zero */
    0,  /* "146" - bit 0 is lowest zero */ 2,  /* "147" - bit 2 is lowest zero */
    0,  /* "148" - bit 0 is lowest zero */ 1,  /* "149" - bit 1 is lowest zero */
    0,  /* "150" - bit 0 is lowest zero */ 3,  /* "151" - bit 3 is lowest zero */
    0,  /* "152" - bit 0 is lowest zero */ 1,  /* "153" - bit 1 is lowest zero */
    0,  /* "154" - bit 0 is lowest zero */ 2,  /* "155" - bit 2 is lowest zero */
    0,  /* "156" - bit 0 is lowest zero */ 1,  /* "157" - bit 1 is lowest zero */
    0,  /* "158" - bit 0 is lowest zero */ 5,  /* "159" - bit 5 is lowest zero */
    0,  /* "160" - bit 0 is lowest zero */ 1,  /* "161" - bit 1 is lowest zero */
    0,  /* "162" - bit 0 is lowest zero */ 2,  /* "163" - bit 2 is lowest zero */
    0,  /* "164" - bit 0 is lowest zero */ 1,  /* "165" - bit 1 is lowest zero */
    0,  /* "166" - bit 0 is lowest zero */ 3,  /* "167" - bit 3 is lowest zero */
    0,  /* "168" - bit 0 is lowest zero */ 1,  /* "169" - bit 1 is lowest zero */
    0,  /* "170" - bit 0 is lowest zero */ 2,  /* "171" - bit 2 is lowest zero */
    0,  /* "172" - bit 0 is lowest zero */ 1,  /* "173" - bit 1 is lowest zero */
    0,  /* "174" - bit 0 is lowest zero */ 4,  /* "175" - bit 4 is lowest zero */
    0,  /* "176" - bit 0 is lowest zero */ 1,  /* "177" - bit 1 is lowest zero */
    0,  /* "178" - bit 0 is lowest zero */ 2,  /* "179" - bit 2 is lowest zero */
    0,  /* "180" - bit 0 is lowest zero */ 1,  /* "181" - bit 1 is lowest zero */
    0,  /* "182" - bit 0 is lowest zero */ 3,  /* "183" - bit 3 is lowest zero */
    0,  /* "184" - bit 0 is lowest zero */ 1,  /* "185" - bit 1 is lowest zero */
    0,  /* "186" - bit 0 is lowest zero */ 2,  /* "187" - bit 2 is lowest zero */
    0,  /* "188" - bit 0 is lowest zero */ 1,  /* "189" - bit 1 is lowest zero */
    0,  /* "190" - bit 0 is lowest zero */ 6,  /* "191" - bit 6 is lowest zero */
    0,  /* "192" - bit 0 is lowest zero */ 1,  /* "193" - bit 1 is lowest zero */
    0,  /* "194" - bit 0 is lowest zero */ 2,  /* "195" - bit 2 is lowest zero */
    0,  /* "196" - bit 0 is lowest zero */ 1,  /* "197" - bit 1 is lowest zero */
    0,  /* "198" - bit 0 is lowest zero */ 3,  /* "199" - bit 3 is lowest zero */
    0,  /* "200" - bit 0 is lowest zero */ 1,  /* "201" - bit 1 is lowest zero */
    0,  /* "202" - bit 0 is lowest zero */ 2,  /* "203" - bit 2 is lowest zero */
    0,  /* "204" - bit 0 is lowest zero */ 1,  /* "205" - bit 1 is lowest zero */
    0,  /* "206" - bit 0 is lowest zero */ 4,  /* "207" - bit 4 is lowest zero */
    0,  /* "208" - bit 0 is lowest zero */ 1,  /* "209" - bit 1 is lowest zero */
    0,  /* "210" - bit 0 is lowest zero */ 2,  /* "211" - bit 2 is lowest zero */
    0,  /* "212" - bit 0 is lowest zero */ 1,  /* "213" - bit 1 is lowest zero */
    0,  /* "214" - bit 0 is lowest zero */ 3,  /* "215" - bit 3 is lowest zero */
    0,  /* "216" - bit 0 is lowest zero */ 1,  /* "217" - bit 1 is lowest zero */
    0,  /* "218" - bit 0 is lowest zero */ 2,  /* "219" - bit 2 is lowest zero */
    0,  /* "220" - bit 0 is lowest zero */ 1,  /* "221" - bit 1 is lowest zero */
    0,  /* "222" - bit 0 is lowest zero */ 5,  /* "223" - bit 5 is lowest zero */
    0,  /* "224" - bit 0 is lowest zero */ 1,  /* "225" - bit 1 is lowest zero */
    0,  /* "226" - bit 0 is lowest zero */ 2,  /* "227" - bit 2 is lowest zero */
    0,  /* "228" - bit 0 is lowest zero */ 1,  /* "229" - bit 1 is lowest zero */
    0,  /* "230" - bit 0 is lowest zero */ 3,  /* "231" - bit 3 is lowest zero */
    0,  /* "232" - bit 0 is lowest zero */ 1,  /* "233" - bit 1 is lowest zero */
    0,  /* "234" - bit 0 is lowest zero */ 2,  /* "235" - bit 2 is lowest zero */
    0,  /* "236" - bit 0 is lowest zero */ 1,  /* "237" - bit 1 is lowest zero */
    0,  /* "238" - bit 0 is lowest zero */ 4,  /* "239" - bit 4 is lowest zero */
    0,  /* "240" - bit 0 is lowest zero */ 1,  /* "241" - bit 1 is lowest zero */
    0,  /* "242" - bit 0 is lowest zero */ 2,  /* "243" - bit 2 is lowest zero */
    0,  /* "244" - bit 0 is lowest zero */ 1,  /* "245" - bit 1 is lowest zero */
    0,  /* "246" - bit 0 is lowest zero */ 3,  /* "247" - bit 3 is lowest zero */
    0,  /* "248" - bit 0 is lowest zero */ 1,  /* "249" - bit 1 is lowest zero */
    0,  /* "250" - bit 0 is lowest zero */ 2,  /* "251" - bit 2 is lowest zero */
    0,  /* "252" - bit 0 is lowest zero */ 1,  /* "253" - bit 1 is lowest zero */
    0,  /* "254" - bit 0 is lowest zero */ 8   /* "255" - bit 8 is lowest zero */
};

/* Table of "number of 1 bits" for each byte value */
/*  (This will need to be changed/expanded if another base type is used) */
static const int8 bv_num_ones[256]={
    0,  /* "0" - n bits are 1's */ 1,  /* "1" - n bits are 1's */
    1,  /* "2" - n bits are 1's */ 2,  /* "3" - n bits are 1's */
    1,  /* "4" - n bits are 1's */ 2,  /* "5" - n bits are 1's */
    2,  /* "6" - n bits are 1's */ 3,  /* "7" - n bits are 1's */
    1,  /* "8" - n bits are 1's */ 2,  /* "9" - n bits are 1's */
    2,  /* "10" - n bits are 1's */ 3,  /* "11" - n bits are 1's */
    2,  /* "12" - n bits are 1's */ 3,  /* "13" - n bits are 1's */
    3,  /* "14" - n bits are 1's */ 4,  /* "15" - n bits are 1's */
    1,  /* "16" - n bits are 1's */ 2,  /* "17" - n bits are 1's */
    2,  /* "18" - n bits are 1's */ 3,  /* "19" - n bits are 1's */
    2,  /* "20" - n bits are 1's */ 3,  /* "21" - n bits are 1's */
    3,  /* "22" - n bits are 1's */ 4,  /* "23" - n bits are 1's */
    2,  /* "24" - n bits are 1's */ 3,  /* "25" - n bits are 1's */
    3,  /* "26" - n bits are 1's */ 4,  /* "27" - n bits are 1's */
    3,  /* "28" - n bits are 1's */ 3,  /* "29" - n bits are 1's */
    4,  /* "30" - n bits are 1's */ 5,  /* "31" - n bits are 1's */
    1,  /* "32" - n bits are 1's */ 2,  /* "33" - n bits are 1's */
    2,  /* "34" - n bits are 1's */ 3,  /* "35" - n bits are 1's */
    2,  /* "36" - n bits are 1's */ 3,  /* "37" - n bits are 1's */
    3,  /* "38" - n bits are 1's */ 4,  /* "39" - n bits are 1's */
    2,  /* "40" - n bits are 1's */ 3,  /* "41" - n bits are 1's */
    3,  /* "42" - n bits are 1's */ 4,  /* "43" - n bits are 1's */
    3,  /* "44" - n bits are 1's */ 4,  /* "45" - n bits are 1's */
    4,  /* "46" - n bits are 1's */ 5,  /* "47" - n bits are 1's */
    2,  /* "48" - n bits are 1's */ 3,  /* "49" - n bits are 1's */
    3,  /* "50" - n bits are 1's */ 4,  /* "51" - n bits are 1's */
    3,  /* "52" - n bits are 1's */ 4,  /* "53" - n bits are 1's */
    4,  /* "54" - n bits are 1's */ 5,  /* "55" - n bits are 1's */
    3,  /* "56" - n bits are 1's */ 4,  /* "57" - n bits are 1's */
    4,  /* "58" - n bits are 1's */ 5,  /* "59" - n bits are 1's */
    4,  /* "60" - n bits are 1's */ 5,  /* "61" - n bits are 1's */
    5,  /* "62" - n bits are 1's */ 6,  /* "63" - n bits are 1's */
    1,  /* "64" - n bits are 1's */ 2,  /* "65" - n bits are 1's */
    2,  /* "66" - n bits are 1's */ 3,  /* "67" - n bits are 1's */
    2,  /* "68" - n bits are 1's */ 3,  /* "69" - n bits are 1's */
    3,  /* "70" - n bits are 1's */ 4,  /* "71" - n bits are 1's */
    2,  /* "72" - n bits are 1's */ 3,  /* "73" - n bits are 1's */
    3,  /* "74" - n bits are 1's */ 4,  /* "75" - n bits are 1's */
    3,  /* "76" - n bits are 1's */ 4,  /* "77" - n bits are 1's */
    4,  /* "78" - n bits are 1's */ 5,  /* "79" - n bits are 1's */
    2,  /* "80" - n bits are 1's */ 3,  /* "81" - n bits are 1's */
    3,  /* "82" - n bits are 1's */ 4,  /* "83" - n bits are 1's */
    3,  /* "84" - n bits are 1's */ 4,  /* "85" - n bits are 1's */
    4,  /* "86" - n bits are 1's */ 5,  /* "87" - n bits are 1's */
    3,  /* "88" - n bits are 1's */ 4,  /* "89" - n bits are 1's */
    4,  /* "90" - n bits are 1's */ 5,  /* "91" - n bits are 1's */
    4,  /* "92" - n bits are 1's */ 5,  /* "93" - n bits are 1's */
    5,  /* "94" - n bits are 1's */ 6,  /* "95" - n bits are 1's */
    2,  /* "96" - n bits are 1's */ 3,  /* "97" - n bits are 1's */
    3,  /* "98" - n bits are 1's */ 4,  /* "99" - n bits are 1's */
    3,  /* "100" - n bits are 1's */ 4,  /* "101" - n bits are 1's */
    4,  /* "102" - n bits are 1's */ 5,  /* "103" - n bits are 1's */
    3,  /* "104" - n bits are 1's */ 4,  /* "105" - n bits are 1's */
    4,  /* "106" - n bits are 1's */ 5,  /* "107" - n bits are 1's */
    3,  /* "108" - n bits are 1's */ 4,  /* "109" - n bits are 1's */
    4,  /* "110" - n bits are 1's */ 5,  /* "111" - n bits are 1's */
    3,  /* "112" - n bits are 1's */ 4,  /* "113" - n bits are 1's */
    4,  /* "114" - n bits are 1's */ 5,  /* "115" - n bits are 1's */
    4,  /* "116" - n bits are 1's */ 5,  /* "117" - n bits are 1's */
    5,  /* "118" - n bits are 1's */ 6,  /* "119" - n bits are 1's */
    4,  /* "120" - n bits are 1's */ 5,  /* "121" - n bits are 1's */
    5,  /* "122" - n bits are 1's */ 6,  /* "123" - n bits are 1's */
    5,  /* "124" - n bits are 1's */ 6,  /* "125" - n bits are 1's */
    6,  /* "126" - n bits are 1's */ 7,  /* "127" - n bits are 1's */
    1,  /* "128" - n bits are 1's */ 2,  /* "129" - n bits are 1's */
    2,  /* "130" - n bits are 1's */ 3,  /* "131" - n bits are 1's */
    2,  /* "132" - n bits are 1's */ 3,  /* "133" - n bits are 1's */
    3,  /* "134" - n bits are 1's */ 4,  /* "135" - n bits are 1's */
    2,  /* "136" - n bits are 1's */ 3,  /* "137" - n bits are 1's */
    3,  /* "138" - n bits are 1's */ 4,  /* "139" - n bits are 1's */
    3,  /* "140" - n bits are 1's */ 4,  /* "141" - n bits are 1's */
    4,  /* "142" - n bits are 1's */ 5,  /* "143" - n bits are 1's */
    2,  /* "144" - n bits are 1's */ 3,  /* "145" - n bits are 1's */
    3,  /* "146" - n bits are 1's */ 4,  /* "147" - n bits are 1's */
    3,  /* "148" - n bits are 1's */ 4,  /* "149" - n bits are 1's */
    4,  /* "150" - n bits are 1's */ 5,  /* "151" - n bits are 1's */
    3,  /* "152" - n bits are 1's */ 4,  /* "153" - n bits are 1's */
    4,  /* "154" - n bits are 1's */ 5,  /* "155" - n bits are 1's */
    4,  /* "156" - n bits are 1's */ 5,  /* "157" - n bits are 1's */
    5,  /* "158" - n bits are 1's */ 6,  /* "159" - n bits are 1's */
    2,  /* "160" - n bits are 1's */ 3,  /* "161" - n bits are 1's */
    3,  /* "162" - n bits are 1's */ 4,  /* "163" - n bits are 1's */
    3,  /* "164" - n bits are 1's */ 4,  /* "165" - n bits are 1's */
    4,  /* "166" - n bits are 1's */ 5,  /* "167" - n bits are 1's */
    3,  /* "168" - n bits are 1's */ 4,  /* "169" - n bits are 1's */
    4,  /* "170" - n bits are 1's */ 5,  /* "171" - n bits are 1's */
    4,  /* "172" - n bits are 1's */ 5,  /* "173" - n bits are 1's */
    5,  /* "174" - n bits are 1's */ 6,  /* "175" - n bits are 1's */
    3,  /* "176" - n bits are 1's */ 4,  /* "177" - n bits are 1's */
    4,  /* "178" - n bits are 1's */ 5,  /* "179" - n bits are 1's */
    4,  /* "180" - n bits are 1's */ 5,  /* "181" - n bits are 1's */
    5,  /* "182" - n bits are 1's */ 6,  /* "183" - n bits are 1's */
    4,  /* "184" - n bits are 1's */ 5,  /* "185" - n bits are 1's */
    5,  /* "186" - n bits are 1's */ 6,  /* "187" - n bits are 1's */
    5,  /* "188" - n bits are 1's */ 6,  /* "189" - n bits are 1's */
    6,  /* "190" - n bits are 1's */ 7,  /* "191" - n bits are 1's */
    2,  /* "192" - n bits are 1's */ 3,  /* "193" - n bits are 1's */
    3,  /* "194" - n bits are 1's */ 4,  /* "195" - n bits are 1's */
    3,  /* "196" - n bits are 1's */ 4,  /* "197" - n bits are 1's */
    4,  /* "198" - n bits are 1's */ 5,  /* "199" - n bits are 1's */
    3,  /* "200" - n bits are 1's */ 4,  /* "201" - n bits are 1's */
    4,  /* "202" - n bits are 1's */ 5,  /* "203" - n bits are 1's */
    4,  /* "204" - n bits are 1's */ 5,  /* "205" - n bits are 1's */
    5,  /* "206" - n bits are 1's */ 6,  /* "207" - n bits are 1's */
    3,  /* "208" - n bits are 1's */ 4,  /* "209" - n bits are 1's */
    4,  /* "210" - n bits are 1's */ 5,  /* "211" - n bits are 1's */
    4,  /* "212" - n bits are 1's */ 5,  /* "213" - n bits are 1's */
    5,  /* "214" - n bits are 1's */ 6,  /* "215" - n bits are 1's */
    4,  /* "216" - n bits are 1's */ 5,  /* "217" - n bits are 1's */
    5,  /* "218" - n bits are 1's */ 6,  /* "219" - n bits are 1's */
    5,  /* "220" - n bits are 1's */ 6,  /* "221" - n bits are 1's */
    6,  /* "222" - n bits are 1's */ 7,  /* "223" - n bits are 1's */
    3,  /* "224" - n bits are 1's */ 4,  /* "225" - n bits are 1's */
    4,  /* "226" - n bits are 1's */ 5,  /* "227" - n bits are 1's */
    4,  /* "228" - n bits are 1's */ 5,  /* "229" - n bits are 1's */
    5,  /* "230" - n bits are 1's */ 6,  /* "231" - n bits are 1's */
    4,  /* "232" - n bits are 1's */ 5,  /* "233" - n bits are 1's */
    5,  /* "234" - n bits are 1's */ 6,  /* "235" - n bits are 1's */
    5,  /* "236" - n bits are 1's */ 6,  /* "237" - n bits are 1's */
    6,  /* "238" - n bits are 1's */ 7,  /* "239" - n bits are 1's */
    4,  /* "240" - n bits are 1's */ 5,  /* "241" - n bits are 1's */
    5,  /* "242" - n bits are 1's */ 6,  /* "243" - n bits are 1's */
    5,  /* "244" - n bits are 1's */ 6,  /* "245" - n bits are 1's */
    6,  /* "246" - n bits are 1's */ 7,  /* "247" - n bits are 1's */
    5,  /* "248" - n bits are 1's */ 6,  /* "249" - n bits are 1's */
    6,  /* "250" - n bits are 1's */ 7,  /* "251" - n bits are 1's */
    6,  /* "252" - n bits are 1's */ 7,  /* "253" - n bits are 1's */
    7,  /* "254" - n bits are 1's */ 8   /* "255" - n bits are 1's */
};

/* Useful routines for generally private use */

#endif /* BV_MASTER | BV_TESTER */
#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */
HDFLIBAPI bv_ptr bv_new(int32 num_bits, uint32 flags);

HDFLIBAPI intn bv_delete(bv_ptr b);

HDFLIBAPI intn bv_set(bv_ptr b, int32 bit_num, bv_bool value);

HDFLIBAPI intn bv_get(bv_ptr b, int32 bit_num);

HDFLIBAPI intn bv_clear(bv_ptr b, bv_bool value);

HDFLIBAPI int32 bv_size(bv_ptr b);

HDFLIBAPI uint32 bv_flags(bv_ptr b);

HDFLIBAPI int32 bv_find(bv_ptr b, int32 last_find, bv_bool value);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#endif /* __BITVECT_H */

