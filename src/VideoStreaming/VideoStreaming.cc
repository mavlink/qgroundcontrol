    /****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Streaming Initialization
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QtQml>
#include <QDebug>

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

#include "VideoStreaming.h"
#include "VideoItem.h"
#include "VideoSurface.h"

#if defined(QGC_GST_STREAMING)
    G_BEGIN_DECLS
    // Our own plugin
    GST_PLUGIN_STATIC_DECLARE(QGC_VIDEOSINK_PLUGIN);
    // The static plugins we use
#if defined(__mobile__)
    GST_PLUGIN_STATIC_DECLARE(coreelements);
    GST_PLUGIN_STATIC_DECLARE(libav);
    GST_PLUGIN_STATIC_DECLARE(rtp);
    GST_PLUGIN_STATIC_DECLARE(udp);
    GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
    GST_PLUGIN_STATIC_DECLARE(x264);
#endif
    G_END_DECLS
#endif

#if defined(QGC_GST_STREAMING)
#if defined(__macos__)
#ifdef QGC_INSTALL_RELEASE
static void qgcputenv(const QString& key, const QString& root, const QString& path)
{
    QString value = root + path;
    qputenv(key.toStdString().c_str(), QByteArray(value.toStdString().c_str()));
}
#endif
#endif
#endif

void initializeVideoStreaming(int &argc, char* argv[])
{
#if defined(QGC_GST_STREAMING)
    #ifdef __macos__
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
    #endif
        // Initialize GStreamer
        GError* error = NULL;
        if (!gst_init_check(&argc, &argv, &error)) {
            qCritical() << "gst_init_check() failed: " << error->message;
            g_error_free(error);
        }
        // Our own plugin
        GST_PLUGIN_STATIC_REGISTER(QGC_VIDEOSINK_PLUGIN);
        // The static plugins we use
    #if defined(__mobile__)
        GST_PLUGIN_STATIC_REGISTER(coreelements);
        GST_PLUGIN_STATIC_REGISTER(libav);
        GST_PLUGIN_STATIC_REGISTER(rtp);
        GST_PLUGIN_STATIC_REGISTER(udp);
        GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
        GST_PLUGIN_STATIC_REGISTER(x264);
    #endif
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
#endif
    qmlRegisterType<VideoItem>              ("QGroundControl.QgcQtGStreamer", 1, 0, "VideoItem");
    qmlRegisterUncreatableType<VideoSurface>("QGroundControl.QgcQtGStreamer", 1, 0, "VideoSurface", QLatin1String("VideoSurface from QML is not supported"));
}

void shutdownVideoStreaming()
{
    /* From: http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-Gst.html#gst-deinit
     *
     * "It is normally not needed to call this function in a normal application as the resources will automatically
     * be freed when the program terminates. This function is therefore mostly used by testsuites and other memory
     * profiling tools."
     *
     * It's causing a hang on exit. It hangs while deleting some thread.
     *
#if defined(QGC_GST_STREAMING)
     gst_deinit();
#endif
    */
}
