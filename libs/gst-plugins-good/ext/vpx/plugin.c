/* VP8
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2010 Entropy Wave Inc
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

#include "gstvp8dec.h"
#include "gstvp8enc.h"
#include "gstvp9dec.h"
#include "gstvp9enc.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
#ifdef HAVE_VP8_DECODER
  gst_element_register (plugin, "vp8dec", GST_RANK_PRIMARY,
      gst_vp8_dec_get_type ());
#endif

#ifdef HAVE_VP8_ENCODER
  gst_element_register (plugin, "vp8enc", GST_RANK_PRIMARY,
      gst_vp8_enc_get_type ());
#endif

#ifdef HAVE_VP9_DECODER
  gst_element_register (plugin, "vp9dec", GST_RANK_PRIMARY,
      gst_vp9_dec_get_type ());
#endif

#ifdef HAVE_VP9_ENCODER
  gst_element_register (plugin, "vp9enc", GST_RANK_PRIMARY,
      gst_vp9_enc_get_type ());
#endif

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    vpx,
    "VP8 plugin",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
