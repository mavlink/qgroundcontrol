#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(karchiveLog)

namespace karchive
{
    /// Method to zip files in a given directory
    bool createArchive(const QString &directoryPath, const QString &archivePath);

    /// Method to unzip files to a given directory
    bool extractArchive(const QString &archivePath, const QString &outputDirectoryPath);
}
