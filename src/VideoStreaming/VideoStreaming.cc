/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Initialization
 *   @author Gus Grubba <gus@auterion.com>
 */

#include <QtQml>
#include <QDebug>

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#if defined(__android__)
#include <android/log.h>

static void gst_android_log(GstDebugCategory * category,
                            GstDebugLevel      level,
                            const gchar      * file,
                            const gchar      * function,
                            gint               line,
                            GObject          * object,
                            GstDebugMessage  * message,
                            gpointer           data)
{
    if (level <= gst_debug_category_get_threshold (category)) {
        __android_log_print(ANDROID_LOG_ERROR, "GST", "%s, %s: %s", file, function, gst_debug_message_get(message));
    }
}
#elif defined(__ios__)
#include "gst_ios_init.h"
#endif
#else
#include "GLVideoItemStub.h"
#endif

#include "VideoStreaming.h"

#if defined(QGC_GST_STREAMING)
    G_BEGIN_DECLS
    // The static plugins we use
#if defined(__android__) || defined(__ios__)
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
    GST_PLUGIN_STATIC_DECLARE(opengl);
#if defined(__android__)
    GST_PLUGIN_STATIC_DECLARE(androidmedia);
#elif defined(__ios__)
    GST_PLUGIN_STATIC_DECLARE(applemedia);
#endif
#endif
    GST_PLUGIN_STATIC_DECLARE(qmlgl);
    G_END_DECLS
#endif

#if defined(QGC_GST_STREAMING)
#if (defined(Q_OS_MAC) && defined(QGC_INSTALL_RELEASE)) || defined(Q_OS_WIN)
static void qgcputenv(const QString& key, const QString& root, const QString& path)
{
    QString value = root + path;
    qputenv(key.toStdString().c_str(), QByteArray(value.toStdString().c_str()));
}
#endif
#endif

void initializeVideoStreaming(int &argc, char* argv[], char* logpath, char* debuglevel)
{
#if defined(QGC_GST_STREAMING)
    #ifdef Q_OS_MAC
        #ifdef QGC_INSTALL_RELEASE
            QString currentDir = QCoreApplication::applicationDirPath();
            qgcputenv("GST_PLUGIN_SCANNER",           currentDir, "/../Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0/gst-plugin-scanner");
            qgcputenv("GTK_PATH",                     currentDir, "/../Frameworks/GStreamer.framework/Versions/Current");
            qgcputenv("GIO_EXTRA_MODULES",            currentDir, "/../Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules");
            qgcputenv("GST_PLUGIN_SYSTEM_PATH_1_0",   currentDir, "/../Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0");
            qgcputenv("GST_PLUGIN_SYSTEM_PATH",       currentDir, "/../Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0");
            qgcputenv("GST_PLUGIN_PATH_1_0",          currentDir, "/../Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0");
            qgcputenv("GST_PLUGIN_PATH",              currentDir, "/../Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0");
        #endif
    #elif defined(Q_OS_WIN)
        QString currentDir = QCoreApplication::applicationDirPath();
        qgcputenv("GST_PLUGIN_PATH", currentDir, "/gstreamer-plugins");
    #endif

    //-- Generic initialization
    if (qgetenv("GST_DEBUG").isEmpty() && logpath) {
        QString gstDebugFile = QString("%1/%2").arg(logpath).arg("gstreamer-log.txt");
        qDebug() << "GStreamer debug output:" << gstDebugFile;
        if (debuglevel) {
            qputenv("GST_DEBUG", debuglevel);
        }
        qputenv("GST_DEBUG_NO_COLOR", "1");
        qputenv("GST_DEBUG_FILE", gstDebugFile.toUtf8());
        qputenv("GST_DEBUG_DUMP_DOT_DIR", logpath);
    }

    // Initialize GStreamer
#if defined(__android__)
    gst_debug_add_log_function(gst_android_log, nullptr, nullptr);
#elif defined(__ios__)
    //-- iOS specific initialization
    gst_ios_pre_init();
#endif

    GError* error = nullptr;
    if (!gst_init_check(&argc, &argv, &error)) {
        qCritical() << "gst_init_check() failed: " << error->message;
        g_error_free(error);
    }

    // The static plugins we use
#if defined(__android__) || defined(__ios__)
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(libav);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtsp);
    GST_PLUGIN_STATIC_REGISTER(udp);
    GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
    GST_PLUGIN_STATIC_REGISTER(x264);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
    GST_PLUGIN_STATIC_REGISTER(isomp4);
    GST_PLUGIN_STATIC_REGISTER(matroska);
    GST_PLUGIN_STATIC_REGISTER(opengl);

#if defined(__android__)
    GST_PLUGIN_STATIC_REGISTER(androidmedia);
#elif defined(__ios__)
    GST_PLUGIN_STATIC_REGISTER(applemedia);
#endif
#endif

#if defined(__ios__)
    gst_ios_post_init();
#endif

    /* the plugin must be loaded before loading the qml file to register the
     * GstGLVideoItem qml item
     * FIXME Add a QQmlExtensionPlugin into qmlglsink to register GstGLVideoItem
     * with the QML engine, then remove this */
    GstElement *sink = gst_element_factory_make("qmlglsink", nullptr);

    if (sink == nullptr) {
        GST_PLUGIN_STATIC_REGISTER(qmlgl);
        sink = gst_element_factory_make("qmlglsink", nullptr);
    }

    if (sink != nullptr) {
        gst_object_unref(sink);
        sink = nullptr;
    } else {
        qCritical() << "unable to find qmlglsink - you need to build it yourself and add to GST_PLUGIN_PATH";
    }
#else
    qmlRegisterType<GLVideoItemStub>("org.freedesktop.gstreamer.GLVideoItem", 1, 0, "GstGLVideoItem");
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    Q_UNUSED(logpath)
    Q_UNUSED(debuglevel)
#endif
}
