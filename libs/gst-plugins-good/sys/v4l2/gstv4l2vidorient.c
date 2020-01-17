/* GStreamer
 *
 * Copyright (C) 2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2vidorient.c: video orientation interface implementation for V4L2
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

#include <gst/gst.h>

#include "gstv4l2object.h"
#include "gstv4l2vidorient.h"
#include "gstv4l2object.h"

GST_DEBUG_CATEGORY_STATIC (v4l2vo_debug);
#define GST_CAT_DEFAULT v4l2vo_debug

void
gst_v4l2_video_orientation_interface_init (GstVideoOrientationInterface * iface)
{
  GST_DEBUG_CATEGORY_INIT (v4l2vo_debug, "v4l2vo", 0,
      "V4L2 VideoOrientation interface debugging");
}


gboolean
gst_v4l2_video_orientation_get_hflip (GstV4l2Object * v4l2object,
    gboolean * flip)
{

  return gst_v4l2_get_attribute (v4l2object, V4L2_CID_HFLIP, flip);
}

gboolean
gst_v4l2_video_orientation_get_vflip (GstV4l2Object * v4l2object,
    gboolean * flip)
{
  return gst_v4l2_get_attribute (v4l2object, V4L2_CID_VFLIP, flip);
}

/* named hcenter because of historical v4l2 naming */
gboolean
gst_v4l2_video_orientation_get_hcenter (GstV4l2Object * v4l2object,
    gint * center)
{
  return gst_v4l2_get_attribute (v4l2object, V4L2_CID_PAN_RESET, center);
}

/* named vcenter because of historical v4l2 naming */
gboolean
gst_v4l2_video_orientation_get_vcenter (GstV4l2Object * v4l2object,
    gint * center)
{
  return gst_v4l2_get_attribute (v4l2object, V4L2_CID_TILT_RESET, center);
}

gboolean
gst_v4l2_video_orientation_set_hflip (GstV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_HFLIP, flip);
}

gboolean
gst_v4l2_video_orientation_set_vflip (GstV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_VFLIP, flip);
}

gboolean
gst_v4l2_video_orientation_set_hcenter (GstV4l2Object * v4l2object, gint center)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_PAN_RESET, center);
}

gboolean
gst_v4l2_video_orientation_set_vcenter (GstV4l2Object * v4l2object, gint center)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_TILT_RESET, center);
}
