#include "FFmpegStreamRecorder.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <QtCore/QFile>

#include "FFmpegVideoReceiver.h"
#include "QGCLoggingCategory.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
}

namespace {

QString ffmpegRecorderError(int code)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    (void) av_strerror(code, buffer, sizeof(buffer));
    return QString::fromUtf8(buffer);
}

const char* ffmpegMuxerName(VideoReceiver::FileFormat format)
{
    switch (format) {
        case VideoReceiver::FileFormat::Mov:
            return "mov";
        case VideoReceiver::FileFormat::Mp4:
            return "mp4";
        case VideoReceiver::FileFormat::Mkv:
            break;
    }
    return "matroska";
}

}  // namespace

FFmpegStreamRecorder::FFmpegStreamRecorder(Listener& listener)
    : _listener(listener)
    , _comment(VideoReceiver::recordingComment().toUtf8())
{}

void FFmpegStreamRecorder::completeParameters(AVCodecParameters& parameters, const AVPacket& keyframe)
{
    if (parameters.extradata_size <= 0) {
        AVBSFContext* context = nullptr;
        const AVBitStreamFilter* const filter = av_bsf_get_by_name("extract_extradata");
        if (filter && (av_bsf_alloc(filter, &context) == 0)) {
            const FFmpegPointers::BitstreamFilter bsf(context);
            FFmpegPointers::Packet in(av_packet_clone(&keyframe));
            const FFmpegPointers::Packet out(av_packet_alloc());
            if (in && out && (avcodec_parameters_copy(bsf->par_in, &parameters) >= 0) &&
                (av_bsf_init(bsf.get()) >= 0) && (av_bsf_send_packet(bsf.get(), in.get()) >= 0) &&
                (av_bsf_receive_packet(bsf.get(), out.get()) >= 0)) {
                size_t size = 0;
                const uint8_t* const data = av_packet_get_side_data(out.get(), AV_PKT_DATA_NEW_EXTRADATA, &size);
                if (data && (size > 0)) {
                    auto* const extradata = static_cast<uint8_t*>(av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE));
                    if (extradata) {
                        std::memcpy(extradata, data, size);
                        av_freep(&parameters.extradata);
                        parameters.extradata = extradata;
                        parameters.extradata_size = static_cast<int>(size);
                    }
                }
            }
        }
    }

    if ((parameters.width <= 0) || (parameters.height <= 0)) {
        const FFmpegPointers::Parser parser(av_parser_init(parameters.codec_id));
        const FFmpegPointers::CodecContext context(avcodec_alloc_context3(nullptr));
        if (parser && context && (avcodec_parameters_to_context(context.get(), &parameters) >= 0)) {
            parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
            uint8_t* parsed = nullptr;
            int parsedSize = 0;
            (void) av_parser_parse2(parser.get(), context.get(), &parsed, &parsedSize, keyframe.data, keyframe.size,
                                    AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if ((parser->width > 0) && (parser->height > 0)) {
                parameters.width = parser->width;
                parameters.height = parser->height;
            }
        }
    }
}

FFmpegStreamRecorder::~FFmpegStreamRecorder()
{
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        _quit = true;
    }
    _wake.notify_one();
    if (_thread.joinable()) {
        _thread.join();
    }
}

void FFmpegStreamRecorder::record(const QString& file, VideoReceiver::FileFormat format, quint64 session)
{
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        _target = Target{.file = file, .format = format, .session = session};
        ++_targetVersion;
        if (!_thread.joinable()) {
            _thread = std::thread([this] { _run(); });
        }
    }
    _wake.notify_one();
}

void FFmpegStreamRecorder::stop()
{
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        if (!_target) {
            return;
        }
        _target.reset();
        ++_targetVersion;
        _queue.clear();
    }
    _wake.notify_one();
}

void FFmpegStreamRecorder::write(quint64 session, const std::shared_ptr<const Stream>& stream, const AVPacket& packet)
{
    const bool keyframe = (packet.flags & AV_PKT_FLAG_KEY) != 0;
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        if (!_target || (_target->session != session) || _quit) {
            return;
        }
        if (_awaitingKeyframe) {
            if (!keyframe) {
                return;
            }
            _awaitingKeyframe = false;
        }
        if (_queue.size() >= static_cast<size_t>(kMaxQueuedPackets)) {
            qCWarning(FFmpegVideoReceiverLog) << "Recording can't keep up: packets dropped up to the next keyframe";
            _awaitingKeyframe = true;
            return;
        }
        FFmpegPointers::Packet copy(av_packet_clone(&packet));
        if (!copy) {
            return;
        }
        _queue.push_back(Queued{.session = session, .stream = stream, .packet = std::move(copy)});
    }
    _wake.notify_one();
}

void FFmpegStreamRecorder::_run()
{
    std::unique_lock<std::mutex> lock(_mutex);
    while (true) {
        _wake.wait(lock, [this] { return _quit || !_queue.empty() || (_targetVersion != _currentTargetVersion); });
        if (_targetVersion != _currentTargetVersion) {
            const std::optional<Target> target = _target;
            _currentTargetVersion = _targetVersion;
            lock.unlock();
            _retarget(target);
            lock.lock();
            continue;
        }
        if (!_queue.empty()) {
            Queued queued = std::move(_queue.front());
            _queue.pop_front();
            lock.unlock();
            _writeQueued(queued);
            lock.lock();
            continue;
        }
        if (_quit) {
            break;
        }
    }
    lock.unlock();
    _close();
}

void FFmpegStreamRecorder::_retarget(const std::optional<Target>& target)
{
    _currentTarget = target;
    _targetRefused = false;
    if (_output && (!target || (target->session != _openSession))) {
        _close();
    }
}

void FFmpegStreamRecorder::_writeQueued(Queued& queued)
{
    if (!_currentTarget || _targetRefused || (queued.session != _currentTarget->session) || !queued.stream) {
        return;
    }
    AVPacket& packet = *queued.packet;
    if (!_output || (_openFile != _currentTarget->file)) {
        // A file opens at a keyframe; the session's file before it, an earlier part, takes the packets until then.
        if ((packet.flags & AV_PKT_FLAG_KEY) == 0) {
            if (!_output) {
                return;
            }
        } else {
            _close();
            if (!_open(*_currentTarget, *queued.stream, packet)) {
                return;
            }
        }
    }

    const AVRational inTimeBase{_inTimeBaseNum, _inTimeBaseDen};
    const AVRational outTimeBase = _output->streams[0]->time_base;
    const int64_t dts = (packet.dts != AV_NOPTS_VALUE) ? packet.dts : packet.pts;
    const int64_t pts = (packet.pts != AV_NOPTS_VALUE) ? packet.pts : dts;
    // A packet without a time can't be placed in the file.
    if (dts == AV_NOPTS_VALUE) {
        return;
    }
    if (!_anyWritten) {
        _firstDts = dts;
    }
    int64_t outDts = av_rescale_q(dts - _firstDts, inTimeBase, outTimeBase);
    int64_t outPts = av_rescale_q(pts - _firstDts, inTimeBase, outTimeBase);
    // A muxer refuses times that don't increase, which a jump in the stream's times or the rescaling can make.
    if (_anyWritten && (outDts <= _lastDts)) {
        outDts = _lastDts + 1;
    }
    outPts = std::max(outPts, outDts);
    packet.dts = outDts;
    packet.pts = outPts;
    packet.duration = av_rescale_q(packet.duration, inTimeBase, outTimeBase);
    packet.stream_index = 0;
    packet.pos = -1;
    const int result = av_write_frame(_output.get(), &packet);
    if (result < 0) {
        _targetRefused = true;
        _close(ffmpegRecorderError(result));
        return;
    }
    _lastDts = outDts;
    _anyWritten = true;
}

bool FFmpegStreamRecorder::_open(const Target& target, const Stream& stream, const AVPacket& keyframe)
{
    const QByteArray path = QFile::encodeName(target.file);
    // @p created: the file was created here, and is removed.
    const auto refuse = [this, &target](const QString& error, bool created = false) {
        qCWarning(FFmpegVideoReceiverLog) << "Can't record to" << target.file << ":" << error;
        _targetRefused = true;
        if (created) {
            (void) QFile::remove(target.file);
        }
        _listener.onRecordingFailed(target.file, error);
        return false;
    };

    AVFormatContext* context = nullptr;
    const int allocated =
        avformat_alloc_output_context2(&context, nullptr, ffmpegMuxerName(target.format), path.constData());
    FFmpegPointers::Output output(context);
    if ((allocated < 0) || !output) {
        return refuse(ffmpegRecorderError(allocated));
    }
    if ((target.format != VideoReceiver::FileFormat::Mkv) &&
        (avformat_query_codec(output->oformat, stream.parameters->codec_id, FF_COMPLIANCE_NORMAL) != 1)) {
        _targetRefused = true;
        _listener.onRecordingNeedsFormat(target.file, VideoReceiver::FileFormat::Mkv);
        return false;
    }

    AVStream* const outStream = avformat_new_stream(output.get(), nullptr);
    if (!outStream || (avcodec_parameters_copy(outStream->codecpar, stream.parameters.get()) < 0)) {
        return refuse(ffmpegRecorderError(AVERROR(ENOMEM)));
    }
    outStream->codecpar->codec_tag = 0;
    completeParameters(*outStream->codecpar, keyframe);
    outStream->time_base = AVRational{stream.timeBaseNum, stream.timeBaseDen};
    (void) av_dict_set(&output->metadata, "comment", _comment.constData(), 0);

    int result = avio_open(&output->pb, path.constData(), AVIO_FLAG_WRITE);
    if (result < 0) {
        return refuse(ffmpegRecorderError(result));
    }
    AVDictionary* options = nullptr;
    if (target.format != VideoReceiver::FileFormat::Mkv) {
        (void) av_dict_set(&options, "movflags", "+frag_keyframe+empty_moov+default_base_moof", 0);
    }
    result = avformat_write_header(output.get(), &options);
    av_dict_free(&options);
    if (result < 0) {
        (void) avio_closep(&output->pb);
        return refuse(ffmpegRecorderError(result), true);
    }

    _output = std::move(output);
    _openFile = target.file;
    _openSession = target.session;
    _inTimeBaseNum = stream.timeBaseNum;
    _inTimeBaseDen = stream.timeBaseDen;
    _anyWritten = false;
    qCDebug(FFmpegVideoReceiverLog) << "Recording to" << _openFile;
    _listener.onRecordingFileOpened(_openFile);
    return true;
}

void FFmpegStreamRecorder::_close(const QString& error)
{
    if (!_output) {
        return;
    }
    QString closeError = error;
    const int result = av_write_trailer(_output.get());
    if ((result < 0) && closeError.isEmpty()) {
        closeError = ffmpegRecorderError(result);
    }
    if (avio_closep(&_output->pb) < 0 && closeError.isEmpty()) {
        closeError = ffmpegRecorderError(AVERROR(EIO));
    }
    _output.reset();
    const QString file = std::exchange(_openFile, QString());
    qCDebug(FFmpegVideoReceiverLog) << "Recorded" << file << closeError;
    _listener.onRecordingFileClosed(file, closeError);
}
