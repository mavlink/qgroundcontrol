/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim@fluendo.com>
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

#ifndef __GST_RTSP_EXT_H__
#define __GST_RTSP_EXT_H__

#include <gst/gst.h>
#include <gst/rtsp/gstrtspextension.h>

G_BEGIN_DECLS

typedef struct _GstRTSPExtensionList GstRTSPExtensionList;

struct _GstRTSPExtensionList
{
  GList *extensions;
};

void                    gst_rtsp_ext_list_init    (void);

GstRTSPExtensionList *  gst_rtsp_ext_list_get     (void);
void                    gst_rtsp_ext_list_free    (GstRTSPExtensionList *ext);

gboolean      gst_rtsp_ext_list_detect_server     (GstRTSPExtensionList *ext, GstRTSPMessage *resp);
  
GstRTSPResult gst_rtsp_ext_list_before_send       (GstRTSPExtensionList *ext, GstRTSPMessage *req);
GstRTSPResult gst_rtsp_ext_list_after_send        (GstRTSPExtensionList *ext, GstRTSPMessage *req,
                                                   GstRTSPMessage *resp);
GstRTSPResult gst_rtsp_ext_list_parse_sdp         (GstRTSPExtensionList *ext, GstSDPMessage *sdp,
                                                   GstStructure *s);
GstRTSPResult gst_rtsp_ext_list_setup_media       (GstRTSPExtensionList *ext, GstSDPMedia *media);
gboolean      gst_rtsp_ext_list_configure_stream  (GstRTSPExtensionList *ext, GstCaps *caps);
GstRTSPResult gst_rtsp_ext_list_get_transports    (GstRTSPExtensionList *ext, GstRTSPLowerTrans protocols,
                                                   gchar **transport);
GstRTSPResult gst_rtsp_ext_list_stream_select     (GstRTSPExtensionList *ext, GstRTSPUrl *url);

void          gst_rtsp_ext_list_connect           (GstRTSPExtensionList *ext,
			                           const gchar *detailed_signal, GCallback c_handler,
                                                   gpointer data);
GstRTSPResult gst_rtsp_ext_list_receive_request   (GstRTSPExtensionList *ext, GstRTSPMessage *req);

G_END_DECLS

#endif /* __GST_RTSP_EXT_H__ */
