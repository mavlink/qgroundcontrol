#include "GstPluginRegistry.h"

#include "GStreamer.h"

#include <QtCore/QByteArray>

#include <gst/gst.h>

// X-macro tables for GStreamer static plugin registration.
// Add a new required plugin by appending X(name) to QGC_GST_REQUIRED_PLUGINS.
// Add a new optional plugin by appending IF_GST_PLUGIN(name, X) to QGC_GST_OPTIONAL_PLUGINS.

// Required plugins for ingest session: source ingest, depayload/parse, MPEG-TS
// muxing, and appsink output. Decode/presentation stays in QtMultimedia.
#define QGC_GST_REQUIRED_PLUGINS(X)                                                            \
    X(app) X(coreelements) X(mpegtsdemux) X(mpegtsmux) X(rtp) X(rtpmanager) X(rtsp) X(sdpelem) X(tcp) \
        X(typefindfunctions) X(udp) X(videoparsersbad)

// Helper: expands X(name) only when the corresponding CMake detection macro is defined.
#define IF_GST_PLUGIN(name, X) _QGC_GST_OPT(_QGC_HAS_##name, name, X)
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

// Expands X(name) when _QGC_HAS_<name> == 1, nothing otherwise.
#define _QGC_GST_OPT_1(name, X) X(name)
#define _QGC_GST_OPT_0(name, X)
#define _QGC_GST_OPT_SELECT(has, name, X) _QGC_GST_OPT_##has(name, X)
#define _QGC_GST_OPT(has, name, X) _QGC_GST_OPT_SELECT(has, name, X)

// Optional transport plugins — guarded by CMake detection macros.
#define QGC_GST_OPTIONAL_PLUGINS(X) \
    IF_GST_PLUGIN(srt, X)          \
    IF_GST_PLUGIN(rswebrtc, X)

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD

#define _DECLARE_STATIC_PLUGIN(name) GST_PLUGIN_STATIC_DECLARE(name);
QGC_GST_REQUIRED_PLUGINS(_DECLARE_STATIC_PLUGIN)
QGC_GST_OPTIONAL_PLUGINS(_DECLARE_STATIC_PLUGIN)
#undef _DECLARE_STATIC_PLUGIN

#endif  // QGC_GST_STATIC_BUILD

G_END_DECLS

namespace GStreamer {

void registerPlugins()
{
#ifdef QGC_GST_STATIC_BUILD
#define _REGISTER_STATIC_PLUGIN(name) GST_PLUGIN_STATIC_REGISTER(name);
    QGC_GST_REQUIRED_PLUGINS(_REGISTER_STATIC_PLUGIN)
    QGC_GST_OPTIONAL_PLUGINS(_REGISTER_STATIC_PLUGIN)
#undef _REGISTER_STATIC_PLUGIN
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
