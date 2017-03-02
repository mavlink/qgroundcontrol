/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCMobileFileDialogController.h"

#include <QStandardPaths>
#include <QDebug>
#include <QDir>

QGC_LOGGING_CATEGORY(QGCMobileFileDialogControllerLog, "QGCMobileFileDialogControllerLog")

QStringList QGCMobileFileDialogController::getFiles(const QString& fileExtension)
{
    QStringList files;

    QDir fileDir(_getSaveLocation());

    QFileInfoList fileInfoList = fileDir.entryInfoList(QStringList(QString("*.%1").arg(fileExtension)),  QDir::Files, QDir::Name);

    foreach (const QFileInfo& fileInfo, fileInfoList) {
        files << fileInfo.baseName() + QStringLiteral(".") + fileExtension;
    }

    return files;
}

QString QGCMobileFileDialogController::fullPath(const QString& filename, const QString& fileExtension)
{
    qDebug() << "QGCMobileFileDialogController::fullPath" << filename << fileExtension;
    QString saveLocation(_getSaveLocation());
    if (saveLocation.isEmpty()) {
        return filename;
    }

    QString fixedFilename(filename);
    QString correctExtension = QString(".%1").arg(fileExtension);
    if (!filename.endsWith(correctExtension)) {
        fixedFilename += correctExtension;
    }

    QString fullPath = saveLocation + QDir::separator() + fixedFilename;
    qCDebug(QGCMobileFileDialogControllerLog) << "Full path" << fullPath;
    return fullPath;
}

bool QGCMobileFileDialogController::fileExists(const QString& filename, const QString& fileExtension)
{
    QFile file(fullPath(filename, fileExtension));
    qDebug() << "QGCMobileFileDialogController::fileExists" << file.fileName();
    return file.exists();
}

QString QGCMobileFileDialogController::_getSaveLocation(void)
{
    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qCWarning(QGCMobileFileDialogControllerLog) << "No save location";
        return QString();
    }

    QString saveDirectory = docDirs[0];
    if (!QDir(saveDirectory).exists()) {
        QDir().mkdir(saveDirectory);
    }
    qCDebug(QGCMobileFileDialogControllerLog) << "Save directory" << saveDirectory;
    
    return saveDirectory;
}
