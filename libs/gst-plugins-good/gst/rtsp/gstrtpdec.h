/* GStreamer
 * Copyright (C) <2005,2006> Wim Taymans <wim@fluendo.com>
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
/*
 * Unless otherwise indicated, Source Code is licensed under MIT license.
 * See further explanation attached in License Statement (distributed in the file
 * LICENSE).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __GST_RTP_DEC_H__
#define __GST_RTP_DEC_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_RTP_DEC  		(gst_rtp_dec_get_type())
#define GST_IS_RTP_DEC(obj)  		(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RTP_DEC))
#define GST_IS_RTP_DEC_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RTP_DEC))
#define GST_RTP_DEC(obj)  		(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RTP_DEC, GstRTPDec))
#define GST_RTP_DEC_CLASS(klass)  	(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RTP_DEC, GstRTPDecClass))

typedef struct _GstRTPDec GstRTPDec;
typedef struct _GstRTPDecClass GstRTPDecClass;
typedef struct _GstRTPDecSession GstRTPDecSession;

struct _GstRTPDec {
  GstElement  element;

  guint       latency;
  GSList     *sessions;
  GstClock   *provided_clock;
};

struct _GstRTPDecClass {
  GstElementClass parent_class;

  /* get the caps for pt */
  GstCaps* (*request_pt_map)  (GstRTPDec *rtpdec, guint session, guint pt);

  void     (*clear_pt_map)    (GstRTPDec *rtpdec);

  void     (*on_new_ssrc)       (GstRTPDec *rtpdec, guint session, guint32 ssrc);
  void     (*on_ssrc_collision) (GstRTPDec *rtpdec, guint session, guint32 ssrc);
  void     (*on_ssrc_validated) (GstRTPDec *rtpdec, guint session, guint32 ssrc);
  void     (*on_bye_ssrc)       (GstRTPDec *rtpdec, guint session, guint32 ssrc);
  void     (*on_bye_timeout)    (GstRTPDec *rtpdec, guint session, guint32 ssrc);
  void     (*on_timeout)        (GstRTPDec *rtpdec, guint session, guint32 ssrc);
};

GType gst_rtp_dec_get_type(void);

G_END_DECLS

#endif /* __GST_RTP_DEC_H__ */
