#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

/// A recording's telemetry subtitle file, read for display over the video: the events shown at the playback position,
/// in the file's own coordinates (playResX × playResY).
///
/// Reads the Advanced SubStation Alpha subset SubtitleWriter writes: the Default style's font size, alignment and
/// margins, and Dialogue events with {\anN} and {\pos(x,y)} overrides and \N line breaks. Other override tags are
/// dropped.
class SubtitleTrack : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int playResX READ playResX NOTIFY sourceChanged)
    Q_PROPERTY(int playResY READ playResY NOTIFY sourceChanged)
    Q_PROPERTY(double fontSize READ fontSize NOTIFY sourceChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)

    friend class SubtitleTrackTest;

public:
    explicit SubtitleTrack(QObject* parent = nullptr);

    /// A local file; empty for none.
    QUrl source() const { return _source; }

    void setSource(const QUrl& source);

    /// Milliseconds from the start of the recording.
    qint64 position() const { return _position; }

    void setPosition(qint64 position);

    int playResX() const { return _playResX; }

    int playResY() const { return _playResY; }

    double fontSize() const { return _fontSize; }

    /// The events shown at position(), in the file's order, each as a map: "text" (lines apart by '\n'), "x" and "y",
    /// the point the text is anchored at, and "alignment", which side of the text the point is on, numbered as a
    /// numeric keypad (1 bottom left ... 9 top right).
    QVariantList events() const { return _shown; }

signals:
    void sourceChanged();
    void positionChanged();
    void eventsChanged();

private:
    struct Event
    {
        qint64 startMs = 0;
        qint64 endMs = 0;
        QString text;
        double x = 0;
        double y = 0;
        int alignment = 1;
    };

    /// Reads the events of @p content, an ASS file's text, replacing the current ones.
    void _parse(const QString& content);
    void _updateShown();

    QUrl _source;
    qint64 _position = 0;
    int _playResX = 384;
    int _playResY = 288;
    double _fontSize = 20;
    QList<Event> _events;
    /// The indexes in _events of the events shown.
    QList<qsizetype> _shownIndexes;
    QVariantList _shown;
};
