#include "VideoRecordingList.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>

#include "AppSettings.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "VideoReceiver.h"

QGC_LOGGING_CATEGORY(VideoRecordingListLog, "AnalyzeView.VideoRecordingList")

namespace {

QString subtitleFile(const QFileInfo& video)
{
    return QStringLiteral("%1/%2.ass").arg(video.path(), video.completeBaseName());
}

}  // namespace

VideoRecordingList::VideoRecordingList(QObject* parent)
    : QObject(parent)
{
    (void) connect(&_watcher, &QFileSystemWatcher::directoryChanged, this, &VideoRecordingList::_refresh);

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    (void) connect(appSettings, &AppSettings::savePathsChanged, this, [this, appSettings] {
        if (!_directorySet) {
            _setDirectory(appSettings->videoSavePath());
        }
    });
    _setDirectory(appSettings->videoSavePath());
}

void VideoRecordingList::setDirectory(const QString& directory)
{
    _directorySet = true;
    _setDirectory(directory);
}

void VideoRecordingList::_setDirectory(const QString& directory)
{
    if (directory != _directory) {
        if (!_watcher.directories().isEmpty()) {
            (void) _watcher.removePaths(_watcher.directories());
        }
        _directory = directory;
        if (!_directory.isEmpty() && QFileInfo(_directory).isDir()) {
            (void) _watcher.addPath(_directory);
        }
        emit directoryChanged();
    }
    _refresh();
}

void VideoRecordingList::_refresh()
{
    QStringList nameFilters;
    for (const VideoReceiver::FileFormat format :
         {VideoReceiver::FileFormat::Mkv, VideoReceiver::FileFormat::Mov, VideoReceiver::FileFormat::Mp4}) {
        nameFilters.append(QStringLiteral("*.") + VideoReceiver::fileExtension(format));
    }

    QVariantList recordings;
    if (!_directory.isEmpty()) {
        const QFileInfoList files =
            QDir(_directory).entryInfoList(nameFilters, QDir::Files | QDir::Readable, QDir::Time | QDir::Name);
        for (const QFileInfo& file : files) {
            const QFileInfo subtitles(subtitleFile(file));
            recordings.append(QVariantMap{
                {QStringLiteral("name"), file.fileName()},
                {QStringLiteral("path"), file.filePath()},
                {QStringLiteral("url"), QUrl::fromLocalFile(file.filePath())},
                {QStringLiteral("size"), file.size()},
                {QStringLiteral("modified"), file.lastModified()},
                {QStringLiteral("subtitles"), subtitles.isFile() ? QUrl::fromLocalFile(subtitles.filePath()) : QUrl()},
            });
        }
    }
    if (recordings != _recordings) {
        _recordings = recordings;
        emit recordingsChanged();
    }
}

bool VideoRecordingList::remove(const QString& path)
{
    const QFileInfo video(path);
    if (video.absolutePath() != QFileInfo(_directory).absoluteFilePath()) {
        qCWarning(VideoRecordingListLog) << "Not a recording in" << _directory << path;
        return false;
    }

    const bool removed = QFile::remove(path);
    if (removed) {
        const QString subtitles = subtitleFile(video);
        if (QFile::exists(subtitles) && !QFile::remove(subtitles)) {
            qCWarning(VideoRecordingListLog) << "Unable to delete" << subtitles;
        }
    } else {
        qCWarning(VideoRecordingListLog) << "Unable to delete" << path;
    }
    _refresh();
    return removed;
}
