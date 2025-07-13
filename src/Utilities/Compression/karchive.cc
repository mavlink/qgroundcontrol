#include "karchive.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDebug>

#include <memory>

#include <KCompressionDevice>
#include <ktar.h>
#include <kzip.h>

#if __has_include(<k7zip.h>)
#include <k7zip.h>
#define KARCHIVE_HAS_K7ZIP 1
#else
#define KARCHIVE_HAS_K7ZIP 0
#endif

QGC_LOGGING_CATEGORY(karchiveLog, "qgc.utilities.compression.karchive")

namespace
{

/// Returns only the file/folder name component of a path ("/a/b/c" -> "c")
QString _archiveRootName(const QString &path)
{
    return QFileInfo(path).fileName();
}

/// Factory: choose the correct *writer* class based on the file extension.
std::unique_ptr<KArchive> _makeWriter(const QString &archivePath)
{
    const QString lower = archivePath.toLower();

    if (lower.endsWith(".zip")) {
        return std::make_unique<KZip>(archivePath);
    }

    // Tar family (plain, gzip, bzip2, xz)
    if (lower.endsWith(".tar")   || lower.endsWith(".tar.gz") || lower.endsWith(".tgz")  ||
        lower.endsWith(".tar.bz2")|| lower.endsWith(".tbz2")  || lower.endsWith(".tbz")  ||
        lower.endsWith(".tar.xz") || lower.endsWith(".txz")) {
        return std::make_unique<KTar>(archivePath);
    }

#if KARCHIVE_HAS_K7ZIP
    // NOTE: K7Zip (at least in KF5/KF6) is **read‑only**. We therefore return nullptr
    // to tell the caller this archive type cannot be *created*.
    if (lower.endsWith(".7z")) {
        return nullptr;
    }
#endif

    return nullptr; // Unknown format
}

/// Factory: choose the correct *reader* class based on the file extension.
std::unique_ptr<KArchive> _makeReader(const QString &archivePath)
{
    const QString lower = archivePath.toLower();

    if (lower.endsWith(".zip")) {
        return std::make_unique<KZip>(archivePath);
    }

    if (lower.endsWith(".tar")   || lower.endsWith(".tar.gz") || lower.endsWith(".tgz")  ||
        lower.endsWith(".tar.bz2")|| lower.endsWith(".tbz2")  || lower.endsWith(".tbz")  ||
        lower.endsWith(".tar.xz") || lower.endsWith(".txz")) {
        return std::make_unique<KTar>(archivePath);
    }

#if KARCHIVE_HAS_K7ZIP
    if (lower.endsWith(".7z")) {
        return std::make_unique<K7Zip>(archivePath);
    }
#endif

    return nullptr; // Unknown format
}

}

namespace karchive
{

bool createArchive(const QString &directoryPath, const QString &archivePath)
{
    // Sanity checks --------------------------------------------------
    if (!QDir(directoryPath).exists()) {
        qCWarning(karchiveLog) << "Directory does not exist:" << directoryPath;
        return false;
    }

    const QFileInfo info(archivePath);
    if (!QDir().mkpath(info.absolutePath())) {
        qCWarning(karchiveLog) << "Unable to create directory for archive:" << info.absolutePath();
        return false;
    }

    // Select writer ---------------------------------------------------
    auto archive = _makeWriter(archivePath);
    if (!archive) {
        qCWarning(karchiveLog) << "Unsupported or read‑only archive type for writing:" << archivePath;
        return false;
    }

    if (!archive->open(QIODevice::WriteOnly)) {
        qCWarning(karchiveLog) << "Cannot create archive:" << archivePath << "error:" << archive->errorString();
        return false;
    }

    // Add contents ----------------------------------------------------
    if (!archive->addLocalDirectory(directoryPath, _archiveRootName(directoryPath))) {
        qCWarning(karchiveLog) << "Failed to add directory" << directoryPath << "to archive";
        archive->close();
        return false;
    }

    archive->close();
    return true;
}

bool extractArchive(const QString &archivePath, const QString &outputDirectoryPath)
{
    // Sanity checks --------------------------------------------------
    if (!QFile::exists(archivePath)) {
        qCWarning(karchiveLog) << "Archive does not exist:" << archivePath;
        return false;
    }

    if (!QDir().mkpath(outputDirectoryPath)) {
        qCWarning(karchiveLog) << "Cannot create output directory:" << outputDirectoryPath;
        return false;
    }

    // Select reader ---------------------------------------------------
    auto archive = _makeReader(archivePath);
    if (!archive) {
        qCWarning(karchiveLog) << "Unsupported archive type:" << archivePath;
        return false;
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        qCWarning(karchiveLog) << "Cannot open" << archivePath << "error:" << archive->errorString();
        return false;
    }

    // Extract ---------------------------------------------------------
    const KArchiveDirectory *root = archive->directory();
    if (!root) {
        qCWarning(karchiveLog) << "Failed to obtain root directory from archive";
        archive->close();
        return false;
    }

    root->copyTo(outputDirectoryPath, true);
    archive->close();
    return true;
}

} // namespace karchive
