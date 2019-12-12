/*
 * Copyright (C) 2014 Collabora Ltd.
 *     Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
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

#ifndef __V4L2_UTILS_H__
#define __V4L2_UTILS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_V4L2_ERROR_INIT { NULL, NULL }
#define GST_V4L2_ERROR(v4l2err,domain,code,msg,dbg) \
{\
  if (v4l2err) { \
    gchar *_msg = _gst_element_error_printf msg; \
    v4l2err->error = g_error_new_literal (GST_##domain##_ERROR, \
        GST_##domain##_ERROR_##code, _msg); \
    g_free (_msg); \
    v4l2err->dbg_message = _gst_element_error_printf dbg; \
    v4l2err->file = __FILE__; \
    v4l2err->func = GST_FUNCTION; \
    v4l2err->line = __LINE__; \
  } \
}

/**
 * GST_V4L2_IS_M2M:
 * @_dcaps: The device capabilities
 *
 * Checks if the device caps represent an M2M device. Note that modern M2M
 * devices uses V4L2_CAP_VIDEO_M2M* flag, but legacy uses to set both CAPTURE
 * and OUTPUT flags instead.
 *
 * Returns: %TRUE if this is a M2M device.
 */
#define GST_V4L2_IS_M2M(_dcaps) \
  (((_dcaps) & (V4L2_CAP_VIDEO_M2M | V4L2_CAP_VIDEO_M2M_MPLANE)) ||\
            (((_dcaps) & \
                    (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) && \
                ((_dcaps) & \
                    (V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_OUTPUT_MPLANE))))


typedef struct _GstV4l2Iterator GstV4l2Iterator;
typedef struct _GstV4l2Error GstV4l2Error;

struct _GstV4l2Iterator
{
    const gchar *device_path;
    const gchar *device_name;
    const gchar *sys_path;
};

struct _GstV4l2Error
{
    GError *error;
    gchar *dbg_message;
    const gchar *file;
    const gchar *func;
    gint line;
};

GstV4l2Iterator *  gst_v4l2_iterator_new (void);
gboolean           gst_v4l2_iterator_next (GstV4l2Iterator *it);
void               gst_v4l2_iterator_free (GstV4l2Iterator *it);

const gchar *      gst_v4l2_iterator_get_device_path (GstV4l2Iterator *it);
const gchar *      gst_v4l2_iterator_get_device_name (GstV4l2Iterator *it);
const gchar *      gst_v4l2_iterator_get_sys_path (GstV4l2Iterator *it);

void               gst_v4l2_clear_error (GstV4l2Error *error);
void               gst_v4l2_error (gpointer element, GstV4l2Error *error);

G_END_DECLS

#endif /* __V4L2_UTILS_H__ */


