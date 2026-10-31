#include "GStreamerSelfTest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

// gdbusintrospection.h's `signals` member collides with Qt's `signals` keyword macro.
#pragma push_macro("signals")
#undef signals
#include <gio/gio.h>
#pragma pop_macro("signals")

#include <QtCore/QDeadlineTimer>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtCore/QTemporaryDir>

#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"
#include "RecordingBranch.h"

QGC_LOGGING_CATEGORY(GStreamerSelfTestLog, "Video.GStreamer.SelfTest")

namespace GStreamer::SelfTest {

namespace {

using namespace std::chrono_literals;

constexpr std::chrono::milliseconds kBudget = 15s;
constexpr std::chrono::milliseconds kPipelineTimeout = 4s;
constexpr const char* kProxyLookupUri = "rtsp://127.0.0.1";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
// The plugins and the OpenSSL GIO module are linked into the app (StaticInit.cmake).
constexpr bool kStaticRuntime = true;
#else
constexpr bool kStaticRuntime = false;
#endif

#ifdef Q_OS_WIN
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseInsensitive;
#else
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseSensitive;
#endif

/// An encoded sample in the app's resources (test/VideoManager/GStreamer/Media/generate.sh makes them).
struct Sample
{
    const char* codec;
    const char* resource;
    const char* mediaType;
    const char* parser;
};

constexpr Sample kH264Sample{"H.264", ":/gstreamer/selftest/h264_160x120.h264", "video/x-h264", "h264parse"};
constexpr Sample kH265Sample{"H.265", ":/gstreamer/selftest/h265_160x120.h265", "video/x-h265", "h265parse"};

struct RecordingFormat
{
    const char* name;
    VideoReceiver::FileFormat format;
    const char* extension;
    const char* demuxer;
};

constexpr RecordingFormat kRecordingFormats[] = {
    {"MP4", VideoReceiver::FileFormat::Mp4, "mp4", "qtdemux"},
    {"MKV", VideoReceiver::FileFormat::Mkv, "mkv", "matroskademux"},
};

QString normalizedPath(const QString& path)
{
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}

QString pathKey(const QString& path)
{
    const QString normalized = normalizedPath(path);
    return (kPathCase == Qt::CaseInsensitive) ? normalized.toCaseFolded() : normalized;
}

// The files GStreamer's registry scan loads (gstregistry.c): the platform's module suffix, also .dylib on macOS.
bool isPluginFile(const QString& fileName)
{
#if defined(Q_OS_WIN)
    return fileName.endsWith(QLatin1String(".dll"), Qt::CaseInsensitive);
#elif defined(Q_OS_MACOS)
    return fileName.endsWith(QLatin1String(".dylib")) || fileName.endsWith(QLatin1String(".so"));
#else
    return fileName.endsWith(QLatin1String(".so"));
#endif
}

QStringList pluginFilesIn(const QStringList& dirs)
{
    QStringList files;
    for (const QString& dir : dirs) {
        QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            if (isPluginFile(file)) {
                files.append(file);
            }
        }
    }
    return files;
}

bool hasGioModule(const QString& dir, const QString& module)
{
    return !dir.isEmpty() && !QDir(dir).entryList({QStringLiteral("*%1.*").arg(module)}, QDir::Files).isEmpty();
}

GstCapsPtr byteStreamCaps(const Sample& sample)
{
    return adoptCaps(gst_caps_new_simple(sample.mediaType, "stream-format", G_TYPE_STRING, "byte-stream", nullptr));
}

/// Whether decodebin3 has a decoder for @p sample's codec to plug: one ranked at least MARGINAL.
bool hasDecoderFor(const Sample& sample)
{
    const GstCapsPtr caps = adoptCaps(gst_caps_new_empty_simple(sample.mediaType));
    bool found = false;
    forEachVideoDecoderFactory(GST_RANK_MARGINAL, [&caps, &found](GstElementFactory* factory) {
        found = found || gst_element_factory_can_sink_any_caps(factory, caps.get());
    });
    return found;
}

/// Restores every video decoder's rank on destruction.
class DecoderRankRestorer
{
public:
    DecoderRankRestorer()
    {
        forEachVideoDecoderFactory(GST_RANK_NONE, [this](GstElementFactory* factory) {
            GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
            _ranks.emplace_back(adoptFeature(GST_PLUGIN_FEATURE(gst_object_ref(feature))),
                                gst_plugin_feature_get_rank(feature));
        });
    }

    ~DecoderRankRestorer()
    {
        for (const auto& [feature, rank] : _ranks) {
            gst_plugin_feature_set_rank(feature.get(), rank);
        }
    }

    DecoderRankRestorer(const DecoderRankRestorer&) = delete;
    DecoderRankRestorer& operator=(const DecoderRankRestorer&) = delete;

private:
    std::vector<std::pair<GstFeaturePtr, guint>> _ranks;
};

QString errorText(GstMessage* message)
{
    GError* rawError = nullptr;
    gchar* rawDebug = nullptr;
    gst_message_parse_error(message, &rawError, &rawDebug);
    const GErrorPtr error = adoptError(rawError);
    const GCharPtr debug = adoptGChar(rawDebug);
    QString text = QStringLiteral("%1: %2").arg(QString::fromUtf8(GST_MESSAGE_SRC_NAME(message)),
                                                error ? QString::fromUtf8(error->message) : QStringLiteral("error"));
    if (debug) {
        text += QStringLiteral(" (%1)").arg(QString::fromUtf8(debug.get()));
    }
    return text;
}

GstElementPtr newPipeline(const char* name)
{
    GstElement* const pipeline = gst_pipeline_new(name);
    return adoptElement(pipeline ? GST_ELEMENT(gst_object_ref_sink(pipeline)) : nullptr);
}

/// Plays @p pipeline, hands its appsrc @p source, if any, @p data and the end of the stream, and waits for the EOS.
/// Returns an empty string on EOS, else what ended the run. Leaves the pipeline in NULL.
QString playToEos(GstElement* pipeline, GstElement* source, const QByteArray& data, std::chrono::milliseconds timeout)
{
    const GstBusPtr bus = adoptBus(gst_element_get_bus(pipeline));
    const bool started = gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
    if (started && source) {
        (void) gst_app_src_push_buffer(GST_APP_SRC(source),
                                       gst_buffer_new_memdup(data.constData(), static_cast<gsize>(data.size())));
        (void) gst_app_src_end_of_stream(GST_APP_SRC(source));
    }
    const GstClockTime wait =
        started ? static_cast<GstClockTime>(std::chrono::nanoseconds(std::max(timeout, 0ms)).count()) : 0;
    const GstMessagePtr message = adoptMessage(
        gst_bus_timed_pop_filtered(bus.get(), wait, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR)));
    (void) gst_element_set_state(pipeline, GST_STATE_NULL);

    if (message && (GST_MESSAGE_TYPE(message.get()) == GST_MESSAGE_EOS)) {
        return {};
    }
    if (message) {
        return errorText(message.get());
    }
    return started ? QStringLiteral("no end of stream within %1 ms").arg(timeout.count())
                   : QStringLiteral("the pipeline failed to start");
}

void linkNewSrcPad([[maybe_unused]] GstElement* element, GstPad* pad, gpointer sinkPad)
{
    if (GST_PAD_IS_SRC(pad) && !gst_pad_is_linked(GST_PAD(sinkPad))) {
        (void) gst_pad_link(pad, GST_PAD(sinkPad));
    }
}

GstPadProbeReturn countBuffer([[maybe_unused]] GstPad* pad, [[maybe_unused]] GstPadProbeInfo* info, gpointer count)
{
    static_cast<std::atomic<int>*>(count)->fetch_add(1);
    return GST_PAD_PROBE_OK;
}

/// The decoders decodebin3 plugs, as its streaming threads add them.
struct DecoderWatch
{
    std::mutex mutex;
    QStringList decoders;
    bool hardware = false;
};

void watchDecoder([[maybe_unused]] GstBin* bin, [[maybe_unused]] GstBin* subBin, GstElement* element, gpointer data)
{
    GstElementFactory* const factory = gst_element_get_factory(element);
    if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER)) {
        return;
    }
    auto* const watch = static_cast<DecoderWatch*>(data);
    const std::lock_guard lock(watch->mutex);
    watch->decoders.append(QString::fromUtf8(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory))));
    watch->hardware = watch->hardware || isHardwareDecoderFactory(factory);
}

struct DecodeResult
{
    QString failure;
    QStringList decoders;
    bool hardware = false;
};

/// appsrc ! <parser> ! decodebin3 ! appsink, the sink taking raw video in system memory, which every decoder can give.
DecodeResult decodeSample(const Sample& sample, const QByteArray& data, std::chrono::milliseconds timeout)
{
    DecoderWatch watch;
    std::atomic<int> frames = 0;
    const GstElementPtr pipeline = newPipeline("selftest-decode");
    GstElementPtr sourceOwner = makeElement("appsrc");
    GstElementPtr parserOwner = makeElement(sample.parser);
    GstElementPtr decodebinOwner = makeElement("decodebin3");
    GstElementPtr sinkOwner = makeElement("appsink");
    DecodeResult result;
    if (!pipeline || !sourceOwner || !parserOwner || !decodebinOwner || !sinkOwner) {
        result.failure = QStringLiteral("cannot create appsrc, %1, decodebin3 or appsink").arg(sample.parser);
        return result;
    }

    const GstCapsPtr inputCaps = byteStreamCaps(sample);
    const GstCapsPtr rawCaps = adoptCaps(gst_caps_new_empty_simple("video/x-raw"));
    g_object_set(sourceOwner.get(), "caps", inputCaps.get(), nullptr);
    // Nothing pulls the samples: keep only the latest, and post the EOS without waiting for them to be taken.
    g_object_set(sinkOwner.get(), "caps", rawCaps.get(), "sync", FALSE, "max-buffers", 1u, "drop", TRUE, "wait-on-eos",
                 FALSE, nullptr);

    GstElement* const source = addToBin(pipeline.get(), std::move(sourceOwner), "appsrc");
    GstElement* const parser = addToBin(pipeline.get(), std::move(parserOwner), sample.parser);
    GstElement* const decodebin = addToBin(pipeline.get(), std::move(decodebinOwner), "decodebin3");
    GstElement* const sink = addToBin(pipeline.get(), std::move(sinkOwner), "appsink");
    if (!source || !parser || !decodebin || !sink || !gst_element_link_many(source, parser, decodebin, nullptr)) {
        result.failure = QStringLiteral("cannot link appsrc ! %1 ! decodebin3").arg(sample.parser);
        return result;
    }

    const GstPadPtr sinkPad = adoptPad(gst_element_get_static_pad(sink, "sink"));
    (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, countBuffer, &frames, nullptr);
    (void) g_signal_connect(decodebin, "pad-added", G_CALLBACK(linkNewSrcPad), sinkPad.get());
    (void) g_signal_connect(pipeline.get(), "deep-element-added", G_CALLBACK(watchDecoder), &watch);

    result.failure = playToEos(pipeline.get(), source, data, timeout);
    if (result.failure.isEmpty() && (frames.load() == 0)) {
        result.failure = QStringLiteral("no decoded frame reached the sink");
    }

    const std::lock_guard lock(watch.mutex);
    result.decoders = watch.decoders;
    result.decoders.removeDuplicates();
    result.hardware = watch.hardware;
    return result;
}

/// appsrc ! RecordingBranch's file sink (h264parse ! muxer ! filesink), as a recording is muxed.
QString recordSample(const QString& file, VideoReceiver::FileFormat format, const QByteArray& h264,
                     std::chrono::milliseconds timeout)
{
    const GstElementPtr pipeline = newPipeline("selftest-record");
    const GstCapsPtr caps = byteStreamCaps(kH264Sample);
    GstElementPtr sourceOwner = makeElement("appsrc");
    GstElementPtr fileSinkOwner = RecordingBranch::makeFileSink(file, format, caps.get());
    if (!pipeline || !sourceOwner || !fileSinkOwner) {
        return QStringLiteral("cannot create appsrc or the recording sink");
    }
    g_object_set(sourceOwner.get(), "caps", caps.get(), nullptr);

    GstElement* const source = addToBin(pipeline.get(), std::move(sourceOwner), "appsrc");
    GstElement* const fileSink = addToBin(pipeline.get(), std::move(fileSinkOwner), "recording sink");
    if (!source || !fileSink || !gst_element_link(source, fileSink)) {
        return QStringLiteral("cannot link appsrc to the recording sink");
    }
    return playToEos(pipeline.get(), source, h264, timeout);
}

/// filesrc ! <demuxer> ! fakesink: the recorded file opens and holds video.
QString readBack(const QString& file, const char* demuxer, std::chrono::milliseconds timeout)
{
    std::atomic<int> buffers = 0;
    const GstElementPtr pipeline = newPipeline("selftest-read-back");
    GstElementPtr sourceOwner = makeElement("filesrc");
    GstElementPtr demuxerOwner = makeElement(demuxer);
    GstElementPtr sinkOwner = makeElement("fakesink");
    if (!pipeline || !sourceOwner || !demuxerOwner || !sinkOwner) {
        return QStringLiteral("cannot create filesrc, %1 or fakesink").arg(demuxer);
    }
    g_object_set(sourceOwner.get(), "location", QFile::encodeName(file).constData(), nullptr);
    g_object_set(sinkOwner.get(), "sync", FALSE, nullptr);

    GstElement* const source = addToBin(pipeline.get(), std::move(sourceOwner), "filesrc");
    GstElement* const demux = addToBin(pipeline.get(), std::move(demuxerOwner), demuxer);
    GstElement* const sink = addToBin(pipeline.get(), std::move(sinkOwner), "fakesink");
    if (!source || !demux || !sink || !gst_element_link(source, demux)) {
        return QStringLiteral("cannot link filesrc ! %1").arg(demuxer);
    }

    const GstPadPtr sinkPad = adoptPad(gst_element_get_static_pad(sink, "sink"));
    (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, countBuffer, &buffers, nullptr);
    (void) g_signal_connect(demux, "pad-added", G_CALLBACK(linkNewSrcPad), sinkPad.get());

    const QString failure = playToEos(pipeline.get(), nullptr, {}, timeout);
    if (!failure.isEmpty()) {
        return failure;
    }
    return (buffers.load() > 0) ? QString() : QStringLiteral("%1 found no video in it").arg(demuxer);
}

struct ProxyLookup
{
    bool done = false;
    QString proxy;
    QString failure;
};

void onProxyLookedUp(GObject* resolver, GAsyncResult* result, gpointer data)
{
    auto* const lookup = static_cast<ProxyLookup*>(data);
    GError* error = nullptr;
    gchar** const proxies = g_proxy_resolver_lookup_finish(G_PROXY_RESOLVER(resolver), result, &error);
    if (proxies && proxies[0]) {
        lookup->proxy = QString::fromUtf8(proxies[0]);
    } else {
        lookup->failure = error ? QString::fromUtf8(error->message) : QStringLiteral("no proxy returned");
    }
    g_strfreev(proxies);
    g_clear_error(&error);
    lookup->done = true;
}

gboolean markTimedOut(gpointer flag)
{
    *static_cast<bool*>(flag) = true;
    return G_SOURCE_REMOVE;
}

/// Looks @p uri up with @p resolver, giving up after @p timeout. Returns the failure, if any, else the first proxy.
std::pair<QString, QString> lookUpProxy(GProxyResolver* resolver, const char* uri, std::chrono::milliseconds timeout)
{
    // A private context bounds the wait; Qt's event loop doesn't run during the self-test anyway.
    GMainContext* const context = g_main_context_new();
    g_main_context_push_thread_default(context);

    bool timedOut = false;
    GSource* const timer = g_timeout_source_new(static_cast<guint>(std::max(timeout, 0ms).count()));
    g_source_set_callback(timer, markTimedOut, &timedOut, nullptr);
    (void) g_source_attach(timer, context);

    auto lookup = std::make_unique<ProxyLookup>();
    g_proxy_resolver_lookup_async(resolver, uri, nullptr, onProxyLookedUp, lookup.get());
    while (!lookup->done && !timedOut) {
        (void) g_main_context_iteration(context, TRUE);
    }

    g_source_destroy(timer);
    g_source_unref(timer);
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);

    if (!lookup->done) {
        // Nothing iterates the context again, so the callback never runs: leave the lookup to it.
        (void) lookup.release();
        return {QStringLiteral("no answer within %1 ms").arg(timeout.count()), QString()};
    }
    return {lookup->failure, lookup->proxy};
}

class Checker
{
public:
    explicit Checker(const Environment::BundledRuntime& runtime)
        : _runtime(runtime)
    {}

    bool run();

private:
    std::chrono::milliseconds _timeout() const;
    void _fail(const QString& message);
    QByteArray _readSample(const Sample& sample);
    void _checkBundledPlugins();
    void _checkPluginScanner();
    void _checkFeatures();
    void _checkTls();
    void _checkProxy();
    void _checkDecoding(const Sample& sample, const QByteArray& data);
    void _checkRecordings(const QByteArray& h264);

    const Environment::BundledRuntime _runtime;
    const QDeadlineTimer _budget{kBudget};
    int _failures = 0;
    QStringList _notes;
};

bool Checker::run()
{
    QElapsedTimer elapsed;
    elapsed.start();

    const bool bundled = !_runtime.pluginDirs.isEmpty();
    if (bundled) {
        _checkBundledPlugins();
        _checkPluginScanner();
    }
    _checkFeatures();
    if (bundled || kStaticRuntime) {
        _checkTls();
    }
    // libproxy loads its backend with dlopen(), so only a lookup shows it was bundled with what it needs.
    if (hasGioModule(_runtime.gioModuleDir, QStringLiteral("giolibproxy"))) {
        _checkProxy();
    }

    const QByteArray h264 = _readSample(kH264Sample);
    if (!h264.isEmpty()) {
        _checkDecoding(kH264Sample, h264);
        _checkRecordings(h264);
    }
    // Decoder availability differs per platform; H.264 is the codec QGC must always play.
    if (hasDecoderFor(kH265Sample)) {
        const QByteArray h265 = _readSample(kH265Sample);
        if (!h265.isEmpty()) {
            _checkDecoding(kH265Sample, h265);
        }
    } else {
        _notes.append(QStringLiteral("no H.265 decoder to test"));
    }

    if (_failures == 0) {
        qCInfo(GStreamerSelfTestLog).noquote() << QStringLiteral("GStreamer self-test passed in %1 ms: %2")
                                                      .arg(elapsed.elapsed())
                                                      .arg(_notes.join(QStringLiteral(", ")));
        return true;
    }
    qCCritical(GStreamerSelfTestLog).noquote()
        << QStringLiteral("GStreamer self-test failed %1 check(s) in %2 ms").arg(_failures).arg(elapsed.elapsed());
    return false;
}

std::chrono::milliseconds Checker::_timeout() const
{
    return std::min(kPipelineTimeout,
                    std::chrono::duration_cast<std::chrono::milliseconds>(_budget.remainingTimeAsDuration()));
}

void Checker::_fail(const QString& message)
{
    qCCritical(GStreamerSelfTestLog).noquote() << message;
    ++_failures;
}

QByteArray Checker::_readSample(const Sample& sample)
{
    QFile file(QString::fromLatin1(sample.resource));
    if (!file.open(QIODevice::ReadOnly) || (file.size() == 0)) {
        _fail(QStringLiteral("The %1 sample %2 is missing from the app's resources")
                  .arg(QLatin1String(sample.codec), QLatin1String(sample.resource)));
        return {};
    }
    return file.readAll();
}

void Checker::_checkBundledPlugins()
{
    QList<RegisteredPlugin> registered;
    forEachPlugin(gst_registry_get(), [&registered](GstPlugin* plugin) {
        const gchar* const filename = gst_plugin_get_filename(plugin);
        registered.append({.name = QString::fromUtf8(gst_plugin_get_name(plugin)),
                           .filename = filename ? QString::fromUtf8(filename) : QString(),
                           .blacklisted = GST_OBJECT_FLAG_IS_SET(plugin, GST_PLUGIN_FLAG_BLACKLISTED) != 0});
    });

    const QStringList files = pluginFilesIn(_runtime.pluginDirs);
    const QStringList issues = bundledPluginIssues(registered, _runtime.pluginDirs, files);
    for (const QString& issue : issues) {
        _fail(issue);
    }
    if (!issues.isEmpty()) {
        Environment::logDiagnostics();
    }
    _notes.append(QStringLiteral("%1 bundled plugins").arg(files.size()));
}

void Checker::_checkPluginScanner()
{
    if (!externalPluginLoaderFailed()) {
        return;
    }
    const QString scanner = qEnvironmentVariable("GST_PLUGIN_SCANNER_1_0");
    _fail(QStringLiteral("GStreamer's plugin scanner %1 failed to run, so GStreamer loaded the plugins into QGC's "
                         "process: check that the bundled gst-plugin-scanner is executable and signed and finds the "
                         "GStreamer libraries")
              .arg(scanner.isEmpty() ? QStringLiteral("(GST_PLUGIN_SCANNER unset)") : scanner));
}

void Checker::_checkFeatures()
{
    // completeInit() only needs the display core, but every package ships every feature's plugins (the common
    // gstreamer.plugins of build-config.json, GSTREAMER_BUNDLE_REQUIRED_PLUGINS), so all are required on every
    // platform.
    const QList<MissingPluginFeature> missing = missingPluginFeatures(
        [](const char* name) { return static_cast<bool>(adoptFactory(gst_element_factory_find(name))); });
    for (const MissingPluginFeature& feature : missing) {
        _fail(
            QStringLiteral("%1 unavailable: GStreamer elements %2 not found; their plugin is missing or failed to load")
                .arg(feature.feature, feature.factories.join(QStringLiteral(", "))));
    }
}

void Checker::_checkTls()
{
    GTlsBackend* const backend = g_tls_backend_get_default();
    if (!backend || !g_tls_backend_supports_tls(backend)) {
        const QString where =
            _runtime.gioModuleDir.isEmpty() ? QString() : QStringLiteral(" from ") + _runtime.gioModuleDir;
        _fail(QStringLiteral("GIO has no TLS support: no TLS GIO module (gioopenssl, giognutls) loaded%1, so RTSP over "
                             "TLS and HTTPS sources fail")
                  .arg(where));
        return;
    }

    GTlsDatabase* const database = g_tls_backend_get_default_database(backend);
    if (!database) {
        _fail(QStringLiteral("GIO's TLS backend %1 has no certificate database, so no server certificate verifies")
                  .arg(QLatin1String(G_OBJECT_TYPE_NAME(backend))));
        return;
    }
    g_object_unref(database);
    _notes.append(QStringLiteral("TLS by %1").arg(QLatin1String(G_OBJECT_TYPE_NAME(backend))));
}

void Checker::_checkProxy()
{
    // Loads the GIO modules, so the extension point lists the libproxy resolver if its module loaded.
    (void) g_proxy_resolver_get_default();
    GIOExtensionPoint* const point = g_io_extension_point_lookup(G_PROXY_RESOLVER_EXTENSION_POINT_NAME);
    GIOExtension* const extension = point ? g_io_extension_point_get_extension_by_name(point, "libproxy") : nullptr;
    if (!extension) {
        _fail(
            QStringLiteral("The libproxy GIO module in %1 did not load: libproxy or a library it links is missing, so "
                           "proxy settings are ignored")
                .arg(_runtime.gioModuleDir));
        return;
    }

    auto* const resolver = static_cast<GProxyResolver*>(g_object_new(g_io_extension_get_type(extension), nullptr));
    const auto unref = qScopeGuard([resolver] { g_object_unref(resolver); });
    if (!g_proxy_resolver_is_supported(resolver)) {
        _fail(QStringLiteral("The libproxy GIO module in %1 loaded, but libproxy failed to start")
                  .arg(_runtime.gioModuleDir));
        return;
    }

    const auto [failure, proxy] = lookUpProxy(resolver, kProxyLookupUri, std::min(_timeout(), 3000ms));
    if (!failure.isEmpty()) {
        _fail(QStringLiteral("libproxy failed to look up the proxy for %1: %2; its backend (pxbackend), which it loads "
                             "by name, may be missing from the bundle")
                  .arg(QLatin1String(kProxyLookupUri), failure));
        return;
    }
    _notes.append(QStringLiteral("libproxy: %1").arg(proxy));
}

void Checker::_checkDecoding(const Sample& sample, const QByteArray& data)
{
    DecodeResult result = decodeSample(sample, data, _timeout());
    if (!result.failure.isEmpty() && result.hardware) {
        // decodebin3 doesn't fall back when the decoder it plugged fails, as a hardware decoder may in a VM: retry once
        // the way the Software decoder setting does.
        const DecoderRankRestorer restorer;
        setCodecPriorities(ForceVideoDecoderSoftware);
        const DecodeResult software = decodeSample(sample, data, _timeout());
        if (software.failure.isEmpty()) {
            qCWarning(GStreamerSelfTestLog).noquote()
                << QStringLiteral("%1 hardware decoding with %2 failed (%3); software decoding with %4 works")
                       .arg(QLatin1String(sample.codec), result.decoders.join(QLatin1Char('+')), result.failure,
                            software.decoders.join(QLatin1Char('+')));
            result = software;
        } else {
            result.failure += QStringLiteral("; with software decoders forced: %1").arg(software.failure);
            result.decoders += software.decoders;
        }
    }

    if (result.failure.isEmpty()) {
        // decodebin3 replaces a decoder that fails to start, so the last one decoded.
        const QString decoder = result.decoders.isEmpty() ? QStringLiteral("?") : result.decoders.takeLast();
        QString note = QStringLiteral("%1 decoded by %2").arg(QLatin1String(sample.codec), decoder);
        if (!result.decoders.isEmpty()) {
            note += QStringLiteral(" (%1 failed to start)").arg(result.decoders.join(QLatin1Char('+')));
        }
        _notes.append(note);
        return;
    }
    _fail(QStringLiteral("%1 decoding failed with decoders [%2]: %3")
              .arg(QLatin1String(sample.codec), result.decoders.join(QStringLiteral(", ")), result.failure));
}

void Checker::_checkRecordings(const QByteArray& h264)
{
    const QTemporaryDir dir;
    if (!dir.isValid()) {
        _fail(QStringLiteral("Cannot create a directory to record into: %1").arg(dir.errorString()));
        return;
    }

    QStringList recorded;
    for (const RecordingFormat& format : kRecordingFormats) {
        const QString file = dir.filePath(QStringLiteral("selftest.%1").arg(QLatin1String(format.extension)));
        QString failure = recordSample(file, format.format, h264, _timeout());
        if (failure.isEmpty() && (QFileInfo(file).size() <= 0)) {
            failure = QStringLiteral("the file is empty");
        }
        if (failure.isEmpty()) {
            failure = readBack(file, format.demuxer, _timeout());
        }
        if (failure.isEmpty()) {
            recorded.append(QLatin1String(format.name));
        } else {
            _fail(QStringLiteral("%1 recording to %2 failed: %3").arg(QLatin1String(format.name), file, failure));
        }
    }
    if (!recorded.isEmpty()) {
        _notes.append(QStringLiteral("recorded %1").arg(recorded.join(QLatin1Char('/'))));
    }
}

}  // namespace

QStringList bundledPluginIssues(const QList<RegisteredPlugin>& registered, const QStringList& pluginDirs,
                                const QStringList& pluginFiles)
{
    QStringList dirs;
    for (const QString& dir : pluginDirs) {
        dirs.append(normalizedPath(dir) + QLatin1Char('/'));
    }
    const auto inBundle = [&dirs](const QString& file) {
        return std::ranges::any_of(dirs, [&file](const QString& dir) { return file.startsWith(dir, kPathCase); });
    };

    QSet<QString> shipped;
    for (const QString& file : pluginFiles) {
        shipped.insert(pathKey(file));
    }

    QStringList issues;
    QSet<QString> registeredFiles;
    for (const RegisteredPlugin& plugin : registered) {
        if (plugin.filename.isEmpty()) {
            continue;
        }
        const QString file = normalizedPath(plugin.filename);
        registeredFiles.insert(pathKey(file));
        if (!inBundle(file)) {
            issues.append(QStringLiteral("Plugin %1 was loaded from %2, outside the bundled plugin directory %3: a "
                                         "GStreamer installed outside the app leaks into it")
                              .arg(plugin.name, file, pluginDirs.join(QStringLiteral(", "))));
        } else if (!shipped.contains(pathKey(file))) {
            issues.append(QStringLiteral("Plugin %1 is registered from %2, which the bundle does not ship: the plugin "
                                         "registry is stale")
                              .arg(plugin.name, file));
        } else if (plugin.blacklisted) {
            issues.append(
                QStringLiteral("Plugin %1 (%2) is blacklisted: it failed to load, typically because a library "
                               "it links is missing from the bundle")
                    .arg(plugin.name, file));
        }
    }

    for (const QString& file : pluginFiles) {
        if (!registeredFiles.contains(pathKey(file))) {
            issues.append(QStringLiteral("Plugin file %1 is not in GStreamer's registry: GStreamer failed to load it")
                              .arg(normalizedPath(file)));
        }
    }
    return issues;
}

bool run(const Environment::BundledRuntime& runtime)
{
    return Checker(runtime).run();
}

}  // namespace GStreamer::SelfTest

bool GStreamer::selfTest()
{
    return SelfTest::run(Environment::bundledRuntime());
}
