#include "QGCZip.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/private/qzipreader_p.h>
#include <QtCore/private/qzipwriter_p.h>

QGC_LOGGING_CATEGORY(QGCZipLog, "qgc.utilities.compression.qgczip")

namespace QGCZip {

bool zipDirectory(const QString &directoryPath, const QString &zipFilePath)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        qCDebug(QGCZipLog) << "Directory does not exist:" << directoryPath;
        return false;
    }

    QFile zipFile(zipFilePath);
    if (!zipFile.open(QIODevice::WriteOnly)) {
        qCDebug(QGCZipLog) << "Could not open zip file for writing:" << zipFilePath;
        return false;
    }

    QZipWriter zipWriter(&zipFile);
    zipWriter.setCompressionPolicy(QZipWriter::AutoCompress);

    QStringList fileList = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::AllDirs, QDir::DirsFirst);

    for (const QString &fileName : fileList) {
        QString filePath = directoryPath + "/" + fileName;

        if (QFileInfo(filePath).isDir()) {
            zipWriter.addDirectory(fileName);
        } else {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                zipWriter.addFile(fileName, file.readAll());
                file.close();
            } else {
                qCDebug(QGCZipLog) << "Could not open file for reading:" << filePath;
                return false;
            }
        }
    }

    zipWriter.close();
    return true;
}

bool unzipFile(const QString &zipFilePath, const QString &outputDirectoryPath)
{
    QDir outputDir(outputDirectoryPath);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(outputDirectoryPath)) {
            qCDebug(QGCZipLog) << "Failed to create output directory:" << outputDirectoryPath;
            return false;
        }
    }

    QFile zipFile(zipFilePath);
    if (!zipFile.open(QIODevice::ReadOnly)) {
        qCDebug(QGCZipLog) << "Could not open zip file:" << zipFilePath;
        return false;
    }

    QZipReader zipReader(&zipFile);
    if (!zipReader.isReadable()) {
        qCDebug(QGCZipLog) << "Could not read zip file:" << zipFilePath;
        return false;
    }

    const QList<QZipReader::FileInfo> allFiles = zipReader.fileInfoList();

    for (const QZipReader::FileInfo &fileInfo : allFiles) {
        QString filePath = outputDirectoryPath + "/" + fileInfo.filePath;

        if (fileInfo.isDir) {
            QDir().mkpath(filePath);
        } else if (fileInfo.isFile) {
            QFile outFile(filePath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(zipReader.fileData(fileInfo.filePath));
                outFile.close();
            } else {
                qDebug() << "Could not open file for writing:" << filePath;
                return false;
            }
        }
    }

    zipReader.close();
    return true;
}

} // namespace QGCZip
