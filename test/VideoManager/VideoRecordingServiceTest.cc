#include "VideoRecordingServiceTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "SubtitleWriter.h"
#include "VideoRecordingService.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

namespace {

void writeFile(const QString& path, qsizetype size, const QDateTime& modified)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(QByteArray(size, 'x')), size);
    QVERIFY(file.flush());
    QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
}

}  // namespace

void VideoRecordingServiceTest::init()
{
    UnitTest::init();
    // The subtitle writer reads the telemetry bar layout, which needs the offline vehicle.
    MultiVehicleManager::instance()->init();

    _saveDir = std::make_unique<QTemporaryDir>();
    QVERIFY(_saveDir->isValid());

    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    _facts = std::make_unique<VideoTestHelpers::FactRestorer>();
    _facts->set(appSettings->savePath(), _saveDir->path());
    _facts->set(videoSettings->videoSource(), QString(VideoSettings::videoSourceUDPH264));
    _facts->set(videoSettings->udpUrl(), QStringLiteral("0.0.0.0:5600"));
    _facts->set(videoSettings->recordingFormat(), static_cast<int>(VideoReceiver::FileFormat::Mkv));
    _facts->set(videoSettings->enableStorageLimit(), false);
    _videoDir = appSettings->videoSavePath();
    QVERIFY(QDir().mkpath(_videoDir));

    _owner = std::make_unique<QObject>();
    _primaryStream = new VideoStreamController(VideoStreamController::Role::Primary, videoSettings, _owner.get());
    _thermalStream = new VideoStreamController(VideoStreamController::Role::Thermal, videoSettings, _owner.get());
    _primary = new FakeVideoReceiver();
    _thermal = new FakeVideoReceiver();
    _primaryStream->setReceiver(_primary);
    _thermalStream->setReceiver(_thermal);
    _service = new VideoRecordingService(_primaryStream, _thermalStream, _owner.get());
}

void VideoRecordingServiceTest::cleanup()
{
    _service = nullptr;
    _primary = nullptr;
    _thermal = nullptr;
    _owner.reset();
    _facts.reset();
    _saveDir.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);

    UnitTest::cleanup();
}

void VideoRecordingServiceTest::_activate(VideoStreamController* stream)
{
    stream->setEnabled(true);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(stream->active());
}

void VideoRecordingServiceTest::_testUniqueBaseName()
{
    const QDateTime time(QDate(2026, 1, 2), QTime(3, 4, 5));
    const QString dir = _saveDir->path();
    QCOMPARE(VideoRecordingService::uniqueBaseName(dir, time), QStringLiteral("2026-01-02_03.04.05"));

    // Any file of that name is taken, a thermal recording or a subtitle file too.
    writeFile(dir + QStringLiteral("/2026-01-02_03.04.05.thermalVideo.mkv"), 1, time);
    QCOMPARE(VideoRecordingService::uniqueBaseName(dir, time), QStringLiteral("2026-01-02_03.04.05_2"));
    writeFile(dir + QStringLiteral("/2026-01-02_03.04.05_2.ass"), 1, time);
    QCOMPARE(VideoRecordingService::uniqueBaseName(dir, time), QStringLiteral("2026-01-02_03.04.05_3"));
}

void VideoRecordingServiceTest::_testRecordsActiveStreams()
{
    _activate(_primaryStream);
    const auto thermalInfo = VideoTestHelpers::makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal",
                                                              VIDEO_STREAM_ENCODING_H264, true);
    _thermalStream->setStreamInfo(thermalInfo.get());
    _activate(_thermalStream);

    _service->startRecording(QStringLiteral("flight"));
    QCOMPARE(_primary->recordings.size(), 1);
    QCOMPARE(_primary->recordings.first().file, _videoDir + QStringLiteral("/flight.mkv"));
    QCOMPARE(_primary->recordings.first().format, VideoReceiver::FileFormat::Mkv);
    QCOMPARE(_thermal->recordings.size(), 1);
    QCOMPARE(_thermal->recordings.first().file, _videoDir + QStringLiteral("/flight.thermalVideo.mkv"));

    // A generated name is shared by both streams.
    _service->startRecording();
    const QString primaryFile = _primary->recordings.last().file;
    QVERIFY(primaryFile.endsWith(QStringLiteral(".mkv")));
    QCOMPARE(_thermal->recordings.last().file, primaryFile.chopped(4) + QStringLiteral(".thermalVideo.mkv"));

    QSignalSpy recordingSpy(_service, &VideoRecordingService::recordingChanged);
    _thermal->setRecording(true);
    _primary->setRecording(true);
    QVERIFY(_service->recording());
    _thermal->setRecording(false);
    QVERIFY(_service->recording());
    _primary->setRecording(false);
    QVERIFY(!_service->recording());
    QCOMPARE(recordingSpy.count(), 2);

    _service->stopRecording();
    QCOMPARE(_primary->recordingStops, 1);
    QCOMPARE(_thermal->recordingStops, 1);
}

void VideoRecordingServiceTest::_testNothingToRecord()
{
    expectAppMessage(QRegularExpression(QStringLiteral("no video stream is active")));
    _service->startRecording();
    verifyExpectedLogMessage();
    QVERIFY(_primary->recordings.isEmpty());
    QVERIFY(_thermal->recordings.isEmpty());
}

void VideoRecordingServiceTest::_testSubtitlesFollowEachFile()
{
    const SubtitleWriter* const subtitles = _service->_subtitleWriter;
    const QString file = _videoDir + QStringLiteral("/flight.mkv");
    const QString part2 = _videoDir + QStringLiteral("/flight_part2.mkv");

    emit _primary->recordingStarted(file);
    QVERIFY(subtitles->capturing());
    QCOMPARE(subtitles->videoFile(), file);
    QVERIFY(QFile::exists(_videoDir + QStringLiteral("/flight.ass")));

    // After a reconnect the recording continues into a new part, with its own subtitle file.
    emit _primary->recordingStarted(part2);
    QCOMPARE(subtitles->videoFile(), part2);
    QVERIFY(QFile::exists(_videoDir + QStringLiteral("/flight_part2.ass")));
    emit _primary->recordingFinished(file, QString());
    QVERIFY(subtitles->capturing());

    // The thermal stream has no subtitles.
    emit _thermal->recordingStarted(_videoDir + QStringLiteral("/flight.thermalVideo.mkv"));
    QCOMPARE(subtitles->videoFile(), part2);

    emit _primary->recordingFinished(part2, QString());
    QVERIFY(!subtitles->capturing());
}

void VideoRecordingServiceTest::_testRecordingFailureReachesUser()
{
    expectLogMessage("Video.VideoRecordingService", QtWarningMsg, QRegularExpression(QStringLiteral("disk full")));
    expectAppMessage(QRegularExpression(QStringLiteral("flight\\.thermalVideo\\.mkv failed: disk full")));
    emit _thermal->recordingFinished(_videoDir + QStringLiteral("/flight.thermalVideo.mkv"),
                                     QStringLiteral("disk full"));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void VideoRecordingServiceTest::_testPruneStorage()
{
    const QString dir = _saveDir->path();
    const QDateTime now = QDateTime::currentDateTime();
    writeFile(dir + QStringLiteral("/notes.txt"), 1000, now.addSecs(-500));
    writeFile(dir + QStringLiteral("/a.ass"), 100, now.addSecs(-400));
    writeFile(dir + QStringLiteral("/a.mp4"), 400, now.addSecs(-300));
    writeFile(dir + QStringLiteral("/b.mkv"), 400, now.addSecs(-200));

    // 900 bytes of recordings and subtitles: the oldest go until less than 600 remain. Other files don't count.
    const QStringList removed = VideoRecordingService::pruneStorage(dir, 600);
    QCOMPARE(removed, (QStringList{dir + QStringLiteral("/a.ass"), dir + QStringLiteral("/a.mp4")}));
    QVERIFY(QFile::exists(dir + QStringLiteral("/b.mkv")));
    QVERIFY(QFile::exists(dir + QStringLiteral("/notes.txt")));
}

UT_REGISTER_TEST(VideoRecordingServiceTest, TestLabel::Unit)
