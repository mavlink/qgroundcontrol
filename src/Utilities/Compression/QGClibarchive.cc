#include "QGClibarchive.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <archive.h>
#include <archive_entry.h>

QGC_LOGGING_CATEGORY(QGClibarchiveLog, "Utilities.QGClibarchive")

namespace QGClibarchive {

bool zipDirectory(const QString &directoryPath, const QString &zipFilePath)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        qCWarning(QGClibarchiveLog) << "Directory does not exist:" << directoryPath;
        return false;
    }

    struct archive *a = archive_write_new();
    if (!a) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive writer";
        return false;
    }

    // Set ZIP format with deflate compression
    archive_write_set_format_zip(a);
    archive_write_zip_set_compression_deflate(a);

    if (archive_write_open_filename(a, zipFilePath.toLocal8Bit().constData()) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to open zip file:" << zipFilePath
                                        << archive_error_string(a);
        archive_write_free(a);
        return false;
    }

    bool success = true;
    QDirIterator it(directoryPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        QString relativePath = dir.relativeFilePath(filePath);

        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, relativePath.toUtf8().constData());

        if (fileInfo.isDir()) {
            archive_entry_set_filetype(entry, AE_IFDIR);
            archive_entry_set_perm(entry, 0755);
            archive_entry_set_size(entry, 0);
        } else {
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);
            archive_entry_set_size(entry, fileInfo.size());
        }

        archive_entry_set_mtime(entry, fileInfo.lastModified().toSecsSinceEpoch(), 0);

        if (archive_write_header(a, entry) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to write header for:" << relativePath
                                            << archive_error_string(a);
            archive_entry_free(entry);
            success = false;
            break;
        }

        if (fileInfo.isFile()) {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                qCWarning(QGClibarchiveLog) << "Failed to open file:" << filePath;
                archive_entry_free(entry);
                success = false;
                break;
            }

            char buffer[16384];
            qint64 bytesRead;
            while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
                if (archive_write_data(a, buffer, static_cast<size_t>(bytesRead)) < 0) {
                    qCWarning(QGClibarchiveLog) << "Failed to write data for:" << relativePath
                                                    << archive_error_string(a);
                    success = false;
                    break;
                }
            }

            if (bytesRead < 0) {
                qCWarning(QGClibarchiveLog) << "Failed to read file:" << filePath;
                success = false;
            }

            if (!success) {
                archive_entry_free(entry);
                break;
            }
        }

        archive_entry_free(entry);
    }

    if (archive_write_close(a) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to close archive:" << archive_error_string(a);
        success = false;
    }

    archive_write_free(a);
    return success;
}

bool unzipFile(const QString &zipFilePath, const QString &outputDirectoryPath)
{
    QDir outputDir(outputDirectoryPath);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(outputDirectoryPath)) {
            qCWarning(QGClibarchiveLog) << "Failed to create output directory:" << outputDirectoryPath;
            return false;
        }
    }

    // Read file through QFile to support Qt resources (:/path) and regular files
    QFile inputFile(zipFilePath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qCWarning(QGClibarchiveLog) << "Failed to open zip file:" << zipFilePath
                                    << inputFile.errorString();
        return false;
    }
    const QByteArray archiveData = inputFile.readAll();
    inputFile.close();

    if (archiveData.isEmpty()) {
        qCWarning(QGClibarchiveLog) << "Zip file is empty:" << zipFilePath;
        return false;
    }

    struct archive *a = archive_read_new();
    if (!a) {
        qCWarning(QGClibarchiveLog) << "Failed to create archive reader";
        return false;
    }

    archive_read_support_format_zip(a);
    archive_read_support_filter_all(a);

    // Open from memory buffer (works with Qt resources)
    if (archive_read_open_memory(a, archiveData.constData(),
                                  static_cast<size_t>(archiveData.size())) != ARCHIVE_OK) {
        qCWarning(QGClibarchiveLog) << "Failed to open zip data:" << archive_error_string(a);
        archive_read_free(a);
        return false;
    }

    struct archive *ext = archive_write_disk_new();
    // Note: Not using ARCHIVE_EXTRACT_PERM as some archive formats (like QZipWriter)
    // may not store proper directory permissions, causing traversal issues
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_SECURE_NODOTDOT);
    archive_write_disk_set_standard_lookup(ext);

    bool success = true;
    struct archive_entry *entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *currentFile = archive_entry_pathname(entry);
        qCDebug(QGClibarchiveLog) << "Reading entry:" << currentFile
                                      << "size:" << archive_entry_size(entry)
                                      << "filetype:" << archive_entry_filetype(entry);

        QString entryName = QString::fromUtf8(currentFile);
        QString outputPath = outputDirectoryPath + "/" + entryName;

        // Prevent path traversal attacks
        QFileInfo outputInfo(outputPath);
        QString canonicalOutput = outputInfo.absoluteFilePath();
        QFileInfo outputDirInfo(outputDirectoryPath);
        QString canonicalOutputDir = outputDirInfo.absoluteFilePath();

        if (!canonicalOutput.startsWith(canonicalOutputDir + "/") &&
            canonicalOutput != canonicalOutputDir) {
            qCWarning(QGClibarchiveLog) << "Skipping path traversal attempt:" << currentFile
                                        << "resolved to:" << canonicalOutput;
            continue;
        }

        // Security: reject filenames with embedded null bytes (libarchive issue #2774)
        if (entryName.contains(QChar('\0'))) {
            qCWarning(QGClibarchiveLog) << "Skipping path with embedded null byte";
            continue;
        }

        archive_entry_set_pathname(entry, outputPath.toUtf8().constData());

        // Ensure parent directories exist before extracting
        // (some ZIP files don't include explicit directory entries)
        QDir().mkpath(QFileInfo(outputPath).absolutePath());

        int r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to write header for" << outputPath << ":" << archive_error_string(ext);
            success = false;
            break;
        }

        if (archive_entry_size(entry) > 0) {
            const void *buff;
            size_t size;
            la_int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    qCWarning(QGClibarchiveLog) << "Failed to write data:" << archive_error_string(ext);
                    success = false;
                    break;
                }
            }

            if (r != ARCHIVE_EOF && r != ARCHIVE_OK) {
                qCWarning(QGClibarchiveLog) << "Failed to read data:" << archive_error_string(a);
                success = false;
                break;
            }
        }

        if (archive_write_finish_entry(ext) != ARCHIVE_OK) {
            qCWarning(QGClibarchiveLog) << "Failed to finish entry:" << archive_error_string(ext);
            success = false;
            break;
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return success;
}

} // namespace QGClibarchive
