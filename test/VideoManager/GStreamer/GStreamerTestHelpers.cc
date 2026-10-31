#include "GStreamerTestHelpers.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <rhi/qrhi.h>
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <QtCore/QFile>
#include <QtNetwork/QTcpSocket>

#include "GStreamer.h"
#include "GstSourceFactory.h"

namespace GstTest {

QString initGStreamer()
{
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
    return QStringLiteral("GStreamer init deadlocks under AddressSanitizer (bindtextdomain lock)");
#else
    if (gst_is_initialized()) {
        return {};
    }
    (void) GStreamer::prepareEnvironment();

    GError* error = nullptr;
    if (!gst_init_check(nullptr, nullptr, &error)) {
        const GStreamer::GErrorPtr owned = GStreamer::adoptError(error);
        return QStringLiteral("GStreamer unavailable: %1")
            .arg(owned ? QString::fromUtf8(owned->message) : QStringLiteral("unknown error"));
    }
    return {};
#endif
}

QString firstMissingFactory(std::initializer_list<const char*> names)
{
    for (const char* name : names) {
        if (!GStreamer::adoptFactory(gst_element_factory_find(name))) {
            return QString::fromLatin1(name);
        }
    }
    return {};
}

QString errorText(GstMessage* message, bool withDetails)
{
    GError* rawError = nullptr;
    gchar* rawDebug = nullptr;
    gst_message_parse_error(message, &rawError, &rawDebug);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    const GStreamer::GCharPtr debug = GStreamer::adoptGChar(rawDebug);
    const QString text = error ? QString::fromUtf8(error->message) : QStringLiteral("unknown error");
    return withDetails ? QStringLiteral("%1 (%2)").arg(text, debug ? QString::fromUtf8(debug.get()) : QString()) : text;
}

QString currentCapsString(GstPad* pad)
{
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_pad_get_current_caps(pad));
    const GStreamer::GCharPtr text = GStreamer::adoptGChar(caps ? gst_caps_to_string(caps.get()) : nullptr);
    return text ? QString::fromUtf8(text.get()) : QString();
}

GstPadProbeReturn countBuffers([[maybe_unused]] GstPad* pad, [[maybe_unused]] GstPadProbeInfo* info, gpointer userData)
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1, std::memory_order_relaxed);
    return GST_PAD_PROBE_OK;
}

std::vector<GStreamer::GstElementPtr> childElements(GstBin* bin, bool recurse)
{
    std::vector<GStreamer::GstElementPtr> children;
    GstIterator* const it = recurse ? gst_bin_iterate_recurse(bin) : gst_bin_iterate_elements(bin);
    GValue item = G_VALUE_INIT;
    bool done = false;
    while (!done) {
        switch (gst_iterator_next(it, &item)) {
            case GST_ITERATOR_OK:
                children.push_back(GStreamer::adoptElement(GST_ELEMENT(g_value_dup_object(&item))));
                g_value_reset(&item);
                break;
            case GST_ITERATOR_RESYNC:
                children.clear();
                gst_iterator_resync(it);
                break;
            case GST_ITERATOR_ERROR:
            case GST_ITERATOR_DONE:
                done = true;
                break;
        }
    }
    g_value_unset(&item);
    gst_iterator_free(it);
    return children;
}

QStringList childFactoryNames(GstBin* bin, bool recurse)
{
    QStringList names;
    for (const GStreamer::GstElementPtr& child : childElements(bin, recurse)) {
        GstElementFactory* const factory = gst_element_get_factory(child.get());
        names << QString::fromUtf8(factory ? gst_plugin_feature_get_name(factory) : "?");
    }
    names.sort();
    return names;
}

GStreamer::GstElementPtr findByFactory(GstElement* bin, const char* factoryName)
{
    if (!bin) {
        return {};
    }
    GstIterator* const it = gst_bin_iterate_all_by_element_factory_name(GST_BIN(bin), factoryName);
    GValue item = G_VALUE_INIT;
    GStreamer::GstElementPtr found;
    if (it && (gst_iterator_next(it, &item) == GST_ITERATOR_OK)) {
        found = GStreamer::adoptElement(GST_ELEMENT(g_value_dup_object(&item)));
    }
    g_value_unset(&item);
    if (it) {
        gst_iterator_free(it);
    }
    return found;
}

GstSample* makeVideoSample(GstVideoFormat format, int width, int height, GstVideoInfo* info)
{
    GstVideoInfo videoInfo;
    if (!gst_video_info_set_format(&videoInfo, format, static_cast<guint>(width), static_cast<guint>(height))) {
        return nullptr;
    }
    GstBuffer* const buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&videoInfo), nullptr);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_video_info_to_caps(&videoInfo));
    GstSample* const sample = gst_sample_new(buffer, caps.get(), nullptr, nullptr);
    gst_buffer_unref(buffer);
    if (info) {
        *info = videoInfo;
    }
    return sample;
}

void watchRelease(GstMiniObject* object, bool* released)
{
    gst_mini_object_weak_ref(object, [](gpointer data, GstMiniObject*) { *static_cast<bool*>(data) = true; }, released);
}

GStreamer::GstElementPtr createSource(const QString& uri, const GStreamer::SourceFactory::Config& config)
{
    GstElement* const bin = GStreamer::SourceFactory::create(uri, config);
    return GStreamer::adoptElement(bin ? GST_ELEMENT(gst_object_ref_sink(bin)) : nullptr);
}

std::unique_ptr<QRhi> makeNullRhi()
{
    QRhiNullInitParams params;
    return std::unique_ptr<QRhi>(QRhi::create(QRhi::Null, &params));
}

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
int makeMemfd(const QByteArray& bytes)
{
    const long page = sysconf(_SC_PAGESIZE);
    const off_t size = static_cast<off_t>(((bytes.size() + page - 1) / page) * page);
    const int fd = memfd_create("qgc-test", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        return -1;
    }
    if ((ftruncate(fd, size) != 0) || (pwrite(fd, bytes.constData(), size_t(bytes.size()), 0) != bytes.size())) {
        ::close(fd);
        return -1;
    }
    return fd;
}
#endif

QList<int> nalTypes(const QByteArray& data, bool h265)
{
    QList<int> types;
    for (qsizetype start = data.indexOf(QByteArrayView("\x00\x00\x01", 3)); (start >= 0) && (start + 3 < data.size());
         start = data.indexOf(QByteArrayView("\x00\x00\x01", 3), start + 3)) {
        const auto header = static_cast<quint8>(data.at(start + 3));
        types.append(h265 ? ((header >> 1) & 0x3F) : (header & 0x1F));
    }
    return types;
}

int countPlayableFrames(const QString& path, QString& failure)
{
    std::atomic<int> frames{0};
    TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("filesrc name=src ! qtdemux ! fakesink name=sink sync=false"))) {
        failure = pipeline.error();
        return -1;
    }
    g_object_set(pipeline.element("src").get(), "location", QFile::encodeName(path).constData(), nullptr);
    const GStreamer::GstPadPtr sinkPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("sink").get(), "sink"));
    (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, countBuffers, &frames, nullptr);

    if (!pipeline.play()) {
        failure = QStringLiteral("%1: demux pipeline failed to start").arg(path);
        return -1;
    }
    const QString error = pipeline.runToEos();
    if (!error.isEmpty()) {
        failure = QStringLiteral("%1: %2").arg(path, error);
        return -1;
    }
    return frames.load(std::memory_order_relaxed);
}

TestPipeline::TestPipeline(GstElement* pipeline)
    : _pipeline(GStreamer::adoptElement(
          (pipeline && g_object_is_floating(pipeline)) ? GST_ELEMENT(gst_object_ref_sink(pipeline)) : pipeline))
{}

TestPipeline::~TestPipeline()
{
    if (_pipeline) {
        (void) gst_element_set_state(_pipeline.get(), GST_STATE_NULL);
    }
}

bool TestPipeline::launch(const QString& description)
{
    GError* rawError = nullptr;
    GstElement* const pipeline = gst_parse_launch(description.toUtf8().constData(), &rawError);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    if (error || !pipeline) {
        _error = error ? QString::fromUtf8(error->message) : QStringLiteral("gst_parse_launch failed");
        if (pipeline) {
            gst_object_unref(gst_object_ref_sink(pipeline));
        }
        return false;
    }
    _pipeline = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(pipeline)));
    return true;
}

GStreamer::GstElementPtr TestPipeline::element(const char* name) const
{
    return GStreamer::adoptElement(gst_bin_get_by_name(bin(), name));
}

bool TestPipeline::play()
{
    return gst_element_set_state(_pipeline.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
}

GStreamer::GstMessagePtr TestPipeline::waitForEosOrError(std::chrono::milliseconds timeout) const
{
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(_pipeline.get()));
    if (!bus) {
        return {};
    }
    return GStreamer::adoptMessage(gst_bus_timed_pop_filtered(
        bus.get(), clockTime(timeout), static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR)));
}

QString TestPipeline::runToEos(std::chrono::milliseconds timeout) const
{
    const GStreamer::GstMessagePtr message = waitForEosOrError(timeout);
    if (!message) {
        return QStringLiteral("timed out waiting for EOS");
    }
    return (GST_MESSAGE_TYPE(message.get()) == GST_MESSAGE_EOS) ? QString() : errorText(message.get());
}

QList<AccessUnit> loadAccessUnits(const QString& resource, QString* error)
{
    const auto fail = [error](const QString& why) {
        if (error) {
            *error = why;
        }
        return QList<AccessUnit>();
    };
    QFile file(resource);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("%1: %2").arg(resource, file.errorString()));
    }
    const QByteArray stream = file.readAll();
    const char* const codec = resource.endsWith(QLatin1String(".h265")) ? "h265" : "h264";

    TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("appsrc name=src caps=video/x-%1,stream-format=byte-stream ! %1parse ! "
                                        "video/x-%1,stream-format=byte-stream,alignment=au ! "
                                        "appsink name=sink sync=false")
                             .arg(QLatin1String(codec)))) {
        return fail(pipeline.error());
    }
    const GStreamer::GstElementPtr src = pipeline.element("src");
    const GStreamer::GstElementPtr sink = pipeline.element("sink");
    if (!pipeline.play()) {
        return fail(QStringLiteral("%1: the parse pipeline did not start").arg(resource));
    }
    (void) gst_app_src_push_buffer(GST_APP_SRC(src.get()),
                                   gst_buffer_new_memdup(stream.constData(), static_cast<gsize>(stream.size())));
    (void) gst_app_src_end_of_stream(GST_APP_SRC(src.get()));

    QList<AccessUnit> units;
    while (GstSample* const sample =
               gst_app_sink_try_pull_sample(GST_APP_SINK(sink.get()), clockTime(TestTimeout::mediumDuration()))) {
        GstBuffer* const buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (buffer && gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            units.append({QByteArray(reinterpret_cast<const char*>(map.data), static_cast<qsizetype>(map.size)),
                          !GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)});
            gst_buffer_unmap(buffer, &map);
        }
        gst_sample_unref(sample);
    }
    if (!gst_app_sink_is_eos(GST_APP_SINK(sink.get())) || units.isEmpty() || !units.constFirst().keyframe) {
        return fail(
            QStringLiteral("%1: parsed %2 units, not a stream starting at a keyframe").arg(resource).arg(units.size()));
    }
    return units;
}

FixtureRtpSender::FixtureRtpSender(const QString& resource)
    : _h265(resource.endsWith(QLatin1String(".h265")))
    , _units(loadAccessUnits(resource, &_error))
{}

FixtureRtpSender::~FixtureRtpSender() = default;

bool FixtureRtpSender::start(quint16 port)
{
    stop();
    if ((port == 0) || _units.isEmpty()) {
        return false;
    }
    {
        const std::lock_guard lock(_mutex);
        _next = 0;
        _pts = 0;
    }
    const char* const codec = _h265 ? "h265" : "h264";
    _pipeline = std::make_unique<TestPipeline>();
    // max-bytes keeps one unit queued, so switchTo() and holdKeyframes() apply to the next frame sent.
    if (!_pipeline->launch(
            QStringLiteral("appsrc name=src is-live=true format=time max-bytes=1 "
                           "caps=video/x-%1,stream-format=byte-stream,alignment=au ! "
                           "rtp%1pay config-interval=-1 pt=96 ! udpsink host=127.0.0.1 port=%2 sync=true async=false")
                .arg(QLatin1String(codec))
                .arg(port))) {
        _error = _pipeline->error();
        return false;
    }
    const GStreamer::GstElementPtr source = _pipeline->element("src");
    const GStreamer::GstPadPtr sourcePad = GStreamer::adoptPad(gst_element_get_static_pad(source.get(), "src"));
    if (!sourcePad || !gst_pad_add_probe(sourcePad.get(), GST_PAD_PROBE_TYPE_BUFFER, countBuffers, &_frames, nullptr)) {
        return false;
    }
    (void) g_signal_connect(source.get(), "need-data", G_CALLBACK(_needData), this);
    return _pipeline->play();
}

void FixtureRtpSender::stop()
{
    _pipeline.reset();
}

bool FixtureRtpSender::switchTo(const QString& resource)
{
    if (resource.endsWith(QLatin1String(".h265")) != _h265) {
        _error = QStringLiteral("%1: not the codec being sent").arg(resource);
        return false;
    }
    QList<AccessUnit> units = loadAccessUnits(resource, &_error);
    if (units.isEmpty()) {
        return false;
    }
    const std::lock_guard lock(_mutex);
    _units = std::move(units);
    _next = 0;
    return true;
}

void FixtureRtpSender::holdKeyframes(bool hold)
{
    const std::lock_guard lock(_mutex);
    _keyframesHeld = hold;
}

void FixtureRtpSender::_needData(GstElement* appsrc, [[maybe_unused]] guint length, gpointer self)
{
    auto* const sender = static_cast<FixtureRtpSender*>(self);
    GstBuffer* buffer = nullptr;
    {
        const std::lock_guard lock(sender->_mutex);
        for (qsizetype skipped = 0;
             sender->_keyframesHeld && sender->_units.at(sender->_next).keyframe && (skipped < sender->_units.size());
             ++skipped) {
            sender->_next = (sender->_next + 1) % sender->_units.size();
        }
        const QByteArray& bytes = sender->_units.at(sender->_next).bytes;
        buffer = gst_buffer_new_memdup(bytes.constData(), static_cast<gsize>(bytes.size()));
        if (!sender->_units.at(sender->_next).keyframe) {
            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        }
        GST_BUFFER_PTS(buffer) = sender->_pts;
        GST_BUFFER_DTS(buffer) = sender->_pts;
        GST_BUFFER_DURATION(buffer) = GST_SECOND / kStreamFrameRate;
        sender->_pts += GST_SECOND / kStreamFrameRate;
        sender->_next = (sender->_next + 1) % sender->_units.size();
    }
    (void) gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

DecoderRankSnapshot::DecoderRankSnapshot()
{
    GList* const factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);
    for (GList* node = factories; node; node = node->next) {
        auto* const feature = GST_PLUGIN_FEATURE(node->data);
        _ranks.emplace_back(GStreamer::adoptFeature(GST_PLUGIN_FEATURE(gst_object_ref(feature))),
                            gst_plugin_feature_get_rank(feature));
    }
    gst_plugin_feature_list_free(factories);
}

DecoderRankSnapshot::~DecoderRankSnapshot()
{
    for (const auto& [feature, rank] : _ranks) {
        gst_plugin_feature_set_rank(feature.get(), rank);
    }
}

FakeRtspServer::FakeRtspServer(const QByteArray& sdp)
    : _sdp(sdp)
{
    QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this] {
        while (QTcpSocket* const socket = _server.nextPendingConnection()) {
            ++_connections;
            if (!_sdp.isEmpty()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] { _serve(socket); });
            }
        }
    });
}

void FakeRtspServer::_serve(QTcpSocket* socket)
{
    QByteArray& pending = _pending[socket];
    pending += socket->readAll();
    for (qsizetype end = pending.indexOf("\r\n\r\n"); end >= 0; end = pending.indexOf("\r\n\r\n")) {
        const QList<QByteArray> lines = pending.left(end).split('\n');
        pending.remove(0, end + 4);

        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        const QByteArray method = requestLine.value(0);
        QByteArray cseq;
        for (const QByteArray& line : lines) {
            if (line.toLower().startsWith("cseq:")) {
                cseq = line.mid(5).trimmed();
            }
        }

        if (method == "SETUP") {
            _setupUrls.append(requestLine.value(1));
            continue;
        }

        QByteArray reply = "RTSP/1.0 200 OK\r\nCSeq: " + cseq + "\r\n";
        if (method == "OPTIONS") {
            reply += "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n\r\n";
        } else if (method == "DESCRIBE") {
            reply += "Content-Base: " + uri().toUtf8() +
                     "/\r\nContent-Type: application/sdp\r\nContent-Length: " + QByteArray::number(_sdp.size()) +
                     "\r\n\r\n" + _sdp;
        } else {
            reply += "\r\n";
        }
        (void) socket->write(reply);
    }
}

}  // namespace GstTest

#endif
