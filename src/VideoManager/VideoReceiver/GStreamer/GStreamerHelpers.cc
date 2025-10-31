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
#include <QtCore/QString>
#include <QtCore/QStringList>

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

bool is_hardware_decoder_factory(GstElementFactory *factory)
{
    if (!factory) {
        return false;
    }

    const gchar *factoryName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
    if (!factoryName) {
        return false;
    }

    // Exclude Android software decoders (OMXGoogle / C2Android)
    QString name = QString::fromUtf8(factoryName).toLower();
    if (name.startsWith("amcviddec-omxgoogle") || name.startsWith("amcviddec-c2android")) {
        return false;
    }

    const auto containsHardware = [](const gchar *value) {
        return value && (g_strrstr(value, "Hardware") != nullptr || g_strrstr(value, "hardware") != nullptr);
    };

    if (containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS))) {
        return true;
    }

    if (containsHardware(gst_element_factory_get_klass(factory))) {
        return true;
    }

    const QString nameLower = QString::fromUtf8(factoryName).toLower();
    static const QStringList kHardwareTags = {
        QStringLiteral("va"),      // vaapi family
        QStringLiteral("nv"),      // nvidia nvcodec
        QStringLiteral("qsv"),     // intel quick sync
        QStringLiteral("msdk"),    // intel media sdk
        QStringLiteral("vulkan"),  // vulkan accelerated
        QStringLiteral("d3d"),     // direct3d
        QStringLiteral("dxva"),    // directx video accel
        QStringLiteral("vtdec"),   // apple video toolbox
        QStringLiteral("metal")    // metal-based decoders
    };

    for (const QString &tag : kHardwareTags) {
        if (nameLower.contains(tag)) {
            return true;
        }
    }

    return false;
}

} // namespace GStreamer
