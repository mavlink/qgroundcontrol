#include "QtMultimediaReceiver.h"
#include <QGCLoggingCategory.h>

#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QVideoSink>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaMetaData>


QGC_LOGGING_CATEGORY(QtMultimediaReceiverLog, "qgc.video.qtmultimedia.qtmultimediareceiver")

static void* createVideoSink(QObject *parent, QQuickItem *widget)
{
    Q_UNUSED(widget);
    QVideoSink* const videoSink = new QVideoSink(parent);
    /*if (widget) {
        QQuickVideoOutput* const videoOutput = reinterpret_cast<QQuickVideoOutput*>(widget);
        *videoOutput->videoSink();
    }*/
    return videoSink;
}

static void releaseVideoSink(void *sink)
{
    if (!sink) {
        return;
    }

    QVideoSink* const videoSink = reinterpret_cast<QVideoSink*>(sink);
    videoSink->deleteLater();
}

static VideoReceiver* createVideoReceiver(QObject *parent)
{
    return new QtMultimediaReceiver(parent);
}

QtMultimediaReceiver::QtMultimediaReceiver(QObject *parent)
    : VideoReceiver(parent)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_captureSession(new QMediaCaptureSession(this))
    , m_mediaRecorder(new QMediaRecorder(this))
    , m_frameTimer(new QTimer(this))
{
    m_captureSession->setRecorder(m_mediaRecorder);

    (void) connect(m_mediaPlayer, &QMediaPlayer::playingChanged, this, &QtMultimediaReceiver::streamingChanged);
    (void) connect(m_mediaPlayer, &QMediaPlayer::hasVideoChanged, this, &QtMultimediaReceiver::decodingChanged);
    (void) connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState newState){
        if (newState == QMediaPlayer::PlaybackState::PlayingState) {
            m_frameTimer->start();
        } else if (newState == QMediaPlayer::PlaybackState::StoppedState) {
            m_frameTimer->stop();
        }
    });
    (void) connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        switch (status) {
            case QMediaPlayer::MediaStatus::LoadingMedia:
                m_streamDevice = m_mediaPlayer->sourceDevice();
                break;

            default:
                break;
        }
    });
    (void) connect(m_mediaPlayer, &QMediaPlayer::metaDataChanged, this, [](){
        /*const QMediaMetaData metaData = m_mediaPlayer->metaData();
        const QVariant resolution = metaData.value(QMediaMetaData::Key::Resolution);
        const QSize videoSize = resolution.toSize();*/
    });
    (void) connect(m_mediaPlayer, &QMediaPlayer::bufferProgressChanged, this, [](float filled){
            qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << "Buffer Progress:" << filled;
    });
    (void) connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString){
        switch (error) {
            case QMediaPlayer::Error::NetworkError:
                break;

            default:
                break;
        }

        qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << errorString;
    });

    // m_mediaRecorder->setEncodingMode(QMediaRecorder::EncodingMode::AverageBitRateEncoding);
    // m_mediaRecorder->setQuality(QMediaRecorder::Quality::HighQuality);
    // m_mediaRecorder->setVideoBitRate()
    m_mediaRecorder->setVideoFrameRate(0);
    m_mediaRecorder->setVideoResolution(QSize());
    (void) connect(m_mediaRecorder, &QMediaRecorder::recorderStateChanged, this, [this](QMediaRecorder::RecorderState state){
        if (state == QMediaRecorder::RecorderState::RecordingState) {
            emit recordingStarted();
        }
        emit recordingChanged(m_mediaRecorder->recorderState() == QMediaRecorder::RecorderState::RecordingState);
    });
    (void) connect(m_mediaRecorder, &QMediaRecorder::errorOccurred, this, [this](QMediaRecorder::Error error, const QString &errorString){
        switch (error) {
            case QMediaRecorder::Error::OutOfSpaceError:
                break;

            default:
                break;
        }

        qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << errorString;
    });

    m_frameTimer->setSingleShot(true);
    m_frameTimer->setTimerType(Qt::PreciseTimer);
    (void) connect(m_frameTimer, &QTimer::timeout, this, &QtMultimediaReceiver::timeout);

    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << this;
}

QtMultimediaReceiver::~QtMultimediaReceiver()
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO << this;
}

void QtMultimediaReceiver::start(const QString &uri, unsigned timeout, int buffer)
{
    Q_UNUSED(buffer);

    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    if (m_mediaPlayer->isPlaying()) {
        qCDebug(QtMultimediaReceiverLog) << "Already running!";
        emit onStartComplete(STATUS_INVALID_STATE);
        return;
    }

    if (uri.isEmpty()) {
        qCDebug(QtMultimediaReceiverLog) << "Failed because URI is not specified";
        emit onStartComplete(STATUS_INVALID_URL);
        return;
    }
    m_mediaPlayer->setSource(uri);

    m_frameTimer->setInterval(timeout);

    // QAbstractVideoBuffer *buffer = m_videoSink->videoFrame()->videoBuffer();

    /*if (!m_mediaPlayer->hasVideo()) {
        emit onStartComplete(STATUS_FAIL);
    }*/

    m_mediaPlayer->play();

    qCDebug(QtMultimediaReceiverLog) << "Starting";

    emit onStartComplete(STATUS_OK);
}

void QtMultimediaReceiver::stop()
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    if (!m_mediaPlayer->isPlaying()) {
        qCDebug(QtMultimediaReceiverLog) << "Already stopped!";
        emit onStartComplete(STATUS_INVALID_STATE);
        return;
    }

    if (m_mediaPlayer->source().isEmpty()) {
        qCWarning(QtMultimediaReceiverLog) << "Stop called on empty URI";
        emit onStopComplete(STATUS_FAIL);
        return;
    }

    m_mediaPlayer->stop();

    qCDebug(QtMultimediaReceiverLog) << "Stopped";

    emit onStopComplete(STATUS_OK);
}

void QtMultimediaReceiver::startDecoding(void *sink)
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    if (sink == nullptr) {
        qCCritical(QtMultimediaReceiverLog) << "VideoSink is NULL";
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    if (m_videoSink) {
        qCWarning(QtMultimediaReceiverLog) << "VideoSink is already set";
    }

    if (m_videoSizeUpdater) {
        qCWarning(QtMultimediaReceiverLog) << "VideoSizeConnection is already set";
    }

    m_videoSink = reinterpret_cast<QVideoSink*>(sink);
    m_videoSizeUpdater = connect(m_videoSink, &QVideoSink::videoSizeChanged, this, [this](){
        emit videoSizeChanged(m_videoSink->videoSize());
    });
    m_videoFrameUpdater = connect(m_videoSink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame){
        if (frame.isValid()) {
            m_frameTimer->start();
        }
    });
    m_rhi = m_videoSink->rhi();
    m_videoSink->setSubtitleText("");

    m_mediaPlayer->setVideoSink(m_videoSink);

    qCDebug(QtMultimediaReceiverLog) << "Decoding";

    emit onStartDecodingComplete(STATUS_OK);
}

void QtMultimediaReceiver::stopDecoding()
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    if (m_videoSink == nullptr) {
        qCWarning(QtMultimediaReceiverLog) << "VideoSink is NULL";
        emit onStartDecodingComplete(STATUS_INVALID_STATE);
        return;
    }

    disconnect(m_videoSizeUpdater);
    m_mediaPlayer->setVideoSink(nullptr);
    m_videoSink = nullptr;

    qCDebug(QtMultimediaReceiverLog) << "Stopped Decoding";

    emit onStopDecodingComplete(STATUS_OK);
}

void QtMultimediaReceiver::startRecording(const QString &videoFile, FILE_FORMAT format)
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    if (!m_mediaRecorder->isAvailable()) {
        qCWarning(QtMultimediaReceiverLog) << "Recording Unavailable";
        emit onStartRecordingComplete(STATUS_FAIL);
        return;
    }

    switch (format) {
        case FILE_FORMAT_MKV:
            m_mediaRecorder->setMediaFormat(QMediaFormat::FileFormat::Matroska);
            break;

        case FILE_FORMAT_MOV:
            m_mediaRecorder->setMediaFormat(QMediaFormat::FileFormat::QuickTime);
            break;

        case FILE_FORMAT_MP4:
            m_mediaRecorder->setMediaFormat(QMediaFormat::FileFormat::MPEG4);
            break;

        default:
            // QMediaFormat::AVI, WMV, Ogg, WebM
            m_mediaRecorder->setMediaFormat(QMediaFormat::FileFormat::UnspecifiedFormat);
            break;
    }

    m_mediaRecorder->setOutputLocation(QUrl::fromLocalFile(videoFile));

    m_mediaRecorder->record();

    qCDebug(QtMultimediaReceiverLog) << "Recording";

    emit onStartRecordingComplete(STATUS_OK);
}

void QtMultimediaReceiver::stopRecording()
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    m_mediaRecorder->stop();

    qCDebug(QtMultimediaReceiverLog) << "Stopped Recording";

    emit onStopRecordingComplete(STATUS_OK);
}

void QtMultimediaReceiver::takeScreenshot(const QString &imageFile)
{
    qCDebug(QtMultimediaReceiverLog) << Q_FUNC_INFO;

    if (!m_videoSink) {
        qCWarning(QtMultimediaReceiverLog) << "Video Sink is NULL";
        emit onTakeScreenshotComplete(STATUS_FAIL);
    }

    const QVideoFrame frame = m_videoSink->videoFrame();
    if (frame.isValid() && frame.isReadable()) {
        // const QVideoFrameFormat frameFormat = frame.surfaceFormat();
        const QImage screenshot = frame.toImage();
    } else {
        qCWarning(QtMultimediaReceiverLog) << "Screenshot Frame is Invalid";
        emit onTakeScreenshotComplete(STATUS_FAIL);
    }

    // QQuickItem* const quickItem = reinterpret_cast<QQuickItem*>(m_mediaPlayer->videoOutput());
    // const QSize targetSize = m_mediaRecorder->videoResolution();
    // QSharedPointer<QQuickItemGrabResult> screenshot = quickItem->grabToImage(targetSize);
    // screenshot->saveToFile(imageFile);

    qCDebug(QtMultimediaReceiverLog) << "Screenshot";

    emit onTakeScreenshotComplete(STATUS_NOT_IMPLEMENTED);
}
