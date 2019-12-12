/* GStreamer
 *
 * Copyright (C) 2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2vidorient.h: video orientation interface implementation for V4L2
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

#ifndef __GST_V4L2_VIDORIENT_H__
#define __GST_V4L2_VIDORIENT_H__

#include <gst/gst.h>
#include <gst/video/videoorientation.h>

#include "gstv4l2object.h"

G_BEGIN_DECLS

void     gst_v4l2_video_orientation_interface_init (GstVideoOrientationInterface * iface);

gboolean gst_v4l2_video_orientation_get_hflip 	(GstV4l2Object *v4l2object, gboolean *flip);
gboolean gst_v4l2_video_orientation_get_vflip 	(GstV4l2Object *v4l2object, gboolean *flip);
gboolean gst_v4l2_video_orientation_get_hcenter (GstV4l2Object *v4l2object, gint *center);
gboolean gst_v4l2_video_orientation_get_vcenter (GstV4l2Object *v4l2object, gint *center);

gboolean gst_v4l2_video_orientation_set_hflip 	(GstV4l2Object *v4l2object, gboolean flip);
gboolean gst_v4l2_video_orientation_set_vflip 	(GstV4l2Object *v4l2object, gboolean flip);
gboolean gst_v4l2_video_orientation_set_hcenter (GstV4l2Object *v4l2object, gint center);
gboolean gst_v4l2_video_orientation_set_vcenter (GstV4l2Object *v4l2object, gint center);

#define GST_IMPLEMENT_V4L2_VIDORIENT_METHODS(Type, interface_as_function)                         \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_get_hflip (GstVideoOrientation *vo, gboolean *flip)       \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_get_hflip (this->v4l2object, flip);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_get_vflip (GstVideoOrientation *vo, gboolean *flip)       \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_get_vflip (this->v4l2object, flip);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_get_hcenter (GstVideoOrientation *vo, gint *center)       \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_get_hcenter (this->v4l2object, center);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_get_vcenter (GstVideoOrientation *vo, gint *center)       \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_get_vcenter (this->v4l2object, center);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_set_hflip (GstVideoOrientation *vo, gboolean flip)        \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_set_hflip (this->v4l2object, flip);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_set_vflip (GstVideoOrientation *vo, gboolean flip)        \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_set_vflip (this->v4l2object, flip);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_set_hcenter (GstVideoOrientation *vo, gint center)        \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_set_hcenter (this->v4l2object, center);		          \
  }                                                                                               \
                                                                                                  \
  static gboolean                                                                                 \
  interface_as_function ## _video_orientation_set_vcenter (GstVideoOrientation *vo, gint center)        \
  {                                                                                               \
    Type *this = (Type*) vo;                                                                      \
    return gst_v4l2_video_orientation_set_vcenter (this->v4l2object, center);		          \
  }                                                                                               \
                                                                                                  \
  static void                                                                                     \
  interface_as_function ## _video_orientation_interface_init (GstVideoOrientationInterface * iface)          \
  {                                                                                               \
    /* default virtual functions */                                                               \
    iface->get_hflip   = interface_as_function ## _video_orientation_get_hflip;                   \
    iface->get_vflip   = interface_as_function ## _video_orientation_get_vflip;                   \
    iface->get_hcenter = interface_as_function ## _video_orientation_get_hcenter;                 \
    iface->get_vcenter = interface_as_function ## _video_orientation_get_vcenter;                 \
    iface->set_hflip   = interface_as_function ## _video_orientation_set_hflip;                   \
    iface->set_vflip   = interface_as_function ## _video_orientation_set_vflip;                   \
    iface->set_hcenter = interface_as_function ## _video_orientation_set_hcenter;                 \
    iface->set_vcenter = interface_as_function ## _video_orientation_set_vcenter;                 \
  }

G_END_DECLS
#endif /* __GST_V4L2_VIDORIENT_H__ */
