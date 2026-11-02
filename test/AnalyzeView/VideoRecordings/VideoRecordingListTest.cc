#include "VideoRecordingListTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtTest/QSignalSpy>

#include "VideoRecordingList.h"

namespace {

/// Creates @p name in @p dir, last modified @p secondsAgo seconds ago.
bool createFile(const QTemporaryDir& dir, const QString& name, int secondsAgo, const QByteArray& content = "data")
{
    QFile file(dir.filePath(name));
    // Flushed first: writing later would set the time again.
    if (!file.open(QIODevice::WriteOnly) || (file.write(content) != content.size()) || !file.flush()) {
        return false;
    }
    return file.setFileTime(QDateTime::currentDateTime().addSecs(-secondsAgo), QFileDevice::FileModificationTime);
}

QStringList names(const QVariantList& recordings)
{
    QStringList result;
    for (const QVariant& recording : recordings) {
        result.append(recording.toMap().value(QStringLiteral("name")).toString());
    }
    return result;
}

}  // namespace

void VideoRecordingListTest::_testListsRecordingsNewestFirst()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(createFile(dir, QStringLiteral("old.mp4"), 300));
    QVERIFY(createFile(dir, QStringLiteral("new.mkv"), 100, "longer data"));
    QVERIFY(createFile(dir, QStringLiteral("new.ass"), 100));
    QVERIFY(createFile(dir, QStringLiteral("middle.mov"), 200));
    QVERIFY(createFile(dir, QStringLiteral("notes.txt"), 0));

    VideoRecordingList list;
    QSignalSpy directorySpy(&list, &VideoRecordingList::directoryChanged);
    list.setDirectory(dir.path());
    QCOMPARE(directorySpy.count(), 1);
    QCOMPARE(names(list.recordings()),
             (QStringList{QStringLiteral("new.mkv"), QStringLiteral("middle.mov"), QStringLiteral("old.mp4")}));

    const QVariantMap newest = list.recordings().at(0).toMap();
    QCOMPARE(newest.value(QStringLiteral("path")).toString(), dir.filePath(QStringLiteral("new.mkv")));
    QCOMPARE(newest.value(QStringLiteral("url")).toUrl(), QUrl::fromLocalFile(dir.filePath(QStringLiteral("new.mkv"))));
    QCOMPARE(newest.value(QStringLiteral("size")).toLongLong(), 11);
    QCOMPARE(newest.value(QStringLiteral("subtitles")).toUrl(),
             QUrl::fromLocalFile(dir.filePath(QStringLiteral("new.ass"))));
    QVERIFY(list.recordings().at(1).toMap().value(QStringLiteral("subtitles")).toUrl().isEmpty());

    // A recording starting in the directory joins the list.
    QSignalSpy recordingsSpy(&list, &VideoRecordingList::recordingsChanged);
    QVERIFY(createFile(dir, QStringLiteral("latest.mp4"), 0));
    QVERIFY(recordingsSpy.wait(TestTimeout::mediumMs()));
    QCOMPARE(names(list.recordings()).first(), QStringLiteral("latest.mp4"));
}

void VideoRecordingListTest::_testRemove()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(createFile(dir, QStringLiteral("flight.mp4"), 100));
    QVERIFY(createFile(dir, QStringLiteral("flight.ass"), 100));
    QVERIFY(createFile(dir, QStringLiteral("other.mp4"), 200));

    VideoRecordingList list;
    list.setDirectory(dir.path());
    QCOMPARE(list.recordings().size(), 2);

    // Deletes the recording and its subtitles, and lists the rest at once.
    QVERIFY(list.remove(dir.filePath(QStringLiteral("flight.mp4"))));
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("flight.mp4"))));
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("flight.ass"))));
    QCOMPARE(names(list.recordings()), QStringList{QStringLiteral("other.mp4")});

    // Only recordings in its directory.
    QTemporaryDir elsewhere;
    QVERIFY(elsewhere.isValid());
    QVERIFY(createFile(elsewhere, QStringLiteral("keep.mp4"), 0));
    expectLogMessage("AnalyzeView.VideoRecordingList", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Not a recording")));
    QVERIFY(!list.remove(elsewhere.filePath(QStringLiteral("keep.mp4"))));
    verifyExpectedLogMessage();
    QVERIFY(QFile::exists(elsewhere.filePath(QStringLiteral("keep.mp4"))));

    expectLogMessage("AnalyzeView.VideoRecordingList", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unable to delete")));
    QVERIFY(!list.remove(dir.filePath(QStringLiteral("gone.mp4"))));
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(VideoRecordingListTest, TestLabel::Unit, TestLabel::AnalyzeView)
