/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GStreamerHelpers.h"

#include <gst/rtsp/gstrtspurl.h>

namespace GStreamer
{

gboolean
is_valid_rtsp_uri(const gchar *uri_str)
{
    GstRTSPUrl *url = NULL;
    GstRTSPResult res;

    if (!gst_uri_is_valid(uri_str)) {
        return FALSE;
    }

    res = gst_rtsp_url_parse(uri_str, &url);
    if ((res != GST_RTSP_OK) || (url == NULL)) {
        return FALSE;
    }

    gst_rtsp_url_free(url);
    return TRUE;
}

} // namespace GStreamer
