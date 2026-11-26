#include "FFmpegStreamSession.h"

#include <algorithm>
#include <cstring>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QLibrary>
#include <QtCore/QUrl>
#ifdef Q_OS_ANDROID
#include <QtCore/QJniEnvironment>
#endif

#include "FFmpegQtInternals.h"
#include "FFmpegVideoReceiver.h"
#include "QGCLoggingCategory.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/hwcontext.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#ifdef Q_OS_ANDROID
#include <libavcodec/jni.h>
#endif
}

namespace {

/// Frame threads the software decoder takes at most, as the GStreamer receiver's libav decoders do.
constexpr int kFFmpegMaxFrameThreads = 4;
constexpr int kFFmpegSdpBufferBytes = 4096;

/// How often a packet the decoder doesn't take yet is offered again, a millisecond apart.
constexpr int kFFmpegSendAttempts = 50;
/// A decoder's frame is waited for no longer than this, whatever the time between packets.
constexpr std::chrono::milliseconds kFFmpegFrameWaitLimit{20};
/// A longer time between two packets, as after a pause, counts as this in the smoothed one.
constexpr std::chrono::milliseconds kFFmpegPacketIntervalLimit{100};

/// The hardware the decoder is tried on, in this order, before software.
#if defined(Q_OS_ANDROID)
constexpr AVHWDeviceType kFFmpegHardwareDevices[] = {AV_HWDEVICE_TYPE_MEDIACODEC};
#elif defined(Q_OS_LINUX)
constexpr AVHWDeviceType kFFmpegHardwareDevices[] = {AV_HWDEVICE_TYPE_VAAPI, AV_HWDEVICE_TYPE_CUDA};
#else
// Untried elsewhere: software decoding only.
constexpr AVHWDeviceType kFFmpegHardwareDevices[] = {AV_HWDEVICE_TYPE_NONE};
#endif

QString ffmpegErrorString(int code)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    (void) av_strerror(code, buffer, sizeof(buffer));
    return QString::fromUtf8(buffer);
}

QString ffmpegHardwareName(AVHWDeviceType type)
{
    switch (type) {
        case AV_HWDEVICE_TYPE_VAAPI:
            return QStringLiteral("VA-API");
        case AV_HWDEVICE_TYPE_CUDA:
            return QStringLiteral("NVDEC");
        case AV_HWDEVICE_TYPE_MEDIACODEC:
            return QStringLiteral("MediaCodec");
        default:
            return QString::fromUtf8(av_hwdevice_get_type_name(type));
    }
}

/// FFmpeg may crash creating a device whose driver isn't there (seen by Qt: QFFmpeg::precheckDriver()), so the
/// NVIDIA driver and the library NVDEC needs are looked for first.
bool ffmpegHardwareDevicePresent(AVHWDeviceType type)
{
    if (type == AV_HWDEVICE_TYPE_CUDA) {
        if (!QFile::exists(QStringLiteral("/proc/driver/nvidia/version"))) {
            return false;
        }
        QLibrary nvcuvid(QStringLiteral("nvcuvid"), 1);
        const bool loaded = nvcuvid.load();
        (void) nvcuvid.unload();
        return loaded;
    }
    return type != AV_HWDEVICE_TYPE_NONE;
}

bool ffmpegDecoderTakesDevice(const AVCodec* codec, AVHWDeviceType type)
{
    for (int i = 0;; ++i) {
        const AVCodecHWConfig* const config = avcodec_get_hw_config(codec, i);
        if (!config) {
            return false;
        }
        if (((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) != 0) && (config->device_type == type)) {
            return true;
        }
    }
}

/// The decoder of @p codec that decodes on @p device: the one FFmpeg decodes the codec with, where it takes the
/// device (VA-API, NVDEC), else one that is the device's own (MediaCodec's are); null where there is none.
const AVCodec* ffmpegDecoderOn(AVCodecID codec, AVHWDeviceType device)
{
    const AVCodec* const standard = avcodec_find_decoder(codec);
    if (standard && ffmpegDecoderTakesDevice(standard, device)) {
        return standard;
    }
    void* position = nullptr;
    while (const AVCodec* const candidate = av_codec_iterate(&position)) {
        if ((candidate->id == codec) && (av_codec_is_decoder(candidate) != 0) &&
            ffmpegDecoderTakesDevice(candidate, device)) {
            return candidate;
        }
    }
    return nullptr;
}

/// Has @p frame keep @p decoder alive. FFmpeg's MediaCodec decoder logs through its AVCodecContext whenever a
/// frame's buffer is released, also once the decoder is closed, and a frame on its way to the screen, or the last
/// one shown, outlives the session. A frame releases its opaque_ref after its buffer.
void ffmpegKeepDecoder(AVFrame& frame, const std::shared_ptr<AVCodecContext>& decoder)
{
    if (frame.opaque_ref) {
        return;
    }
    auto* const keeper = new std::shared_ptr<AVCodecContext>(decoder);
    frame.opaque_ref = av_buffer_create(
        reinterpret_cast<uint8_t*>(keeper), sizeof(*keeper),
        [](void*, uint8_t* data) { delete reinterpret_cast<std::shared_ptr<AVCodecContext>*>(data); }, nullptr, 0);
    if (!frame.opaque_ref) {
        delete keeper;
    }
}

/// Reads the SDP description an RTP stream is opened from.
struct FFmpegSdpReader
{
    QByteArray sdp;
    qsizetype position = 0;

    static int read(void* opaque, uint8_t* buffer, int size)
    {
        auto* const reader = static_cast<FFmpegSdpReader*>(opaque);
        const qsizetype left = reader->sdp.size() - reader->position;
        if (left <= 0) {
            return AVERROR_EOF;
        }
        const int count = static_cast<int>(std::min<qsizetype>(left, size));
        std::memcpy(buffer, reader->sdp.constData() + reader->position, static_cast<size_t>(count));
        reader->position += count;
        return count;
    }
};

/// Owns the options handed to an FFmpeg open call.
struct FFmpegOptions
{
    AVDictionary* dictionary = nullptr;

    FFmpegOptions() = default;
    FFmpegOptions(const FFmpegOptions&) = delete;
    FFmpegOptions& operator=(const FFmpegOptions&) = delete;

    ~FFmpegOptions() { av_dict_free(&dictionary); }

    void set(const char* key, const char* value) { (void) av_dict_set(&dictionary, key, value, 0); }

    void set(const char* key, int64_t value) { (void) av_dict_set_int(&dictionary, key, value, 0); }
};

/// An SDP description of a stream of @p encoding RTP packets, payload type 96, arriving at @p port.
QByteArray ffmpegRtpSdp(const QString& host, int port, QByteArrayView encoding)
{
    const QByteArray address = (host.isEmpty() ? QStringLiteral("0.0.0.0") : host).toLatin1();
    return QByteArrayLiteral("v=0\r\n") + "o=- 0 0 IN IP4 " + address + "\r\ns=QGroundControl\r\nc=IN IP4 " + address +
           "\r\nt=0 0\r\nm=video " + QByteArray::number(port) + " RTP/AVP 96\r\na=rtpmap:96 " + encoding + "/90000\r\n";
}

/// The codec's name as the other receivers' statistics give it.
QString ffmpegCodecName(AVCodecID codec)
{
    switch (codec) {
        case AV_CODEC_ID_H264:
            return QStringLiteral("H.264");
        case AV_CODEC_ID_HEVC:
            return QStringLiteral("H.265");
        case AV_CODEC_ID_AV1:
            return QStringLiteral("AV1");
        case AV_CODEC_ID_VP8:
            return QStringLiteral("VP8");
        case AV_CODEC_ID_VP9:
            return QStringLiteral("VP9");
        case AV_CODEC_ID_MJPEG:
            return QStringLiteral("MJPEG");
        case AV_CODEC_ID_MPEG2VIDEO:
            return QStringLiteral("MPEG-2");
        case AV_CODEC_ID_MPEG4:
            return QStringLiteral("MPEG-4");
        default:
            return QString::fromUtf8(avcodec_get_name(codec));
    }
}

QString ffmpegTranslate(const char* text)
{
    return QCoreApplication::translate("FFmpegVideoReceiver", text);
}

}  // namespace

FFmpegStreamSession::FFmpegStreamSession(quint64 id, const Settings& settings, Listener& listener,
                                         FFmpegStreamRecorder& recorder)
    : _id(id)
    , _settings(settings)
    , _listener(listener)
    , _recorder(recorder)
{}

FFmpegStreamSession::~FFmpegStreamSession()
{
    requestStop();
    if (_thread.joinable()) {
        _thread.join();
    }
}

std::optional<FFmpegStreamSession::Source> FFmpegStreamSession::sourceFor(const QString& uri)
{
    const QUrl url(uri);
    if (uri.isEmpty() || !url.isValid() || url.scheme().isEmpty()) {
        return std::nullopt;
    }

    using VideoStreamUri::Transport;
    const std::optional<Transport> transport = VideoStreamUri::transportFor(url);
    // A free port (0) is no use to listen on: the sender must know it.
    if (transport &&
        (VideoStreamUri::addressError(*transport, url) || (VideoStreamUri::listens(*transport) && (url.port() == 0)))) {
        return std::nullopt;
    }

    Source source{.url = QString::fromUtf8(url.toEncoded()), .sdp = {}, .transport = transport};
    if ((transport == Transport::RtpUdpH264) || (transport == Transport::RtpUdpH265)) {
        source.url = QStringLiteral("stream.sdp");
        source.sdp = ffmpegRtpSdp(url.host(), url.port(), (transport == Transport::RtpUdpH265) ? "H265" : "H264");
    } else if (transport == Transport::MpegTsUdp) {
        QUrl udp(url);
        udp.setScheme(QStringLiteral("udp"));
        source.url = QString::fromUtf8(udp.toEncoded());
    } else if ((transport == Transport::Rtsp) &&
               (url.scheme().compare(QStringLiteral("rtspt"), Qt::CaseInsensitive) == 0)) {
        QUrl rtsp(url);
        rtsp.setScheme(QStringLiteral("rtsp"));
        source.url = QString::fromUtf8(rtsp.toEncoded());
        source.rtspOverTcp = true;
    }
    return source;
}

qint64 FFmpegStreamSession::nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void FFmpegStreamSession::start()
{
    if (!_thread.joinable()) {
        _thread = std::thread([this] { _run(); });
    }
}

void FFmpegStreamSession::requestStop()
{
    _stopRequested.store(true);
}

void FFmpegStreamSession::setDecoding(bool decoding)
{
    _decodingWanted.store(decoding);
}

FFmpegStreamSession::Counters FFmpegStreamSession::counters() const
{
    return {.packets = _packets.load(),
            .bytes = _bytes.load(),
            .framesDecoded = _framesDecoded.load(),
            .hardwareFrames = _hardwareFrames.load(),
            .lastPacketMs = _lastPacketMs.load(),
            .keyframeIntervalMs = _keyframeIntervalMs.load()};
}

int FFmpegStreamSession::_interrupt(void* opaque)
{
    return static_cast<const FFmpegStreamSession*>(opaque)->_stopRequested.load() ? 1 : 0;
}

void FFmpegStreamSession::_run()
{
    std::optional<Failure> failure = _receive();
    // An open or read the stop interrupted fails, which is no failure of the stream.
    if (_stopRequested.load()) {
        failure.reset();
    }
    _decoder.reset();
    _hardwareFramesContext.reset();
    _listener.onSessionEnded(_id, failure);
}

std::optional<FFmpegStreamSession::Failure> FFmpegStreamSession::_receive()
{
    const Source& source = _settings.source;
    using VideoStreamUri::Transport;
    const bool rtp = (source.transport == Transport::Rtsp) || (source.transport == Transport::RtpUdpH264) ||
                     (source.transport == Transport::RtpUdpH265);
    const int64_t networkTimeoutUs =
        std::chrono::duration_cast<std::chrono::microseconds>(_settings.networkTimeout).count();

    FFmpegOptions options;
    options.set("fflags", "nobuffer");
    if (rtp) {
        options.set("allowed_media_types", "video");
        // How long a packet that arrives out of order is waited for, as GStreamer's RTP jitter buffer would.
        options.set("max_delay", _settings.lowLatency ? int64_t{0} : int64_t{_settings.rtpJitterLatencyMs} * 1000);
    }
    if (source.transport == Transport::Rtsp) {
        options.set("timeout", networkTimeoutUs);
        if (source.rtspOverTcp) {
            options.set("rtsp_transport", "tcp");
        }
    } else if (!source.transport || (source.transport == Transport::MpegTsTcp)) {
        options.set("timeout", networkTimeoutUs);
    }

    // Declared before the input, which reads them until it is closed.
    FFmpegSdpReader sdpReader{.sdp = source.sdp};
    FFmpegPointers::IoContext sdpIo;
    FFmpegPointers::Input input(avformat_alloc_context());
    if (!input) {
        return Failure{ffmpegErrorString(AVERROR(ENOMEM))};
    }
    input->interrupt_callback.callback = &FFmpegStreamSession::_interrupt;
    input->interrupt_callback.opaque = this;
    const AVInputFormat* format = nullptr;
    if (!source.sdp.isEmpty()) {
        auto* const buffer = static_cast<unsigned char*>(av_malloc(kFFmpegSdpBufferBytes));
        sdpIo.reset(
            avio_alloc_context(buffer, kFFmpegSdpBufferBytes, 0, &sdpReader, &FFmpegSdpReader::read, nullptr, nullptr));
        if (!sdpIo) {
            av_free(buffer);
            return Failure{ffmpegErrorString(AVERROR(ENOMEM))};
        }
        input->pb = sdpIo.get();
        format = av_find_input_format("sdp");
    }

    qCDebug(FFmpegVideoReceiverLog) << "Opening" << _settings.redactedUri;
    AVFormatContext* opened = input.release();
    const QByteArray url = source.url.toUtf8();
    const int openResult = avformat_open_input(&opened, url.constData(), format, &options.dictionary);
    input.reset(opened);
    if (openResult < 0) {
        const bool permanent = (openResult == AVERROR_PROTOCOL_NOT_FOUND) || (openResult == AVERROR_DEMUXER_NOT_FOUND);
        return Failure{ffmpegErrorString(openResult), permanent};
    }

    // The streams are known from the SDP or from what was read to open the input, but an MPEG-TS stream can announce
    // its video later. The decoder takes the picture's size and parameters from the stream itself, so there is no
    // avformat_find_stream_info(): it would read, and drop, the stream up to its next keyframe at least.
    int videoStream = av_find_best_stream(input.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    FFmpegPointers::Packet packet(av_packet_alloc());
    if (!packet) {
        return Failure{ffmpegErrorString(AVERROR(ENOMEM))};
    }
    while (!_stopRequested.load()) {
        const int readResult = av_read_frame(input.get(), packet.get());
        if (readResult == AVERROR(EAGAIN)) {
            continue;
        }
        if (readResult < 0) {
            return Failure{(readResult == AVERROR_EOF) ? ffmpegTranslate("the stream ended")
                                                       : ffmpegErrorString(readResult)};
        }
        if ((videoStream < 0) && (packet->stream_index >= 0) &&
            (static_cast<unsigned>(packet->stream_index) < input->nb_streams) &&
            (input->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)) {
            videoStream = packet->stream_index;
        }
        if ((videoStream >= 0) && (packet->stream_index == videoStream)) {
            _onVideoPacket(*input->streams[videoStream], *packet);
        }
        av_packet_unref(packet.get());
    }
    return std::nullopt;
}

void FFmpegStreamSession::_onVideoPacket(const AVStream& stream, AVPacket& packet)
{
    const bool first = (_packets.fetch_add(1) == 0);
    _bytes.fetch_add(static_cast<quint64>(std::max(packet.size, 0)));
    _lastPacketMs.store(nowMs());
    if (first) {
        _listener.onSessionStreaming(_id, ffmpegCodecName(stream.codecpar->codec_id));
    }
    if ((packet.flags & AV_PKT_FLAG_KEY) != 0) {
        _noteKeyframe(stream, packet);
    }
    if (!_recordedStream) {
        auto recorded = std::make_shared<FFmpegStreamRecorder::Stream>();
        recorded->parameters.reset(avcodec_parameters_alloc());
        if (recorded->parameters && (avcodec_parameters_copy(recorded->parameters.get(), stream.codecpar) >= 0)) {
            recorded->timeBaseNum = stream.time_base.num;
            recorded->timeBaseDen = stream.time_base.den;
            _recordedStream = std::move(recorded);
        }
    }
    if (_recordedStream) {
        _recorder.write(_id, _recordedStream, packet);
    }
    _decode(stream, packet);
}

void FFmpegStreamSession::_noteKeyframe(const AVStream& stream, const AVPacket& packet)
{
    const int64_t timestamp = (packet.pts != AV_NOPTS_VALUE) ? packet.pts : packet.dts;
    if ((timestamp == AV_NOPTS_VALUE) || (stream.time_base.den <= 0)) {
        return;
    }
    const double timeMs = static_cast<double>(timestamp) * 1000.0 * stream.time_base.num / stream.time_base.den;
    // A time that goes back is a new start, as after the sender restarted.
    if (!_keyframeTimesMs.empty() && (timeMs <= _keyframeTimesMs.back())) {
        _keyframeTimesMs.clear();
    }
    _keyframeTimesMs.push_back(timeMs);
    if (_keyframeTimesMs.size() > static_cast<size_t>(kKeyframeWindow)) {
        _keyframeTimesMs.pop_front();
    }
    if (_keyframeTimesMs.size() >= 2) {
        _keyframeIntervalMs.store((_keyframeTimesMs.back() - _keyframeTimesMs.front()) /
                                  static_cast<double>(_keyframeTimesMs.size() - 1));
    }
}

void FFmpegStreamSession::_decode(const AVStream& stream, AVPacket& packet)
{
    if (!_decodingWanted.load()) {
        _awaitingKeyframe = true;
        return;
    }
    if (_decoderFailed) {
        return;
    }
    if (_awaitingKeyframe) {
        if ((packet.flags & AV_PKT_FLAG_KEY) == 0) {
            return;
        }
        _awaitingKeyframe = false;
        if (_decoder) {
            avcodec_flush_buffers(_decoder.get());
        }
    }
    if (!_decoder) {
        if (const std::optional<QString> error = _openDecoder(stream, packet)) {
            _decoderFailed = true;
            qCWarning(FFmpegVideoReceiverLog) << "Can't decode" << _settings.redactedUri << ":" << *error;
            _listener.onSessionDecoderFailed(_id, *error);
            return;
        }
    }
    int sent = avcodec_send_packet(_decoder.get(), &packet);
    // A decoder that gives its frames when they are ready takes no packet while it is full of them.
    for (int attempt = 1; (sent == AVERROR(EAGAIN)) && (attempt < kFFmpegSendAttempts) && !_stopRequested.load();
         ++attempt) {
        (void) _deliverFrames();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        sent = avcodec_send_packet(_decoder.get(), &packet);
    }
    if (sent < 0) {
        qCDebug(FFmpegVideoReceiverLog) << "Decoder refused a packet:" << ffmpegErrorString(sent);
        if (sent == AVERROR(EAGAIN)) {
            // Without this packet the pictures that follow are damaged, up to the next keyframe.
            _awaitingKeyframe = true;
        }
    }
    if (_framesComeLater && (sent >= 0)) {
        _awaitFrame();
    } else {
        (void) _deliverFrames();
    }
}

void FFmpegStreamSession::_awaitFrame()
{
    const auto now = std::chrono::steady_clock::now();
    if (_lastSendTime != std::chrono::steady_clock::time_point{}) {
        const auto interval = std::min<std::chrono::microseconds>(
            std::chrono::duration_cast<std::chrono::microseconds>(now - _lastSendTime), kFFmpegPacketIntervalLimit);
        _packetInterval = (_packetInterval.count() == 0) ? interval : ((_packetInterval * 7) + interval) / 8;
    }
    _lastSendTime = now;
    const auto deadline = now + std::min<std::chrono::microseconds>(_packetInterval / 2, kFFmpegFrameWaitLimit);
    // A frame that is ready at once is an earlier packet's, whose own didn't come within the wait: this packet's
    // wouldn't either.
    while ((_deliverFrames() == 0) && !_stopRequested.load() && (std::chrono::steady_clock::now() < deadline)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

std::optional<QString> FFmpegStreamSession::_openDecoder(const AVStream& stream, const AVPacket& keyframe)
{
    const AVCodec* const standard = avcodec_find_decoder(stream.codecpar->codec_id);

    const auto open = [this, &stream, &keyframe](const AVCodec* codec,
                                                 AVHWDeviceType device) -> FFmpegPointers::CodecContext {
        const AVCodecParameters* parameters = stream.codecpar;
        FFmpegPointers::CodecParameters completed;
        if (device == AV_HWDEVICE_TYPE_MEDIACODEC) {
            // MediaCodec is set up with the picture's size and parameter sets, which the parameters of a stream
            // that wasn't probed (_receive()) lack.
            completed.reset(avcodec_parameters_alloc());
            if (!completed || (avcodec_parameters_copy(completed.get(), stream.codecpar) < 0)) {
                return {};
            }
            FFmpegStreamRecorder::completeParameters(*completed, keyframe);
            parameters = completed.get();
        }
        FFmpegPointers::CodecContext context(avcodec_alloc_context3(codec));
        if (!context || (avcodec_parameters_to_context(context.get(), parameters) < 0)) {
            return {};
        }
        context->pkt_timebase = stream.time_base;
        if (device != AV_HWDEVICE_TYPE_NONE) {
#ifdef Q_OS_ANDROID
            // FFmpeg reaches MediaCodec through the Java VM. Qt's FFmpeg plugin gives it one as it is loaded; given
            // here as well, the receiver doesn't depend on that.
            if (av_jni_set_java_vm(QJniEnvironment::javaVM(), nullptr) < 0) {
                return {};
            }
#endif
            AVBufferRef* deviceContext = nullptr;
            if (av_hwdevice_ctx_create(&deviceContext, device, nullptr, nullptr, 0) < 0) {
                return {};
            }
            context->hw_device_ctx = deviceContext;
            // As Qt's player: a stream above the level the hardware reports is still tried on it.
            context->hwaccel_flags |= AV_HWACCEL_FLAG_IGNORE_LEVEL;
            context->thread_count = 1;
        } else if (_settings.softwareDecoderThreads > 1) {
            // Each frame thread holds a frame back.
            context->thread_type = FF_THREAD_FRAME;
            context->thread_count = std::min(_settings.softwareDecoderThreads, kFFmpegMaxFrameThreads);
        } else {
            context->thread_type = FF_THREAD_SLICE;
            context->thread_count = 0;
        }
        if (_settings.lowLatency) {
            context->flags |= AV_CODEC_FLAG_LOW_DELAY;
        }
        FFmpegQtInternals::chooseFormatsAsQt(*context);
        if (avcodec_open2(context.get(), codec, nullptr) < 0) {
            return {};
        }
        return context;
    };

    DecoderInfo info;
    if (_settings.hardwareDecoding) {
        for (const AVHWDeviceType device : kFFmpegHardwareDevices) {
            const AVCodec* const codec =
                ffmpegHardwareDevicePresent(device) ? ffmpegDecoderOn(stream.codecpar->codec_id, device) : nullptr;
            if (!codec) {
                continue;
            }
            if (!(_decoder = open(codec, device))) {
                qCDebug(FFmpegVideoReceiverLog) << codec->name << "didn't open on" << ffmpegHardwareName(device);
                continue;
            }
            info.name = QString::fromUtf8(codec->name);
            if (codec == standard) {
                info.name += QStringLiteral(" (%1)").arg(ffmpegHardwareName(device));
            }
            info.hardware = true;
            _framesComeLater = (device == AV_HWDEVICE_TYPE_MEDIACODEC);
            break;
        }
    }
    if (!_decoder) {
        if (!standard) {
            return ffmpegTranslate("no decoder for %1")
                .arg(QString::fromUtf8(avcodec_get_name(stream.codecpar->codec_id)));
        }
        info.name = QString::fromUtf8(standard->name);
        _decoder = open(standard, AV_HWDEVICE_TYPE_NONE);
    }
    if (!_decoder) {
        return ffmpegTranslate("the %1 decoder didn't open").arg(info.name);
    }
    qCDebug(FFmpegVideoReceiverLog) << "Decoding" << _settings.redactedUri << "with" << info.name;
    _listener.onSessionDecoder(_id, info);
    return std::nullopt;
}

int FFmpegStreamSession::_deliverFrames()
{
    int delivered = 0;
    while (true) {
        FFmpegPointers::Frame frame(av_frame_alloc());
        if (!frame || (avcodec_receive_frame(_decoder.get(), frame.get()) < 0)) {
            return delivered;
        }
        ++delivered;
        _framesDecoded.fetch_add(1);
        if (frame->format == AV_PIX_FMT_MEDIACODEC) {
            ffmpegKeepDecoder(*frame, _decoder);
        }
        FFmpegQtInternals::completeHardwareFrame(*_decoder, *frame, _hardwareFramesContext);
        _hardwareFrames.store(frame->hw_frames_ctx != nullptr);
        const int64_t timestamp = frame->best_effort_timestamp;
        const AVRational timeBase = _decoder->pkt_timebase;
        QVideoFrame videoFrame = FFmpegQtInternals::videoFrame(std::move(frame), _settings.squarePixels);
        if ((timestamp != AV_NOPTS_VALUE) && (timeBase.den > 0)) {
            videoFrame.setStartTime(av_rescale_q(timestamp, timeBase, AVRational{1, 1000000}));
        }
        if (videoFrame.isValid()) {
            _listener.onSessionFrame(_id, std::move(videoFrame));
        }
    }
}
