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

#include "gstrtspext.h"

GST_DEBUG_CATEGORY_STATIC (rtspext_debug);
#define GST_CAT_DEFAULT (rtspext_debug)

static GList *extensions;

static gboolean
gst_rtsp_ext_list_filter (GstPluginFeature * feature, gpointer user_data)
{
  GstElementFactory *factory;
  guint rank;

  /* we only care about element factories */
  if (!GST_IS_ELEMENT_FACTORY (feature))
    return FALSE;

  factory = GST_ELEMENT_FACTORY (feature);

  if (!gst_element_factory_has_interface (factory, "GstRTSPExtension"))
    return FALSE;

  /* only select elements with autoplugging rank */
  rank = gst_plugin_feature_get_rank (feature);
  if (rank < GST_RANK_MARGINAL)
    return FALSE;

  return TRUE;
}

void
gst_rtsp_ext_list_init (void)
{
  GST_DEBUG_CATEGORY_INIT (rtspext_debug, "rtspext", 0, "RTSP extension");

  /* get a list of all extensions */
  extensions = gst_registry_feature_filter (gst_registry_get (),
      (GstPluginFeatureFilter) gst_rtsp_ext_list_filter, FALSE, NULL);
}

GstRTSPExtensionList *
gst_rtsp_ext_list_get (void)
{
  GstRTSPExtensionList *result;
  GList *walk;

  result = g_new0 (GstRTSPExtensionList, 1);

  for (walk = extensions; walk; walk = g_list_next (walk)) {
    GstElementFactory *factory = GST_ELEMENT_FACTORY (walk->data);
    GstElement *element;

    element = gst_element_factory_create (factory, NULL);
    if (!element) {
      GST_ERROR ("could not create extension instance");
      continue;
    }

    GST_DEBUG ("added extension interface for '%s'",
        GST_ELEMENT_NAME (element));
    result->extensions = g_list_prepend (result->extensions, element);
  }
  return result;
}

void
gst_rtsp_ext_list_free (GstRTSPExtensionList * ext)
{
  GList *walk;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    gst_object_unref (GST_OBJECT_CAST (elem));
  }
  g_list_free (ext->extensions);
  g_free (ext);
}

gboolean
gst_rtsp_ext_list_detect_server (GstRTSPExtensionList * ext,
    GstRTSPMessage * resp)
{
  GList *walk;
  gboolean res = TRUE;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_detect_server (elem, resp);
  }
  return res;
}

GstRTSPResult
gst_rtsp_ext_list_before_send (GstRTSPExtensionList * ext, GstRTSPMessage * req)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_OK;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_before_send (elem, req);
  }
  return res;
}

GstRTSPResult
gst_rtsp_ext_list_after_send (GstRTSPExtensionList * ext, GstRTSPMessage * req,
    GstRTSPMessage * resp)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_OK;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_after_send (elem, req, resp);
  }
  return res;
}

GstRTSPResult
gst_rtsp_ext_list_parse_sdp (GstRTSPExtensionList * ext, GstSDPMessage * sdp,
    GstStructure * s)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_OK;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_parse_sdp (elem, sdp, s);
  }
  return res;
}

GstRTSPResult
gst_rtsp_ext_list_setup_media (GstRTSPExtensionList * ext, GstSDPMedia * media)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_OK;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_setup_media (elem, media);
  }
  return res;
}

gboolean
gst_rtsp_ext_list_configure_stream (GstRTSPExtensionList * ext, GstCaps * caps)
{
  GList *walk;
  gboolean res = TRUE;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_configure_stream (elem, caps);
    if (!res)
      break;
  }
  return res;
}

GstRTSPResult
gst_rtsp_ext_list_get_transports (GstRTSPExtensionList * ext,
    GstRTSPLowerTrans protocols, gchar ** transport)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_OK;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_get_transports (elem, protocols, transport);
  }
  return res;
}

GstRTSPResult
gst_rtsp_ext_list_stream_select (GstRTSPExtensionList * ext, GstRTSPUrl * url)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_OK;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_stream_select (elem, url);
  }
  return res;
}

void
gst_rtsp_ext_list_connect (GstRTSPExtensionList * ext,
    const gchar * detailed_signal, GCallback c_handler, gpointer data)
{
  GList *walk;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    g_signal_connect (elem, detailed_signal, c_handler, data);
  }
}

GstRTSPResult
gst_rtsp_ext_list_receive_request (GstRTSPExtensionList * ext,
    GstRTSPMessage * req)
{
  GList *walk;
  GstRTSPResult res = GST_RTSP_ENOTIMPL;

  for (walk = ext->extensions; walk; walk = g_list_next (walk)) {
    GstRTSPExtension *elem = (GstRTSPExtension *) walk->data;

    res = gst_rtsp_extension_receive_request (elem, req);
    if (res != GST_RTSP_ENOTIMPL)
      break;
  }
  return res;
}
