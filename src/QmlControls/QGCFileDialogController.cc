/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCFileDialogController.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

QGC_LOGGING_CATEGORY(QGCFileDialogControllerLog, "QGCFileDialogControllerLog")

QStringList QGCFileDialogController::getFiles(const QString& directoryPath, const QStringList& fileExtensions)
{
    qCDebug(QGCFileDialogControllerLog) << "getFiles" << directoryPath << fileExtensions;
    QStringList files;

    QDir fileDir(directoryPath);

    QStringList infoListExtensions;
    for (const QString& extension: fileExtensions) {
        infoListExtensions.append(QStringLiteral("*.%1").arg(extension));
    }

    QFileInfoList fileInfoList = fileDir.entryInfoList(infoListExtensions,  QDir::Files, QDir::Name);

    for (const QFileInfo& fileInfo: fileInfoList) {
        qCDebug(QGCFileDialogControllerLog) << "getFiles found" << fileInfo.fileName();
        files << fileInfo.fileName();
    }

    return files;
}

QString QGCFileDialogController::filenameWithExtension(const QString& filename, const QString& fileExtension)
{
    QString filenameWithExtension(filename);

    QString correctExtension = QString(".%1").arg(fileExtension);
    if (!filenameWithExtension.endsWith(correctExtension)) {
        filenameWithExtension += correctExtension;
    }

    return filenameWithExtension;
}

bool QGCFileDialogController::fileExists(const QString& filename)
{
    return QFile(filename).exists();
}

QString QGCFileDialogController::fullyQualifiedFilename(const QString& directoryPath, const QString& filename, const QString& fileExtension)
{
    return directoryPath + QStringLiteral("/") + filenameWithExtension(filename, fileExtension);
}

void QGCFileDialogController::deleteFile(const QString& filename)
{
    QFile::remove(filename);
}
