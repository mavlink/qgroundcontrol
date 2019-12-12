/* Smoke codec
 * Copyright (C) <2004> Wim Taymans <wim@fluendo.com> 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* this is a hack hack hack to get around jpeglib header bugs... */
#ifdef HAVE_STDLIB_H
# undef HAVE_STDLIB_H
#endif
#include <jpeglib.h>

#include "smokecodec.h"
#include "smokeformat.h"

#include <gst/gstinfo.h>

struct _SmokeCodecInfo
{
  unsigned int width;
  unsigned int height;
  unsigned int fps_num;
  unsigned int fps_denom;

  unsigned int minquality;
  unsigned int maxquality;
  unsigned int bitrate;
  unsigned int threshold;

  unsigned int refdec;

  unsigned char **line[3];
  unsigned char *compbuf[3];

  struct jpeg_error_mgr jerr;

  struct jpeg_compress_struct cinfo;
  struct jpeg_destination_mgr jdest;

  struct jpeg_decompress_struct dinfo;
  struct jpeg_source_mgr jsrc;

  int need_keyframe;
  unsigned char *reference;
};

static void
smokecodec_init_destination (j_compress_ptr cinfo)
{
}

static boolean
smokecodec_flush_destination (j_compress_ptr cinfo)
{
  return 1;
}

static void
smokecodec_term_destination (j_compress_ptr cinfo)
{
}

static void
smokecodec_init_source (j_decompress_ptr cinfo)
{
}

static boolean
smokecodec_fill_input_buffer (j_decompress_ptr cinfo)
{
  return 1;
}

static void
smokecodec_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
}

static boolean
smokecodec_resync_to_restart (j_decompress_ptr cinfo, int desired)
{
  return 1;
}

static void
smokecodec_term_source (j_decompress_ptr cinfo)
{
}


int
smokecodec_encode_new (SmokeCodecInfo ** info,
    const unsigned int width,
    const unsigned int height,
    const unsigned int fps_num, const unsigned int fps_denom)
{
  SmokeCodecInfo *newinfo;
  int i, j;
  unsigned char *base[3];

  if (!info)
    return SMOKECODEC_NULLPTR;
  if ((width & 0xf) || (height & 0xf))
    return SMOKECODEC_WRONGSIZE;

  newinfo = malloc (sizeof (SmokeCodecInfo));
  if (!newinfo) {
    return SMOKECODEC_NOMEM;
  }
  newinfo->width = width;
  newinfo->height = height;
  newinfo->fps_num = fps_num;
  newinfo->fps_denom = fps_denom;

  /* setup jpeglib */
  memset (&newinfo->cinfo, 0, sizeof (newinfo->cinfo));
  memset (&newinfo->jerr, 0, sizeof (newinfo->jerr));
  newinfo->cinfo.err = jpeg_std_error (&newinfo->jerr);
  jpeg_create_compress (&newinfo->cinfo);
  newinfo->cinfo.input_components = 3;
  jpeg_set_defaults (&newinfo->cinfo);

  newinfo->cinfo.dct_method = JDCT_FASTEST;

  /* prepare for raw input */
#if JPEG_LIB_VERSION >= 70
  newinfo->cinfo.do_fancy_downsampling = FALSE;
#endif

  newinfo->cinfo.raw_data_in = TRUE;
  newinfo->cinfo.in_color_space = JCS_YCbCr;
  newinfo->cinfo.comp_info[0].h_samp_factor = 2;
  newinfo->cinfo.comp_info[0].v_samp_factor = 2;
  newinfo->cinfo.comp_info[1].h_samp_factor = 1;
  newinfo->cinfo.comp_info[1].v_samp_factor = 1;
  newinfo->cinfo.comp_info[2].h_samp_factor = 1;
  newinfo->cinfo.comp_info[2].v_samp_factor = 1;

  newinfo->line[0] = malloc (DCTSIZE * 2 * sizeof (char *));
  newinfo->line[1] = malloc (DCTSIZE * sizeof (char *));
  newinfo->line[2] = malloc (DCTSIZE * sizeof (char *));
  base[0] = newinfo->compbuf[0] = malloc (256 * 2 * DCTSIZE * 2 * DCTSIZE);
  base[1] = newinfo->compbuf[1] = malloc (256 * DCTSIZE * DCTSIZE);
  base[2] = newinfo->compbuf[2] = malloc (256 * DCTSIZE * DCTSIZE);

  for (i = 0, j = 0; i < 2 * DCTSIZE; i += 2, j++) {
    newinfo->line[0][i] = base[0];
    base[0] += 2 * DCTSIZE * 256;
    newinfo->line[0][i + 1] = base[0];
    base[0] += 2 * DCTSIZE * 256;
    newinfo->line[1][j] = base[1];
    base[1] += DCTSIZE * 256;
    newinfo->line[2][j] = base[2];
    base[2] += DCTSIZE * 256;
  }

  newinfo->jdest.init_destination = smokecodec_init_destination;
  newinfo->jdest.empty_output_buffer = smokecodec_flush_destination;
  newinfo->jdest.term_destination = smokecodec_term_destination;
  newinfo->cinfo.dest = &newinfo->jdest;

  jpeg_suppress_tables (&newinfo->cinfo, FALSE);

  memset (&newinfo->dinfo, 0, sizeof (newinfo->dinfo));
  newinfo->dinfo.err = jpeg_std_error (&newinfo->jerr);
  jpeg_create_decompress (&newinfo->dinfo);

  newinfo->jsrc.init_source = smokecodec_init_source;
  newinfo->jsrc.fill_input_buffer = smokecodec_fill_input_buffer;
  newinfo->jsrc.skip_input_data = smokecodec_skip_input_data;
  newinfo->jsrc.resync_to_restart = smokecodec_resync_to_restart;
  newinfo->jsrc.term_source = smokecodec_term_source;
  newinfo->dinfo.src = &newinfo->jsrc;

  newinfo->need_keyframe = 1;
  newinfo->threshold = 4000;
  newinfo->minquality = 10;
  newinfo->maxquality = 85;
  newinfo->reference = malloc (3 * (width * height) / 2);
  newinfo->refdec = 0;

  *info = newinfo;

  return SMOKECODEC_OK;
}

int
smokecodec_decode_new (SmokeCodecInfo ** info)
{
  return smokecodec_encode_new (info, 16, 16, 1, 1);
}

int
smokecodec_info_free (SmokeCodecInfo * info)
{
  free (info->line[0]);
  free (info->line[1]);
  free (info->line[2]);
  free (info->compbuf[0]);
  free (info->compbuf[1]);
  free (info->compbuf[2]);
  free (info->reference);
  jpeg_destroy_compress (&info->cinfo);
  jpeg_destroy_decompress (&info->dinfo);
  free (info);

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_set_quality (SmokeCodecInfo * info,
    const unsigned int min, const unsigned int max)
{
  info->minquality = min;
  info->maxquality = max;

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_get_quality (SmokeCodecInfo * info,
    unsigned int *min, unsigned int *max)
{
  *min = info->minquality;
  *max = info->maxquality;

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_set_threshold (SmokeCodecInfo * info, const unsigned int threshold)
{
  info->threshold = threshold;

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_get_threshold (SmokeCodecInfo * info, unsigned int *threshold)
{
  *threshold = info->threshold;

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_set_bitrate (SmokeCodecInfo * info, const unsigned int bitrate)
{
  info->bitrate = bitrate;

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_get_bitrate (SmokeCodecInfo * info, unsigned int *bitrate)
{
  *bitrate = info->bitrate;

  return SMOKECODEC_OK;
}

static void
find_best_size (int blocks, unsigned int *width, unsigned int *height)
{
  int sqchng;
  int w, h;
  int best, bestw;
  int free;

  sqchng = ceil (sqrt (blocks));
  w = sqchng;
  h = sqchng;

  GST_DEBUG ("guess: %d %d", w, h);

  free = w * h - blocks;
  best = free;
  bestw = w;

  while (w < 256) {
    GST_DEBUG ("current: %d %d", w, h);
    if (free < best) {
      best = free;
      bestw = w;
      if (free == 0)
        break;
    }
    // if we cannot reduce the height, increase width
    if (free < w) {
      w++;
      free += h;
    }
    // reduce height while possible
    while (free >= w) {
      h--;
      free -= w;
    }
  }
  *width = bestw;
  *height = (blocks + best) / bestw;
}

static int
abs_diff (const unsigned char *in1, const unsigned char *in2, const int stride)
{
  int s;
  int i, j, diff;

  s = 0;

  for (i = 0; i < 2 * DCTSIZE; i++) {
    for (j = 0; j < 2 * DCTSIZE; j++) {
      diff = in1[j] - in2[j];
      s += diff * diff;
    }
    in1 += stride;
    in2 += stride;
  }
  return s;
}

static void
put (const unsigned char *src, unsigned char *dest,
    int width, int height, int srcstride, int deststride)
{
  int i, j;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      dest[j] = src[j];
    }
    src += srcstride;
    dest += deststride;
  }
}

/* encoding */
SmokeCodecResult
smokecodec_encode_id (SmokeCodecInfo * info,
    unsigned char *out, unsigned int *outsize)
{
  int i;

  *out++ = SMOKECODEC_TYPE_ID;
  for (i = 0; i < strlen (SMOKECODEC_ID_STRING); i++) {
    *out++ = SMOKECODEC_ID_STRING[i];
  }
  *out++ = 0;
  *out++ = 1;
  *out++ = 0;

  *outsize = 9;

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_encode (SmokeCodecInfo * info,
    const unsigned char *in,
    SmokeCodecFlags flags, unsigned char *out, unsigned int *outsize)
{
  unsigned int i, j, s;
  const unsigned char *ip;
  unsigned char *op;
  unsigned int blocks, encoding;
  unsigned int size;
  unsigned int width, height;
  unsigned int blocks_w, blocks_h;
  unsigned int threshold;
  unsigned int max;

  if (info->need_keyframe) {
    flags |= SMOKECODEC_KEYFRAME;
    info->need_keyframe = 0;
  }

  if (flags & SMOKECODEC_KEYFRAME)
    threshold = 0;
  else
    threshold = info->threshold;

  ip = in;
  op = info->reference;

  width = info->width;
  height = info->height;

  blocks_w = width / (DCTSIZE * 2);
  blocks_h = height / (DCTSIZE * 2);

  max = blocks_w * blocks_h;

  out[IDX_TYPE] = SMOKECODEC_TYPE_DATA;

#define STORE16(var, pos, x) \
   var[pos]   = (x >> 8); \
   var[pos+1] = (x & 0xff);
#define STORE32(var, pos, x) \
   var[pos]   = ((x >> 24) & 0xff); \
   var[pos+1] = ((x >> 16) & 0xff); \
   var[pos+2] = ((x >> 8) & 0xff); \
   var[pos+3] =  (x & 0xff);

  /* write dimension */
  STORE16 (out, IDX_WIDTH, width);
  STORE16 (out, IDX_HEIGHT, height);

  /* write framerate */
  STORE32 (out, IDX_FPS_NUM, info->fps_num);
  STORE32 (out, IDX_FPS_DENOM, info->fps_denom);

  if (!(flags & SMOKECODEC_KEYFRAME)) {
    int block = 0;

    blocks = 0;
    for (i = 0; i < height; i += 2 * DCTSIZE) {
      for (j = 0; j < width; j += 2 * DCTSIZE) {
        s = abs_diff (ip, op, width);
        if (s >= threshold) {
          STORE16 (out, blocks * 2 + IDX_BLOCKS, block);
          blocks++;
        }

        ip += 2 * DCTSIZE;
        op += 2 * DCTSIZE;
        block++;
      }
      ip += (2 * DCTSIZE - 1) * width;
      op += (2 * DCTSIZE - 1) * width;
    }
    if (blocks == max) {
      flags |= SMOKECODEC_KEYFRAME;
      blocks = 0;
      encoding = max;
    } else {
      encoding = blocks;
    }
  } else {
    blocks = 0;
    encoding = max;
  }
  STORE16 (out, IDX_NUM_BLOCKS, blocks);
  out[IDX_FLAGS] = (flags & 0xff);

  GST_DEBUG ("blocks %d, encoding %d", blocks, encoding);

  info->jdest.next_output_byte = &out[blocks * 2 + OFFS_PICT];
  info->jdest.free_in_buffer = (*outsize) - OFFS_PICT;

  if (encoding > 0) {
    int quality;

    if (!(flags & SMOKECODEC_KEYFRAME))
      find_best_size (encoding, &blocks_w, &blocks_h);

    GST_DEBUG ("best: %d %d", blocks_w, blocks_h);

    info->cinfo.image_width = blocks_w * DCTSIZE * 2;
    info->cinfo.image_height = blocks_h * DCTSIZE * 2;

    if (flags & SMOKECODEC_KEYFRAME) {
      quality = (info->maxquality * 60) / 100;
    } else {
      quality =
          info->maxquality - ((info->maxquality -
              info->minquality) * blocks) / max;
    }

    GST_DEBUG ("set q %d %d %d", quality, encoding, max);
    jpeg_set_quality (&info->cinfo, quality, TRUE);
    GST_DEBUG ("start");
    jpeg_start_compress (&info->cinfo, TRUE);

    for (i = 0; i < encoding; i++) {
      int pos;
      int x, y;

      if (flags & SMOKECODEC_KEYFRAME)
        pos = i;
      else
        pos = (out[i * 2 + IDX_BLOCKS] << 8) | (out[i * 2 + IDX_BLOCKS + 1]);

      x = pos % (width / (DCTSIZE * 2));
      y = pos / (width / (DCTSIZE * 2));

      ip = in + (x * (DCTSIZE * 2)) + (y * (DCTSIZE * 2) * width);
      op = info->compbuf[0] + (i % blocks_w) * (DCTSIZE * 2);
      put (ip, op, 2 * DCTSIZE, 2 * DCTSIZE, width, 256 * (DCTSIZE * 2));

      ip = in + width * height + (x * DCTSIZE) + (y * DCTSIZE * width / 2);
      op = info->compbuf[1] + (i % blocks_w) * (DCTSIZE);
      put (ip, op, DCTSIZE, DCTSIZE, width / 2, 256 * DCTSIZE);

      ip = in + 5 * (width * height) / 4 + (x * DCTSIZE) +
          (y * DCTSIZE * width / 2);
      op = info->compbuf[2] + (i % blocks_w) * (DCTSIZE);
      put (ip, op, DCTSIZE, DCTSIZE, width / 2, 256 * DCTSIZE);

      if ((i % blocks_w) == (blocks_w - 1) || (i == encoding - 1)) {
        GST_DEBUG ("write %d", pos);
        jpeg_write_raw_data (&info->cinfo, info->line, 2 * DCTSIZE);
      }
    }
    GST_DEBUG ("finish");
    jpeg_finish_compress (&info->cinfo);
  }

  size = ((((*outsize) - OFFS_PICT - info->jdest.free_in_buffer) + 3) & ~3);
  STORE16 (out, IDX_SIZE, size);

  *outsize = size + blocks * 2 + OFFS_PICT;
  GST_DEBUG ("outsize %d", *outsize);

  // and decode in reference frame again
  if (info->refdec) {
    smokecodec_decode (info, out, *outsize, info->reference);
  } else {
    memcpy (info->reference, in, 3 * (width * height) / 2);
  }

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_parse_id (SmokeCodecInfo * info,
    const unsigned char *in, const unsigned int insize)
{
  int i;

  if (insize < 4 + strlen (SMOKECODEC_ID_STRING)) {
    return SMOKECODEC_WRONGVERSION;
  }

  if (*in++ != SMOKECODEC_TYPE_ID)
    return SMOKECODEC_ERROR;

  for (i = 0; i < strlen (SMOKECODEC_ID_STRING); i++) {
    if (*in++ != SMOKECODEC_ID_STRING[i])
      return SMOKECODEC_ERROR;
  }
  if (*in++ != 0 || *in++ != 1 || *in++ != 0)
    return SMOKECODEC_ERROR;

  return SMOKECODEC_OK;
}

#define READ16(var, pos, x) \
 x = var[pos]<<8 | var[pos+1];

#define READ32(var, pos, x) \
 x = var[pos]<<24 | var[pos+1]<<16 | \
     var[pos+2]<<8 | var[pos+3];

/* decoding */
SmokeCodecResult
smokecodec_parse_header (SmokeCodecInfo * info,
    const unsigned char *in,
    const unsigned int insize,
    SmokeCodecFlags * flags,
    unsigned int *width,
    unsigned int *height, unsigned int *fps_num, unsigned int *fps_denom)
{

  READ16 (in, IDX_WIDTH, *width);
  READ16 (in, IDX_HEIGHT, *height);
  *flags = in[IDX_FLAGS];
  READ32 (in, IDX_FPS_NUM, *fps_num);
  READ32 (in, IDX_FPS_DENOM, *fps_denom);

  if (info->width != *width ||
      info->height != *height ||
      info->fps_num != *fps_num || info->fps_denom != *fps_denom) {
    GST_DEBUG ("new width: %d %d", *width, *height);

    info->reference = realloc (info->reference, 3 * ((*width) * (*height)) / 2);
    info->width = *width;
    info->height = *height;
    info->fps_num = *fps_num;
    info->fps_denom = *fps_denom;
  }

  return SMOKECODEC_OK;
}

SmokeCodecResult
smokecodec_decode (SmokeCodecInfo * info,
    const unsigned char *in, const unsigned int insize, unsigned char *out)
{
  unsigned int width, height;
  unsigned int fps_num, fps_denom;
  SmokeCodecFlags flags;
  int i, j;
  int blocks_w, blocks_h;
  int blockptr;
  int blocks, decoding;
  const unsigned char *ip;
  unsigned char *op;
  int res;

  smokecodec_parse_header (info, in, insize, &flags, &width, &height,
      &fps_num, &fps_denom);

  READ16 (in, IDX_NUM_BLOCKS, blocks);
  GST_DEBUG ("blocks %d", blocks);

  if (flags & SMOKECODEC_KEYFRAME)
    decoding = width / (DCTSIZE * 2) * height / (DCTSIZE * 2);
  else
    decoding = blocks;

  if (decoding > 0) {
    info->jsrc.next_input_byte = &in[blocks * 2 + OFFS_PICT];
    info->jsrc.bytes_in_buffer = insize - (blocks * 2 + OFFS_PICT);

    GST_DEBUG ("header %02x %d", in[blocks * 2 + OFFS_PICT], insize);
    res = jpeg_read_header (&info->dinfo, TRUE);
    GST_DEBUG ("header %d %d %d", res, info->dinfo.image_width,
        info->dinfo.image_height);

    blocks_w = info->dinfo.image_width / (2 * DCTSIZE);
    blocks_h = info->dinfo.image_height / (2 * DCTSIZE);

    info->dinfo.output_width = info->dinfo.image_width;
    info->dinfo.output_height = info->dinfo.image_height;

    GST_DEBUG ("start");
    info->dinfo.do_fancy_upsampling = FALSE;
    info->dinfo.do_block_smoothing = FALSE;
    info->dinfo.out_color_space = JCS_YCbCr;
    info->dinfo.dct_method = JDCT_IFAST;
    info->dinfo.raw_data_out = TRUE;
    jpeg_start_decompress (&info->dinfo);

    blockptr = 0;

    for (i = 0; i < blocks_h; i++) {
      GST_DEBUG ("read");
      jpeg_read_raw_data (&info->dinfo, info->line, 2 * DCTSIZE);

      GST_DEBUG ("copy %d", blocks_w);
      for (j = 0; j < blocks_w; j++) {
        int pos;
        int x, y;

        if (flags & SMOKECODEC_KEYFRAME)
          pos = blockptr;
        else
          READ16 (in, blockptr * 2 + IDX_BLOCKS, pos);

        x = pos % (width / (DCTSIZE * 2));
        y = pos / (width / (DCTSIZE * 2));

        GST_DEBUG ("block %d %d %d", pos, x, y);

        ip = info->compbuf[0] + j * (DCTSIZE * 2);
        op = info->reference + (x * (DCTSIZE * 2)) +
            (y * (DCTSIZE * 2) * width);
        put (ip, op, 2 * DCTSIZE, 2 * DCTSIZE, 256 * (DCTSIZE * 2), width);

        ip = info->compbuf[1] + j * (DCTSIZE);
        op = info->reference + width * height + (x * DCTSIZE) +
            (y * DCTSIZE * width / 2);
        put (ip, op, DCTSIZE, DCTSIZE, 256 * DCTSIZE, width / 2);

        ip = info->compbuf[2] + j * (DCTSIZE);
        op = info->reference + 5 * (width * height) / 4 + (x * DCTSIZE) +
            (y * DCTSIZE * width / 2);
        put (ip, op, DCTSIZE, DCTSIZE, 256 * DCTSIZE, width / 2);

        GST_DEBUG ("block done %d %d %d", pos, x, y);
        blockptr++;
        if (blockptr >= decoding)
          break;
      }
    }
    GST_DEBUG ("finish");
    jpeg_finish_decompress (&info->dinfo);
  }

  GST_DEBUG ("copy");
  if (out != info->reference)
    memcpy (out, info->reference, 3 * (width * height) / 2);
  GST_DEBUG ("copy done");

  return SMOKECODEC_OK;
}
