/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_SHOUT2SEND_H__
#define __GST_SHOUT2SEND_H__

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <shout/shout.h>

G_BEGIN_DECLS

  /* Protocol type enum */
typedef enum {
  SHOUT2SEND_PROTOCOL_XAUDIOCAST = 1,
  SHOUT2SEND_PROTOCOL_ICY,
  SHOUT2SEND_PROTOCOL_HTTP
} GstShout2SendProtocol;


/* Definition of structure storing data for this element. */
typedef struct _GstShout2send GstShout2send;
struct _GstShout2send {
  GstBaseSink parent;

  GstShout2SendProtocol protocol;

  GstPoll *timer;

  shout_t *conn;

  guint64 prev_queuelen;
  guint64 data_sent;
  GstClockTime datasent_reset_ts;
  gboolean stalled;
  GstClockTime stalled_ts;

  gchar *ip;
  guint port;
  gchar *password;
  gchar *username;
  gchar *streamname;
  gchar *description;
  gchar *genre;
  gchar *mount;
  gchar *url;
  gboolean connected;
  gboolean ispublic;
  gchar *songmetadata;
  gchar *songartist;
  gchar *songtitle;
  gint  format;
  guint timeout;

  GstTagList* tags;
};



/* Standard definition defining a class for this element. */
typedef struct _GstShout2sendClass GstShout2sendClass;
struct _GstShout2sendClass {
  GstBaseSinkClass parent_class;

  /* signal callbacks */
  void (*connection_problem) (GstElement *element,guint errno);
};

/* Standard macros for defining types for this element.  */
#define GST_TYPE_SHOUT2SEND \
  (gst_shout2send_get_type())
#define GST_SHOUT2SEND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SHOUT2SEND,GstShout2send))
#define GST_SHOUT2SEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SHOUT2SEND,GstShout2sendClass))
#define GST_IS_SHOUT2SEND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SHOUT2SEND))
#define GST_IS_SHOUT2SEND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SHOUT2SEND))

/* Standard function returning type information. */
GType gst_shout2send_get_type(void);


G_END_DECLS

#endif /* __GST_SHOUT2SEND_H__ */
