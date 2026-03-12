#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QString>

#include <atomic>
#include <memory>

QGC_LOGGING_CATEGORY(GStreamerLog, "Video.GStreamer")
QGC_LOGGING_CATEGORY(GStreamerDecoderRanksLog, "Video.GStreamerDecoderRanks")
QGC_LOGGING_CATEGORY_ON(GStreamerAPILog, "Video.GStreamerAPI")

namespace {

std::atomic_bool g_externalPluginLoaderFailed {false};

void glib_print_handler(const gchar *string)
{
    qCInfo(GStreamerLog) << string;
}

void glib_printerr_handler(const gchar *string)
{
    qCWarning(GStreamerLog) << string;
}

void glib_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                      const gchar *message, gpointer user_data)
{
    Q_UNUSED(user_data);
    const QString domain = log_domain ? QString::fromUtf8(log_domain) : QStringLiteral("GLib");
    const QString msg = QString::fromUtf8(message);

    if (msg.contains(QStringLiteral("External plugin loader failed"), Qt::CaseInsensitive)) {
        g_externalPluginLoaderFailed.store(true);
    }

    switch (log_level & G_LOG_LEVEL_MASK) {
    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
        qCCritical(GStreamerLog) << domain << msg;
        break;
    case G_LOG_LEVEL_WARNING:
        qCWarning(GStreamerLog) << domain << msg;
        break;
    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
        qCInfo(GStreamerLog) << domain << msg;
        break;
    case G_LOG_LEVEL_DEBUG:
    default:
        qCDebug(GStreamerLog) << domain << msg;
        break;
    }
}

} // anonymous namespace

namespace GStreamer
{

void resetExternalPluginLoaderFailure()
{
    g_externalPluginLoaderFailed.store(false);
}

bool didExternalPluginLoaderFail()
{
    return g_externalPluginLoaderFailed.load();
}

void redirectGLibLogging()
{
    g_set_print_handler(glib_print_handler);
    g_set_printerr_handler(glib_printerr_handler);
    g_log_set_default_handler(glib_log_handler, nullptr);
}

void qtGstLog(GstDebugCategory *category,
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

    struct GFree { void operator()(gchar *p) const { g_free(p); } };
    const std::unique_ptr<gchar, GFree> object_info(
        gst_info_strdup_printf("%" GST_PTR_FORMAT, object));

    switch (level) {
    case GST_LEVEL_ERROR:
        log.critical(GStreamerAPILog, "%s %s", object_info.get(), gst_debug_message_get(message));
        break;
    case GST_LEVEL_WARNING:
        log.warning(GStreamerAPILog, "%s %s", object_info.get(), gst_debug_message_get(message));
        break;
    case GST_LEVEL_FIXME:
    case GST_LEVEL_INFO:
        log.info(GStreamerAPILog, "%s %s", object_info.get(), gst_debug_message_get(message));
        break;
    case GST_LEVEL_DEBUG:
#ifdef QT_DEBUG
    // In release builds LOG/TRACE/MEMDUMP are intentionally dropped to reduce
    // noise. Only debug builds route these verbose levels through Qt logging.
    case GST_LEVEL_LOG:
    case GST_LEVEL_TRACE:
    case GST_LEVEL_MEMDUMP:
#endif
        log.debug(GStreamerAPILog, "%s %s", object_info.get(), gst_debug_message_get(message));
        break;
    default:
        break;
    }
}

} // namespace GStreamer
