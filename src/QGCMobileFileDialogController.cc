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

QStringList QGCMobileFileDialogController::getFiles(const QString& fileExtension)
{
    QStringList files;

    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return QStringList();
    }
    QDir fileDir = docDirs.at(0);

    QFileInfoList fileInfoList = fileDir.entryInfoList(QStringList(QString("*.%1").arg(fileExtension)),  QDir::Files, QDir::Name);

    foreach (const QFileInfo& fileInfo, fileInfoList) {
        files << fileInfo.baseName() + QStringLiteral(".") + fileExtension;
    }

    return files;
}

QString QGCMobileFileDialogController::fullPath(const QString& filename, const QString& fileExtension)
{
    QStringList docDirs = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    if (docDirs.count() <= 0) {
        qWarning() << "No Documents location";
        return filename;
    }

    QString fixedFilename(filename);
    QString correctExtension = QString(".%1").arg(fileExtension);
    if (!filename.endsWith(correctExtension)) {
        fixedFilename += correctExtension;
    }

    QString fullPath = docDirs.at(0) + QDir::separator() + fixedFilename;
    qDebug() << fullPath;
    return fullPath;
}

bool QGCMobileFileDialogController::fileExists(const QString& filename, const QString& fileExtension)
{
    QFile file(fullPath(filename, fileExtension));
    return file.exists();
}
