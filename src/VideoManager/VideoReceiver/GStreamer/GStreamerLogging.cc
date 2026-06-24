#include "GStreamerLogging.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <atomic>
#include <memory>

#include "GStreamer.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(GStreamerLoggingLog, "Video.GStreamer.GStreamerLogging")
QGC_LOGGING_CATEGORY_ON(GStreamerAPILog, "Video.GStreamer.GStreamerAPI")

namespace {

std::atomic_bool g_externalPluginLoaderFailed {false};

QString _redactDiagnostic(const QString& diagnostic)
{
    static const QStringList sensitiveMarkers = {
        QStringLiteral("authorization"), QStringLiteral("user-pw"), QStringLiteral("proxy-pw"),
        QStringLiteral("password"),      QStringLiteral("passwd"),
    };
    for (const QString& marker : sensitiveMarkers) {
        if (diagnostic.contains(marker, Qt::CaseInsensitive)) {
            return QStringLiteral("<redacted sensitive GStreamer diagnostic>");
        }
    }

    static const QRegularExpression urlPattern(QStringLiteral(R"((?:https?|wss?)://[^\s\"'<>(),]+)"),
                                               QRegularExpression::CaseInsensitiveOption);
    QString result = diagnostic;
    QList<QRegularExpressionMatch> matches;
    QRegularExpressionMatchIterator matchIterator = urlPattern.globalMatch(result);
    while (matchIterator.hasNext()) {
        matches.append(matchIterator.next());
    }
    for (auto reverseIterator = matches.crbegin(); reverseIterator != matches.crend(); ++reverseIterator) {
        const QRegularExpressionMatch& match = *reverseIterator;
        result.replace(match.capturedStart(), match.capturedLength(),
                       QGCNetworkHelper::redactedUrlForLogging(match.captured()));
    }
    return result;
}

void glib_print_handler(const gchar *string)
{
    qCInfo(GStreamerLoggingLog) << _redactDiagnostic(QString::fromUtf8(string));
}

void glib_printerr_handler(const gchar *string)
{
    qCWarning(GStreamerLoggingLog) << _redactDiagnostic(QString::fromUtf8(string));
}

void glib_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                      const gchar *message, gpointer user_data)
{
    Q_UNUSED(user_data);
    const QString domain = log_domain ? QString::fromUtf8(log_domain) : QStringLiteral("GLib");
    const QString msg = _redactDiagnostic(QString::fromUtf8(message));

    if (msg.contains(QStringLiteral("External plugin loader failed"), Qt::CaseInsensitive)) {
        g_externalPluginLoaderFailed.store(true);
    }

    if (msg.contains(QStringLiteral("pygobject initialization failed"), Qt::CaseInsensitive)) {
        qCDebug(GStreamerLoggingLog) << domain << msg;
        return;
    }

    switch (log_level & G_LOG_LEVEL_MASK) {
    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
        qCCritical(GStreamerLoggingLog) << domain << msg;
        break;
    case G_LOG_LEVEL_WARNING:
        qCWarning(GStreamerLoggingLog) << domain << msg;
        break;
    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
        qCInfo(GStreamerLoggingLog) << domain << msg;
        break;
    case G_LOG_LEVEL_DEBUG:
    default:
        qCDebug(GStreamerLoggingLog) << domain << msg;
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

QString redactDiagnosticForLogging(const QString& diagnostic)
{
    return _redactDiagnostic(diagnostic);
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

    struct GFree { void operator()(gchar *p) const { g_free(p); } };
    const std::unique_ptr<gchar, GFree> object_info(
        gst_info_strdup_printf("%" GST_PTR_FORMAT, object));
    const QString diagnostic = _redactDiagnostic(QStringLiteral("%1 %2").arg(
        QString::fromUtf8(object_info.get()), QString::fromUtf8(gst_debug_message_get(message))));
    const QByteArray encodedDiagnostic = diagnostic.toUtf8();
    QMessageLogger log(file, line, function);

    switch (level) {
    case GST_LEVEL_ERROR:
        log.critical(GStreamerAPILog, "%s", encodedDiagnostic.constData());
        break;
    case GST_LEVEL_WARNING:
        log.warning(GStreamerAPILog, "%s", encodedDiagnostic.constData());
        break;
    case GST_LEVEL_FIXME:
    case GST_LEVEL_INFO:
        log.info(GStreamerAPILog, "%s", encodedDiagnostic.constData());
        break;
    case GST_LEVEL_DEBUG:
#ifdef QT_DEBUG
    // In release builds LOG/TRACE/MEMDUMP are intentionally dropped to reduce
    // noise. Only debug builds route these verbose levels through Qt logging.
    case GST_LEVEL_LOG:
    case GST_LEVEL_TRACE:
    case GST_LEVEL_MEMDUMP:
#endif
        log.debug(GStreamerAPILog, "%s", encodedDiagnostic.constData());
        break;
    default:
        break;
    }
}

} // namespace GStreamer
