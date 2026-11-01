#include "QtVideoRecorderTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMetaEnum>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameInput>
#include <QtTest/QSignalSpy>

#include "QtVideoRecorder.h"

void QtVideoRecorderTest::_ignoreEncoderFallbacks()
{
    ignoreLogMessage("qt.multimedia.ffmpeg.encoder", QtInfoMsg,
                     QRegularExpression(QStringLiteral("^Recording new media")));
    ignoreLogMessage(
        "default", QtWarningMsg,
        QRegularExpression(QStringLiteral("^(Cannot find software target format|Could not find target format)")));
    ignoreLogMessage("qt.multimedia.ffmpeg.videoencoder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No valid video codecs found")));
    ignoreLogMessage("Video.QtVideoRecorder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .* \"Could not initialize encoder\"$")));
}

void QtVideoRecorderTest::_testRecordingFormat_data()
{
    QTest::addColumn<QMediaFormat::FileFormat>("container");
    QTest::newRow("mkv") << QMediaFormat::Matroska;
    QTest::newRow("mov") << QMediaFormat::QuickTime;
    QTest::newRow("mp4") << QMediaFormat::MPEG4;
}

void QtVideoRecorderTest::_testRecordingFormat()
{
    QFETCH(QMediaFormat::FileFormat, container);

    // Whatever this platform's Qt encodes, best first: H.264 where Qt counts it encodable, in the container asked for
    // where Qt can write it.
    const QList<QMediaFormat> formats = QtVideoRecorder::recordingFormats(container);
    QVERIFY(!formats.isEmpty());
    const QMediaFormat::FileFormat used = formats.first().fileFormat();
    for (const QMediaFormat& format : formats) {
        QVERIFY(format.isSupported(QMediaFormat::Encode));
        QCOMPARE(format.fileFormat(), used);
        QCOMPARE(formats.count(format), 1);
    }
    QMediaFormat h264(used);
    h264.setVideoCodec(QMediaFormat::VideoCodec::H264);
    if (h264.isSupported(QMediaFormat::Encode)) {
        QCOMPARE(formats.first().videoCodec(), QMediaFormat::VideoCodec::H264);
    }
    QMediaFormat asked(container);
    asked.setVideoCodec(formats.first().videoCodec());
    if (asked.isSupported(QMediaFormat::Encode)) {
        QCOMPARE(used, container);
    }
}

void QtVideoRecorderTest::_testRecordingFormatTriesCodecFirst()
{
    const QStringList codecs = QtVideoRecorder::recordingCodecs();
    QVERIFY(!codecs.isEmpty());
    const QList<QMediaFormat> fallbacks = QtVideoRecorder::recordingFormats(QMediaFormat::Matroska);
    for (const QString& key : codecs) {
        const QMediaFormat::VideoCodec codec = QtVideoRecorder::recordingCodec(key);
        const QList<QMediaFormat> formats = QtVideoRecorder::recordingFormats(QMediaFormat::Matroska, codec);
        QVERIFY2(!formats.isEmpty(), qPrintable(key));
        QCOMPARE(formats.first().videoCodec(), codec);
        QVERIFY(formats.first().isSupported(QMediaFormat::Encode));
        // The fallbacks follow, in their order, without the codec's own format again.
        QList<QMediaFormat> rest = fallbacks;
        rest.removeAll(formats.first());
        QCOMPARE(formats.mid(1), rest);
    }
    QCOMPARE(QtVideoRecorder::recordingFormats(QMediaFormat::Matroska, QMediaFormat::VideoCodec::Unspecified),
             fallbacks);
}

void QtVideoRecorderTest::_testRecordingCodecs()
{
    // Each container's fallbacks appear in the list in the order a recording tries them. The list covers every
    // container, so it can hold codecs a given container can't.
    const QStringList codecs = QtVideoRecorder::recordingCodecs();
    for (const QMediaFormat::FileFormat container :
         {QMediaFormat::MPEG4, QMediaFormat::Matroska, QMediaFormat::QuickTime}) {
        qsizetype previous = -1;
        for (const QMediaFormat& format : QtVideoRecorder::recordingFormats(container)) {
            const qsizetype index = codecs.indexOf(QString::fromLatin1(
                QMetaEnum::fromType<QMediaFormat::VideoCodec>().valueToKey(static_cast<int>(format.videoCodec()))));
            QVERIFY(index > previous);
            previous = index;
        }
    }
    for (const QString& key : codecs) {
        QCOMPARE(codecs.count(key), 1);
        QVERIFY(!QtVideoRecorder::recordingCodecName(key).isEmpty());
    }
    QCOMPARE(QtVideoRecorder::recordingCodec(QStringLiteral("H264")), QMediaFormat::VideoCodec::H264);
    QCOMPARE(QtVideoRecorder::recordingCodec(QString()), QMediaFormat::VideoCodec::Unspecified);
    QCOMPARE(QtVideoRecorder::recordingCodec(QStringLiteral("NoSuchCodec")), QMediaFormat::VideoCodec::Unspecified);
}

void QtVideoRecorderTest::_testContainers()
{
    QCOMPARE(QtVideoRecorder::container(VideoReceiver::FileFormat::Mkv), QMediaFormat::Matroska);
    QCOMPARE(QtVideoRecorder::container(VideoReceiver::FileFormat::Mov), QMediaFormat::QuickTime);
    QCOMPARE(QtVideoRecorder::container(VideoReceiver::FileFormat::Mp4), QMediaFormat::MPEG4);
}

void QtVideoRecorderTest::_testRecordsFrameInput()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    _ignoreEncoderFallbacks();
    QMediaCaptureSession session;
    QVideoFrameInput input;
    session.setVideoFrameInput(&input);
    QtVideoRecorder recorder;
    session.setRecorder(recorder.mediaRecorder());
    QSignalSpy startedSpy(&recorder, &QtVideoRecorder::recordingStarted);
    QSignalSpy finishedSpy(&recorder, &QtVideoRecorder::recordingFinished);

    recorder.start(tempDir.filePath(QStringLiteral("frames.mkv")), QMediaFormat::Matroska);
    QVERIFY(recorder.active());
    QVERIFY(!recorder.recording());

    // 30 fps frames, as a live stream sends them, until a second after the recording started: the encoders this
    // platform lacks fail on the first frames, and the recorder moves on to the next format.
    QImage image(QSize(320, 240), QImage::Format_RGBA8888);
    constexpr qint64 kFrameUs = 1000000 / 30;
    QElapsedTimer sinceStart;
    sinceStart.start();
    qint64 frames = 0;
    qint64 framesRecorded = 0;
    while (framesRecorded < 30) {
        QVERIFY2(sinceStart.elapsed() < TestTimeout::longMs(), "The recording didn't start");
        image.fill(QColor::fromHsv(static_cast<int>((frames * 12) % 360), 255, 255));
        QVideoFrame frame(image);
        frame.setStartTime(frames * kFrameUs);
        frame.setEndTime((frames + 1) * kFrameUs);
        if (input.sendVideoFrame(frame)) {
            frames++;
            if (recorder.recording()) {
                framesRecorded++;
            }
        } else {
            QSignalSpy ready(&input, &QVideoFrameInput::readyToSendVideoFrame);
            (void) ready.wait(TestTimeout::shortMs() / 10);
        }
    }
    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(recorder.recording());
    const QString file = startedSpy.first().first().toString();

    recorder.stop();
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::longMs());
    QCOMPARE(finishedSpy.first().at(0).toString(), file);
    QVERIFY2(finishedSpy.first().at(1).toString().isEmpty(), qPrintable(finishedSpy.first().at(1).toString()));
    QVERIFY(!recorder.recording());
    QVERIFY(!recorder.active());

    QMediaPlayer player;
    player.setSource(QUrl::fromLocalFile(file));
    QTRY_VERIFY_WITH_TIMEOUT(player.mediaStatus() == QMediaPlayer::LoadedMedia, TestTimeout::mediumMs());
    QVERIFY2(player.duration() >= 900, qPrintable(QString::number(player.duration())));
}

void QtVideoRecorderTest::_testQueuedRetryHonorsStop()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QMediaCaptureSession session;
    QVideoFrameInput input;
    session.setVideoFrameInput(&input);
    QtVideoRecorder recorder;
    session.setRecorder(recorder.mediaRecorder());
    QSignalSpy finishedSpy(&recorder, &QtVideoRecorder::recordingFinished);

    // A format failed with others left to try: the next one is queued...
    recorder._pendingFormats = QtVideoRecorder::recordingFormats(QMediaFormat::Matroska);
    QVERIFY(!recorder._pendingFormats.isEmpty());
    recorder._recordingBase = tempDir.filePath(QStringLiteral("failed"));
    recorder._recordingError = QStringLiteral("encoder failed");
    recorder._onRecordingNotStarted();

    // ...and a stop before it runs leaves the recorder stopped.
    recorder.stop();
    QCoreApplication::sendPostedEvents(&recorder, QEvent::MetaCall);
    QVERIFY(!recorder.active());
    QVERIFY(finishedSpy.isEmpty());
}

void QtVideoRecorderTest::_testQueuedRetryYieldsToNewStart()
{
    _ignoreEncoderFallbacks();
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QMediaCaptureSession session;
    QVideoFrameInput input;
    session.setVideoFrameInput(&input);
    QtVideoRecorder recorder;
    session.setRecorder(recorder.mediaRecorder());

    recorder._pendingFormats = QtVideoRecorder::recordingFormats(QMediaFormat::Matroska);
    recorder._recordingBase = tempDir.filePath(QStringLiteral("failed"));
    recorder._recordingError = QStringLiteral("encoder failed");
    recorder._onRecordingNotStarted();

    // A new recording starts before the queued retry runs, which then leaves its file and formats alone.
    recorder.start(tempDir.filePath(QStringLiteral("new.mkv")), QMediaFormat::Matroska);
    QVERIFY(recorder.active());
    const QString file = recorder._recordingFile;
    const qsizetype pending = recorder._pendingFormats.size();
    QCoreApplication::sendPostedEvents(&recorder, QEvent::MetaCall);
    QCOMPARE(recorder._recordingFile, file);
    QCOMPARE(recorder._pendingFormats.size(), pending);

    recorder.stop();
    QTRY_VERIFY_WITH_TIMEOUT(!recorder.active(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(QtVideoRecorderTest, TestLabel::Integration)
