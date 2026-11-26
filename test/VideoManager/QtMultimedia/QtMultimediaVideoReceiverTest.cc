#include "QtMultimediaVideoReceiverTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtTest/QSignalSpy>

#include "QtMultimediaReceiverTestSupport.h"
#include "QtMultimediaVideoReceiver.h"
#include "QtVideoRecorder.h"
#include "VideoTestHelpers.h"

using QtMultimediaTest::ReceiverUnderTest;
using VideoTestHelpers::freeUdpPort;

void QtMultimediaVideoReceiverTest::sampleOften(QtMultimediaVideoReceiver& receiver)
{
    receiver._sampleTimer.setInterval(std::chrono::milliseconds(200));
}

void QtMultimediaVideoReceiverTest::_ignorePlayerLogs()
{
    for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::playerLogs()) {
        ignoreLogMessage(log.category, log.type, log.pattern);
    }
}

void QtMultimediaVideoReceiverTest::_ignoreEncoderFallbacks()
{
    for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::encoderFallbackLogs()) {
        ignoreLogMessage(log.category, log.type, log.pattern);
    }
}

void QtMultimediaVideoReceiverTest::_testUnplayableUri()
{
    // The URI's credentials stay out of the log and the message.
    ReceiverUnderTest stream(QStringLiteral("udp://user:secret@0.0.0.0"));
    expectLogMessage("Video.QtMultimediaVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^(?!.*secret).*can't play")));
    stream.receiver->setActive(true);
    verifyExpectedLogMessage();
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Stopped);
    QCOMPARE(stream.errorSpy.count(), 1);
    QVERIFY(!stream.errorSpy.first().first().toString().contains(QStringLiteral("secret")));
}

void QtMultimediaVideoReceiverTest::_testRecordingNeedsActiveStream()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file = dir.filePath(QStringLiteral("stream.mkv"));

    ReceiverUnderTest stream(QStringLiteral("udp://127.0.0.1:%1").arg(freeUdpPort()));
    expectLogMessage("Video.VideoReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("not active")));
    stream.receiver->startRecording(file, VideoReceiver::FileFormat::Mkv);
    verifyExpectedLogMessage();
    QCOMPARE(stream.errorSpy.count(), 1);
    QVERIFY(!stream.receiver->recording());
    QVERIFY(!QFile::exists(file));
}

void QtMultimediaVideoReceiverTest::_testStartWhileRecordingIsRefused()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString second = dir.filePath(QStringLiteral("second.mkv"));

    _ignorePlayerLogs();
    _ignoreEncoderFallbacks();
    // Nobody sends the stream, so the recorder waits for its first frame.
    ReceiverUnderTest stream(QStringLiteral("udp://127.0.0.1:%1").arg(freeUdpPort()));
    stream.receiver->setActive(true);
    stream.receiver->startRecording(dir.filePath(QStringLiteral("first.mkv")), VideoReceiver::FileFormat::Mkv);
    const QtVideoRecorder* const recorder = stream.receiver->_recorder;
    QVERIFY(recorder->active());
    const QUrl recorded = recorder->mediaRecorder()->outputLocation();

    // Whoever asks for another recording meanwhile is told that its file isn't recorded: the first one goes on.
    QSignalSpy finishedSpy(stream.receiver.get(), &VideoReceiver::recordingFinished);
    expectLogMessage("Video.QtMultimediaVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("recording already")));
    stream.receiver->startRecording(second, VideoReceiver::FileFormat::Mkv);
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().at(0).toString(), second);
    QVERIFY(!finishedSpy.first().at(1).toString().isEmpty());
    QVERIFY(recorder->active());
    QVERIFY(stream.receiver->recording());
    QCOMPARE(recorder->mediaRecorder()->outputLocation(), recorded);
    QVERIFY(!QFile::exists(second));
    QVERIFY(stream.errorSpy.isEmpty());
}

void QtMultimediaVideoReceiverTest::_testClosingEndsRecordingNotStarted()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file = dir.filePath(QStringLiteral("first.mkv"));

    _ignorePlayerLogs();
    _ignoreEncoderFallbacks();
    {
        // Nobody sends the stream, so the recorder waits for its first frame, with the file open.
        ReceiverUnderTest stream(QStringLiteral("udp://127.0.0.1:%1").arg(freeUdpPort()));
        stream.receiver->setActive(true);
        stream.receiver->startRecording(file, VideoReceiver::FileFormat::Mkv);
        QVERIFY(stream.receiver->_recorder->active());
        QVERIFY(QFile::exists(file));
    }
    // The receiver waited for the recorder to close the file, which is then removed as any recording without a frame.
    QVERIFY(!QFile::exists(file));
}

void QtMultimediaVideoReceiverTest::_testRecordingUsesEncodingSet()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // The last of the codecs Qt encodes here: not the one a recording tries first when none is set.
    const QStringList codecs = QtVideoRecorder::recordingCodecs();
    QVERIFY(!codecs.isEmpty());
    const VideoReceiver::RecordingEncoding encoding{.codec = QtVideoRecorder::recordingCodec(codecs.last()),
                                                    .quality = QMediaRecorder::LowQuality};

    _ignorePlayerLogs();
    _ignoreEncoderFallbacks();
    // Nobody sends the stream, so the recorder waits for its first frame, set up as the recording was started.
    ReceiverUnderTest stream(QStringLiteral("udp://127.0.0.1:%1").arg(freeUdpPort()));
    stream.receiver->setActive(true);
    stream.receiver->startRecording(dir.filePath(QStringLiteral("first.mkv")), VideoReceiver::FileFormat::Mkv,
                                    encoding);
    const QtVideoRecorder* const recorder = stream.receiver->_recorder;
    QVERIFY(recorder->active());
    QCOMPARE(recorder->mediaRecorder()->mediaFormat().videoCodec(), encoding.codec);
    QCOMPARE(recorder->mediaRecorder()->quality(), encoding.quality);

    // The next recording is encoded as it is told, not as the one before.
    stream.receiver->stopRecording();
    QTRY_VERIFY_WITH_TIMEOUT(!recorder->active(), TestTimeout::mediumMs());
    stream.receiver->startRecording(dir.filePath(QStringLiteral("next.mkv")), VideoReceiver::FileFormat::Mkv,
                                    {.quality = QMediaRecorder::VeryHighQuality});
    QVERIFY(recorder->active());
    QCOMPARE(recorder->mediaRecorder()->mediaFormat().videoCodec(),
             QtVideoRecorder::recordingFormats(QMediaFormat::Matroska).first().videoCodec());
    QCOMPARE(recorder->mediaRecorder()->quality(), QMediaRecorder::VeryHighQuality);
    QVERIFY(stream.errorSpy.isEmpty());
}

void QtMultimediaVideoReceiverTest::_testRecordingTimes_data()
{
    QTest::addColumn<qint64>("lastRecordedUs");
    QTest::addColumn<qint64>("lastStreamUs");
    QTest::addColumn<qint64>("streamUs");
    QTest::addColumn<qint64>("arrivalUs");
    QTest::addColumn<qint64>("expectedUs");

    QTest::newRow("first frame") << qint64(-1) << qint64(-1) << qint64(5000000) << qint64(0) << qint64(0);
    QTest::newRow("stream spacing") << qint64(100000) << qint64(5000000) << qint64(5033333) << qint64(140000)
                                    << qint64(133333);
    // Frames the GUI thread took together keep the stream's spacing.
    QTest::newRow("arrived together") << qint64(100000) << qint64(5000000) << qint64(5033333) << qint64(100010)
                                      << qint64(133333);
    // A jump in the stream's times, or a frame without one, goes by its arrival...
    QTest::newRow("stream jumped") << qint64(100000) << qint64(5000000) << qint64(90000000) << qint64(400000)
                                   << qint64(400000);
    QTest::newRow("stream went back") << qint64(100000) << qint64(5000000) << qint64(0) << qint64(400000)
                                      << qint64(400000);
    QTest::newRow("no stream time") << qint64(100000) << qint64(5000000) << qint64(-1) << qint64(140000)
                                    << qint64(140000);
    // ...but never sooner than a millisecond after the last frame, which the encoder couldn't tell apart.
    QTest::newRow("no time to tell apart")
        << qint64(100000) << qint64(-1) << qint64(-1) << qint64(100000) << qint64(101000);
    QTest::newRow("same stream time") << qint64(100000) << qint64(5000000) << qint64(5000000) << qint64(100000)
                                      << qint64(101000);
}

void QtMultimediaVideoReceiverTest::_testRecordingTimes()
{
    QFETCH(qint64, lastRecordedUs);
    QFETCH(qint64, lastStreamUs);
    QFETCH(qint64, streamUs);
    QFETCH(qint64, arrivalUs);
    QFETCH(qint64, expectedUs);

    QCOMPARE(QtMultimediaVideoReceiver::recordingTimeUs(lastRecordedUs, lastStreamUs, streamUs, arrivalUs), expectedUs);
}

UT_REGISTER_TEST(QtMultimediaVideoReceiverTest, TestLabel::Integration)
