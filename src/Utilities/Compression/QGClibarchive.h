#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(QGClibarchiveLog)

namespace QGClibarchive {

/// Compress a directory into a ZIP file using libarchive
/// @param directoryPath Path to the directory to compress
/// @param zipFilePath Path where the ZIP file will be created
/// @return true on success, false on failure
bool zipDirectory(const QString &directoryPath, const QString &zipFilePath);

/// Extract a ZIP file to a directory using libarchive
/// @param zipFilePath Path to the ZIP file
/// @param outputDirectoryPath Path where files will be extracted
/// @return true on success, false on failure
bool unzipFile(const QString &zipFilePath, const QString &outputDirectoryPath);

} // namespace QGClibarchive
