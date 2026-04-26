#include "SubtitleWriter.h"

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtMultimedia/QVideoSink>

#include "Fact.h"
#include "FactValueGrid.h"
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

QGC_LOGGING_CATEGORY(SubtitleWriterLog, "Video.SubtitleWriter")

SubtitleWriter::SubtitleWriter(QObject* parent) : QObject(parent)
{
    // qCDebug(SubtitleWriterLog) << Q_FUNC_INFO << this;

    (void)connect(&_timer, &QTimer::timeout, this, &SubtitleWriter::_captureTelemetry);
}

SubtitleWriter::~SubtitleWriter()
{
    // Ensure the subtitle file is closed and the timer stopped even if the
    // caller forgot to call stopCapturingTelemetry() before destruction
    // (e.g., crash path or early cleanup).
    if (_file.isOpen())
        stopCapturingTelemetry();
}

void SubtitleWriter::startCapturingTelemetry(const QString& videoFile, QSize size)
{
    _size = size;
    // Pin recording-time Facts to the vehicle that started recording. The
    // file-write path below uses _recordingFacts; switching active vehicle
    // mid-recording does not change what gets written.
    _recordingFacts = _gatherFacts();

    // One subtitle always starts where the previous ended
    _lastEndTime = QTime(0, 0);

    const QFileInfo videoFileInfo(videoFile);
    const QString subtitleFilePath =
        QStringLiteral("%1/%2.ass").arg(videoFileInfo.path(), videoFileInfo.completeBaseName());
    qCDebug(SubtitleWriterLog) << "Writing overlay to file:" << subtitleFilePath;
    _file.setFileName(subtitleFilePath);

    if (!_file.open(QIODevice::ReadWrite)) {
        qCWarning(SubtitleWriterLog) << "Unable to write subtitle data to file";
        return;
    }

    QTextStream stream(&_file);

    // Calculate the scaled font size based on the recording width
    static constexpr int baseWidth = 640;
    static constexpr int baseFontSize = 12;
    const int scaledFontSize = (_size.width() * baseFontSize) / baseWidth;

    // This is file header
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

    _timer.start(1000 / _kSampleRate);
}

void SubtitleWriter::setLiveVideoSink(QVideoSink* sink)
{
    _liveSink = sink;

    if (sink) {
        // Always refresh; the frame-delivery swap that triggered this call usually
        // means the displayed video changed (receiver rebuild or vehicle swap),
        // and the OSD must follow what the user sees.
        _liveFacts = _gatherFacts();
        if (!_timer.isActive())
            _timer.start(1000 / _kSampleRate);
    }
}

void SubtitleWriter::stopCapturingTelemetry()
{
    qCDebug(SubtitleWriterLog) << "Stopping writing";
    _file.close();

    // Keep the timer running if a live sink still needs OSD updates
    if (!_liveSink)
        _timer.stop();
}

QList<Fact*> SubtitleWriter::_gatherFacts() const
{
    QList<Fact*> facts;
    FactValueGrid* grid = new FactValueGrid();
    (void)grid->setProperty("settingsGroup", HorizontalFactValueGrid::telemetryBarSettingsGroup);
    grid->componentComplete();
    for (int colIndex = 0; colIndex < grid->columns()->count(); colIndex++) {
        const QmlObjectListModel* list = grid->columns()->value<const QmlObjectListModel*>(colIndex);
        for (int rowIndex = 0; rowIndex < list->count(); rowIndex++) {
            const InstrumentValueData* value = list->value<InstrumentValueData*>(rowIndex);
            if (value->fact())
                facts += value->fact();
        }
    }
    grid->deleteLater();
    return facts;
}

void SubtitleWriter::_captureTelemetry()
{
    // Live OSD: tracks active vehicle (whose video is on screen). Skipped if
    // there is no active vehicle, but file-write continues — recording can
    // outlive the active-vehicle changeover.
    if (_liveSink && MultiVehicleManager::instance()->activeVehicle())
        _liveSink->setSubtitleText(_buildTelemetrySummary(_liveFacts));

    // File-write: pinned to the recording vehicle.
    if (!_file.isOpen() || _recordingFacts.isEmpty())
        return;

    QStringList namesStrings;
    QStringList valuesStrings;
    for (const Fact* fact : std::as_const(_recordingFacts)) {
        valuesStrings << QStringLiteral("%2 %3").arg(fact->cookedValueString(), fact->cookedUnits());
        namesStrings << QStringLiteral("%1:").arg(fact->shortDescription());
    }

    const QTime start = _lastEndTime;
    const QTime end = start.addMSecs(1000 / _kSampleRate);
    _lastEndTime = end;

    static constexpr int offsetFactor = 100;
    static constexpr float nRows = 3;
    const int rowWidth = (_size.width() + offsetFactor) / (nRows + 1);
    const int nValuesByRow = ceil(_recordingFacts.length() / nRows);

    QStringList stringColumns;

    const QString namesLine = QStringLiteral("Dialogue: 0,%3,%4,Default,,0,0,0,,{\\an3\\pos(%1,%2)}%5\n");
    const QString valuesLine = QStringLiteral("Dialogue: 0,%3,%4,Default,,0,0,0,,{\\pos(%1,%2)}%5\n");

    for (int i = 0; i < nRows; i++) {
        const QStringList currentColumnNameStrings = namesStrings.mid(i * nValuesByRow, nValuesByRow);
        const QStringList currentColumnValueStrings = valuesStrings.mid(i * nValuesByRow, nValuesByRow);

        const QString names =
            namesLine.arg(QString::number((-offsetFactor / 2) + (rowWidth * (i + 1)) - 10),
                          QString::number(_size.height() - 30), start.toString("H:mm:ss.zzz").chopped(2),
                          end.toString("H:mm:ss.zzz").chopped(2), currentColumnNameStrings.join("\\N"));
        stringColumns << names;

        const QString values =
            valuesLine.arg(QString::number((-offsetFactor / 2) + (rowWidth * (i + 1))),
                           QString::number(_size.height() - 30), start.toString("H:mm:ss.zzz").chopped(2),
                           end.toString("H:mm:ss.zzz").chopped(2), currentColumnValueStrings.join("\\N"));
        stringColumns << values;
    }

    stringColumns << QStringLiteral("Dialogue: 0,%1,%2,Default,,0,0,0,,{\\pos(10,35)}%3\n")
                         .arg(
                             start.toString("H:mm:ss.zzz").chopped(2), end.toString("H:mm:ss.zzz").chopped(2),
                             QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)));

    QTextStream stream(&_file);
    for (const QString& col : std::as_const(stringColumns)) {
        stream << col;
    }
}

QString SubtitleWriter::_buildTelemetrySummary(const QList<Fact*>& facts) const
{
    QStringList parts;
    for (const Fact* fact : facts) {
        parts << QStringLiteral("%1: %2 %3")
                     .arg(fact->shortDescription(), fact->cookedValueString(), fact->cookedUnits());
    }
    return parts.join(QStringLiteral("  |  "));
}
