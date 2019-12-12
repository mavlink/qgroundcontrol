/* GStreamer H.265 parser types
 * Copyright (C) 2013 Intel Corporation
 * Copyright (C) 2013 Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *
 *  Contact: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
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

#ifndef __GST_RTP_H265_TYPES_H__
#define __GST_RTP_H265_TYPES_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  GST_H265_NAL_SLICE_TRAIL_N    = 0,
  GST_H265_NAL_SLICE_TRAIL_R    = 1,
  GST_H265_NAL_SLICE_TSA_N      = 2,
  GST_H265_NAL_SLICE_TSA_R      = 3,
  GST_H265_NAL_SLICE_STSA_N     = 4,
  GST_H265_NAL_SLICE_STSA_R     = 5,
  GST_H265_NAL_SLICE_RADL_N     = 6,
  GST_H265_NAL_SLICE_RADL_R     = 7,
  GST_H265_NAL_SLICE_RASL_N     = 8,
  GST_H265_NAL_SLICE_RASL_R     = 9,
  GST_H265_NAL_SLICE_BLA_W_LP   = 16,
  GST_H265_NAL_SLICE_BLA_W_RADL = 17,
  GST_H265_NAL_SLICE_BLA_N_LP   = 18,
  GST_H265_NAL_SLICE_IDR_W_RADL = 19,
  GST_H265_NAL_SLICE_IDR_N_LP   = 20,
  GST_H265_NAL_SLICE_CRA_NUT    = 21,
  GST_H265_NAL_VPS              = 32,
  GST_H265_NAL_SPS              = 33,
  GST_H265_NAL_PPS              = 34,
  GST_H265_NAL_AUD              = 35,
  GST_H265_NAL_EOS              = 36,
  GST_H265_NAL_EOB              = 37,
  GST_H265_NAL_FD               = 38,
  GST_H265_NAL_PREFIX_SEI       = 39,
  GST_H265_NAL_SUFFIX_SEI       = 40
} GstH265NalUnitType;

#define RESERVED_NON_IRAP_SUBLAYER_NAL_TYPE_MIN 10
#define RESERVED_NON_IRAP_SUBLAYER_NAL_TYPE_MAX 15

#define RESERVED_IRAP_NAL_TYPE_MIN 22
#define RESERVED_IRAP_NAL_TYPE_MAX 23

#define RESERVED_NON_IRAP_NAL_TYPE_MIN 24
#define RESERVED_NON_IRAP_NAL_TYPE_MAX 31

#define RESERVED_NON_VCL_NAL_TYPE_MIN 41
#define RESERVED_NON_VCL_NAL_TYPE_MAX 47

#define UNSPECIFIED_NON_VCL_NAL_TYPE_MIN 48
#define UNSPECIFIED_NON_VCL_NAL_TYPE_MAX 63

G_END_DECLS

#endif
