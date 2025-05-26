/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GStreamer.h"
#include "AppSettings.h"
#include "GstVideoReceiver.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtQuick/QQuickItem>

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerLog, "qgc.videomanager.videoreceiver.gstreamer")
QGC_LOGGING_CATEGORY(GStreamerAPILog, "qgc.videomanager.videoreceiver.gstreamer.api")

// TODO: Clean These up with Macros or CMake
G_BEGIN_DECLS
GST_PLUGIN_STATIC_DECLARE(androidmedia);
GST_PLUGIN_STATIC_DECLARE(applemedia);
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(d3d);
GST_PLUGIN_STATIC_DECLARE(d3d11);
GST_PLUGIN_STATIC_DECLARE(d3d12);
GST_PLUGIN_STATIC_DECLARE(dav1d);
GST_PLUGIN_STATIC_DECLARE(dxva);
GST_PLUGIN_STATIC_DECLARE(isomp4);
GST_PLUGIN_STATIC_DECLARE(libav);
GST_PLUGIN_STATIC_DECLARE(matroska);
GST_PLUGIN_STATIC_DECLARE(mpegtsdemux);
GST_PLUGIN_STATIC_DECLARE(msdk);
GST_PLUGIN_STATIC_DECLARE(nvcodec);
GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(openh264);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(qml6);
GST_PLUGIN_STATIC_DECLARE(qsv);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(rtsp);
GST_PLUGIN_STATIC_DECLARE(sdpelem);
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
GST_PLUGIN_STATIC_DECLARE(udp);
GST_PLUGIN_STATIC_DECLARE(va);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(vpx);
GST_PLUGIN_STATIC_DECLARE(vulkan);

GST_PLUGIN_STATIC_DECLARE(qgc);
G_END_DECLS

namespace
{

void _registerPlugins()
{
#ifdef QGC_GST_STATIC_BUILD
    #ifdef GST_PLUGIN_androidmedia_FOUND
        GST_PLUGIN_STATIC_REGISTER(androidmedia);
    #endif
    #ifdef GST_PLUGIN_applemedia_FOUND
        GST_PLUGIN_STATIC_REGISTER(applemedia);
    #endif
        GST_PLUGIN_STATIC_REGISTER(coreelements);
    #ifdef GST_PLUGIN_d3d_FOUND
        GST_PLUGIN_STATIC_REGISTER(d3d);
    #endif
    #ifdef GST_PLUGIN_d3d11_FOUND
        GST_PLUGIN_STATIC_REGISTER(d3d11);
    #endif
    #ifdef GST_PLUGIN_d3d12_FOUND
        GST_PLUGIN_STATIC_REGISTER(d3d12);
    #endif
    #ifdef GST_PLUGIN_dav1d_FOUND
        GST_PLUGIN_STATIC_REGISTER(dav1d);
    #endif
    #ifdef GST_PLUGIN_dxva_FOUND
        GST_PLUGIN_STATIC_REGISTER(dxva);
    #endif
        GST_PLUGIN_STATIC_REGISTER(isomp4);
        GST_PLUGIN_STATIC_REGISTER(libav);
        GST_PLUGIN_STATIC_REGISTER(matroska);
        GST_PLUGIN_STATIC_REGISTER(mpegtsdemux);
    #ifdef GST_PLUGIN_msdk_FOUND
        GST_PLUGIN_STATIC_REGISTER(msdk);
    #endif
    #ifdef GST_PLUGIN_nvcodec_FOUND
        GST_PLUGIN_STATIC_REGISTER(nvcodec);
    #endif
        GST_PLUGIN_STATIC_REGISTER(opengl);
        GST_PLUGIN_STATIC_REGISTER(openh264);
        GST_PLUGIN_STATIC_REGISTER(playback);
    #ifdef GST_PLUGIN_qsv_FOUND
        GST_PLUGIN_STATIC_REGISTER(qsv);
    #endif
        GST_PLUGIN_STATIC_REGISTER(rtp);
        GST_PLUGIN_STATIC_REGISTER(rtpmanager);
        GST_PLUGIN_STATIC_REGISTER(rtsp);
        GST_PLUGIN_STATIC_REGISTER(sdpelem);
        GST_PLUGIN_STATIC_REGISTER(tcp);
        GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
        GST_PLUGIN_STATIC_REGISTER(udp);
    #ifdef GST_PLUGIN_va_FOUND
        GST_PLUGIN_STATIC_REGISTER(va);
    #endif
        GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
        GST_PLUGIN_STATIC_REGISTER(vpx);
    #ifdef GST_PLUGIN_vulkan_FOUND
        GST_PLUGIN_STATIC_REGISTER(vulkan);
    #endif
#endif

// #if !defined(GST_PLUGIN_qml6_FOUND) && defined(QGC_GST_STATIC_BUILD)
    GST_PLUGIN_STATIC_REGISTER(qml6);
// #endif

    GST_PLUGIN_STATIC_REGISTER(qgc);
}

void _qtGstLog(GstDebugCategory *category,
               GstDebugLevel level,
               const gchar *file,
               const gchar *function,
               gint line,
               GObject *object,
               GstDebugMessage *message,
               gpointer data)
{
    Q_UNUSED(data);

    if (level > gst_debug_category_get_threshold(category)) {
        return;
    }

    QMessageLogger log(file, line, function);

    char *object_info = gst_info_strdup_printf("%" GST_PTR_FORMAT, static_cast<void*>(object));

    switch (level) {
    case GST_LEVEL_ERROR:
        log.critical(GStreamerAPILog, "%s %s", object_info, gst_debug_message_get(message));
        break;
    case GST_LEVEL_WARNING:
        log.warning(GStreamerAPILog, "%s %s", object_info, gst_debug_message_get(message));
        break;
    case GST_LEVEL_FIXME:
    case GST_LEVEL_INFO:
        log.info(GStreamerAPILog, "%s %s", object_info, gst_debug_message_get(message));
        break;
    case GST_LEVEL_DEBUG:
#ifdef QT_DEBUG
    case GST_LEVEL_LOG:
    case GST_LEVEL_TRACE:
    case GST_LEVEL_MEMDUMP:
#endif
        log.debug(GStreamerAPILog, "%s %s", object_info, gst_debug_message_get(message));
        break;
    default:
        break;
    }

    g_clear_pointer(&object_info, g_free);
}

void _setGstEnvVars()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    qCDebug(GStreamerLog) << "App Directory:" << appDir;

#if defined(Q_OS_MACOS) && defined(QGC_GST_MACOS_FRAMEWORK)
    const QString frameworkDir = QDir(appDir).filePath("../Frameworks/GStreamer.framework");
    const QString rootDir = QDir(frameworkDir).filePath("Versions/1.0");
    const QString libDir = QDir(rootDir).filePath("../lib");
    const QString pluginDir = QDir(libDir).filePath("gstreamer-1.0");
    const QString gioMod = QDir(libDir).filePath("gio/modules");
    const QString libexecDir = QDir(appDir).filePath("../libexec");
    const QString scanner = QDir(libexecDir).filePath("gstreamer-1.0/gst-plugin-scanner");
    const QString ptp = QDir(libexecDir).filePath("gstreamer-1.0/gst-ptp-helper");

    if (QFileInfo::exists(frameworkDir)) {
        qputenv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", "no");
        qputenv("GIO_EXTRA_MODULES", gioMod.toUtf8().constData());
        qputenv("GST_PTP_HELPER_1_0", ptp.toUtf8().constData());
        qputenv("GST_PTP_HELPER", ptp.toUtf8().constData());
        qputenv("GST_PLUGIN_SCANNER_1_0", scanner.toUtf8().constData());
        qputenv("GST_PLUGIN_SCANNER", scanner.toUtf8().constData());
        qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginDir.toUtf8().constData());
        qputenv("GST_PLUGIN_SYSTEM_PATH", pluginDir.toUtf8().constData());
        qputenv("GST_PLUGIN_PATH_1_0", pluginDir.toUtf8().constData());
        qputenv("GST_PLUGIN_PATH", pluginDir.toUtf8().constData());
        qputenv("GTK_PATH", rootDir.toUtf8().constData());
    }
#elif defined(Q_OS_WIN)
    const QString binDir = appDir;
    const QString libDir = QDir(binDir).filePath("../lib");
    const QString pluginDir = QDir(libDir).filePath("gstreamer-1.0");
    const QString gioMod = QDir(libDir).filePath("gio/modules");
    const QString libexecDir = QDir(binDir).filePath("../libexec");
    const QString scanner = QDir(libexecDir).filePath("gstreamer-1.0/gst-plugin-scanner");
    const QString ptp = QDir(libexecDir).filePath("gstreamer-1.0/gst-ptp-helper");

    if (QFileInfo::exists(pluginDir)) {
        qputenv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", "no");
        qputenv("GIO_EXTRA_MODULES", gioMod.toUtf8().constData());
        qputenv("GST_PTP_HELPER_1_0", ptp.toUtf8().constData());
        qputenv("GST_PTP_HELPER", ptp.toUtf8().constData());
        qputenv("GST_PLUGIN_SCANNER_1_0", scanner.toUtf8().constData());
        qputenv("GST_PLUGIN_SCANNER", scanner.toUtf8().constData());
        qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginDir.toUtf8().constData());
        qputenv("GST_PLUGIN_SYSTEM_PATH", pluginDir.toUtf8().constData());
        qputenv("GST_PLUGIN_PATH_1_0", pluginDir.toUtf8().constData());
        qputenv("GST_PLUGIN_PATH", pluginDir.toUtf8().constData());
    }
#endif
}

void _checkPlugin(gpointer data, gpointer user_data)
{
    GstPlugin *plugin = static_cast<GstPlugin*>(data);
    if (!plugin) {
        return;
    }

    const gchar *name = gst_plugin_get_name(plugin);
    const gchar *version = gst_plugin_get_version(plugin);
    qCDebug(GStreamerLog) << QString("Plugin %1: (Version %2)").arg(name, version);
}

bool _verifyPlugins()
{
    bool result = true;

    GstRegistry *registry = gst_registry_get();

    GList *plugins = gst_registry_get_plugin_list(registry);
    g_list_foreach(plugins, _checkPlugin, NULL);
    g_list_free(plugins);

    static constexpr const char *pluginNames[2] = {"qml6", "qgc"};
    for (const char *name : pluginNames) {
        GstPlugin *plugin = gst_registry_find_plugin(registry, name);
        if (!plugin) {
            qCCritical(GStreamerLog) << name << "plugin NOT found.";
            result = false;
            continue;
        }
        gst_clear_object(&plugin);
    }

    if (!result) {
        QString pluginPath;
        if (qEnvironmentVariableIsSet("GST_PLUGIN_PATH_1_0")) {
            pluginPath = qgetenv("GST_PLUGIN_PATH_1_0");
        } else if (qEnvironmentVariableIsSet("GST_PLUGIN_PATH")) {
            pluginPath = qgetenv("GST_PLUGIN_PATH");
        }

#ifdef QGC_GST_STATIC_BUILD
        qCCritical(GStreamerLog) << "Please update the list of static plugins in GStreamer.cc";
#else
        if (!pluginPath.isEmpty()) {
            qCCritical(GStreamerLog) << "Please check in GST_PLUGIN_PATH=" << pluginPath;
        } else {
            qCCritical(GStreamerLog) << "Please set GST_PLUGIN_PATH to the path of your plugin";
        }
#endif
    }

    return result;
}

void _setCodecPriorities(GStreamer::VideoDecoderOptions option)
{
    GstRegistry *registry = gst_registry_get();

    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get gstreamer registry.";
        return;
    }

    const auto changeRank = [registry](const char *featureName, uint16_t rank) {
        GstPluginFeature *feature = gst_registry_lookup_feature(registry, featureName);
        if (!feature) {
            qCDebug(GStreamerLog) << "Failed to change ranking of feature. Feature does not exist:" << featureName;
            return;
        }

        qCDebug(GStreamerLog) << "Changing feature (" << featureName << ") to use rank:" << rank;
        gst_plugin_feature_set_rank(feature, rank);
        (void) gst_registry_add_feature(registry, feature);
        gst_clear_object(&feature);
    };

    // TODO: ForceVideoDecoderCustom in VideoSettings with textbox in QML

    static constexpr uint16_t PrioritizedRank = GST_RANK_PRIMARY + 1;

    switch (option) {
    case GStreamer::ForceVideoDecoderDefault:
        break;
    case GStreamer::ForceVideoDecoderSoftware:
        for (const char *name : {"avdec_h264", "avdec_h265", "avdec_mjpeg", "avdec_mpeg2video", "avdec_mpeg4", "avdec_vp8", "avdec_vp9",
                                 "dav1ddec", "vp8dec", "vp9dec"}) {
            changeRank(name, PrioritizedRank);
        }
        break;
    case GStreamer::ForceVideoDecoderVAAPI:
        for (const char *name : {"vaav1dec", "vah264dec", "vah265dec", "vajpegdec", "vampeg2dec", "vavp8dec", "vavp9dec"}) {
            changeRank(name, PrioritizedRank);
        }
        break;
    case GStreamer::ForceVideoDecoderNVIDIA:
        for (const char *name : {"nvav1dec", "nvh264dec", "nvh265dec", "nvjpegdec", "nvmpeg2videodec", "nvmpeg4videodec", "nvmpegvideodec", "nvvp8dec", "nvvp9dec"}) {
            changeRank(name, PrioritizedRank);
        }
        break;
    case GStreamer::ForceVideoDecoderDirectX3D:
        for (const char *name : {"d3d11av1dec", "d3d11h264dec", "d3d11h265dec", "d3d11mpeg2dec", "d3d11vp8dec", "d3d11vp9dec",
                                 "d3d12av1dec", "d3d12h264dec", "d3d12h265dec", "d3d12mpeg2dec", "d3d12vp8dec", "d3d12vp9dec",
                                 "dxvaav1decoder", "dxvah264decoder", "dxvah265decoder", "dxvampeg2decoder", "dxvavp8decoder", "dxvavp9decoder" }) {
            changeRank(name, PrioritizedRank);
        }
        break;
    case GStreamer::ForceVideoDecoderVideoToolbox:
        for (const char *name : {"vtdec_hw", "vtdec"}) {
            changeRank(name, PrioritizedRank);
        }
        break;
    case GStreamer::ForceVideoDecoderIntel:
        for (const char *name : {"qsvh264dec", "qsvh265dec", "qsvjpegdec", "qsvvp9dec", "msdkav1dec", "msdkh264dec", "msdkh265dec", "msdkmjpegdec", "msdkmpeg2dec", "msdkvc1dec", "msdkvp8dec", "msdkvp9dec"}) {
            changeRank(name, PrioritizedRank);
        }
        break;
    case GStreamer::ForceVideoDecoderVulkan:
        for (const char *name : {"vulkanh264dec", "vulkanh265dec"}) {
            changeRank(name, PrioritizedRank);
        }
        break;
    default:
        qCWarning(GStreamerLog) << "Can't handle decode option:" << option;
        break;
    }
}

} // namespace

namespace GStreamer
{

bool initialize()
{
    _setGstEnvVars();

    if (qEnvironmentVariableIsEmpty("GST_DEBUG")) {
        int gstDebugLevel = 0;
        QSettings settings;
        if (settings.contains(AppSettings::gstDebugLevelName)) {
            gstDebugLevel = settings.value(AppSettings::gstDebugLevelName).toInt();
        }
        gst_debug_set_default_threshold(static_cast<GstDebugLevel>(gstDebugLevel));
        gst_debug_remove_log_function(gst_debug_log_default);
        gst_debug_add_log_function(_qtGstLog, nullptr, nullptr);
    }

    const QStringList args = QCoreApplication::arguments();
    int gstArgc = args.size();

    QByteArrayList argData;
    argData.reserve(gstArgc);

    QVarLengthArray<char*, 16> rawArgv;
    rawArgv.reserve(gstArgc);

    for (const QString &arg : args) {
        argData.append(arg.toUtf8());
        rawArgv.append(argData.last().data());
    }

    char **argvPtr = rawArgv.data();
    GError *error = nullptr;
    const gboolean ok = gst_init_check(&gstArgc, &argvPtr, &error);
    if (!ok) {
        qCritical(GStreamerLog) << "Failed to initialize GStreamer:" << error->message;
        g_clear_error(&error);
        return false;
    }

    const gchar *version = gst_version_string();
    qCDebug(GStreamerLog) << QString("GStreamer Initialized (Version: %1)").arg(version);

    _registerPlugins();

    if (!_verifyPlugins()) {
        qCCritical(GStreamerLog) << "Failed to verify plugins - Check your GStreamer setup";
        return false;
    }

    _setCodecPriorities(static_cast<GStreamer::VideoDecoderOptions>(SettingsManager::instance()->videoSettings()->forceVideoDecoder()->rawValue().toInt()));

    GstElement *sink = gst_element_factory_make("qml6glsink", nullptr);
    if (!sink) {
        qCCritical(GStreamerLog) << "failed to init qml6glsink";
        return false;
    }

    gst_clear_object(&sink);
    return true;
}

void *createVideoSink(QQuickItem *widget, QObject *parent)
{
    GstElement *videoSinkBin = gst_element_factory_make("qgcvideosinkbin", NULL);
    if (videoSinkBin) {
        if (widget) {
            g_object_set(videoSinkBin, "widget", widget, NULL);
        }
    } else {
        qCCritical(GStreamerLog) << "gst_element_factory_make('qgcvideosinkbin') failed";
    }

    return videoSinkBin;
}

void releaseVideoSink(void *sink)
{
    GstElement *videoSink = GST_ELEMENT(sink);
    gst_clear_object(&videoSink);
}

VideoReceiver *createVideoReceiver(QObject *parent)
{
    return new GstVideoReceiver(parent);
}

} // namespace GStreamer
