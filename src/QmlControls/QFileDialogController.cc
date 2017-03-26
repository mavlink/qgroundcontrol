/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QFileDialogController.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

QGC_LOGGING_CATEGORY(QFileDialogControllerLog, "QFileDialogControllerLog")

QStringList QFileDialogController::getFiles(const QString& directoryPath, const QString& fileExtension)
{
    qCDebug(QFileDialogControllerLog) << "getFiles" << directoryPath << fileExtension;
    QStringList files;

    QDir fileDir(directoryPath);

    QFileInfoList fileInfoList = fileDir.entryInfoList(QStringList(QString("*.%1").arg(fileExtension)),  QDir::Files, QDir::Name);

    foreach (const QFileInfo& fileInfo, fileInfoList) {
        qCDebug(QFileDialogControllerLog) << "getFiles found" << fileInfo.baseName();
        files << fileInfo.baseName() + QStringLiteral(".") + fileExtension;
    }

    return files;
}

QString QFileDialogController::filenameWithExtension(const QString& filename, const QString& fileExtension)
{
    QString filenameWithExtension(filename);

    QString correctExtension = QString(".%1").arg(fileExtension);
    if (!filenameWithExtension.endsWith(correctExtension)) {
        filenameWithExtension += correctExtension;
    }

    return filenameWithExtension;
}

bool QFileDialogController::fileExists(const QString& filename)
{
    return QFile(filename).exists();
}

QString QFileDialogController::fullyQualifiedFilename(const QString& directoryPath, const QString& filename, const QString& fileExtension)
{
    return directoryPath + QStringLiteral("/") + filenameWithExtension(filename, fileExtension);
}
