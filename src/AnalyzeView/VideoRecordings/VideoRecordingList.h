#pragma once

#include <QtCore/QFileSystemWatcher>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

/// The video recordings in a directory, newest first, for the Video Recordings page. Follows recordings being added
/// and removed.
class VideoRecordingList : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(QVariantList recordings READ recordings NOTIFY recordingsChanged)

public:
    explicit VideoRecordingList(QObject* parent = nullptr);

    /// AppSettings::videoSavePath() until set.
    QString directory() const { return _directory; }

    void setDirectory(const QString& directory);

    /// Each recording as a map: "name" (the file name), "path", "url", "size" (bytes), "modified" (a QDateTime) and
    /// "subtitles", the url of its telemetry subtitle file (SubtitleWriter), empty without one.
    QVariantList recordings() const { return _recordings; }

    /// Deletes the recording at @p path, one of recordings(), with its subtitle file. False, with a warning logged,
    /// when the recording remains.
    Q_INVOKABLE bool remove(const QString& path);

signals:
    void directoryChanged();
    void recordingsChanged();

private:
    void _setDirectory(const QString& directory);
    void _refresh();

    QString _directory;
    bool _directorySet = false;
    QVariantList _recordings;
    QFileSystemWatcher _watcher;
};
