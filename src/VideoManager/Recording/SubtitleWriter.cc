#include "SubtitleWriter.h"

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QTextStream>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SubtitleWriterLog, "Video.SubtitleWriter")

namespace {

// Layout of the telemetry columns, in subtitle pixels.
constexpr int kColumnCount = 3;
constexpr int kOffsetFactor = 100;  // narrows the outer margins
// Used when the recording's resolution is not known yet.
constexpr QSize kDefaultVideoSize{1920, 1080};

/// ASS timestamp, H:MM:SS.CC
QString assTime(qint64 ms)
{
    const QChar zero(u'0');
    return QStringLiteral("%1:%2:%3.%4")
        .arg(ms / 3'600'000)
        .arg((ms / 60'000) % 60, 2, 10, zero)
        .arg((ms / 1000) % 60, 2, 10, zero)
        .arg((ms / 10) % 100, 2, 10, zero);
}

}  // namespace

SubtitleWriter::SubtitleWriter(QObject* parent)
    : QObject(parent)
{
    (void) connect(&_timer, &QTimer::timeout, this, &SubtitleWriter::_captureTelemetry);
}

SubtitleWriter::~SubtitleWriter()
{
    stopCapturingTelemetry();
}

void SubtitleWriter::startCapturingTelemetry(const QString& videoFile, QSize videoSize)
{
    stopCapturingTelemetry();

    _videoFile = videoFile;
    _size = videoSize.isEmpty() ? kDefaultVideoSize : videoSize;
    _rowWidth = (_size.width() + kOffsetFactor) / (kColumnCount + 1);
    _telemetry.readLayout();

    const QFileInfo videoFileInfo(videoFile);
    _file.setFileName(QStringLiteral("%1/%2.ass").arg(videoFileInfo.path(), videoFileInfo.completeBaseName()));
    qCDebug(SubtitleWriterLog) << "Writing overlay to file:" << _file.fileName();
    if (!_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qCWarning(SubtitleWriterLog) << "Unable to write subtitle file" << _file.fileName() << _file.errorString();
        return;
    }

    static constexpr int baseWidth = 640;
    static constexpr int baseFontSize = 12;
    const int scaledFontSize = (_size.width() * baseFontSize) / baseWidth;

    QTextStream stream(&_file);
    stream << QStringLiteral(
                  "[Script Info]\n"
                  "Title: QGroundControl Subtitle Telemetry file\n"
                  "ScriptType: v4.00+\n"
                  "WrapStyle: 0\n"
                  "ScaledBorderAndShadow: yes\n"
                  "YCbCr Matrix: TV.601\n"
                  "PlayResX: %1\n"
                  "PlayResY: %2\n"
                  "\n"
                  "[V4+ Styles]\n"
                  "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, "
                  "Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, "
                  "Alignment, MarginL, MarginR, MarginV, Encoding\n"
                  "Style: "
                  "Default,Monospace,%3,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,2,1,10,10,"
                  "10,1\n"
                  "\n"
                  "[Events]\n"
                  "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n")
                  .arg(_size.width())
                  .arg(_size.height())
                  .arg(scaledFontSize);
    stream.flush();

    _elapsed.start();
    _pending = _sample(0);
    _timer.start(1000 / _kSampleRate);
}

void SubtitleWriter::stopCapturingTelemetry()
{
    _timer.stop();
    if (!_file.isOpen()) {
        return;
    }
    qCDebug(SubtitleWriterLog) << "Stopping writing" << _file.fileName();
    _writeSample(_pending, _elapsed.elapsed());
    _pending = Sample();
    _file.close();
}

void SubtitleWriter::_captureTelemetry()
{
    if (!_file.isOpen()) {
        return;
    }
    const qint64 nowMs = _elapsed.elapsed();
    _writeSample(_pending, nowMs);
    _pending = _sample(nowMs);
}

SubtitleWriter::Sample SubtitleWriter::_sample(qint64 startMs) const
{
    Sample sample;
    sample.startMs = startMs;

    const std::optional<QList<TelemetryBarValues::Value>> values = _telemetry.read();
    if (!values) {
        return sample;
    }

    sample.date = QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat));
    for (const TelemetryBarValues::Value& value : *values) {
        sample.names << QStringLiteral("%1:").arg(value.name);
        sample.values << value.value;
    }
    return sample;
}

void SubtitleWriter::_writeSample(const Sample& sample, qint64 endMs)
{
    if ((sample.startMs < 0) || sample.date.isEmpty() || (endMs <= sample.startMs) || !_file.isOpen()) {
        return;
    }

    const QString start = assTime(sample.startMs);
    const QString end = assTime(endMs);
    const int valuesPerColumn = (static_cast<int>(sample.names.size()) + kColumnCount - 1) / kColumnCount;
    const QString y = QString::number(_size.height() - 30);

    QTextStream stream(&_file);
    // Each column is a right-aligned list of names and a left-aligned list of values.
    for (int column = 0; column < kColumnCount; column++) {
        const int x = (-kOffsetFactor / 2) + (_rowWidth * (column + 1));
        stream << QStringLiteral("Dialogue: 0,%1,%2,Default,,0,0,0,,{\\an3\\pos(%3,%4)}%5\n")
                      .arg(start, end, QString::number(x - 10), y,
                           sample.names.mid(column * valuesPerColumn, valuesPerColumn).join(QStringLiteral("\\N")));
        stream << QStringLiteral("Dialogue: 0,%1,%2,Default,,0,0,0,,{\\pos(%3,%4)}%5\n")
                      .arg(start, end, QString::number(x), y,
                           sample.values.mid(column * valuesPerColumn, valuesPerColumn).join(QStringLiteral("\\N")));
    }
    stream << QStringLiteral("Dialogue: 0,%1,%2,Default,,0,0,0,,{\\pos(10,35)}%3\n").arg(start, end, sample.date);
}
