/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __GST_ASTERISK_H263_H__
#define __GST_ASTERISK_H263_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#define GST_TYPE_ASTERISK_H263 \
  (gst_asteriskh263_get_type())
#define GST_ASTERISK_H263(obj) \
 (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ASTERISK_H263,GstAsteriskh263))
#define GST_ASTERISK_H263_CLASS(klass) \
 (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ASTERISK_H263,GstAsteriskh263Class))
#define GST_IS_ASTERISK_H263(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ASTERISK_H263))
#define GST_IS_ASTERISK_H263_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ASTERISK_H263))

typedef struct _GstAsteriskh263 GstAsteriskh263;
typedef struct _GstAsteriskh263Class GstAsteriskh263Class;

struct _GstAsteriskh263
{
  GstElement element;

  GstPad *sinkpad;
  GstPad *srcpad;

  GstAdapter *adapter;

  guint32 lastts;
};

struct _GstAsteriskh263Class
{
  GstElementClass parent_class;
};

GType gst_asteriskh263_get_type (void);

gboolean gst_asteriskh263_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_ASTERISK_H263_H__ */
