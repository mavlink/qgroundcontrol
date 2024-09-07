#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCQuaZipLog)

namespace QGCQuaZip
{
    /// Method to unzip files to a given directory
    bool unzipFile(const QString &zipFilePath, const QString &outputDirectoryPath);
}
