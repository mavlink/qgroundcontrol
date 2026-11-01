#include "SubtitleTrack.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SubtitleTrackLog, "AnalyzeView.SubtitleTrack")

namespace {

/// An ASS time, H:MM:SS.CC, in milliseconds; -1 when malformed.
qint64 parseTime(const QString& time)
{
    const QStringList parts = time.trimmed().split(u':');
    if (parts.size() != 3) {
        return -1;
    }
    bool hoursOk = false;
    bool minutesOk = false;
    bool secondsOk = false;
    const qint64 hours = parts.at(0).toLongLong(&hoursOk);
    const qint64 minutes = parts.at(1).toLongLong(&minutesOk);
    const double seconds = parts.at(2).toDouble(&secondsOk);
    if (!hoursOk || !minutesOk || !secondsOk) {
        return -1;
    }
    return (hours * 3'600'000) + (minutes * 60'000) + qRound64(seconds * 1000);
}

/// @p values split at commas into @p count fields, the last keeping the rest of the line (an event's text may hold
/// commas). Fewer fields when the line has fewer.
QStringList splitFields(const QString& values, qsizetype count)
{
    QStringList fields;
    qsizetype start = 0;
    while (fields.size() < (count - 1)) {
        const qsizetype comma = values.indexOf(u',', start);
        if (comma < 0) {
            break;
        }
        fields.append(values.mid(start, comma - start).trimmed());
        start = comma + 1;
    }
    fields.append(values.mid(start));
    return fields;
}

/// The trimmed field names of a section's "Format:" line.
QStringList formatFields(const QString& values)
{
    QStringList fields = values.split(u',');
    for (QString& field : fields) {
        field = field.trimmed();
    }
    return fields;
}

}  // namespace

SubtitleTrack::SubtitleTrack(QObject* parent)
    : QObject(parent)
{}

void SubtitleTrack::setSource(const QUrl& source)
{
    if (source == _source) {
        return;
    }
    _source = source;

    QString content;
    if (!source.isEmpty()) {
        QFile file(source.toLocalFile());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString::fromUtf8(file.readAll());
        } else {
            qCWarning(SubtitleTrackLog) << "Unable to read" << file.fileName() << file.errorString();
        }
    }
    _parse(content);
    emit sourceChanged();
    _updateShown();
}

void SubtitleTrack::setPosition(qint64 position)
{
    if (position == _position) {
        return;
    }
    _position = position;
    emit positionChanged();
    _updateShown();
}

void SubtitleTrack::_parse(const QString& content)
{
    static const QRegularExpression overrideBlock(QStringLiteral(R"(\{([^}]*)\})"));
    static const QRegularExpression alignmentTag(QStringLiteral(R"(\\an([1-9]))"));
    static const QRegularExpression positionTag(QStringLiteral(R"(\\pos\(\s*(-?[\d.]+)\s*,\s*(-?[\d.]+)\s*\))"));

    _events.clear();
    _playResX = 0;
    _playResY = 0;
    _fontSize = 20;
    int styleAlignment = 2;
    int marginL = 10;
    int marginR = 10;
    int marginV = 10;
    bool styleRead = false;

    struct Dialogue
    {
        QStringList fields;
        QStringList format;
    };

    QList<Dialogue> dialogues;

    QString section;
    QStringList styleFormat;
    QStringList eventFormat;
    const QStringList lines = content.split(u'\n');
    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(u'[') && line.endsWith(u']')) {
            section = line.toLower();
            continue;
        }
        const qsizetype colon = line.indexOf(u':');
        if (colon < 0) {
            continue;
        }
        const QString key = line.left(colon).trimmed();
        const QString values = line.mid(colon + 1).trimmed();
        if (section == QStringLiteral("[script info]")) {
            if (key == QStringLiteral("PlayResX")) {
                _playResX = values.toInt();
            } else if (key == QStringLiteral("PlayResY")) {
                _playResY = values.toInt();
            }
        } else if (section == QStringLiteral("[v4+ styles]")) {
            if (key == QStringLiteral("Format")) {
                styleFormat = formatFields(values);
            } else if (key == QStringLiteral("Style")) {
                const QStringList fields = splitFields(values, styleFormat.size());
                const auto field = [&fields, &styleFormat](const QString& name) {
                    const qsizetype index = styleFormat.indexOf(name);
                    return ((index >= 0) && (index < fields.size())) ? fields.at(index).trimmed() : QString();
                };
                // The Default style, else the first.
                if (!styleRead || (field(QStringLiteral("Name")) == QStringLiteral("Default"))) {
                    styleRead = true;
                    _fontSize = field(QStringLiteral("Fontsize")).toDouble();
                    styleAlignment = field(QStringLiteral("Alignment")).toInt();
                    marginL = field(QStringLiteral("MarginL")).toInt();
                    marginR = field(QStringLiteral("MarginR")).toInt();
                    marginV = field(QStringLiteral("MarginV")).toInt();
                }
            }
        } else if (section == QStringLiteral("[events]")) {
            if (key == QStringLiteral("Format")) {
                eventFormat = formatFields(values);
            } else if (key == QStringLiteral("Dialogue")) {
                dialogues.append({splitFields(values, eventFormat.size()), eventFormat});
            }
        }
    }

    // As the ASS renderers do: a missing resolution follows the other in 4:3, or both default to 384 × 288.
    if ((_playResX <= 0) && (_playResY <= 0)) {
        _playResX = 384;
        _playResY = 288;
    } else if (_playResX <= 0) {
        _playResX = (_playResY * 4) / 3;
    } else if (_playResY <= 0) {
        _playResY = (_playResX * 3) / 4;
    }
    if ((styleAlignment < 1) || (styleAlignment > 9)) {
        styleAlignment = 2;
    }

    for (const Dialogue& dialogue : std::as_const(dialogues)) {
        const auto field = [&dialogue](const QString& name) {
            const qsizetype index = dialogue.format.indexOf(name);
            return ((index >= 0) && (index < dialogue.fields.size())) ? dialogue.fields.at(index) : QString();
        };
        Event event;
        event.startMs = parseTime(field(QStringLiteral("Start")));
        event.endMs = parseTime(field(QStringLiteral("End")));
        if ((event.startMs < 0) || (event.endMs <= event.startMs)) {
            continue;
        }
        event.alignment = styleAlignment;
        bool positioned = false;
        QString text = field(QStringLiteral("Text"));
        QRegularExpressionMatchIterator blocks = overrideBlock.globalMatch(text);
        while (blocks.hasNext()) {
            const QString tags = blocks.next().captured(1);
            if (const QRegularExpressionMatch alignment = alignmentTag.match(tags); alignment.hasMatch()) {
                event.alignment = alignment.captured(1).toInt();
            }
            if (const QRegularExpressionMatch position = positionTag.match(tags); position.hasMatch()) {
                event.x = position.captured(1).toDouble();
                event.y = position.captured(2).toDouble();
                positioned = true;
            }
        }
        text.remove(overrideBlock);
        text.replace(QStringLiteral("\\N"), QStringLiteral("\n"));
        text.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
        text.replace(QStringLiteral("\\h"), QString(QChar::Nbsp));
        event.text = text;
        if (!positioned) {
            // Placed by the alignment within the style's margins.
            const int column = (event.alignment - 1) % 3;
            const int row = (event.alignment - 1) / 3;
            event.x = (column == 0) ? marginL : ((column == 1) ? (_playResX / 2.0) : (_playResX - marginR));
            event.y = (row == 0) ? (_playResY - marginV) : ((row == 1) ? (_playResY / 2.0) : marginV);
        }
        _events.append(event);
    }
    qCDebug(SubtitleTrackLog) << "Read" << _events.size() << "events from" << _source;
}

void SubtitleTrack::_updateShown()
{
    QList<qsizetype> shownIndexes;
    for (qsizetype i = 0; i < _events.size(); i++) {
        const Event& event = _events.at(i);
        if ((event.startMs <= _position) && (_position < event.endMs)) {
            shownIndexes.append(i);
        }
    }
    if (shownIndexes == _shownIndexes) {
        return;
    }
    _shownIndexes = shownIndexes;
    _shown.clear();
    for (const qsizetype index : std::as_const(_shownIndexes)) {
        const Event& event = _events.at(index);
        _shown.append(QVariantMap{
            {QStringLiteral("text"), event.text},
            {QStringLiteral("x"), event.x},
            {QStringLiteral("y"), event.y},
            {QStringLiteral("alignment"), event.alignment},
        });
    }
    emit eventsChanged();
}
