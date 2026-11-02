#include "GStreamerSelfTestMedia.h"

#include <algorithm>
#include <atomic>
#include <gst/app/gstappsrc.h>
#include <mutex>

#include <QtCore/QFile>

#include "GStreamerDecoderRanks.h"
#include "GStreamerHelpers.h"
#include "GstScoped.h"
#include "RecordingBranch.h"

using namespace std::chrono_literals;

namespace GStreamer::SelfTest {

namespace {

GstCapsPtr byteStreamCaps(const Sample& sample)
{
    return adoptCaps(gst_caps_new_simple(sample.mediaType, "stream-format", G_TYPE_STRING, "byte-stream", nullptr));
}

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

}  // namespace

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

}  // namespace GStreamer::SelfTest
