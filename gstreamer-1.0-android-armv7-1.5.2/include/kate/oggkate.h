/* Copyright (C) 2008 Vincent Penquerc'h.
   This file is part of the Kate codec library.
   Written by Vincent Penquerc'h.

   Use, distribution and reproduction of this library is governed
   by a BSD style source license included with this source in the
   file 'COPYING'. Please read these terms before distributing. */


#ifndef KATE_oggkate_h_GUARD
#define KATE_oggkate_h_GUARD

/** \file oggkate.h
  The libkate Ogg interface public API.
  */

#include <stddef.h>
#include <ogg/ogg.h>
#include "kate/kate.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup ogg_encode Ogg encoding interface */
extern int kate_ogg_encode_headers(kate_state *k,kate_comment *kc,ogg_packet *op);
extern int kate_ogg_encode_text(kate_state *k,kate_float start_time,kate_float stop_time,const char *text,size_t sz,ogg_packet *op); /* text is not null terminated */
extern int kate_ogg_encode_text_raw_times(kate_state *k,kate_int64_t start_time,kate_int64_t stop_time,const char *text,size_t sz,ogg_packet *op); /* text is not null terminated */
extern int kate_ogg_encode_repeat(kate_state *k,kate_float t,kate_float threshold,ogg_packet *op);
extern int kate_ogg_encode_repeat_raw_times(kate_state *k,kate_int64_t t,kate_int64_t threshold,ogg_packet *op);
extern int kate_ogg_encode_keepalive(kate_state *k,kate_float t,ogg_packet *op);
extern int kate_ogg_encode_keepalive_raw_times(kate_state *k,kate_int64_t t,ogg_packet *op);
extern int kate_ogg_encode_finish(kate_state *k,kate_float t,ogg_packet *op); /* t may be negative to use the end granule of the last event */
extern int kate_ogg_encode_finish_raw_times(kate_state *k,kate_int64_t t,ogg_packet *op); /* t may be negative to use the end granule of the last event */

/** \defgroup ogg_decode Ogg decoding interface */
extern int kate_ogg_decode_is_idheader(const ogg_packet *op);
extern int kate_ogg_decode_headerin(kate_info *ki,kate_comment *kc,ogg_packet *op);
extern int kate_ogg_decode_packetin(kate_state *k,ogg_packet *op);

#ifdef __cplusplus
}
#endif

#endif

