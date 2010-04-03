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
/*  Short term signals                                                   */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_STS_H__
#define _CST_STS_H__

/* Need some lower level functions in case we are doing streaming */
#include "cst_wave.h"
#include "cst_audio.h"

/* The short term signal (sts) structure is the basic unit data info  */
/* it may be diphones or general units.  Indexes and names are held   */
/* else where, this information plus the indexes in the Unit relation */
/* allow reconstruction of the signal itself                          */
struct cst_sts_struct {
    const unsigned short *frame;  
    const int size;      /* in samples */
    const unsigned char *residual;
};
typedef struct cst_sts_struct cst_sts;

/* Because many C compilers can't compile when there are 100Ks of symbols */
/* We store the sts in pages.  Each page of stss gets compiled into an */
/* array in separate files thus reducing the number of symbols -- but */
/* introducing an extra dereference */
struct cst_sts_paged_struct {
    const unsigned short frame_offset;
    const unsigned short res_size;
    const unsigned int res_offset;
    const unsigned short *frame_page;  
    const unsigned char *res_page;  
};
typedef struct cst_sts_paged_struct cst_sts_paged;

/* This represents a database of short-term signals. */
struct cst_sts_list_struct {
    /* If the sts are compiled in, this will point to them. */
    const cst_sts *sts;
    /* Or if the data is paged in different structures */
    const cst_sts_paged *sts_paged;
    /* Or we could have these set (or set later) */
    const unsigned short *frames;
    const unsigned char *residuals;
    const unsigned int *resoffs;

    int num_sts;          /* But I don't think you need that number */
    int num_channels;     /* typically lpc order */
    int sample_rate;
    float coeff_min;      /* used for decoding the short representation */
    float coeff_range;    /* for coefficients  */ 
};
typedef struct cst_sts_list_struct cst_sts_list;

/* This is used to represent a newly constructed waveform to be synthed */
struct cst_lpcres_struct {
    const unsigned short **frames;
    int *times;
    int num_frames;
    int num_channels;
    float lpc_min;
    float lpc_range;
    int num_samples;
    int sample_rate;
    int *sizes;
    unsigned char *residual;

    /* Optional call back function */
    cst_audio_streaming_info *asi;
};
typedef struct cst_lpcres_struct cst_lpcres;

cst_lpcres *new_lpcres();
void delete_lpcres(cst_lpcres *l);
float lpcres_frame_shift(cst_lpcres *t, int frame);
void lpcres_resize_frames(cst_lpcres *l,int num_frames);
void lpcres_resize_samples(cst_lpcres *l,int num_samples);

cst_sts_list *new_sts_list();
void delete_sts_list(cst_sts_list *l);

const unsigned short * get_sts_frame(const cst_sts_list *sts_list, int frame);
const unsigned char * get_sts_residual(const cst_sts_list *sts_list, int frame);
const unsigned char * get_sts_residual_fixed(const cst_sts_list *sts_list, int frame);

int get_frame_size(const cst_sts_list *sts_list, int frame);
int get_unit_size(const cst_sts_list *s,int start, int end);

CST_VAL_USER_TYPE_DCLS(lpcres,cst_lpcres)
CST_VAL_USER_TYPE_DCLS(sts_list,cst_sts_list)

#endif
