#include "GStreamerLogging.h"

#include <algorithm>
#include <atomic>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerLoggingLog, "Video.GStreamer.GStreamerLogging")
QGC_LOGGING_CATEGORY_ON(GStreamerAPILog, "Video.GStreamer.GStreamerAPI")
QGC_LOGGING_CATEGORY(GStreamerDecoderRanksLog, "Video.GStreamer.DecoderRanks")

namespace {

std::atomic<bool> s_externalPluginLoaderFailed = false;

#ifdef QT_DEBUG
constexpr int kMaxBridgedLevel = GST_LEVEL_MEMDUMP;
#else
// LOG and finer would flood Qt logging in release builds; GStreamer's own output (GST_DEBUG) is not capped.
constexpr int kMaxBridgedLevel = GST_LEVEL_DEBUG;
#endif

void glib_print_handler(const gchar* string)
{
    qCInfo(GStreamerLoggingLog) << string;
}

void glib_printerr_handler(const gchar* string)
{
    qCWarning(GStreamerLoggingLog) << string;
}

void glib_log_handler(const gchar* log_domain, GLogLevelFlags log_level, const gchar* message, gpointer user_data)
{
    Q_UNUSED(user_data);
    const QString domain = log_domain ? QString::fromUtf8(log_domain) : QStringLiteral("GLib");
    const QString msg = QString::fromUtf8(message);

    if (msg.contains(QStringLiteral("External plugin loader failed"))) {
        s_externalPluginLoaderFailed.store(true);
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

}  // anonymous namespace

namespace GStreamer {

bool externalPluginLoaderFailed()
{
    return s_externalPluginLoaderFailed.load();
}

void resetExternalPluginLoaderFailed()
{
    s_externalPluginLoaderFailed.store(false);
}

void redirectGLibLogging()
{
    g_set_print_handler(glib_print_handler);
    g_set_printerr_handler(glib_printerr_handler);
    g_log_set_default_handler(glib_log_handler, nullptr);
}

bool nativeDebugLogRequested()
{
    return !qEnvironmentVariableIsEmpty("GST_DEBUG") || !qEnvironmentVariableIsEmpty("GST_DEBUG_FILE") ||
           !qEnvironmentVariableIsEmpty("GST_TRACERS");
}

void qtGstLog(GstDebugCategory* category, GstDebugLevel level, const gchar* file, const gchar* function, gint line,
              GObject* object, GstDebugMessage* message, gpointer data)
{
    Q_UNUSED(category);
    Q_UNUSED(data);

    // gst_debug_log() has already applied the category threshold.
    QtMsgType type = QtDebugMsg;
    switch (level) {
        case GST_LEVEL_ERROR:
            type = QtCriticalMsg;
            break;
        case GST_LEVEL_WARNING:
            type = QtWarningMsg;
            break;
        case GST_LEVEL_FIXME:
        case GST_LEVEL_INFO:
            type = QtInfoMsg;
            break;
        default:
            break;
    }
    if (!GStreamerAPILog().isEnabled(type)) {
        return;
    }

    const GStreamer::GCharPtr objectInfo =
        object ? GStreamer::adoptGChar(gst_info_strdup_printf("%" GST_PTR_FORMAT " ", object)) : nullptr;
    const char* const prefix = objectInfo ? objectInfo.get() : "";
    qt_message_output(type, QMessageLogContext(file, line, function, GStreamerAPILog().categoryName()),
                      QString::fromUtf8(prefix) + QString::fromUtf8(gst_debug_message_get(message)));
}

void configureDebugLogging(std::optional<int> level)
{
    if (nativeDebugLogRequested()) {
        // Keep GStreamer's own output; a bridge left by an earlier call would duplicate every line.
        gst_debug_remove_log_function(GStreamer::qtGstLog);
    } else {
        gst_debug_remove_log_function(gst_debug_log_default);
        gst_debug_remove_log_function(GStreamer::qtGstLog);
        gst_debug_add_log_function(GStreamer::qtGstLog, nullptr, nullptr);
    }

    if (!qEnvironmentVariableIsEmpty("GST_DEBUG")) {
        return;
    }

    // A GStreamer development build (nano > 0, as the official 1.28.8 Windows SDK is) defaults to GST_LEVEL_ERROR
    // rather than GST_LEVEL_NONE, so the setting's default is applied explicitly.
    setDebugLevel(level.value_or(GST_LEVEL_NONE));
}

void setDebugLevel(int level)
{
    if (!gst_is_initialized()) {
        return;
    }
    const int maxLevel = nativeDebugLogRequested() ? static_cast<int>(GST_LEVEL_MEMDUMP) : kMaxBridgedLevel;
    const int clamped = std::clamp(level, 0, maxLevel);
    gst_debug_set_default_threshold(static_cast<GstDebugLevel>(clamped));
    qCDebug(GStreamerLoggingLog) << "GStreamer debug threshold set to" << clamped;
}

void logDecoderRanks()
{
    struct Decoder
    {
        guint rank;
        QByteArray name;
        QString description;
    };

    QList<Decoder> decoders;
    GStreamer::forEachVideoDecoderFactory(GST_RANK_NONE, [&decoders](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        decoders.append({gst_plugin_feature_get_rank(feature), QByteArray(gst_plugin_feature_get_name(feature)),
                         GStreamer::describeDecoderFactory(factory)});
    });

    if (decoders.isEmpty()) {
        qCDebug(GStreamerDecoderRanksLog) << "No video decoder factories found";
        return;
    }

    std::ranges::sort(decoders, [](const Decoder& lhs, const Decoder& rhs) {
        return (lhs.rank != rhs.rank) ? (lhs.rank > rhs.rank) : (lhs.name < rhs.name);
    });

    qCDebug(GStreamerDecoderRanksLog) << "Video decoder ranks:";
    for (const Decoder& decoder : std::as_const(decoders)) {
        qCDebug(GStreamerDecoderRanksLog).noquote() << "  " + decoder.description;
    }
}

}  // namespace GStreamer
