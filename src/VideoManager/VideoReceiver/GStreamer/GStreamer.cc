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
#ifdef Q_OS_IOS
#include "gst_ios_init.h"
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtQuick/QQuickItem>

#include <gst/gst.h>

QGC_LOGGING_CATEGORY(GStreamerLog, "qgc.videomanager.videoreceiver.gstreamer")
QGC_LOGGING_CATEGORY(GStreamerAPILog, "qgc.videomanager.videoreceiver.gstreamer.api")

G_BEGIN_DECLS
#ifdef QGC_GST_STATIC_BUILD
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(libav);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtsp);
GST_PLUGIN_STATIC_DECLARE(udp);
GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
GST_PLUGIN_STATIC_DECLARE(x264);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
GST_PLUGIN_STATIC_DECLARE(isomp4);
GST_PLUGIN_STATIC_DECLARE(matroska);
GST_PLUGIN_STATIC_DECLARE(mpegtsdemux);
GST_PLUGIN_STATIC_DECLARE(opengl);
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(app);
#ifdef Q_OS_ANDROID
GST_PLUGIN_STATIC_DECLARE(androidmedia);
#elif defined(Q_OS_IOS)
GST_PLUGIN_STATIC_DECLARE(applemedia);
#endif
#endif
#if (defined(QGC_GST_STATIC_BUILD) || !defined(QGC_GST_QML6GL_FOUND))
GST_PLUGIN_STATIC_DECLARE(qml6);
#endif
GST_PLUGIN_STATIC_DECLARE(qgc);
G_END_DECLS

static void _registerPlugins()
{
#ifdef QGC_GST_STATIC_BUILD
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(isomp4);
    GST_PLUGIN_STATIC_REGISTER(libav);
    GST_PLUGIN_STATIC_REGISTER(matroska);
    GST_PLUGIN_STATIC_REGISTER(mpegtsdemux);
    GST_PLUGIN_STATIC_REGISTER(opengl);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(tcp);
    GST_PLUGIN_STATIC_REGISTER(udp);
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    GST_PLUGIN_STATIC_REGISTER(x264);
#ifdef Q_OS_ANDROID
    GST_PLUGIN_STATIC_REGISTER(androidmedia);
#elif defined(Q_OS_IOS)
    GST_PLUGIN_STATIC_REGISTER(applemedia);
#endif
#endif

#if (defined(QGC_GST_STATIC_BUILD) || !defined(QGC_GST_QML6GL_FOUND))
    GST_PLUGIN_STATIC_REGISTER(qml6);
#endif

    GST_PLUGIN_STATIC_REGISTER(qgc);
}

static void qt_gst_log(GstDebugCategory *category,
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
    default:
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
    case GST_LEVEL_LOG:
    case GST_LEVEL_TRACE:
    case GST_LEVEL_MEMDUMP:
        log.debug(GStreamerAPILog, "%s %s", object_info, gst_debug_message_get(message));
        break;
    }

    g_free(object_info);
    object_info = nullptr;
}

static void _qgcputenv(const QString &key, const QString &root, const QString &path = "")
{
    const QByteArray keyArray = key.toLocal8Bit();
    const QByteArray valueArray = (root + path).toLocal8Bit();
    (void) qputenv(keyArray.constData(), valueArray);
}

static void _setGstEnvVars()
{
    const QString currentDir = QCoreApplication::applicationDirPath();
    qCDebug(GStreamerLog) << "App Directory:" << currentDir;

#if defined(Q_OS_MACOS) && defined(QGC_GST_MACOS_FRAMEWORK)
    _qgcputenv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", "no");
    _qgcputenv("GST_PLUGIN_SCANNER", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0/gst-plugin-scanner");
    _qgcputenv("GST_PTP_HELPER_1_0", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0/gst-ptp-helper");
    _qgcputenv("GIO_EXTRA_MODULES", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/lib/gio/modules");
    _qgcputenv("GST_PLUGIN_SYSTEM_PATH_1_0", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0"); // PlugIns/gstreamer
    _qgcputenv("GST_PLUGIN_SYSTEM_PATH", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0");
    _qgcputenv("GST_PLUGIN_PATH_1_0", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0");
    _qgcputenv("GST_PLUGIN_PATH", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0");
    _qgcputenv("GTK_PATH", currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0");
#elif defined(Q_OS_WIN)
    const QString binDir = currentDir;
    const QString libDir = QDir(currentDir).filePath("../lib");
    const QString pluginDir = QDir(libDir).filePath("gstreamer-1.0");
    const QString gioMod = QDir(libDir).filePath("gio/modules");
    const QString scanner = QDir(binDir).filePath("gst-plugin-scanner.exe");

    qputenv("GST_REGISTRY_REUSE_PLUGIN_SCANNER", "no");
    qputenv("GIO_EXTRA_MODULES", gioMod.toUtf8().constData());
    qputenv("GST_PLUGIN_SCANNER_1_0", scanner.toUtf8().constData());
    qputenv("GST_PLUGIN_SCANNER", scanner.toUtf8().constData());
    qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", pluginDir.toUtf8().constData());
    qputenv("GST_PLUGIN_SYSTEM_PATH", pluginDir.toUtf8().constData());
    qputenv("GST_PLUGIN_PATH_1_0", pluginDir.toUtf8().constData());
    qputenv("GST_PLUGIN_PATH", pluginDir.toUtf8().constData());
#endif
}

static void _checkPlugin(gpointer data, gpointer user_data)
{
    GstPlugin *plugin = static_cast<GstPlugin*>(data);
    if (!plugin) {
        return;
    }

    const gchar *name = gst_plugin_get_name(plugin);
    const gchar *version = gst_plugin_get_version(plugin);
    qCDebug(GStreamerLog) << QString("Plugin %1: (Version %2)").arg(name, version);
}

static bool _verifyPlugins()
{
    GstRegistry *registry = gst_registry_get();

    GList *plugins = gst_registry_get_plugin_list(registry);
    g_list_foreach(plugins, _checkPlugin, NULL);
    g_list_free(plugins);

    GstPlugin *plugin = gst_registry_find_plugin(registry, "coreelements");
    if (!plugin) {
        qCCritical(GStreamerLog) << "coreelements plugin NOT found. "
                                 << "Please ensure that the GStreamer installation includes the 'coreelements' plugin. "
                                 << "Check your GStreamer setup or reinstall GStreamer";
        return false;
    }
    gst_object_unref(plugin);

    return true;
}

static void _blacklist(GStreamer::VideoDecoderOptions option)
{
    GstRegistry *registry = gst_registry_get();

    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get gstreamer registry.";
        return;
    }

    const auto changeRank = [registry](const char *featureName, uint16_t rank) {
        GstPluginFeature *const feature = gst_registry_lookup_feature(registry, featureName);
        if (!feature) {
            qCDebug(GStreamerLog) << "Failed to change ranking of feature. Featuer does not exist:" << featureName;
            return;
        }

        qCDebug(GStreamerLog) << "Changing feature (" << featureName << ") to use rank:" << rank;
        gst_plugin_feature_set_rank(feature, rank);
        (void) gst_registry_add_feature(registry, feature);
        gst_object_unref(feature);
    };

    changeRank("bcmdec", GST_RANK_NONE);

    switch (option) {
        case GStreamer::ForceVideoDecoderDefault:
            break;
        case GStreamer::ForceVideoDecoderSoftware:
            for (const char *name : {"avdec_h264", "avdec_h265"}) {
                changeRank(name, GST_RANK_PRIMARY + 1);
            }
            break;
        case GStreamer::ForceVideoDecoderVAAPI:
            for (const char *name : {"vaapimpeg2dec", "vaapimpeg4dec", "vaapih263dec", "vaapih264dec", "vaapih265dec", "vaapivc1dec"}) {
                changeRank(name, GST_RANK_PRIMARY + 1);
            }
            break;
        case GStreamer::ForceVideoDecoderNVIDIA:
            for (const char *name : {"nvh265dec", "nvh265sldec", "nvh264dec", "nvh264sldec"}) {
                changeRank(name, GST_RANK_PRIMARY + 1);
            }
            break;
        case GStreamer::ForceVideoDecoderDirectX3D:
            for (const char *name : {"d3d11vp9dec", "d3d11h265dec", "d3d11h264dec"}) {
                changeRank(name, GST_RANK_PRIMARY + 1);
            }
            break;
        case GStreamer::ForceVideoDecoderVideoToolbox:
            changeRank("vtdec", GST_RANK_PRIMARY + 1);
            break;
        default:
            qCWarning(GStreamerLog) << "Can't handle decode option:" << option;
            break;
    }
}

namespace GStreamer
{

bool initialize()
{
    (void) qRegisterMetaType<VideoReceiver::STATUS>("STATUS");

    _setGstEnvVars();

    if (qEnvironmentVariableIsEmpty("GST_DEBUG")) {
        int gstDebugLevel = 0;
        QSettings settings;
        if (settings.contains(AppSettings::gstDebugLevelName)) {
            gstDebugLevel = settings.value(AppSettings::gstDebugLevelName).toInt();
        }
        gst_debug_set_default_threshold(static_cast<GstDebugLevel>(gstDebugLevel));
        gst_debug_remove_log_function(gst_debug_log_default);
        gst_debug_add_log_function(qt_gst_log, nullptr, nullptr);
    }

#ifdef Q_OS_IOS
    gst_ios_pre_init();
#endif

    const QStringList args = QCoreApplication::arguments();
    int argc = args.size();
    QByteArrayList argList;
    argList.reserve(argc);

    char **argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        argList.append(args[i].toUtf8());
        argv[i] = argList[i].data();
    }

    GError *error = nullptr;
    const gboolean initialized = gst_init_check(&argc, &argv, &error);
    delete[] argv;
    if (!initialized) {
        qCCritical(GStreamerLog) << Q_FUNC_INFO << error->message;
        g_error_free(error);
        return false;
    }

    const gchar *const version = gst_version_string();
    qCDebug(GStreamerLog) << QString("GStreamer Initialized (Version: %1)").arg(version);

    _registerPlugins();

#ifdef Q_OS_IOS
    gst_ios_post_init();
#endif

    if (!_verifyPlugins()) {
        qCCritical(GStreamerLog) << "Failed to Init GStreamer Plugins";
        return false;
    }

    _blacklist(static_cast<GStreamer::VideoDecoderOptions>(SettingsManager::instance()->videoSettings()->forceVideoDecoder()->rawValue().toInt()));
    return true;
}

void *createVideoSink(QObject *parent, QQuickItem *widget)
{
    Q_UNUSED(parent)

    GstElement *const sink = gst_element_factory_make("qgcvideosinkbin", NULL);
    if (sink) {
        g_object_set(sink, "widget", widget, NULL);
    } else {
        qCCritical(GStreamerLog) << "gst_element_factory_make('qgcvideosinkbin') failed";
    }

    return sink;
}

void releaseVideoSink(void *sink)
{
    if (sink) {
        gst_object_unref(GST_ELEMENT(sink));
    }
}

VideoReceiver *createVideoReceiver(QObject *parent)
{
    return new GstVideoReceiver(parent);
}

} // namespace GStreamer
