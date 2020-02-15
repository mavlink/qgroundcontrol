/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Subtitle Writer
 *   @author Willian Galvani <williangalvani@gmail.com>
 */

#include "SubtitleWriter.h"
#include "SettingsManager.h"
#include "VideoReceiver.h"
#include "VideoManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include <QDateTime>
#include <QString>
#include <QDate>

QGC_LOGGING_CATEGORY(SubtitleWriterLog, "SubtitleWriterLog")

const int SubtitleWriter::_sampleRate = 1; // Sample rate in Hz for getting telemetry data, most players do weird stuff when > 1Hz

SubtitleWriter::SubtitleWriter(QObject* parent)
    : QObject(parent)
{
}

void SubtitleWriter::setVideoReceiver(VideoReceiver* videoReceiver)
{
    if(!videoReceiver) {
        qCWarning(SubtitleWriterLog) << "Invalid VideoReceiver pointer! Aborting subtitle capture!";
        return;
    }
    _videoReceiver = videoReceiver;

#if defined(QGC_GST_STREAMING)
    // Only start writing subtitles once the recording pipeline actually starts
    connect(_videoReceiver, &VideoReceiver::gotFirstRecordingKeyFrame,  this,  &SubtitleWriter::_startCapturingTelemetry);

    // Captures recordingChanged() signals to stop writing subtitles
    connect(_videoReceiver, &VideoReceiver::recordingChanged,           this,  &SubtitleWriter::_onVideoRecordingChanged);
#endif

    // Timer for telemetry capture and writing to file
    connect(&_timer,        &QTimer::timeout,                           this,  &SubtitleWriter::_captureTelemetry);
}

void SubtitleWriter::_onVideoRecordingChanged()
{
#if defined(QGC_GST_STREAMING)
    // Stop capturing data if recording stopped
    if(!_videoReceiver->recording()) {
        qCDebug(SubtitleWriterLog) << "Stopping writing";
        _timer.stop();
        _file.close();
    }
#endif
}

void SubtitleWriter::_startCapturingTelemetry()
{
    if(!_videoReceiver) {
        qCWarning(SubtitleWriterLog) << "Invalid VideoReceiver pointer! Aborting subtitle capture!";
        _timer.stop();
        return;
    }

    // Get the facts displayed in the values widget and capture them, removing the "Vehicle." prefix.
    QSettings settings;
    settings.beginGroup("ValuesWidget");
    _values = settings.value("large").toStringList().replaceInStrings(QStringLiteral("Vehicle."), QString());
    _values += settings.value("small").toStringList().replaceInStrings(QStringLiteral("Vehicle."), QString());

    _startTime = QDateTime::currentDateTime();

    QFileInfo videoFile(_videoReceiver->videoFile());
    QString subtitleFilePath = QStringLiteral("%1/%2.ass").arg(videoFile.path(), videoFile.completeBaseName());
    qCDebug(SubtitleWriterLog) << "Writing overlay to file:" << subtitleFilePath;
    _file.setFileName(subtitleFilePath);

    if (!_file.open(QIODevice::ReadWrite)) {
        qCWarning(SubtitleWriterLog) << "Unable to write subtitle data to file";
        return;
    }

    QTextStream stream( &_file );

    // This is file header
    stream << QStringLiteral(
        "[Script Info]\n"
        "Title: QGroundControl Subtitle Telemetry file\n"
        "ScriptType: v4.00+\n"
        "WrapStyle: 0\n"
        "ScaledBorderAndShadow: yes\n"
        "YCbCr Matrix: TV.601\n"
        "PlayResX: 1920\n"
        "PlayResY: 1080\n"
        "\n"
        "[V4+ Styles]\n"
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
        "Style: Default,Monospace,30,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,2,2,1,10,10,10,1\n"
        "\n"
        "[Events]\n"
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
    );

    // TODO: Find a good way to input title
    //stream << QStringLiteral("Dialogue: 0,0:00:00.00,999:00:00.00,Default,,0,0,0,,{\\pos(5,35)}%1\n");

    _timer.start(1000/_sampleRate);
}

void SubtitleWriter::_captureTelemetry()
{
    if(!_videoReceiver) {
        qCWarning(SubtitleWriterLog) << "Invalid VideoReceiver pointer! Aborting subtitle capture!";
        _timer.stop();
        return;
    }

    static const float nRows = 3; // number of rows used for displaying data
    static const int offsetFactor = 700; // Used to simulate a larger resolution and reduce the borders in the layout

    auto *vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        qCWarning(SubtitleWriterLog) << "Attempting to capture fact data with no active vehicle!";
        return;
    }
    // Each list corresponds to a column in the subtitles
    QStringList namesStrings;
    QStringList valuesStrings;

    // Make a list of "factname:" strings and other with the values, so one can be aligned left and the other right
    for (const auto& i : _values) {
        valuesStrings << QStringLiteral("%2 %3").arg(vehicle->getFact(i)->cookedValueString())
                                                .arg(vehicle->getFact(i)->cookedUnits());
        namesStrings << QStringLiteral("%1:").arg(vehicle->getFact(i)->shortDescription());
    }

    // The time to start displaying this subtitle text
    QTime start = QTime(0, 0).addMSecs(_startTime.time().msecsTo(QDateTime::currentDateTime().time()));

    // The time to stop displaying this subtitle text
    QTime end = start.addMSecs(1000/_sampleRate);

    // This splits the screen in N parts and uses the N-1 internal parts to align the subtitles to.
    // Should we try to get the resolution from the pipeline? This seems to work fine with other resolutions too.
    static const int rowWidth = (1920 + offsetFactor)/(nRows+1);
    int nValuesByRow = ceil(_values.length() / nRows);

    QList<QStringList> dataColumns;
    QStringList stringColumns;

    // These templates are used for the data columns, one right-aligned for names and one for
    // the facts values. The arguments expected are: start time, end time, xposition, and string content.
    QString namesLine = QStringLiteral("Dialogue: 0,%2,%3,Default,,0,0,0,,{\\an3\\pos(%1,1075)}%4\n");
    QString valuesLine = QStringLiteral("Dialogue: 0,%2,%3,Default,,0,0,0,,{\\pos(%1,1075)}%4\n");

    // Split values into N columns and create a subtitle entry for each column
    for (int i=0; i<nRows; i++) {
        QStringList currentColumnNameStrings = namesStrings.mid((i)*nValuesByRow, nValuesByRow);
        QStringList currentColumnValueStrings = valuesStrings.mid((i)*nValuesByRow, nValuesByRow);

        // Fill templates for names of column i
        QString names = namesLine.arg(-offsetFactor/2 + rowWidth*(i+1) - 10)
                                 .arg(start.toString("H:mm:ss.zzz").chopped(2))
                                 .arg(end.toString("H:mm:ss.zzz").chopped(2))
                                 .arg(currentColumnNameStrings.join("\\N"));
        stringColumns << names;

        // Fill templates for values of column i
        QString values = valuesLine.arg(-offsetFactor/2 +rowWidth*(i+1))
                                   .arg(start.toString("H:mm:ss.zzz").chopped(2))
                                   .arg(end.toString("H:mm:ss.zzz").chopped(2))
                                   .arg(currentColumnValueStrings.join("\\N"));
        stringColumns << values;
    }

    // Write the date to the corner
    stringColumns << QStringLiteral("Dialogue: 0,%1,%2,Default,,0,0,0,,{\\pos(10,35)}%3\n")
        .arg(start.toString("H:mm:ss.zzz").chopped(2))
        .arg(end.toString("H:mm:ss.zzz").chopped(2))
        .arg(QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate));
    // Write new data
    QTextStream stream(&_file);
    for (const auto& i : stringColumns) {
        stream << i;
    }
}
