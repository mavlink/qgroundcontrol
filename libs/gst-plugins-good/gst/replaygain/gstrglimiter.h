/* GStreamer ReplayGain limiter
 *
 * Copyright (C) 2007 Rene Stadler <mail@renestadler.de>
 *
 * gstrglimiter.h: Element to apply signal compression to raw audio data
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __GST_RG_LIMITER_H__
#define __GST_RG_LIMITER_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#define GST_TYPE_RG_LIMITER \
  (gst_rg_limiter_get_type())
#define GST_RG_LIMITER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RG_LIMITER,GstRgLimiter))
#define GST_RG_LIMITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RG_LIMITER,GstRgLimiterClass))
#define GST_IS_RG_LIMITER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RG_LIMITER))
#define GST_IS_RG_LIMITER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RG_LIMITER))

typedef struct _GstRgLimiter GstRgLimiter;
typedef struct _GstRgLimiterClass GstRgLimiterClass;

/**
 * GstRgLimiter:
 *
 * Opaque data structure.
 */
struct _GstRgLimiter
{
  GstBaseTransform element;

  /*< private >*/

  gboolean enabled;
};

struct _GstRgLimiterClass
{
  GstBaseTransformClass parent_class;
};

GType gst_rg_limiter_get_type (void);

#endif /* __GST_RG_LIMITER_H__ */
