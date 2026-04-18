#include "RecordingOrphanScanner.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <QtCore/QStringLiteral>

#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(RecordingSessionLog)

namespace RecordingOrphanScanner {

namespace {

/// Heuristic check: file exists, has a known video extension, and is non-empty.
/// Avoids QMediaPlayer + nested QEventLoop which blocks the calling thread for
/// up to 3 s per file and is not safe to re-enter on the main thread.
bool probeFilePlayable(const QString& filePath)
{
    const QFileInfo fi(filePath);
    if (!fi.exists() || fi.size() == 0)
        return false;

    static const QStringList kVideoExtensions{
        QStringLiteral("mkv"), QStringLiteral("mov"),
        QStringLiteral("mp4"), QStringLiteral("ts"),
    };
    return kVideoExtensions.contains(fi.suffix().toLower());
}

}  // namespace

int scan(const QString& recordingDir, const CorruptionCallback& onCorruption)
{
    QDir dir(recordingDir);
    if (!dir.exists())
        return 0;

    const QStringList manifests =
        dir.entryList(QStringList{QStringLiteral("session-*.json")}, QDir::Files | QDir::Readable);

    int orphanCount = 0;

    for (const QString& name : manifests) {
        const QString fullPath = recordingDir + QStringLiteral("/") + name;

        QFile f(fullPath);
        if (!f.open(QIODevice::ReadOnly))
            continue;

        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();

        // Only our session manifests have version == 1
        if (root.value(QStringLiteral("version")).toInt() != 1)
            continue;

        // If stopTimestamp is present and non-null, this is already finalized
        const QJsonValue stopVal = root.value(QStringLiteral("stopTimestamp"));
        if (!stopVal.isNull() && !stopVal.toString().isEmpty())
            continue;

        ++orphanCount;
        qCWarning(RecordingSessionLog) << "Found orphaned manifest:" << fullPath;

        const QJsonArray streams = root.value(QStringLiteral("streams")).toArray();
        bool allPlayable = true;

        // Compute canonical root once; used to validate every path from the manifest.
        const QString canonicalRoot = QFileInfo(recordingDir).canonicalFilePath();

        for (const QJsonValue& sv : streams) {
            const QString videoPath = sv.toObject().value(QStringLiteral("path")).toString();
            const QString canonical = QFileInfo(videoPath).canonicalFilePath();
            if (canonical.isEmpty() || !canonical.startsWith(canonicalRoot + QLatin1Char('/'))) {
                qCWarning(RecordingSessionLog)
                    << "Orphan scanner rejecting path outside recording dir:" << videoPath;
                allPlayable = false;
                break;
            }
            if (!probeFilePlayable(videoPath)) {
                allPlayable = false;
                break;
            }
        }

        if (allPlayable) {
            // Rename manifest to .json.ok — human can inspect
            const QString okPath = fullPath + QStringLiteral(".ok");
            QFile::rename(fullPath, okPath);
            qCInfo(RecordingSessionLog) << "Orphan files appear playable; manifest renamed to" << okPath;
        } else {
            // Move manifest + video files to corrupted/ subdir
            const QString ts = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_hhmmss"));
            const QString corruptDir = recordingDir + QStringLiteral("/corrupted/") + ts;
            QDir().mkpath(corruptDir);

            QStringList movedFiles;

            for (const QJsonValue& sv : streams) {
                const QString videoPath = sv.toObject().value(QStringLiteral("path")).toString();
                const QString canonical = QFileInfo(videoPath).canonicalFilePath();
                if (canonical.isEmpty() || !canonical.startsWith(canonicalRoot + QLatin1Char('/'))) {
                    qCWarning(RecordingSessionLog)
                        << "Orphan scanner rejecting path outside recording dir:" << videoPath;
                    continue;
                }
                if (!videoPath.isEmpty() && QFile::exists(videoPath)) {
                    const QString dest = corruptDir + QStringLiteral("/") + QFileInfo(videoPath).fileName();
                    if (QFile::rename(videoPath, dest))
                        movedFiles << dest;
                }
            }

            const QString destManifest = corruptDir + QStringLiteral("/") + name;
            if (QFile::rename(fullPath, destManifest))
                movedFiles << destManifest;

            qCWarning(RecordingSessionLog) << "Unplayable orphan; moved to" << corruptDir;

            if (onCorruption)
                onCorruption(movedFiles);
        }
    }

    if (orphanCount > 0)
        qCInfo(RecordingSessionLog) << "scanForOrphans found" << orphanCount << "orphaned session(s)";

    return orphanCount;
}

}  // namespace RecordingOrphanScanner
