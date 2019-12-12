/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the dboolhuff.LICENSE file in this directory.
 *  See the libvpx original distribution for more information,
 *  including patent information, and author information.
 */


#include "dboolhuff.h"

#ifdef _MSC_VER
__declspec (align (16))
     const unsigned char vp8_norm[256] = {
#else
const unsigned char vp8_norm[256] __attribute__ ((aligned (16))) = {
#endif
0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int
vp8dx_start_decode (BOOL_DECODER * br,
    const unsigned char *source, unsigned int source_sz)
{
  br->user_buffer_end = source + source_sz;
  br->user_buffer = source;
  br->value = 0;
  br->count = -8;
  br->range = 255;

  if (source_sz && !source)
    return 1;

  /* Populate the buffer */
  vp8dx_bool_decoder_fill (br);

  return 0;
}


void
vp8dx_bool_decoder_fill (BOOL_DECODER * br)
{
  const unsigned char *bufptr;
  const unsigned char *bufend;
  VP8_BD_VALUE value;
  int count;
  bufend = br->user_buffer_end;
  bufptr = br->user_buffer;
  value = br->value;
  count = br->count;

  VP8DX_BOOL_DECODER_FILL (count, value, bufptr, bufend);

  br->user_buffer = bufptr;
  br->value = value;
  br->count = count;
}
