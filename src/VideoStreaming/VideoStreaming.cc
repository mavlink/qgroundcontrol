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
#ifdef __android__
//#define ANDDROID_GST_DEBUG
#endif
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
    GST_PLUGIN_STATIC_DECLARE(rtsp);
    GST_PLUGIN_STATIC_DECLARE(udp);
    GST_PLUGIN_STATIC_DECLARE(videoparsersbad);
    GST_PLUGIN_STATIC_DECLARE(x264);
    GST_PLUGIN_STATIC_DECLARE(rtpmanager);
    GST_PLUGIN_STATIC_DECLARE(isomp4);
    GST_PLUGIN_STATIC_DECLARE(matroska);
#endif
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

#ifdef ANDDROID_GST_DEBUG
// Redirects stdio and stderr to logcat
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>

static int pfd[2];
static pthread_t thr;
static const char *tag = "myapp";

static void *thread_func(void*)
{
    ssize_t rdsz;
    char buf[128];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    return 0;
}

int start_logger(const char *app_name)
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&thr, 0, thread_func, 0) == -1)
        return -1;
    pthread_detach(thr);
    return 0;
}
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

        // Initialize GStreamer
#if !defined(__ios__)
        if (logpath) {
            if (debuglevel) {
                qputenv("GST_DEBUG", debuglevel);
            }
            qputenv("GST_DEBUG_NO_COLOR", "1");
            qputenv("GST_DEBUG_FILE", QString("%1/%2").arg(logpath).arg("gstreamer-log.txt").toUtf8());
            qputenv("GST_DEBUG_DUMP_DOT_DIR", logpath);
        }
#endif
        GError* error = nullptr;
        if (!gst_init_check(&argc, &argv, &error)) {
            qCritical() << "gst_init_check() failed: " << error->message;
            g_error_free(error);
        }
        // Our own plugin
        GST_PLUGIN_STATIC_REGISTER(QGC_VIDEOSINK_PLUGIN);
        // The static plugins we use
    #if defined(__mobile__) && !defined(Q_OS_MAC)
        GST_PLUGIN_STATIC_REGISTER(coreelements);
        GST_PLUGIN_STATIC_REGISTER(libav);
        GST_PLUGIN_STATIC_REGISTER(rtp);
        GST_PLUGIN_STATIC_REGISTER(rtsp);
        GST_PLUGIN_STATIC_REGISTER(udp);
        GST_PLUGIN_STATIC_REGISTER(videoparsersbad);
        GST_PLUGIN_STATIC_REGISTER(x264);
        GST_PLUGIN_STATIC_REGISTER(rtpmanager);
        GST_PLUGIN_STATIC_REGISTER(isomp4);
        GST_PLUGIN_STATIC_REGISTER(matroska);
    #endif
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    Q_UNUSED(logpath);
    Q_UNUSED(debuglevel);
#endif
    qmlRegisterType<VideoItem>              ("QGroundControl.QgcQtGStreamer", 1, 0, "VideoItem");
    qmlRegisterUncreatableType<VideoSurface>("QGroundControl.QgcQtGStreamer", 1, 0, "VideoSurface", QStringLiteral("VideoSurface from QML is not supported"));
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
