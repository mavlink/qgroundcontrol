/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <gst/gstbin.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_VIDEO_SINK_BIN_D3D11 (gst_qgc_video_sink_bin_d3d11_get_type())
G_DECLARE_FINAL_TYPE (GstQgcVideoSinkBinD3d11, gst_qgc_video_sink_bin_d3d11, GST, QGC_VIDEO_SINK_BIN_D3D11, GstBin)

struct _GstQgcVideoSinkBinD3d11
{
    GstBin parent;
    GstElement *d3d11upload;
    GstElement *d3d11colorconvert;
    GstElement *qml6d3d11sink;
};

G_END_DECLS
