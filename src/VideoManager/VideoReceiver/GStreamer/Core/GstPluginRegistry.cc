#include "GstPluginRegistry.h"

#include "GStreamer.h"

#include <QtCore/QByteArray>

#include <gst/gst.h>

// X-macro tables for GStreamer static plugin registration.
// Add a new required plugin by appending X(name) to QGC_GST_REQUIRED_PLUGINS.
// Add a new optional plugin by appending IF_GST_PLUGIN(name, X) to QGC_GST_OPTIONAL_PLUGINS.

// Required plugins — unconditionally present in every static build.
// Note: `opengl` intentionally excluded. The appsink negotiates CPU caps only
// (GstFormatTable::cpuCapsFormats — no memory:GLMemory feature), so no element
// in our pipeline produces GLMemory output. Hardware decoders (nvcodec, va,
// d3d11, applemedia) use their own native memory types with CPU download
// fallbacks. Re-add if a future pipeline needs glupload/glcolorconvert.
#define QGC_GST_REQUIRED_PLUGINS(X)                                                                               \
    X(coreelements)                                                                                               \
    X(isomp4) X(libav) X(matroska) X(mpegtsdemux) X(openh264) X(playback) X(rtp) X(rtpmanager) X(rtsp) X(sdpelem) \
        X(tcp) X(typefindfunctions) X(udp) X(videoparsersbad) X(vpx)

// videoconvert was split into videoconvertscale in GStreamer 1.22.
#if defined(QGC_GST_BUILD_VERSION_MAJOR) && (QGC_GST_BUILD_VERSION_MAJOR > 1 || QGC_GST_BUILD_VERSION_MINOR >= 22)
#define QGC_GST_VIDEOCONVERT_PLUGINS(X) X(videoconvertscale)
#else
#define QGC_GST_VIDEOCONVERT_PLUGINS(X) X(videoconvert) X(videoscale)
#endif

// Helper: expands X(name) only when the corresponding CMake detection macro is defined.
#define IF_GST_PLUGIN(name, X) _QGC_GST_OPT(_QGC_HAS_##name, name, X)
#ifdef GST_PLUGIN_androidmedia_FOUND
#define _QGC_HAS_androidmedia 1
#else
#define _QGC_HAS_androidmedia 0
#endif
#ifdef GST_PLUGIN_applemedia_FOUND
#define _QGC_HAS_applemedia 1
#else
#define _QGC_HAS_applemedia 0
#endif
#ifdef GST_PLUGIN_d3d_FOUND
#define _QGC_HAS_d3d 1
#else
#define _QGC_HAS_d3d 0
#endif
#ifdef GST_PLUGIN_d3d11_FOUND
#define _QGC_HAS_d3d11 1
#else
#define _QGC_HAS_d3d11 0
#endif
#ifdef GST_PLUGIN_d3d12_FOUND
#define _QGC_HAS_d3d12 1
#else
#define _QGC_HAS_d3d12 0
#endif
#ifdef GST_PLUGIN_dav1d_FOUND
#define _QGC_HAS_dav1d 1
#else
#define _QGC_HAS_dav1d 0
#endif
#ifdef GST_PLUGIN_dxva_FOUND
#define _QGC_HAS_dxva 1
#else
#define _QGC_HAS_dxva 0
#endif
#ifdef GST_PLUGIN_nvcodec_FOUND
#define _QGC_HAS_nvcodec 1
#else
#define _QGC_HAS_nvcodec 0
#endif
#ifdef GST_PLUGIN_qsv_FOUND
#define _QGC_HAS_qsv 1
#else
#define _QGC_HAS_qsv 0
#endif
#ifdef GST_PLUGIN_va_FOUND
#define _QGC_HAS_va 1
#else
#define _QGC_HAS_va 0
#endif
#ifdef GST_PLUGIN_vulkan_FOUND
#define _QGC_HAS_vulkan 1
#else
#define _QGC_HAS_vulkan 0
#endif
#ifdef GST_PLUGIN_srt_FOUND
#define _QGC_HAS_srt 1
#else
#define _QGC_HAS_srt 0
#endif
#ifdef GST_PLUGIN_rswebrtc_FOUND
#define _QGC_HAS_rswebrtc 1
#else
#define _QGC_HAS_rswebrtc 0
#endif
#ifdef GST_PLUGIN_hls_FOUND
#define _QGC_HAS_hls 1
#else
#define _QGC_HAS_hls 0
#endif
#ifdef GST_PLUGIN_dash_FOUND
#define _QGC_HAS_dash 1
#else
#define _QGC_HAS_dash 0
#endif
#ifdef GST_PLUGIN_adaptivedemux2_FOUND
#define _QGC_HAS_adaptivedemux2 1
#else
#define _QGC_HAS_adaptivedemux2 0
#endif

// Expands X(name) when _QGC_HAS_<name> == 1, nothing otherwise.
#define _QGC_GST_OPT_1(name, X) X(name)
#define _QGC_GST_OPT_0(name, X)
#define _QGC_GST_OPT(has, name, X) _QGC_GST_OPT_##has(name, X)

// Optional hardware/platform plugins — guarded by CMake detection macros.
#define QGC_GST_OPTIONAL_PLUGINS(X) \
    IF_GST_PLUGIN(androidmedia, X)  \
    IF_GST_PLUGIN(applemedia, X)    \
    IF_GST_PLUGIN(d3d, X)           \
    IF_GST_PLUGIN(d3d11, X)         \
    IF_GST_PLUGIN(d3d12, X)         \
    IF_GST_PLUGIN(dav1d, X)         \
    IF_GST_PLUGIN(dxva, X)          \
    IF_GST_PLUGIN(nvcodec, X)       \
    IF_GST_PLUGIN(qsv, X)           \
    IF_GST_PLUGIN(va, X)            \
    IF_GST_PLUGIN(vulkan, X)        \
    IF_GST_PLUGIN(srt, X)           \
    IF_GST_PLUGIN(rswebrtc, X)      \
    IF_GST_PLUGIN(hls, X)           \
    IF_GST_PLUGIN(dash, X)          \
    IF_GST_PLUGIN(adaptivedemux2, X)

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD

QGC_GST_REQUIRED_PLUGINS(GST_PLUGIN_STATIC_DECLARE)
QGC_GST_VIDEOCONVERT_PLUGINS(GST_PLUGIN_STATIC_DECLARE)
QGC_GST_OPTIONAL_PLUGINS(GST_PLUGIN_STATIC_DECLARE)

#endif  // QGC_GST_STATIC_BUILD

G_END_DECLS

namespace GStreamer {

void registerPlugins()
{
#ifdef QGC_GST_STATIC_BUILD
    QGC_GST_REQUIRED_PLUGINS(GST_PLUGIN_STATIC_REGISTER)
    QGC_GST_VIDEOCONVERT_PLUGINS(GST_PLUGIN_STATIC_REGISTER)
    QGC_GST_OPTIONAL_PLUGINS(GST_PLUGIN_STATIC_REGISTER)
#endif
}

bool verifyPlugins()
{
    GstRegistry* registry = gst_registry_get();
    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get GStreamer registry";
        return false;
    }

    GList* plugins = gst_registry_get_plugin_list(registry);
    if (plugins) {
        qCDebug(GStreamerLog) << "Installed GStreamer plugins:";
        for (GList* node = plugins; node != nullptr; node = node->next) {
            GstPlugin* plugin = static_cast<GstPlugin*>(node->data);
            if (plugin) {
                qCDebug(GStreamerLog) << "  " << gst_plugin_get_name(plugin) << gst_plugin_get_version(plugin);
            }
        }
        gst_plugin_list_free(plugins);
    }

    bool result = true;

    // Verify all required plugins — not just "coreelements".
    // Missing libav, rtsp, etc. would crash at runtime without this.
#define _VERIFY_PLUGIN(name) #name,
    static constexpr const char* requiredPlugins[] = {QGC_GST_REQUIRED_PLUGINS(_VERIFY_PLUGIN)};
#undef _VERIFY_PLUGIN

    for (const char* name : requiredPlugins) {
        GstPlugin* plugin = gst_registry_find_plugin(registry, name);
        if (!plugin) {
            qCCritical(GStreamerLog) << "Required QGC plugin not found:" << name;
            result = false;
            continue;
        }
        gst_clear_object(&plugin);
    }

    if (!result) {
        const QByteArray pluginPath = qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0") ? qgetenv("GST_PLUGIN_PATH_1_0")
                                                                                       : qgetenv("GST_PLUGIN_PATH");

        if (!pluginPath.isEmpty()) {
            qCCritical(GStreamerLog) << "Check GST_PLUGIN_PATH=" << pluginPath;
        } else {
            qCCritical(GStreamerLog) << "GST_PLUGIN_PATH is not set";
        }

        GList* allPlugins = gst_registry_get_plugin_list(registry);
        for (GList* node = allPlugins; node != nullptr; node = node->next) {
            GstPlugin* p = static_cast<GstPlugin*>(node->data);
            if (!p)
                continue;
            const gchar* desc = gst_plugin_get_description(p);
            const gchar* filename = gst_plugin_get_filename(p);
            if (desc && g_str_has_prefix(desc, "BLACKLIST")) {
                qCWarning(GStreamerLog) << "Blacklisted plugin:" << gst_plugin_get_name(p)
                                        << "file:" << (filename ? filename : "(null)");
            }
        }
        gst_plugin_list_free(allPlugins);

        static constexpr const char* envDiagnostics[] = {
            "GST_PLUGIN_PATH",
            "GST_PLUGIN_PATH_1_0",
            "GST_PLUGIN_SYSTEM_PATH",
            "GST_PLUGIN_SYSTEM_PATH_1_0",
            "GST_PLUGIN_SCANNER",
            "GST_PLUGIN_SCANNER_1_0",
            "GST_REGISTRY_REUSE_PLUGIN_SCANNER",
        };
        qCCritical(GStreamerLog) << "GStreamer environment diagnostics:";
        for (const char* var : envDiagnostics) {
            const QByteArray val = qgetenv(var);
            qCCritical(GStreamerLog) << "  " << var << "=" << (val.isEmpty() ? "(unset)" : val.constData());
        }
    }

    return result;
}

}  // namespace GStreamer
