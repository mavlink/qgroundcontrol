/* VP8
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2010 Entropy Wave Inc
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

/* FIXME: Undef HAVE_CONFIG_H because vpx_codec.h uses it,
 * which causes compilation failures */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#endif

#include <vpx/vpx_codec.h>

#include "gstvp8utils.h"

const char *
gst_vpx_error_name (vpx_codec_err_t status)
{
  switch (status) {
    case VPX_CODEC_OK:
      return "OK";
    case VPX_CODEC_ERROR:
      return "error";
    case VPX_CODEC_MEM_ERROR:
      return "mem error";
    case VPX_CODEC_ABI_MISMATCH:
      return "abi mismatch";
    case VPX_CODEC_INCAPABLE:
      return "incapable";
    case VPX_CODEC_UNSUP_BITSTREAM:
      return "unsupported bitstream";
    case VPX_CODEC_UNSUP_FEATURE:
      return "unsupported feature";
    case VPX_CODEC_CORRUPT_FRAME:
      return "corrupt frame";
    case VPX_CODEC_INVALID_PARAM:
      return "invalid parameter";
    default:
      return "unknown";
  }
}
