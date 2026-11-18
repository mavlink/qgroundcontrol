#include "VideoRecordingServiceTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThreadPool>
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
    _camera = new FakeVideoReceiver();
    _primaryStream->setReceiver(_primary);
    _thermalStream->setReceiver(_thermal);
    _cameraStream =
        new VideoStreamController(VideoStreamController::Role::Camera, videoSettings, _owner.get(), _camera);
    _service = new VideoRecordingService({_primaryStream, _thermalStream, _cameraStream}, _owner.get());
}

void VideoRecordingServiceTest::cleanup()
{
    _service = nullptr;
    _primary = nullptr;
    _thermal = nullptr;
    _camera = nullptr;
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
    QVERIFY(stream->receiver()->active());
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
    // The thermal stream's view is off, so nothing connects to it: it connects to be recorded.
    _thermalStream->setEnabled(true);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!_thermal->active());

    _service->startRecording(QStringLiteral("flight"));
    QCOMPARE(_primary->recordings.size(), 1);
    QCOMPARE(_primary->recordings.first().file, _videoDir + QStringLiteral("/flight.mkv"));
    QCOMPARE(_primary->recordings.first().format, VideoReceiver::FileFormat::Mkv);
    QVERIFY(_thermal->active());
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

void VideoRecordingServiceTest::_testCameraRecordsAsTheVideo()
{
    // Running in place of the primary stream, the camera gets its file name and its subtitles.
    _cameraStream->setCamera(QStringLiteral("QGC Test Camera"));
    _activate(_cameraStream);
    _service->startRecording(QStringLiteral("flight"));
    const QString file = _videoDir + QStringLiteral("/flight.mkv");
    QVERIFY(_primary->recordings.isEmpty());
    QCOMPARE(_camera->recordings.size(), 1);
    QCOMPARE(_camera->recordings.first().file, file);

    emit _camera->recordingStarted(file);
    _camera->setRecording(true);
    QVERIFY(_service->recording());
    QCOMPARE(_service->_subtitleWriter->videoFile(), file);

    _service->stopRecording();
    QCOMPARE(_camera->recordingStops, 1);
    _camera->setRecording(false);
    QVERIFY(!_service->recording());
    QVERIFY(!_service->_subtitleWriter->capturing());
}

void VideoRecordingServiceTest::_testTilesAreNotRecorded()
{
    _activate(_primaryStream);
    const auto tileInfo = VideoTestHelpers::makeStreamInfo(VIDEO_STREAM_TYPE_RTPUDP, "5602");
    auto* const tileStream = new VideoStreamController(VideoStreamController::Role::Tile,
                                                       SettingsManager::instance()->videoSettings(), _owner.get());
    auto* const tile = new FakeVideoReceiver();
    tileStream->setReceiver(tile);
    tileStream->setStreamInfo(tileInfo.get());
    tileStream->setTile(true);
    _service->addSource(tileStream);
    _activate(tileStream);

    _service->startRecording(QStringLiteral("flight"));
    QCOMPARE(_primary->recordings.size(), 1);
    QVERIFY(tile->recordings.isEmpty());

    // A source that moved to a tile while it records goes on recording, until the recording is stopped.
    _primary->setRecording(true);
    _primaryStream->setTile(true);
    QVERIFY(_service->recording());
    _service->stopRecording();
    QCOMPARE(_primary->recordingStops, 1);

    // A source that goes away takes its recording with it.
    tile->setRecording(true);
    _primary->setRecording(false);
    QVERIFY(_service->recording());
    delete tileStream;
    QVERIFY(!_service->recording());
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

void VideoRecordingServiceTest::_testPruneStorageLeavesFilesBeingWritten()
{
    const QString dir = _saveDir->path();
    const QDateTime now = QDateTime::currentDateTime();
    writeFile(dir + QStringLiteral("/flight.mkv"), 400, now.addSecs(-500));
    writeFile(dir + QStringLiteral("/flight.ass"), 100, now.addSecs(-400));
    writeFile(dir + QStringLiteral("/earlier.mkv"), 400, now.addSecs(-300));
    // A part its recording has yet to report: all that tells it is being written is how new it is.
    writeFile(dir + QStringLiteral("/flight_part2.mkv"), 400, now);

    // Far over the limit, and still only the file nobody writes goes, though it isn't the oldest.
    const QStringList removed = VideoRecordingService::pruneStorage(
        dir, 100, {dir + QStringLiteral("/flight.mkv"), dir + QStringLiteral("/flight.ass")});
    QCOMPARE(removed, QStringList{dir + QStringLiteral("/earlier.mkv")});
    QCOMPARE(QDir(dir).entryList(QDir::Files).size(), 3);
}

void VideoRecordingServiceTest::_testPruneStorageCountsOnlyRemovedFiles()
{
    const QString dir = _saveDir->path();
    const QDateTime now = QDateTime::currentDateTime();
    writeFile(dir + QStringLiteral("/a.mp4"), 400, now.addSecs(-300));
    writeFile(dir + QStringLiteral("/b.mp4"), 400, now.addSecs(-200));
    writeFile(dir + QStringLiteral("/c.mp4"), 400, now.addSecs(-100));

    // Nothing can be removed from a directory that can't be written.
    const QFileDevice::Permissions writable = QFile::permissions(dir);
    QVERIFY(QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    const auto restorePermissions = qScopeGuard([&dir, writable]() { (void) QFile::setPermissions(dir, writable); });
    if (QFile(dir + QStringLiteral("/probe")).open(QIODevice::WriteOnly)) {
        QSKIP("Directory permissions aren't enforced (running as root, or on a file system without them)");
    }

    // One file over the limit: a file that stays doesn't count as freed, so the next oldest is tried, to the last.
    const QRegularExpression failedToRemove(QStringLiteral("Failed to remove old recording"));
    for (int i = 0; i < 3; i++) {
        expectLogMessage("Video.VideoRecordingService", QtWarningMsg, failedToRemove);
    }
    const QStringList removed = VideoRecordingService::pruneStorage(dir, 900);
    for (int i = 0; i < 3; i++) {
        verifyExpectedLogMessage();
    }
    QVERIFY(removed.isEmpty());
}

void VideoRecordingServiceTest::_testStorageLimitAppliesWhileRecording()
{
    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    _facts->set(videoSettings->enableStorageLimit(), true);
    _facts->set(videoSettings->maxVideoSize(), 0);
    const QDateTime old = QDateTime::currentDateTime().addSecs(-500);
    const QString earlier = _videoDir + QStringLiteral("/earlier.mkv");
    const QString part1 = _videoDir + QStringLiteral("/flight.mkv");
    const QString part2 = _videoDir + QStringLiteral("/flight_part2.mkv");
    writeFile(earlier, 10, old);
    writeFile(part1, 10, old);
    // What the timer does once a minute while recording, and waits for the thread pool to have done it.
    const auto applyLimit = [this] {
        _service->_applyStorageLimit();
        return QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs());
    };

    emit _primary->recordingStarted(part1);
    _primary->setRecording(true);
    QVERIFY(_service->_storageLimitTimer.isActive());
    QVERIFY(applyLimit());
    QVERIFY(!QFile::exists(earlier));
    QVERIFY(QFile::exists(part1));

    // A long recording's closed part makes room for the one it continues in.
    emit _primary->recordingFinished(part1, QString());
    writeFile(part2, 10, old);
    emit _primary->recordingStarted(part2);
    QVERIFY(applyLimit());
    QVERIFY(!QFile::exists(part1));
    QVERIFY(QFile::exists(part2));

    emit _primary->recordingFinished(part2, QString());
    _primary->setRecording(false);
    QVERIFY(!_service->_storageLimitTimer.isActive());
}

UT_REGISTER_TEST(VideoRecordingServiceTest, TestLabel::Unit)
