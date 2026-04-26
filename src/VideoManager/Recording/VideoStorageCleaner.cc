#include "VideoStorageCleaner.h"

#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QStringList>

QGC_LOGGING_CATEGORY(VideoStorageCleanerLog, "Video.VideoStorageCleaner")

namespace VideoStorageCleaner {

int pruneToLimit(const QString& directory, const QStringList& nameFilters, uint64_t maxBytes)
{
    if (maxBytes == 0 || directory.isEmpty())
        return 0;

    QDir dir(directory);
    dir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    dir.setSorting(QDir::Time);
    dir.setNameFilters(nameFilters);

    QFileInfoList vids = dir.entryInfoList();
    if (vids.isEmpty())
        return 0;

    uint64_t total = 0;
    for (const QFileInfo& v : std::as_const(vids))
        total += static_cast<uint64_t>(v.size());

    int removed = 0;
    while (total >= maxBytes && !vids.isEmpty()) {
        const QFileInfo info = vids.takeLast();
        total -= static_cast<uint64_t>(info.size());
        qCDebug(VideoStorageCleanerLog) << "Removing old video file:" << info.filePath();
        if (QFile::remove(info.filePath()))
            ++removed;
    }
    return removed;
}

}  // namespace VideoStorageCleaner
