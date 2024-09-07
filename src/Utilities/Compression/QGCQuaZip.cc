#include "QGCQuaZip.h"
#include "QGCLoggingCategory.h"

#include <quazip.h>
#include <quazipfile.h>

#include <QtCore/QDir>
#include <QtCore/QFile>

QGC_LOGGING_CATEGORY(QGCQuaZipLog, "qgc.utilities.compression.qgcquazip")

namespace QGCQuaZip
{

bool unzipFile(const QString &zipFilePath, const QString &outputDirectoryPath)
{
    QDir outputDir(outputDirectoryPath);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(outputDirectoryPath)) {
            qCDebug(QGCQuaZipLog) << "Failed to create output directory:" << outputDirectoryPath;
            return false;
        }
    }

    QuaZip zip(zipFilePath);
    if (!zip.open(QuaZip::mdUnzip)) {
        qCDebug(QGCQuaZipLog) << "Could not open zip file:" << zipFilePath;
        return false;
    }

    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
        const QString filePath = outputDirectoryPath + "/" + zip.getCurrentFileName();

        QuaZipFile file(&zip);
        if (!file.open(QIODevice::ReadOnly)) {
            qCDebug(QGCQuaZipLog) << "Could not open file inside zip:" << zip.getCurrentFileName();
            return false;
        }

        QFileInfo fileInfo(filePath);
        if (fileInfo.isDir()) {
            QDir().mkpath(filePath);
        } else {
            QDir().mkpath(fileInfo.absolutePath());
            QFile outFile(filePath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(file.readAll());
                outFile.close();
            } else {
                qCDebug(QGCQuaZipLog) << "Could not open file for writing:" << filePath;
                return false;
            }
        }

        file.close();
    }

    zip.close();
    return true;
}

} // namespace QGCQuaZip
