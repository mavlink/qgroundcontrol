#include "RecordingPlanTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

#include "RecordingPlan.h"

namespace {

using namespace std::chrono_literals;

bool writeFile(const QString& path, qint64 bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && (file.write(QByteArray(bytes, '\0')) == bytes);
}

}  // namespace

void RecordingPlanTest::_testPartFile_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("part");
    QTest::addColumn<QString>("expected");

    QTest::newRow("first part") << QStringLiteral("/v/clip.mkv") << 1 << QStringLiteral("/v/clip.mkv");
    QTest::newRow("later part") << QStringLiteral("/v/clip.mkv") << 3 << QStringLiteral("/v/clip_part3.mkv");
    QTest::newRow("no extension") << QStringLiteral("/v/clip") << 2 << QStringLiteral("/v/clip_part2");
    QTest::newRow("dot in a directory") << QStringLiteral("/v.d/clip") << 2 << QStringLiteral("/v.d/clip_part2");
    QTest::newRow("Windows separator") << QStringLiteral("C:\\v.d\\clip") << 2 << QStringLiteral("C:\\v.d\\clip_part2");
}

void RecordingPlanTest::_testPartFile()
{
    QFETCH(QString, file);
    QFETCH(int, part);
    QFETCH(QString, expected);

    QCOMPARE(RecordingPlan::partFile(file, part), expected);
}

void RecordingPlanTest::_testPartsFollowOpenedSessions()
{
    RecordingPlan plan(QStringLiteral("/v/clip.mp4"), VideoReceiver::FileFormat::Mp4);
    QCOMPARE(plan.format(), VideoReceiver::FileFormat::Mp4);
    QCOMPARE(plan.currentFile(), QStringLiteral("/v/clip.mp4"));

    // A session that ended before its file got a frame leaves the part to the next one.
    plan.onSessionEnded();
    QCOMPARE(plan.part(), 1);

    // Only the current part's file opens it.
    QVERIFY(!plan.onFileOpened(QStringLiteral("/v/other.mp4")));
    QVERIFY(!plan.opened());
    QVERIFY(plan.onFileOpened(QStringLiteral("/v/clip.mp4")));
    QVERIFY(plan.opened());

    plan.onSessionEnded();
    QCOMPARE(plan.part(), 2);
    QVERIFY(!plan.opened());
    QCOMPARE(plan.currentFile(), QStringLiteral("/v/clip_part2.mp4"));
}

void RecordingPlanTest::_testRecordingTimeAndSize()
{
    const QTemporaryDir dir;
    QVERIFY(dir.isValid());
    RecordingPlan plan(dir.filePath(QStringLiteral("clip.mkv")), VideoReceiver::FileFormat::Mkv);
    const RecordingPlan::Clock::time_point start = RecordingPlan::Clock::now();

    QCOMPARE(plan.recordingMs(start), qint64(-1));
    QCOMPARE(plan.bytesWritten(), qint64(0));

    QVERIFY(plan.onFileOpened(plan.currentFile(), start));
    QVERIFY(writeFile(plan.currentFile(), 100));
    plan.onSessionEnded();
    // The time counts from the first part's first frame, across parts.
    QVERIFY(plan.onFileOpened(plan.currentFile(), start + 5s));
    QVERIFY(writeFile(plan.currentFile(), 50));

    QCOMPARE(plan.recordingMs(start + 7s), qint64(7000));
    QCOMPARE(plan.bytesWritten(), qint64(150));
}

UT_REGISTER_TEST(RecordingPlanTest, TestLabel::Unit)
