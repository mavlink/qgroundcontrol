#include "SubtitleTrackTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtTest/QSignalSpy>

#include "SubtitleTrack.h"

namespace {

QUrl writeText(const QTemporaryDir& dir, const QString& name, const QByteArray& content)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || (file.write(content) != content.size())) {
        return QUrl();
    }
    return QUrl::fromLocalFile(path);
}

// As SubtitleWriter writes one sample, for a 1920 × 1080 recording.
constexpr QByteArrayView kWriterFile =
    "[Script Info]\n"
    "Title: QGroundControl Subtitle Telemetry file\n"
    "ScriptType: v4.00+\n"
    "PlayResX: 1920\n"
    "PlayResY: 1080\n"
    "\n"
    "[V4+ Styles]\n"
    "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, "
    "Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, "
    "MarginV, Encoding\n"
    "Style: Default,Monospace,36,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,2,1,10,10,10,1\n"
    "\n"
    "[Events]\n"
    "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
    "Dialogue: 0,0:00:01.00,0:00:02.00,Default,,0,0,0,,{\\an3\\pos(445,1050)}Alt (Rel):\\NSpeed, ground:\n"
    "Dialogue: 0,0:00:01.00,0:00:02.00,Default,,0,0,0,,{\\pos(455,1050)}12.0 m\\N3.5 m/s\n"
    "Dialogue: 0,0:00:01.00,0:00:02.00,Default,,0,0,0,,{\\pos(10,35)}11/1/26\n"
    "Dialogue: 0,0:00:02.00,0:00:03.00,Default,,0,0,0,,{\\pos(455,1050)}13.0 m\\N3.6 m/s\n"
    "Dialogue: 0,0:00:0x.00,0:00:03.00,Default,,0,0,0,,{\\pos(1,1)}malformed start\n";

}  // namespace

void SubtitleTrackTest::_testReadsWriterLayout()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl source = writeText(dir, QStringLiteral("flight.ass"), kWriterFile.toByteArray());
    QVERIFY(!source.isEmpty());

    SubtitleTrack track;
    QSignalSpy eventsSpy(&track, &SubtitleTrack::eventsChanged);
    track.setSource(source);
    QCOMPARE(track.playResX(), 1920);
    QCOMPARE(track.playResY(), 1080);
    QCOMPARE(track.fontSize(), 36.0);
    QVERIFY(track.events().isEmpty());
    QCOMPARE(eventsSpy.count(), 0);

    track.setPosition(1500);
    QCOMPARE(eventsSpy.count(), 1);
    const QVariantList events = track.events();
    QCOMPARE(events.size(), 3);
    const QVariantMap names = events.at(0).toMap();
    // A comma in the text belongs to it; \N breaks lines.
    QCOMPARE(names.value(QStringLiteral("text")).toString(), QStringLiteral("Alt (Rel):\nSpeed, ground:"));
    QCOMPARE(names.value(QStringLiteral("x")).toDouble(), 445.0);
    QCOMPARE(names.value(QStringLiteral("y")).toDouble(), 1050.0);
    QCOMPARE(names.value(QStringLiteral("alignment")).toInt(), 3);
    const QVariantMap values = events.at(1).toMap();
    QCOMPARE(values.value(QStringLiteral("text")).toString(), QStringLiteral("12.0 m\n3.5 m/s"));
    QCOMPARE(values.value(QStringLiteral("alignment")).toInt(), 1);

    // Within the same events nothing changes; the next sample replaces them.
    track.setPosition(1900);
    QCOMPARE(eventsSpy.count(), 1);
    track.setPosition(2000);
    QCOMPARE(eventsSpy.count(), 2);
    QCOMPARE(track.events().size(), 1);
    QCOMPARE(track.events().at(0).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("13.0 m\n3.6 m/s"));

    track.setPosition(3000);
    QVERIFY(track.events().isEmpty());
}

void SubtitleTrackTest::_testStyleAlignmentWithoutPosition()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // The fields in another order, no resolution, and events placed by the style's alignment and margins.
    const QUrl source = writeText(dir, QStringLiteral("other.ass"),
                                  "[Script Info]\n"
                                  "[V4+ Styles]\n"
                                  "Format: Name, Alignment, MarginL, MarginR, MarginV, Fontsize\n"
                                  "Style: Other,1,0,0,0,10\n"
                                  "Style: Default,2,20,30,40,18\n"
                                  "[Events]\n"
                                  "Format: Start, End, Text\n"
                                  "Dialogue: 0:00:00.00,0:00:01.00,{\\b1}bottom\\hcenter\n"
                                  "Dialogue: 0:00:00.00,0:00:01.00,{\\an9}top right\n");
    QVERIFY(!source.isEmpty());

    SubtitleTrack track;
    track.setSource(source);
    QCOMPARE(track.playResX(), 384);
    QCOMPARE(track.playResY(), 288);
    QCOMPARE(track.fontSize(), 18.0);
    track.setPosition(500);
    const QVariantList events = track.events();
    QCOMPARE(events.size(), 2);
    const QVariantMap bottom = events.at(0).toMap();
    QCOMPARE(bottom.value(QStringLiteral("text")).toString(),
             QStringLiteral("bottom") + QChar(QChar::Nbsp) + QStringLiteral("center"));
    QCOMPARE(bottom.value(QStringLiteral("alignment")).toInt(), 2);
    QCOMPARE(bottom.value(QStringLiteral("x")).toDouble(), 192.0);
    QCOMPARE(bottom.value(QStringLiteral("y")).toDouble(), 248.0);
    const QVariantMap topRight = events.at(1).toMap();
    QCOMPARE(topRight.value(QStringLiteral("alignment")).toInt(), 9);
    QCOMPARE(topRight.value(QStringLiteral("x")).toDouble(), 354.0);
    QCOMPARE(topRight.value(QStringLiteral("y")).toDouble(), 40.0);
}

void SubtitleTrackTest::_testSourceChanges()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl source = writeText(dir, QStringLiteral("flight.ass"), kWriterFile.toByteArray());

    SubtitleTrack track;
    track.setPosition(1500);
    track.setSource(source);
    QCOMPARE(track.events().size(), 3);

    QSignalSpy eventsSpy(&track, &SubtitleTrack::eventsChanged);
    track.setSource(QUrl());
    QVERIFY(track.events().isEmpty());
    QCOMPARE(eventsSpy.count(), 1);

    expectLogMessage("AnalyzeView.SubtitleTrack", QtWarningMsg, QRegularExpression(QStringLiteral("Unable to read")));
    track.setSource(QUrl::fromLocalFile(dir.filePath(QStringLiteral("missing.ass"))));
    verifyExpectedLogMessage();
    QVERIFY(track.events().isEmpty());
}

void SubtitleTrackTest::_testSourceReplacesShownEvents()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto oneEvent = [](const char* text) {
        return QByteArray("[Events]\nFormat: Start, End, Text\nDialogue: 0:00:00.00,0:00:10.00,") + text + "\n";
    };
    const QUrl first = writeText(dir, QStringLiteral("first.ass"), oneEvent("first"));
    const QUrl second = writeText(dir, QStringLiteral("second.ass"), oneEvent("second"));

    SubtitleTrack track;
    track.setPosition(1000);
    track.setSource(first);
    QCOMPARE(track.events().size(), 1);

    // The other file's event has the same index and is shown at the same position.
    QSignalSpy eventsSpy(&track, &SubtitleTrack::eventsChanged);
    track.setSource(second);
    QCOMPARE(eventsSpy.count(), 1);
    QCOMPARE(track.events().size(), 1);
    QCOMPARE(track.events().at(0).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("second"));
}

void SubtitleTrackTest::_testImplausibleValues_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<int>("playResX");
    QTest::addColumn<int>("playResY");
    QTest::addColumn<double>("fontSize");

    QTest::newRow("resolution too small to derive the other")
        << QByteArray("[Script Info]\nPlayResX: 1\n") << 1 << 1 << 20.0;
    QTest::newRow("resolution out of range")
        << QByteArray("[Script Info]\nPlayResX: 2000000000\nPlayResY: -5\n") << 384 << 288 << 20.0;
    QTest::newRow("style ahead of its format")
        << QByteArray("[V4+ Styles]\nStyle: Default,Monospace,36\nFormat: Name, Fontname, Fontsize\n") << 384 << 288
        << 20.0;
    QTest::newRow("font size out of range")
        << QByteArray("[V4+ Styles]\nFormat: Name, Fontname, Fontsize\nStyle: Default,Monospace,-3\n") << 384 << 288
        << 20.0;
    // Fields that sum to a time after the start, and an hour count that overflows.
    QTest::newRow("times out of range") << QByteArray(
                                               "[Events]\nFormat: Start, End, Text\n"
                                               "Dialogue: 0:00:00.00,1:-30:00.00,negative minutes\n"
                                               "Dialogue: 0:00:00.00,9999999999999:00:00.00,hours\n")
                                        << 384 << 288 << 20.0;
}

void SubtitleTrackTest::_testImplausibleValues()
{
    QFETCH(QByteArray, content);
    QFETCH(int, playResX);
    QFETCH(int, playResY);
    QFETCH(double, fontSize);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl source = writeText(dir, QStringLiteral("odd.ass"), content);
    QVERIFY(!source.isEmpty());

    SubtitleTrack track;
    track.setPosition(500);
    track.setSource(source);
    QCOMPARE(track.playResX(), playResX);
    QCOMPARE(track.playResY(), playResY);
    QCOMPARE(track.fontSize(), fontSize);
    QVERIFY(track.events().isEmpty());
}

void SubtitleTrackTest::_testOversizedFileIsNotRead()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl source = writeText(dir, QStringLiteral("huge.ass"), kWriterFile.toByteArray());
    QVERIFY(!source.isEmpty());
    // Extended without writing the bytes.
    QVERIFY(QFile::resize(source.toLocalFile(), SubtitleTrack::kMaxFileBytes + 1));

    SubtitleTrack track;
    track.setPosition(1500);
    expectLogMessage("AnalyzeView.SubtitleTrack", QtWarningMsg, QRegularExpression(QStringLiteral("Not reading")));
    track.setSource(source);
    verifyExpectedLogMessage();
    QVERIFY(track.events().isEmpty());
}

UT_REGISTER_TEST(SubtitleTrackTest, TestLabel::Unit, TestLabel::AnalyzeView)
