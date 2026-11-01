#include "GStreamerSelfTest.h"

#include <algorithm>
#include <chrono>
#include <gst/gst.h>
#include <memory>
#include <utility>

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
#include "GStreamerDecoderRanks.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GStreamerSelfTestMedia.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerSelfTestLog, "Video.GStreamer.SelfTest")

namespace GStreamer::SelfTest {

namespace {

using namespace std::chrono_literals;

constexpr std::chrono::milliseconds kBudget = 15s;
constexpr std::chrono::milliseconds kPipelineTimeout = 4s;
constexpr const char* kProxyLookupUri = "rtsp://127.0.0.1";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
// The plugins and the OpenSSL GIO module are linked into the app (Mobile.cmake).
constexpr bool kStaticRuntime = true;
#else
constexpr bool kStaticRuntime = false;
#endif

#ifdef Q_OS_WIN
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseInsensitive;
#else
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseSensitive;
#endif

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
    const QList<MissingPluginFeature> missing = missingPluginFeatures(hasElementFactory);
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
