#include "SubtitleWriterTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>

#include "MultiVehicleManager.h"
#include "SubtitleTrack.h"
#include "SubtitleWriter.h"

namespace {

QString readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

}  // namespace

void SubtitleWriterTest::_testLayoutFollowsEachRecording()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString video = dir.filePath(QStringLiteral("flight.mp4"));
    const QString subtitles = dir.filePath(QStringLiteral("flight.ass"));

    SubtitleWriter writer;
    writer.startCapturingTelemetry(video, QSize(1920, 1080));
    QVERIFY(writer.capturing());
    writer._writeSample(writer._sample(0), 1000);
    writer._writeSample(writer._sample(1000), 2000);
    writer.stopCapturingTelemetry();
    QVERIFY(!writer.capturing());
    const QString first = readFile(subtitles);
    QVERIFY(first.contains(QStringLiteral("PlayResX: 1920")));
    // Values of the first column sit at (offset / -2) + rowWidth, rowWidth being (1920 + 100) / 4.
    QVERIFY(first.contains(QStringLiteral("\\pos(455,1050)")));
    QVERIFY(first.contains(QStringLiteral("Dialogue: 0,0:00:01.00,0:00:02.00,")));

    // A second recording into the same file replaces it and lays out for its own resolution.
    writer.startCapturingTelemetry(video, QSize(640, 480));
    writer._writeSample(writer._sample(0), 1000);
    writer.stopCapturingTelemetry();
    const QString second = readFile(subtitles);
    QVERIFY(second.startsWith(QStringLiteral("[Script Info]")));
    QVERIFY(second.contains(QStringLiteral("PlayResX: 640")));
    QVERIFY(!second.contains(QStringLiteral("1920")));
    QVERIFY(!second.contains(QStringLiteral("0:00:01.00,0:00:02.00")));
    QVERIFY(second.contains(QStringLiteral("\\pos(135,450)")));
}

void SubtitleWriterTest::_testFollowsActiveVehicle()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SubtitleWriter writer;
    writer.startCapturingTelemetry(dir.filePath(QStringLiteral("flight.mp4")), QSize(1280, 720));
    QVERIFY(!writer._sample(0).values.isEmpty());

    // The telemetry Facts belong to the vehicle: once it is gone, samples are empty rather than reading freed Facts.
    _disconnectMockLink();
    QVERIFY(!MultiVehicleManager::instance()->activeVehicle());
    QVERIFY(writer._sample(1000).values.isEmpty());
    writer._captureTelemetry();
    writer.stopCapturingTelemetry();
}

void SubtitleWriterTest::_testReadsBack()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SubtitleWriter writer;
    writer.startCapturingTelemetry(dir.filePath(QStringLiteral("flight.mp4")), QSize(1280, 720));
    const QStringList names = writer._sample(0).names;
    QVERIFY(!names.isEmpty());
    writer._writeSample(writer._sample(0), 1000);
    writer.stopCapturingTelemetry();

    // The Video Recordings page's reader shows each column's names and values, and the date.
    SubtitleTrack track;
    track.setSource(QUrl::fromLocalFile(dir.filePath(QStringLiteral("flight.ass"))));
    QCOMPARE(track.playResX(), 1280);
    QCOMPARE(track.playResY(), 720);
    track.setPosition(500);
    const QVariantList events = track.events();
    QCOMPARE(events.size(), 7);
    const QVariantMap firstNames = events.at(0).toMap();
    QCOMPARE(firstNames.value(QStringLiteral("alignment")).toInt(), 3);
    QVERIFY(firstNames.value(QStringLiteral("text")).toString().startsWith(names.first()));
    QCOMPARE(events.at(1).toMap().value(QStringLiteral("alignment")).toInt(), 1);
    track.setPosition(1000);
    QVERIFY(track.events().isEmpty());
}

UT_REGISTER_TEST(SubtitleWriterTest, TestLabel::Integration, TestLabel::Vehicle)
