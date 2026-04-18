#include "VideoFileNaming.h"

#include <QtCore/QDateTime>

namespace VideoFileNaming {

QString extensionForFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
        case QMediaFormat::QuickTime: return QStringLiteral("mov");
        case QMediaFormat::MPEG4:     return QStringLiteral("mp4");
        default:                      return QStringLiteral("mkv");
    }
}

QString defaultVideoBaseName()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_hh.mm.ss"));
}

QString buildRecordingTemplate(const QString& savePath,
                               const QString& baseName,
                               QMediaFormat::FileFormat format)
{
    const QString base = baseName.isEmpty() ? defaultVideoBaseName() : baseName;
    return savePath + QLatin1Char('/') + base + QStringLiteral(".%1") + extensionForFormat(format);
}

QString subtitleSiblingPath(const QString& videoPath)
{
    // Matches the pre-refactor behavior: a path with no extension collapses to
    // just ".srt". In practice recording paths always carry an extension, so
    // the edge case is unreachable — preserved to avoid behavioral drift.
    const qsizetype dot = videoPath.lastIndexOf(QLatin1Char('.'));
    return (dot < 0 ? QString() : videoPath.left(dot)) + QStringLiteral(".srt");
}

QString buildImageGrabPath(const QString& photoSavePath)
{
    return photoSavePath + QLatin1Char('/') +
           QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_hh.mm.ss.zzz")) +
           QStringLiteral(".jpg");
}

}  // namespace VideoFileNaming
