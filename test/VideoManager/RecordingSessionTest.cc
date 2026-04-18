#include "RecordingSessionTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QSaveFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "RecordingSession.h"
#include "VideoRecorder.h"

// ─── Stub Recorder ────────────────────────────────────────────────────────────
// Minimal VideoRecorder that succeeds on start() and fires stopped() on stop().

class StubRecorder : public VideoRecorder
{
    Q_OBJECT
public:
    explicit StubRecorder(QObject* parent = nullptr) : VideoRecorder(parent) {}

    bool start(const QString& path, QMediaFormat::FileFormat /*format*/) override
    {
        if (_failNextStart) {
            _failNextStart = false;
            return false;
        }
        _currentPath = path;
        setState(State::Recording);
        emit started(path);
        return true;
    }

    void stop() override
    {
        if (_state == State::Idle)
            return;
        const QString p = _currentPath;
        setState(State::Idle);
        emit stopped(p);
    }

    [[nodiscard]] Capabilities capabilities() const override
    {
        return Capabilities{false, {QMediaFormat::MPEG4}, QStringLiteral("stub")};
    }

    bool _failNextStart = false;
};

// ─── Tests ────────────────────────────────────────────────────────────────────

void RecordingSessionTest::_testStartCreatesManifest()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    RecordingSession session;

    const QString videoPath = tmp.filePath(QStringLiteral("video.mp4"));

    RecordingSession::StreamEntry entry;
    entry.role     = QStringLiteral("videoContent");
    entry.path     = videoPath;
    entry.recorder = std::make_unique<StubRecorder>();
    entry.format   = QMediaFormat::MPEG4;

    std::vector<RecordingSession::StreamEntry> entries;
    entries.push_back(std::move(entry));

    const bool ok = session.start(tmp.path(), std::move(entries));
    QVERIFY2(ok, "start() should return true");
    QVERIFY2(!session.manifestPath().isEmpty(), "manifestPath should be set after start()");
    QVERIFY2(QFile::exists(session.manifestPath()), "Manifest file should exist on disk after start()");
}

void RecordingSessionTest::_testStopCleansUpManifest()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    RecordingSession session;

    const QString videoPath = tmp.filePath(QStringLiteral("video.mp4"));

    RecordingSession::StreamEntry entry;
    entry.role     = QStringLiteral("videoContent");
    entry.path     = videoPath;
    entry.recorder = std::make_unique<StubRecorder>();
    entry.format   = QMediaFormat::MPEG4;

    std::vector<RecordingSession::StreamEntry> entries;
    entries.push_back(std::move(entry));

    QVERIFY(session.start(tmp.path(), std::move(entries)));
    const QString manifestPath = session.manifestPath();
    QVERIFY(QFile::exists(manifestPath));

    QSignalSpy stoppedSpy(&session, &RecordingSession::stopped);

    session.stop();

    // stop() is synchronous (StubRecorder::stop emits stopped() synchronously)
    QCoreApplication::processEvents();

    QVERIFY2(!QFile::exists(manifestPath), "Manifest should be deleted after clean stop()");
    QVERIFY2(!session.isActive(), "Session should be inactive after stop()");
    QCOMPARE(stoppedSpy.count(), 1);
}

void RecordingSessionTest::_testScanForOrphansFindsIncomplete()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Write a synthetic orphaned manifest (stopTimestamp: null)
    QJsonObject root;
    root[QStringLiteral("version")]        = 1;
    root[QStringLiteral("startTimestamp")] = QStringLiteral("2026-01-01T00:00:00.000Z");
    root[QStringLiteral("stopTimestamp")]  = QJsonValue(QJsonValue::Null);
    root[QStringLiteral("vehicleUid")]     = QStringLiteral("test-vehicle");
    root[QStringLiteral("subtitleFile")]   = QString{};

    // Reference a dummy (non-existent/empty) video file — will be unplayable
    const QString fakeVideoPath = tmp.filePath(QStringLiteral("crashed.mp4"));
    QJsonArray streamsArr;
    QJsonObject s;
    s[QStringLiteral("role")]   = QStringLiteral("videoContent");
    s[QStringLiteral("path")]   = fakeVideoPath;
    s[QStringLiteral("format")] = QStringLiteral("mp4");
    s[QStringLiteral("status")] = QStringLiteral("recording");
    streamsArr.append(s);
    root[QStringLiteral("streams")] = streamsArr;

    const QString manifestName = QStringLiteral("session-2026-01-01T00-00-00-000Z.json");
    const QString manifestPath = tmp.filePath(manifestName);

    {
        QSaveFile f(manifestPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        QVERIFY(f.commit());
    }

    QVERIFY(QFile::exists(manifestPath));

    const int count = RecordingSession::scanForOrphans(tmp.path());

    QCOMPARE(count, 1);

    // The manifest should have been moved — either renamed to .json.ok (playable)
    // or moved to corrupted/ (unplayable). Either way it should no longer be at
    // its original path.
    QVERIFY2(!QFile::exists(manifestPath),
             "Original manifest path should no longer exist after scanForOrphans");
}

UT_REGISTER_TEST(RecordingSessionTest, TestLabel::Unit)

#include "RecordingSessionTest.moc"
