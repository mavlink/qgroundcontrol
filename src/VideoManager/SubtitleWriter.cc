/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SubtitleWriter.h"
#include "Fact.h"
#include "FactValueGrid.h"
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QString>

QGC_LOGGING_CATEGORY(SubtitleWriterLog, "qgc.videomanager.subtitlewriter")

SubtitleWriter::SubtitleWriter(QObject *parent)
    : QObject(parent)
{
    // qCDebug(SubtitleWriterLog) << Q_FUNC_INFO << this;

    (void) connect(&_timer, &QTimer::timeout, this, &SubtitleWriter::_captureTelemetry);
}

SubtitleWriter::~SubtitleWriter()
{
    // qCDebug(SubtitleWriterLog) << Q_FUNC_INFO << this;
}

void SubtitleWriter::startCapturingTelemetry(const QString &videoFile, QSize size)
{
    _size = size;
    _facts.clear();

    // Gather the facts currently displayed into _facts
    FactValueGrid *grid = new FactValueGrid();
    (void) grid->setProperty("settingsGroup", HorizontalFactValueGrid::telemetryBarSettingsGroup);
    grid->componentComplete();
    for (int colIndex = 0; colIndex < grid->columns()->count(); colIndex++) {
        const QmlObjectListModel *list = grid->columns()->value<const QmlObjectListModel*>(colIndex);
        for (int rowIndex = 0; rowIndex < list->count(); rowIndex++) {
            const InstrumentValueData *value = list->value<InstrumentValueData*>(rowIndex);
            if (value->fact()) {
                _facts += value->fact();
            }
        }
    }
    grid->deleteLater();

    // One subtitle always starts where the previous ended
    _lastEndTime = QTime(0, 0);

    const QFileInfo videoFileInfo(videoFile);
    const QString subtitleFilePath = QStringLiteral("%1/%2.ass").arg(videoFileInfo.path(), videoFileInfo.completeBaseName());
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
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
        "Style: Default,Monospace,%3,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,2,1,10,10,10,1\n"
        "\n"
        "[Events]\n"
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
    ).arg(_size.width()).arg(_size.height()).arg(scaledFontSize);

    // TODO: Find a good way to input title
    // stream << QStringLiteral("Dialogue: 0,0:00:00.00,999:00:00.00,Default,,0,0,0,,{\\pos(5,35)}%1\n");

    _timer.start(1000 / _kSampleRate);
}

void SubtitleWriter::stopCapturingTelemetry()
{
    qCDebug(SubtitleWriterLog) << "Stopping writing";
    _timer.stop();
    _file.close();
}

void SubtitleWriter::_captureTelemetry()
{
    if (!MultiVehicleManager::instance()->activeVehicle()) {
        qCWarning(SubtitleWriterLog) << "Attempting to capture fact data with no active vehicle!";
        return;
    }

    // Each list corresponds to a column in the subtitles
    QStringList namesStrings;
    QStringList valuesStrings;

    // Make a list of "factname:" strings and other with the values, so one can be aligned left and the other right
    for (const Fact *fact : std::as_const(_facts)) {
        valuesStrings << QStringLiteral("%2 %3").arg(fact->cookedValueString(), fact->cookedUnits());
        namesStrings << QStringLiteral("%1:").arg(fact->shortDescription());
    }

    // The time to start displaying this subtitle text
    const QTime start = _lastEndTime;

    // The time to stop displaying this subtitle text
    const QTime end = start.addMSecs(1000 / _kSampleRate);
    _lastEndTime = end;

    // This splits the screen in N parts and uses the N-1 internal parts to align the subtitles to.
    // Should we try to get the resolution from the pipeline? This seems to work fine with other resolutions too.
    static constexpr int offsetFactor = 100; // Used to reduce the borders in the layout
    static constexpr float nRows = 3; // number of rows used for displaying data
    static const int rowWidth = (_size.width() + offsetFactor) / (nRows + 1);
    const int nValuesByRow = ceil(_facts.length() / nRows);

    QStringList stringColumns;

    // These templates are used for the data columns, one right-aligned for names and one for
    // the facts values. The arguments expected are: start time, end time, xposition, and string content.
    static const QString namesLine = QStringLiteral("Dialogue: 0,%3,%4,Default,,0,0,0,,{\\an3\\pos(%1,%2)}%5\n");
    static const QString valuesLine = QStringLiteral("Dialogue: 0,%3,%4,Default,,0,0,0,,{\\pos(%1,%2)}%5\n");

    // Split values into N columns and create a subtitle entry for each column
    for (int i = 0; i < nRows; i++) {
        const QStringList currentColumnNameStrings = namesStrings.mid(i * nValuesByRow, nValuesByRow);
        const QStringList currentColumnValueStrings = valuesStrings.mid(i * nValuesByRow, nValuesByRow);

        // Fill templates for names of column i
        const QString names = namesLine.arg(QString::number((-offsetFactor / 2) + (rowWidth * (i + 1)) - 10),
                                            QString::number(_size.height() - 30),
                                            start.toString("H:mm:ss.zzz").chopped(2),
                                            end.toString("H:mm:ss.zzz").chopped(2),
                                            currentColumnNameStrings.join("\\N"));
        stringColumns << names;

        // Fill templates for values of column i
        const QString values = valuesLine.arg(QString::number((-offsetFactor / 2) + (rowWidth * (i + 1))),
                                              QString::number(_size.height() - 30),
                                              start.toString("H:mm:ss.zzz").chopped(2),
                                              end.toString("H:mm:ss.zzz").chopped(2),
                                              currentColumnValueStrings.join("\\N"));
        stringColumns << values;
    }

    // Write the date to the corner
    stringColumns << QStringLiteral("Dialogue: 0,%1,%2,Default,,0,0,0,,{\\pos(10,35)}%3\n").arg(
                                    start.toString("H:mm:ss.zzz").chopped(2),
                                    end.toString("H:mm:ss.zzz").chopped(2),
                                    QDateTime::currentDateTime().toString(QLocale::system().dateFormat(QLocale::ShortFormat)));
    // Write new data
    QTextStream stream(&_file);
    for (const QString &col : std::as_const(stringColumns)) {
        stream << col;
    }
}
